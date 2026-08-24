/**************************************************************************/
/*  video_stream.cpp                                                      */
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

#include "core/object/class_db.h"
#include "video_stream.h"

// VideoStreamPlayback starts here.

void VideoStreamPlayback::_bind_methods() {}

VideoStreamPlayback::VideoStreamPlayback() {}

VideoStreamPlayback::~VideoStreamPlayback() {}

void VideoStreamPlayback::set_mix_callback(AudioMixCallback p_callback, void* p_userdata)
{
	mix_callback = p_callback;
	mix_udata = p_userdata;
}

int VideoStreamPlayback::mix_audio(int num_frames, PackedFloat32Array buffer, int offset)
{
	if (num_frames <= 0) {
		return 0;
	}
	if (!mix_callback) {
		return -1;
	}
	ERR_FAIL_INDEX_V(offset, buffer.size(), -1);
	ERR_FAIL_INDEX_V(
		(_channel_count < 1 ? 1 : _channel_count) * num_frames - 1, buffer.size() - offset, -1);
	return mix_callback(mix_udata, buffer.ptr() + offset, num_frames);
}

/* --- NOTE VideoStream starts here. ----- */

Ref<VideoStreamPlayback> VideoStream::instantiate_playback()
{
	Ref<VideoStreamPlayback> ret;
	ERR_FAIL_COND_V_MSG(ret.is_null(), nullptr, "Plugin returned null playback");
	ret->set_audio_track(audio_track);
	return ret;
}

void VideoStream::set_file(const String& p_file)
{
	file = p_file;
	emit_changed();
}

String VideoStream::get_file() { return file; }

void VideoStream::_bind_methods() {}

VideoStream::VideoStream() {}

VideoStream::~VideoStream() {}

void VideoStream::set_audio_track(int p_track) { audio_track = p_track; }

void VideoStreamPlayback::stop() {}

void VideoStreamPlayback::play() {}

bool VideoStreamPlayback::is_playing() const { return false; }

void VideoStreamPlayback::set_paused(bool p_paused) {}

bool VideoStreamPlayback::is_paused() const { return false; }

double VideoStreamPlayback::get_length() const { return 0.0; }

double VideoStreamPlayback::get_playback_position() const { return 0.0; }

void VideoStreamPlayback::seek(double p_time) {}

void VideoStreamPlayback::set_audio_track(int p_idx) {}

Ref<Texture2D> VideoStreamPlayback::get_texture() const { return Ref<Texture2D>(); }

void VideoStreamPlayback::update(double p_delta) {}

int VideoStreamPlayback::get_channels() const { return 0; }

int VideoStreamPlayback::get_mix_rate() const { return 0; }


