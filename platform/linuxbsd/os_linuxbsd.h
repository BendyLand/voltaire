/**************************************************************************/
/*  os_linuxbsd.h                                                         */
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

#include "core/input/input_event.h"
#include "core/os/os.h"
// --- Folded from platform/linuxbsd/crash_handler_linuxbsd.h ---
class CrashHandler {
	bool disabled;

public:
	void initialize();

	void disable();
	bool is_disabled() const { return disabled; }

	CrashHandler();
	~CrashHandler();
};

// --- End platform/linuxbsd/crash_handler_linuxbsd.h ---
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

#ifdef PULSEAUDIO_ENABLED
	static inline AudioDriverPulseAudio driver_pulseaudio;
#endif

	static inline CrashHandler crash_handler;

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

	static void delete_main_loop();

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

// --- Folded from platform/linuxbsd/freedesktop_at_spi_monitor.h ---
#ifdef DBUS_ENABLED

#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"

class FreeDesktopAtSPIMonitor {
private:
	Thread thread;

	SafeFlag exit_thread;
	SafeFlag ac_enabled;
	SafeFlag sr_enabled;
	SafeFlag supported;

	static void monitor_thread_func(void *p_userdata);

public:
	FreeDesktopAtSPIMonitor();
	~FreeDesktopAtSPIMonitor();

	bool is_supported() { return supported.is_set(); }
	bool is_active() { return sr_enabled.is_set() && ac_enabled.is_set(); }
};

#endif // DBUS_ENABLED

// --- End platform/linuxbsd/freedesktop_at_spi_monitor.h ---

// --- Folded from platform/linuxbsd/freedesktop_portal_desktop.h ---
#ifdef DBUS_ENABLED

#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/display/display_server_enums.h"
#include "core/types.h"

struct DBusMessage;
struct DBusConnection;
struct DBusMessageIter;

class FreeDesktopPortalDesktop
{
private:
	bool unsupported = false;

	enum ReadVariantType
	{
		VAR_TYPE_UINT32, // u
		VAR_TYPE_BOOL,	 // b
		VAR_TYPE_COLOR,	 // (ddd)
	};

	static bool try_parse_variant(
		DBusMessage* p_reply_message, ReadVariantType p_type, void* r_value);
	// Read a setting from org.freekdesktop.portal.Settings
	bool read_setting(
		const char* p_namespace, const char* p_key, ReadVariantType p_type, void* r_value);

	static void append_dbus_string(DBusMessageIter* p_iter, const String& p_string);
	static void append_dbus_dict_string(DBusMessageIter* p_iter, const String& p_key,
		const String& p_value, bool p_as_byte_array = false);
	static void append_dbus_dict_bool(DBusMessageIter* p_iter, const String& p_key, bool p_value);

	static Error make_request_token(String& r_token);
	bool send_request(DBusMessage* p_message, const String& r_token, String& r_response_path,
		String& r_response_filter);

	struct ColorPickerData
	{
		String filter;
		String path;
	};

	Mutex color_picker_mutex;
	Vector<ColorPickerData> color_pickers;

	struct FileDialogData
	{
		Vector<String> filter_names;
		HashMap<String, String> option_ids;
		DisplayServerEnums::WindowID prev_focus = DisplayServerEnums::INVALID_WINDOW_ID;
		String filter;
		String path;
		bool opt_in_cb = false;
	};

	Mutex file_dialog_mutex;
	Vector<FileDialogData> file_dialogs;
	Thread monitor_thread;
	SafeFlag monitor_thread_abort;
	DBusConnection* monitor_connection = nullptr;

	String theme_path;
	void _system_theme_changed_callback();
	bool _is_interface_supported(const char* p_iface, uint32_t p_minimum_version);

	Mutex inhibit_mutex;
	String inhibit_path;
	String inhibit_filter;

	static void _thread_monitor(void* p_ud);

public:
	FreeDesktopPortalDesktop();
	~FreeDesktopPortalDesktop();

	bool is_supported() { return !unsupported; }

	bool is_file_chooser_supported();
	bool is_settings_supported();
	bool is_screenshot_supported();
	bool is_inhibit_supported();

	void process_callbacks();

	// org.freedesktop.portal.Settings methods.

	// Retrieve the system's preferred color scheme.
	// 0: No preference or unknown.
	// 1: Prefer dark appearance.
	// 2: Prefer light appearance.
	uint32_t get_appearance_color_scheme();
	Color get_appearance_accent_color();

	// Retrieve high-contrast setting.
	// -1: Unknown.
	// 0: Disabled.
	// 1: Enabled.
	uint32_t get_high_contrast();

	// org.freedesktop.portal.Inhibit methods.
	bool inhibit(const String& p_xid);
	void uninhibit();
};

#endif // DBUS_ENABLED



// --- End platform/linuxbsd/freedesktop_portal_desktop.h ---

// --- Folded from platform/linuxbsd/freedesktop_screensaver.h ---
#ifdef DBUS_ENABLED

#include <cstdint>

class FreeDesktopScreenSaver {
private:
	uint32_t cookie = 0;
	bool unsupported = false;

public:
	FreeDesktopScreenSaver();
	void inhibit();
	void uninhibit();
};

#endif // DBUS_ENABLED

// --- End platform/linuxbsd/freedesktop_screensaver.h ---
