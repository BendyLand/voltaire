/**************************************************************************/
/*  video_stream.h                                                        */
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

#include "scene/resources/texture.h"

class VideoStreamPlayback : public Resource
{
public:
	typedef int (*AudioMixCallback)(void* p_udata, const float* p_data, int p_frames);
	struct TheoraData;

private:
	TheoraData* data = nullptr;

protected:
	AudioMixCallback mix_callback = nullptr;
	void* mix_udata = nullptr;
	mutable int _channel_count = 0; // Used only to assist with bounds checking in mix_audio.

	int mix_audio(int num_frames, PackedFloat32Array buffer = {}, int offset = 0);

public:
	VideoStreamPlayback();
	~VideoStreamPlayback();

	void stop();
	void play();

	bool is_playing() const;

	void set_paused(bool p_paused);
	bool is_paused() const;

	double get_length() const;

	double get_playback_position() const;
	void seek(double p_time);

	void set_file(const String& p_file);
	void set_audio_track(int p_idx);

	Ref<Texture2D> get_texture() const;
	void update(double p_delta);

	void set_mix_callback(AudioMixCallback p_callback, void* p_userdata);
	int get_channels() const;
	int get_mix_rate() const;
};

class VideoStream : public Resource
{
protected:
	String file;
	int audio_track = 0;

public:
	void set_file(const String& p_file);
	String get_file();

	void set_audio_track(int p_track);
	Ref<VideoStreamPlayback> instantiate_playback();

	VideoStream();
	~VideoStream();
};
