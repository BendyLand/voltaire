/**************************************************************************/
/*  tts_windows.cpp                                                       */
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

#include "platform/windows/os_windows.h"


#ifdef WINRT_ENABLED
#include "platform/windows/os_windows.h"
#endif

bool TTS_Windows::is_speaking()
{
	if (data && data->driver) {
		return data->driver->is_speaking();
	}
	return false;
}

bool TTS_Windows::is_paused()
{
	if (data && data->driver) {
		return data->driver->is_paused();
	}
	return false;
}

Array TTS_Windows::get_voices()
{
	if (data && data->driver) {
		return data->driver->get_voices();
	}
	return Array();
}

void TTS_Windows::speak(const String& p_text, const String& p_voice, int p_volume, float p_pitch,
	float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	if (data && data->driver) {
		data->driver->speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
	}
}

void TTS_Windows::pause()
{
	if (data && data->driver) {
		data->driver->pause();
	}
}

void TTS_Windows::resume()
{
	if (data && data->driver) {
		data->driver->resume();
	}
}

void TTS_Windows::stop()
{
	if (data && data->driver) {
		data->driver->stop();
	}
}

void TTS_Windows::process_events()
{
	if (data && data->driver) {
		data->driver->process_events();
	}
}

void TTS_Windows::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
#ifdef WINRT_ENABLED
	// Try OneCore driver.
	if (!data->driver) {
		data->driver = memnew(TTSDriverOneCore);
		if (!data->driver->init()) {
			memdelete(data->driver);
			data->driver = nullptr;
		}
	}
#endif
	// Try SAPI driver.
	if (!data->driver) {
		data->driver = memnew(TTSDriverSAPI);
		if (!data->driver->init()) {
			memdelete(data->driver);
			data->driver = nullptr;
		}
	}
}

void TTS_Windows::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}
