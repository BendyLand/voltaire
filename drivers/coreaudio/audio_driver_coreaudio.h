/**************************************************************************/
/*  audio_driver_coreaudio.h                                              */
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

#ifdef COREAUDIO_ENABLED

#include "servers/audio/audio_server.h"

#import <AudioUnit/AudioUnit.h>
#ifdef MACOS_ENABLED
#import <CoreAudio/AudioHardware.h>
#else
#import <AVFoundation/AVFoundation.h>
#endif

class AudioDriverCoreAudio : public AudioDriver {
	AudioComponentInstance audio_unit = nullptr;
	AudioComponentInstance input_unit = nullptr;

	bool active = false;
	Mutex mutex;

	String output_device_name = "Default";
	String input_device_name = "Default";

	int mix_rate = 0;
	int capture_mix_rate = 0;
	unsigned int channels = 2;
	unsigned int capture_channels = 2;
	unsigned int buffer_frames = 0;
	unsigned int capture_buffer_frames = 0;

	Vector<int32_t> samples_in;
	unsigned int buffer_size = 0;

#ifdef MACOS_ENABLED
	PackedStringArray _get_device_list(bool capture = false);
	void _set_device(const String &output_device, bool capture = false);

	static OSStatus input_device_address_cb(AudioObjectID inObjectID,
			UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,
			void *inClientData);

	static OSStatus output_device_address_cb(AudioObjectID inObjectID,
			UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,
			void *inClientData);
#endif

	static OSStatus output_callback(void *inRefCon,
			AudioUnitRenderActionFlags *ioActionFlags,
			const AudioTimeStamp *inTimeStamp,
			UInt32 inBusNumber, UInt32 inNumberFrames,
			AudioBufferList *ioData);

	static OSStatus input_callback(void *inRefCon,
			AudioUnitRenderActionFlags *ioActionFlags,
			const AudioTimeStamp *inTimeStamp,
			UInt32 inBusNumber, UInt32 inNumberFrames,
			AudioBufferList *ioData);

	Error init_input_device();
	void finish_input_device();

public:
	virtual const char *get_name() const override {
		return "CoreAudio";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual int get_input_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

#ifdef MACOS_ENABLED
	virtual PackedStringArray get_output_device_list() override;
	virtual String get_output_device() override;
	virtual void set_output_device(const String &p_name) override;

	virtual PackedStringArray get_input_device_list() override;
	virtual String get_input_device() override;
	virtual void set_input_device(const String &p_name) override;
#endif

	virtual Error input_start() override;
	virtual Error input_stop() override;

	bool try_lock();
	void stop();

	AudioDriverCoreAudio();
	~AudioDriverCoreAudio() {}
};

#endif // COREAUDIO_ENABLED

// --- Folded from drivers/apple/os_log_logger.h ---
#include "core/os/os.h"

#include <os/log.h>

/**
 * @brief Apple unified logging system integration for Godot Engine.
 */
class OsLogLogger : public Logger {
	os_log_t log;
	os_log_t error_log;
	os_log_t warning_log;
	os_log_t script_log;
	os_log_t shader_log;

public:
	void logv(const char *p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;
	void log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify = false, ErrorType p_type = ERR_ERROR, const Vector<Ref<ScriptBacktrace>> &p_script_backtraces = {}) override;

	/**
	 * @brief Constructs an OsLogLogger with the specified subsystem identifier, which is normally the bundle identifier.
	 */
	OsLogLogger(const char *p_subsystem);
};
// --- End drivers/apple/os_log_logger.h ---
