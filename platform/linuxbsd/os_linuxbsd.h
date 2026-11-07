/**************************************************************************/
/*  os_linuxbsd.h                                                         */
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
/* The above copyright notice and this permission notice shall be          */
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

#include "core/input/input_event.h"
#include "core/os/os.h"
#include "crash_handler_linuxbsd.h"
#include "drivers/alsa/audio_driver_alsa.h"
#include "drivers/alsamidi/midi_driver_alsamidi.h"
#include "drivers/pulseaudio/audio_driver_pulseaudio.h"
#include "drivers/unix/os_unix.h"

class JoypadSDL;

#ifdef FONTCONFIG_ENABLED
typedef struct _FcConfig FcConfig;
typedef struct _FcObjectSet FcObjectSet;
#endif

class OS_LinuxBSD final
{
public:
	OS_LinuxBSD() = delete;
	~OS_LinuxBSD() = delete;
	OS_LinuxBSD(const OS_LinuxBSD&) = delete;
	OS_LinuxBSD& operator=(const OS_LinuxBSD&) = delete;

private:
#ifdef FONTCONFIG_ENABLED
	static inline bool font_config_initialized = false;
	static inline FcConfig* config = nullptr;
	static inline FcObjectSet* object_set = nullptr;

	static int _weight_to_fc(int p_weight);
	static int _stretch_to_fc(int p_stretch);
#endif

#ifdef SDL_ENABLED
	static inline JoypadSDL* joypad_sdl = nullptr;
#endif

#ifdef ALSA_ENABLED
	static inline AudioDriverALSA driver_alsa;
#endif

#ifdef ALSAMIDI_ENABLED
	static inline MIDIDriverALSAMidi driver_alsamidi;
#endif

#ifdef PULSEAUDIO_ENABLED
	static inline AudioDriverPulseAudio driver_pulseaudio;
#endif

	static inline CrashHandler crash_handler;
	static inline MainLoop* main_loop = nullptr;

	static String get_systemd_os_release_info_value(const String& key);

	static Vector<String> lspci_device_filter(Vector<String> vendor_device_id_mapping,
		String class_suffix, String check_column, String whitelist);
	static Vector<String> lspci_get_device_value(
		Vector<String> vendor_device_id_mapping, String check_column, String blacklist);

	static inline String system_dir_desktop_cache;

public:
	static void initialize();
	static void initialize_core();
	static void finalize();
	static void finalize_core();

	static void initialize_joypads();

	static void set_main_loop(MainLoop* p_main_loop);
	static void delete_main_loop();
	static MainLoop* get_main_loop();

	static String get_identifier();
	static String get_name();
	static String get_distribution_name();
	static String get_version();

	static Vector<String> get_video_adapter_driver_info();

	static uint64_t get_embedded_pck_offset();

	static Vector<String> get_system_fonts();
	static String get_system_font_path(const String& p_font_name, int p_weight = 400,
		int p_stretch = 100, bool p_italic = false);
	static Vector<String> get_system_font_path_for_text(const String& p_font_name,
		const String& p_text, const String& p_locale = String(), const String& p_script = String(),
		int p_weight = 400, int p_stretch = 100, bool p_italic = false);

	static String get_config_path();
	static String get_data_path();
	static String get_cache_path();

	static String get_system_dir(OS::SystemDir p_dir, bool p_shared_storage = true);

	static Error shell_open(const String& p_uri);

	static String get_unique_id();
	static String get_processor_name();

	static bool is_sandboxed();

	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	static bool _check_internal_feature_support(const String& p_feature);

	static void run();

	static void disable_crash_handler();
	static bool is_disable_crash_handler();

	static Error move_to_trash(const String& p_path);

	static String get_system_ca_certificates();

#ifdef TOOLS_ENABLED
	static bool _test_create_rendering_device_and_gl(const String& p_display_driver);
	static bool _test_create_rendering_device(const String& p_display_driver);
#endif
};
