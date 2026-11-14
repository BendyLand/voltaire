/**************************************************************************/
/*  os_windows.h                                                          */
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

#ifdef _WIN32
#include "core/input/input_event.h"
#include "core/os/os.h"
#include "crash_handler_windows.h"
#include "drivers/wasapi/audio_driver_wasapi.h"
#include "drivers/winmidi/midi_driver_winmidi.h"

#ifdef XAUDIO2_ENABLED
#include "drivers/xaudio2/audio_driver_xaudio2.h"
#endif

#include <dwrite.h>
#include <dwrite_2.h>
#include <io.h>
#include <shellapi.h>
#include <windows.h>
#include <windowsx.h>

#ifdef DEBUG_ENABLED
// Forward error messages to OutputDebugString
#define WINDOWS_DEBUG_OUTPUT_ENABLED
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x)                                                                            \
	if (x != nullptr) {                                                                            \
		x->Release();                                                                              \
		x = nullptr;                                                                               \
	}
#endif

template <typename T> class ComAutoreleaseRef
{
public:
	T* reference = nullptr;

	_FORCE_INLINE_ T* operator->() { return reference; }

	_FORCE_INLINE_ const T* operator->() const { return reference; }

	_FORCE_INLINE_ T* operator*() { return reference; }

	_FORCE_INLINE_ const T* operator*() const { return reference; }

	_FORCE_INLINE_ bool is_valid() const { return reference != nullptr; }

	_FORCE_INLINE_ bool is_null() const { return reference == nullptr; }

	ComAutoreleaseRef() {}

	ComAutoreleaseRef(T* p_ref) { reference = p_ref; }

	~ComAutoreleaseRef()
	{
		if (reference != nullptr) {
			reference->Release();
			reference = nullptr;
		}
	}
};

class OS_Windows final
{
public:
	OS_Windows() = delete;
	~OS_Windows() = delete;
	OS_Windows(const OS_Windows&) = delete;
	OS_Windows& operator=(const OS_Windows&) = delete;

	struct ProcessInfo
	{
		STARTUPINFOEX si;
		PROCESS_INFORMATION pi;
		mutable bool is_running = true;
		mutable int exit_code = -1;
	};

private:
	static inline uint64_t target_ticks = 0;
	static inline uint64_t ticks_start = 0;
	static inline uint64_t ticks_per_second = 0;
	static inline uint64_t delay_resolution = 1000;

	static inline HINSTANCE hInstance = nullptr;
	static inline MainLoop* main_loop = nullptr;

#ifdef WASAPI_ENABLED
	static inline AudioDriverWASAPI driver_wasapi;
#endif
#ifdef XAUDIO2_ENABLED
	static inline AudioDriverXAudio2 driver_xaudio2;
#endif
#ifdef WINMIDI_ENABLED
	static inline MIDIDriverWinMidi driver_midi;
#endif

	static inline CrashHandler crash_handler;

#ifdef WINDOWS_DEBUG_OUTPUT_ENABLED
	static inline ErrorHandlerList error_handlers;
#endif

	static inline HWND main_window = nullptr;

	static inline IDWriteFactory* dwrite_factory = nullptr;
	static inline IDWriteFactory2* dwrite_factory2 = nullptr;
	static inline IDWriteFontCollection* font_collection = nullptr;
	static inline IDWriteFontFallback* system_font_fallback = nullptr;

	static inline bool dwrite_init = false;
	static inline bool dwrite2_init = false;

	static inline HashMap<void*, String> temp_libraries;

	static void _remove_temp_library(void* p_library_handle);
	static String _get_default_fontname(const String& p_font_name);
	static DWRITE_FONT_WEIGHT _weight_to_dw(int p_weight);
	static DWRITE_FONT_STRETCH _stretch_to_dw(int p_stretch);

	static bool is_using_con_wrapper();

	static inline HashMap<String, int> encodings;
	static void _init_encodings();

	static Vector<String> _get_video_adapter_driver_info_reg(const String& p_name);
	static Vector<String> _get_video_adapter_driver_info_wmi(const String& p_name);

	static String _quote_command_line_argument(const String& p_text);

	static inline HashMap<ProcessID, ProcessInfo>* process_map = nullptr;
	static inline Mutex process_map_mutex;

public:
	static void initialize();
	static void set_main_loop(MainLoop* p_main_loop);
	static void delete_main_loop();
	static void finalize();
	static void finalize_core();

