/**************************************************************************/
/*  audio_effect_reverb.cpp                                               */
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

#include "audio_effect_reverb.h"
#include "core/object/class_db.h"
#include "servers/audio/audio_server.h"

void AudioEffectReverbInstance::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	for (int i = 0; i < 2; i++) {
		Reverb& r = reverb[i];

		r.set_predelay(base->predelay);
		r.set_predelay_feedback(base->predelay_fb);
		r.set_highpass(base->hpf);
		r.set_room_size(base->room_size);
		r.set_damp(base->damping);
		r.set_extra_spread(base->spread);
		r.set_wet(base->wet);
		r.set_dry(base->dry);
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

AudioEffectReverbInstance::AudioEffectReverbInstance()
{
	reverb[0].set_mix_rate(AudioServer::get_singleton()->get_mix_rate());
	reverb[0].set_extra_spread_base(0);
	reverb[1].set_mix_rate(AudioServer::get_singleton()->get_mix_rate());
	reverb[1].set_extra_spread_base(0.000521); // for stereo effect
}

Ref<AudioEffectInstance> AudioEffectReverb::instantiate()
{
	Ref<AudioEffectReverbInstance> ins;
	ins.instantiate();
	ins->base = Ref<AudioEffectReverb>(this);
	return ins;
}

void AudioEffectReverb::set_predelay_msec(float p_msec) { predelay = p_msec; }

void AudioEffectReverb::set_predelay_feedback(float p_feedback)
{
	predelay_fb = CLAMP(p_feedback, 0, 0.98);
}

void AudioEffectReverb::set_room_size(float p_size) { room_size = p_size; }

void AudioEffectReverb::set_damping(float p_damping) { damping = p_damping; }

void AudioEffectReverb::set_spread(float p_spread) { spread = p_spread; }

void AudioEffectReverb::set_dry(float p_dry) { dry = p_dry; }

void AudioEffectReverb::set_wet(float p_wet) { wet = p_wet; }

void AudioEffectReverb::set_hpf(float p_hpf) { hpf = p_hpf; }

float AudioEffectReverb::get_predelay_msec() const { return predelay; }

float AudioEffectReverb::get_predelay_feedback() const { return predelay_fb; }

float AudioEffectReverb::get_room_size() const { return room_size; }

float AudioEffectReverb::get_damping() const { return damping; }

float AudioEffectReverb::get_spread() const { return spread; }

float AudioEffectReverb::get_dry() const { return dry; }

float AudioEffectReverb::get_wet() const { return wet; }

float AudioEffectReverb::get_hpf() const { return hpf; }

void AudioEffectReverb::_bind_methods() {}

AudioEffectReverb::AudioEffectReverb()
{
	predelay = 150;
	predelay_fb = 0.4;
	hpf = 0;
	room_size = 0.8;
	damping = 0.5;
	spread = 1.0;
	dry = 1.0;
	wet = 0.5;
}


