/**************************************************************************/
/*  midi_driver_alsamidi.cpp                                              */
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

#ifdef ALSAMIDI_ENABLED

#include "midi_driver_alsamidi.h"

#include "core/os/os.h"

#ifdef SOWRAP_ENABLED
#include "drivers/alsa/asound-so_wrap.h"
#else
#include <alsa/asoundlib.h>
#endif

#include <cerrno>

MIDIDriverALSAMidi::InputConnection::InputConnection(int p_device_index,
		snd_rawmidi_t *p_rawmidi) :
		parser(p_device_index), rawmidi_ptr(p_rawmidi) {}

void MIDIDriverALSAMidi::InputConnection::read() {
	int read_count;
	do {
		uint8_t buffer[32];
		read_count = snd_rawmidi_read(rawmidi_ptr, buffer, sizeof(buffer));

		if (read_count < 0) {
			if (read_count != -EAGAIN) {
				ERR_PRINT("snd_rawmidi_read error: " + String(snd_strerror(read_count)));
			}
		} else {
			for (int i = 0; i < read_count; i++) {
				parser.parse_fragment(buffer[i]);
			}
		}
	} while (read_count > 0);
}

void MIDIDriverALSAMidi::thread_func(void* p_udata)
{
	(void)p_udata;
	if (!data) {
		return;
	}

	while (!data->exit_thread.is_set()) {
		lock();
		for (InputConnection& conn : data->connected_inputs) {
			conn.read();
		}
		unlock();

		OS::delay_usec(1000);
	}
}

void MIDIDriverALSAMidi::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);
	data->exit_thread.clear();
}

void MIDIDriverALSAMidi::finalize()
{
	if (data == nullptr) {
		return;
	}
	close();
	memdelete(data);
	data = nullptr;
}

Error MIDIDriverALSAMidi::open()
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);

	void** hints;

	if (snd_device_name_hint(-1, "rawmidi", &hints) < 0) {
		return ERR_CANT_OPEN;
	}

	lock();
	int device_index = 0;
	for (void** h = hints; *h != nullptr; h++) {
		char* name = snd_device_name_get_hint(*h, "NAME");

		if (name != nullptr) {
			snd_rawmidi_t* midi_in;
			int ret = snd_rawmidi_open(&midi_in, nullptr, name, SND_RAWMIDI_NONBLOCK);
			if (ret >= 0) {
				// Get display name.
				snd_rawmidi_info_t* info;
				snd_rawmidi_info_malloc(&info);
				snd_rawmidi_info(midi_in, info);
				MIDIDriver::add_connected_input(snd_rawmidi_info_get_name(info));
				snd_rawmidi_info_free(info);

				data->connected_inputs.push_back(InputConnection(device_index, midi_in));
				// Only increment device_index for successfully connected devices.
				device_index++;
			}
		}

		if (name != nullptr) {
			free(name);
		}
	}
	snd_device_name_free_hint(hints);
	unlock();

	data->exit_thread.clear();
	data->thread.start(MIDIDriverALSAMidi::thread_func, nullptr);

	return OK;
}

void MIDIDriverALSAMidi::close()
{
	if (data == nullptr) {
		return;
	}

	data->exit_thread.set();
	if (data->thread.is_started()) {
		data->thread.wait_to_finish();
	}

	for (const InputConnection& conn : data->connected_inputs) {
		snd_rawmidi_close(conn.rawmidi_ptr);
	}

	data->connected_inputs.clear();
	MIDIDriver::clear_connected_inputs();
}

void MIDIDriverALSAMidi::lock()
{
	if (data) {
		data->mutex.lock();
	}
}

void MIDIDriverALSAMidi::unlock()
{
	if (data) {
		data->mutex.unlock();
	}
}

#endif // ALSAMIDI_ENABLED