	static String get_stdin_string(int64_t p_buffer_size = 1024);
	static PackedByteArray get_stdin_buffer(int64_t p_buffer_size = 1024);
	static OS::StdHandleType get_stdin_type();
	static OS::StdHandleType get_stdout_type();
	static OS::StdHandleType get_stderr_type();

	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	static Error get_entropy(uint8_t* r_buffer, int p_bytes);

	static Error open_dynamic_library(
		const String& p_path, void*& p_library_handle, OS::GDExtensionData* p_data = nullptr);
	static Error close_dynamic_library(void* p_library_handle);
	static Error get_dynamic_library_symbol_handle(void* p_library_handle, const String& p_name,
		void*& p_symbol_handle, bool p_optional = false);

	static MainLoop* get_main_loop();

	static String get_name();
	static String get_distribution_name();
	static String get_version();
	static String get_version_alias();

	static Vector<String> get_video_adapter_driver_info();
	static bool get_user_prefers_integrated_gpu();

	static void initialize_joypads() {}

	static OS::DateTime get_datetime(bool p_utc);
	static OS::TimeZoneInfo get_time_zone_info();
	static double get_unix_time();

	static Error set_cwd(const String& p_cwd);
	static String get_cwd();

	static void add_frame_delay(bool p_can_draw, bool p_wake_for_events);
	static void delay_usec(uint32_t p_usec);
	static uint64_t get_ticks_usec();

	static Dictionary get_memory_info();

	static Error execute(const String& p_path, const List<String>& p_arguments,
		String* r_pipe = nullptr, int* r_exitcode = nullptr, bool read_stderr = false,
		Mutex* p_pipe_mutex = nullptr, bool p_open_console = false);
	static Dictionary execute_with_pipe(
		const String& p_path, const List<String>& p_arguments, bool p_blocking = true);
	static Error create_process(const String& p_path, const List<String>& p_arguments,
		ProcessID* r_child_id = nullptr, bool p_open_console = false);
	static Error kill(const ProcessID& p_pid);
	static int get_process_id();
	static bool is_process_running(const ProcessID& p_pid);
	static int get_process_exit_code(const ProcessID& p_pid);

	static bool has_environment(const String& p_var);
	static String get_environment(const String& p_var);
	static void set_environment(const String& p_var, const String& p_value);
	static void unset_environment(const String& p_var);

	static Vector<String> get_system_fonts();
	static String get_system_font_path(
		const String& p_font_name, int p_weight = 400, int p_stretch = 100, bool p_italic = false);
	static Vector<String> get_system_font_path_for_text(const String& p_font_name,
		const String& p_text, const String& p_locale = String(), const String& p_script = String(),
		int p_weight = 400, int p_stretch = 100, bool p_italic = false);

	static String get_executable_path();

	static String get_locale();
	static Vector<String> get_preferred_locales();

	static String get_processor_name();
	static String get_model_name();

	static uint64_t get_embedded_pck_offset();

	static String get_config_path();
	static String get_data_path();
	static String get_cache_path();
	static String get_temp_path();
	static String get_godot_dir_name();

	static String get_system_dir(OS::SystemDir p_dir, bool p_shared_storage = true);
	static String get_user_data_dir(const String& p_user_dir);

	static String expand_path(const String& p_path);

	static String get_unique_id();

	static Error shell_open(const String& p_uri);
	static Error shell_show_in_file_manager(String p_path, bool p_open_folder);

	static void run();

	static bool _check_internal_feature_support(const String& p_feature);

	static String multibyte_to_string(const String& p_encoding, const PackedByteArray& p_array);
	static PackedByteArray string_to_multibyte(const String& p_encoding, const String& p_string);

	static void disable_crash_handler();
	static bool is_disable_crash_handler();
	static void initialize_debugging();

	static Error move_to_trash(const String& p_path);

	static String get_system_ca_certificates();

	static void set_main_window(HWND p_main_window) { main_window = p_main_window; }

	static String get_platform_string(OS::PlatformString p_platform_string)
	{
		switch (p_platform_string) {
		case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_OPEN:
			return ETR("Open in File Explorer");
		case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_SHOW:
			return ETR("Show in File Explorer");
		default:
			return OS::get_platform_string(p_platform_string);
		}
	}

#ifdef TOOLS_ENABLED
	static bool _test_create_rendering_device_and_gl(const String& p_display_driver);
	static bool _test_create_rendering_device(const String& p_display_driver);
#endif

	static void set_hinstance(HINSTANCE p_hInstance) { hInstance = p_hInstance; }

	static HINSTANCE get_hinstance() { return hInstance; }
};

#endif // _WIN32

