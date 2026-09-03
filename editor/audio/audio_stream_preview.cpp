/**************************************************************************/
/*  audio_stream_preview.cpp                                              */
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

#include "audio_stream_preview.h"

/////////////////////

float AudioStreamPreview::get_length() const { return length; }

float AudioStreamPreview::get_max(float p_time, float p_time_next) const
{
	if (length == 0) {
		return 0;
	}

	int max = preview.size() / 2;
	if (max == 0) {
		return 0;
	}

	int time_from = p_time / length * max;
	int time_to = p_time_next / length * max;
	time_from = CLAMP(time_from, 0, max - 1);
	time_to = CLAMP(time_to, 0, max - 1);

	if (time_to <= time_from) {
		time_to = time_from + 1;
	}

	uint8_t vmax = 0;

	for (int i = time_from; i < time_to; i++) {
		uint8_t v = preview[i * 2 + 1];
		if (i == 0 || v > vmax) {
			vmax = v;
		}
	}

	return (vmax / 255.0) * 2.0 - 1.0;
}

float AudioStreamPreview::get_min(float p_time, float p_time_next) const
{
	if (length == 0) {
		return 0;
	}

	int max = preview.size() / 2;
	if (max == 0) {
		return 0;
	}

	int time_from = p_time / length * max;
	int time_to = p_time_next / length * max;
	time_from = CLAMP(time_from, 0, max - 1);
	time_to = CLAMP(time_to, 0, max - 1);

	if (time_to <= time_from) {
		time_to = time_from + 1;
	}

	uint8_t vmin = 255;

	for (int i = time_from; i < time_to; i++) {
		uint8_t v = preview[i * 2];
		if (i == 0 || v < vmin) {
			vmin = v;
		}
	}

	return (vmin / 255.0) * 2.0 - 1.0;
}

AudioStreamPreview::AudioStreamPreview() { length = 0; }

////

void AudioStreamPreviewGenerator::_preview_thread(void* p_preview)
{
	Thread::set_name("AudioStreamPreviewGenerator");
	Preview* preview = static_cast<Preview*>(p_preview);
	float muxbuff_chunk_s = 0.25;
	int mixbuff_chunk_frames = AudioServer::get_singleton()->get_mix_rate() * muxbuff_chunk_s;
	Vector<AudioFrame> mix_chunk;
	mix_chunk.resize(mixbuff_chunk_frames);
	int frames_total = AudioServer::get_singleton()->get_mix_rate() * preview->preview->length;
	int frames_todo = frames_total;
	preview->playback->start();
	while (frames_todo) {
		int ofs_write = uint64_t(frames_total - frames_todo) *
						uint64_t(preview->preview->preview.size() / 2) / uint64_t(frames_total);
		int to_read = MIN(frames_todo, mixbuff_chunk_frames);
		int to_write = uint64_t(to_read) * uint64_t(preview->preview->preview.size() / 2) /
					   uint64_t(frames_total);
		to_write = MIN(to_write, (preview->preview->preview.size() / 2) - ofs_write);
		preview->playback->mix(mix_chunk.ptrw(), 1.0, to_read);
		for (int i = 0; i < to_write; i++) {
			float max = -1000;
			float min = 1000;
			int from = uint64_t(i) * to_read / to_write;
			int to = (uint64_t(i) + 1) * to_read / to_write;
			to = MIN(to, to_read);
			from = MIN(from, to_read - 1);
			if (to == from) {
				to = from + 1;
			}
			for (int j = from; j < to; j++) {
				max = MAX(max, mix_chunk[j].left);
				max = MAX(max, mix_chunk[j].right);
				min = MIN(min, mix_chunk[j].left);
				min = MIN(min, mix_chunk[j].right);
			}
			uint8_t pfrom = CLAMP((min * 0.5 + 0.5) * 255, 0, 255);
			uint8_t pto = CLAMP((max * 0.5 + 0.5) * 255, 0, 255);
			preview->preview->preview.write[(ofs_write + i) * 2 + 0] = pfrom;
			preview->preview->preview.write[(ofs_write + i) * 2 + 1] = pto;
		}
		frames_todo -= to_read;
	}
	preview->preview->version++;
	preview->playback->stop();
	preview->generating.clear();
}

void AudioStreamPreviewGenerator::_bind_methods() {}

AudioStreamPreviewGenerator* AudioStreamPreviewGenerator::singleton = nullptr;

AudioStreamPreviewGenerator::AudioStreamPreviewGenerator()
{
	singleton = this;
	set_process(true);
}


