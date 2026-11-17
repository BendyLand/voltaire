/**************************************************************************/
/*  tts_linux.cpp                                                         */
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

#include "tts_linux.h"

#include "core/config/project_settings.h"
#include "servers/display/display_server.h"
#include "servers/text/text_server.h"

void TTS_Linux::_speech_index_mark(int p_msg_id, int p_type, const String& p_index_mark)
{
	ERR_FAIL_NULL(data);
	MutexLock lock(data->mutex);

	if (data->ids.has(p_msg_id)) {
		DisplayServer::tts_post_utterance_event(
			DisplayServerEnums::TTS_UTTERANCE_BOUNDARY, data->ids[p_msg_id], p_index_mark.to_int());
	}
}

void TTS_Linux::_load_voices()
{
	if (!data->voices_loaded) {
		SPDVoice** spd_voices = spd_list_synthesis_voices(data->synth);
		if (spd_voices != nullptr) {
			SPDVoice** voices_ptr = spd_voices;
			while (*voices_ptr != nullptr) {
				VoiceInfo vi;
				vi.language = String::utf8((*voices_ptr)->language);
				vi.variant = String::utf8((*voices_ptr)->variant);
				data->voices[String::utf8((*voices_ptr)->name)] = vi;
				voices_ptr++;
			}
			free_spd_voices(spd_voices);
		}
		data->voices_loaded = true;
	}
}

void TTS_Linux::_speech_event(int p_msg_id, int p_type)
{
	ERR_FAIL_NULL(data);
	MutexLock lock(data->mutex);

	if (!data->paused && data->ids.has(p_msg_id)) {
		if ((SPDNotificationType)p_type == SPD_EVENT_END) {
			DisplayServer::tts_post_utterance_event(
				DisplayServerEnums::TTS_UTTERANCE_ENDED, data->ids[p_msg_id]);
			data->ids.erase(p_msg_id);
			data->last_msg_id = -1;
			data->speaking = false;
		}
		else if ((SPDNotificationType)p_type == SPD_EVENT_CANCEL) {
			DisplayServer::tts_post_utterance_event(
				DisplayServerEnums::TTS_UTTERANCE_CANCELED, data->ids[p_msg_id]);
			data->ids.erase(p_msg_id);
			data->last_msg_id = -1;
			data->speaking = false;
		}
	}
	if (!data->speaking && data->queue.size() > 0) {
		TTSUtterance& message = data->queue.front()->get();

		// Inject index mark after each word.
		String text;
		String language;

		_load_voices();
		const VoiceInfo* voice = data->voices.getptr(message.voice);
		if (voice) {
			language = voice->language;
		}

		PackedInt32Array breaks = TS->string_get_word_breaks(message.text, language);
		int prev_end = -1;
		for (int i = 0; i < breaks.size(); i += 2) {
			const int start = breaks[i];
			const int end = breaks[i + 1];
			if (prev_end != -1 && prev_end != start) {
				text += message.text.substr(prev_end, start - prev_end);
			}
			text += message.text.substr(start, end - start);
			text += "<mark name=\"" + String::num_int64(end, 10) + "\"/>";
			prev_end = end;
		}

		spd_set_synthesis_voice(data->synth, message.voice.utf8().get_data());
		spd_set_volume(data->synth, message.volume * 2 - 100);
		spd_set_voice_pitch(data->synth, (message.pitch - 1) * 100);
		float rate = 0;
		if (message.rate > 1.f) {
			rate = std::log10(MIN(message.rate, 2.5f)) / std::log10(2.5f) * 100;
		}
		else if (message.rate < 1.f) {
			rate = std::log10(MAX(message.rate, 0.5f)) / std::log10(0.5f) * -100;
		}
		spd_set_voice_rate(data->synth, rate);
		spd_set_data_mode(data->synth, SPD_DATA_SSML);
		data->last_msg_id = spd_say(data->synth, SPD_TEXT, text.utf8().get_data());
		data->ids[data->last_msg_id] = message.id;
		DisplayServer::tts_post_utterance_event(
			DisplayServerEnums::TTS_UTTERANCE_STARTED, message.id);

		data->queue.pop_front();
		data->speaking = true;
	}
}

bool TTS_Linux::is_speaking()
{
	return data ? data->speaking : false;
}

bool TTS_Linux::is_paused()
{
	return data ? data->paused : false;
}

void TTS_Linux::speak(const String& p_text, const String& p_voice, int p_volume, float p_pitch,
	float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	ERR_FAIL_NULL(data);
	MutexLock lock(data->mutex);

	ERR_FAIL_NULL(data->synth);
	if (p_interrupt) {
		stop();
	}

	if (p_text.is_empty()) {
		DisplayServer::tts_post_utterance_event(
			DisplayServerEnums::TTS_UTTERANCE_CANCELED, p_utterance_id);
		return;
	}

	TTSUtterance message;
	message.text = p_text;
	message.voice = p_voice;
	message.volume = CLAMP(p_volume, 0, 100);
	message.pitch = CLAMP(p_pitch, 0.f, 2.f);
	message.rate = CLAMP(p_rate, 0.1f, 10.f);
	message.id = p_utterance_id;
	data->queue.push_back(message);

	if (is_paused()) {
		resume();
	}
	else {
		_speech_event(0, (int)SPD_EVENT_BEGIN);
	}
}

void TTS_Linux::pause()
{
	ERR_FAIL_NULL(data);
	MutexLock lock(data->mutex);

	ERR_FAIL_NULL(data->synth);
	if (spd_pause(data->synth) == 0) {
		data->paused = true;
	}
}

void TTS_Linux::resume()
{
	ERR_FAIL_NULL(data);
	MutexLock lock(data->mutex);

	ERR_FAIL_NULL(data->synth);
	spd_resume(data->synth);
	data->paused = false;
}

void TTS_Linux::stop()
{
	ERR_FAIL_NULL(data);
	MutexLock lock(data->mutex);

	ERR_FAIL_NULL(data->synth);
	for (TTSUtterance& message : data->queue) {
		DisplayServer::tts_post_utterance_event(
			DisplayServerEnums::TTS_UTTERANCE_CANCELED, message.id);
	}
	if ((data->last_msg_id != -1) && data->ids.has(data->last_msg_id)) {
		DisplayServer::tts_post_utterance_event(
			DisplayServerEnums::TTS_UTTERANCE_CANCELED, data->ids[data->last_msg_id]);
	}
	data->queue.clear();
	data->ids.clear();
	data->last_msg_id = -1;
	spd_cancel(data->synth);
	spd_resume(data->synth);
	data->speaking = false;
	data->paused = false;
}

void TTS_Linux::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
	// Speech Dispatcher init can be slow, it might wait for helper process to start on background,
	// so run it in the thread.
	data->init_thread.start(speech_init_thread_func, nullptr);
}

void TTS_Linux::finalize()
{
	if (!data) {
		return;
	}
	data->init_thread.wait_to_finish();
	if (data->synth) {
		spd_close(data->synth);
	}
	memdelete(data);
	data = nullptr;
}

void TTS_Linux::speech_init_thread_func(void* p_userdata) {}
