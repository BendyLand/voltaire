/**************************************************************************/
/*  os.h                                                                  */
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

#include <cstdlib>
#include "core/config/engine.h"
#include "core/io/logger.h"
#include "core/io/remote_filesystem_client.h"
#include "core/os/process_id.h"
#include "core/os/time_enums.h"
#include "core/string/ustring.h"
#include "core/templates/list.h"
#include "core/templates/vector.h"
#include "core/types.h"

class MainLoop;

class OS final
{
public:
	OS() = delete;
	~OS() = delete;
	OS(const OS&) = delete;
	OS& operator=(const OS&) = delete;

	typedef void (*ImeCallback)(void* p_inp, const String& p_text, Point2 p_selection);
	typedef bool (*HasServerFeatureCallback)(const String& p_feature);

	enum RenderThreadMode
	{
		RENDER_THREAD_UNSAFE,
		RENDER_THREAD_SAFE,
		RENDER_SEPARATE_THREAD,
	};

	enum StdHandleType
	{
		STD_HANDLE_INVALID,
		STD_HANDLE_CONSOLE,
		STD_HANDLE_FILE,
		STD_HANDLE_PIPE,
		STD_HANDLE_UNKNOWN,
	};

	enum RenderingSource
	{
		RENDERING_SOURCE_DEFAULT,
		RENDERING_SOURCE_PROJECT_SETTING,
		RENDERING_SOURCE_COMMANDLINE,
		RENDERING_SOURCE_FALLBACK
	};

	enum PlatformString
	{
		PLATFORM_STRING_FILE_MANAGER_OPEN,
		PLATFORM_STRING_FILE_MANAGER_SHOW,
	};

	enum PreferredTextureFormat
	{
		PREFERRED_TEXTURE_FORMAT_S3TC_BPTC,
		PREFERRED_TEXTURE_FORMAT_ETC2_ASTC
	};

	enum SystemDir
	{
		SYSTEM_DIR_DESKTOP,
		SYSTEM_DIR_DCIM,
		SYSTEM_DIR_DOCUMENTS,
		SYSTEM_DIR_DOWNLOADS,
		SYSTEM_DIR_MOVIES,
		SYSTEM_DIR_MUSIC,
		SYSTEM_DIR_PICTURES,
		SYSTEM_DIR_RINGTONES,
	};

	struct DateTime
	{
		int64_t year;
		Month month;
		uint8_t day;
		Weekday weekday;
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
		bool dst;
	};

	struct TimeZoneInfo
	{
		int bias;
		String name;
	};

	struct GDExtensionData
	{
		bool also_set_library_path = false;
		String* r_resolved_path = nullptr;
		bool generate_temp_files = false;
		Vector<String>* library_dependencies = nullptr;
	};

private:
	static inline uint64_t target_ticks = 0;
	static inline String _execpath;
	static inline List<String> _cmdline;
	static inline List<String> _user_args;
	static inline bool _keep_screen_on = true;
	static inline bool low_processor_usage_mode = false;
	static inline int low_processor_usage_mode_sleep_usec = 10000;
	static inline bool _delta_smoothing_enabled = false;
	static inline bool _verbose_stdout = false;
	static inline bool _debug_stdout = false;
	static inline String _local_clipboard;
	static inline int _exit_code = EXIT_SUCCESS;
	static inline bool _allow_hidpi = false;
	static inline bool _allow_layered = false;
	static inline bool _stdout_enabled = true;
	static inline bool _stderr_enabled = true;
	static inline bool _writing_movie = false;
	static inline bool _in_editor = false;
	static inline bool _embedded_in_editor = false;

	static inline bool restart_on_exit = false;
	static inline List<String> restart_commandline;

	static inline String _current_rendering_driver_name = "";
	static inline RenderingSource _current_rendering_driver_name_source = RENDERING_SOURCE_DEFAULT;
	static inline String _current_rendering_method;
	static inline RenderingSource _current_rendering_method_source = RENDERING_SOURCE_DEFAULT;
	static inline bool _is_gles_over_gl = false;

	static inline RemoteFilesystemClient default_rfs;

	// Benchmark data tracking
	static inline bool use_benchmark = false;
	static inline String benchmark_file;
	static inline HashMap<Pair<String, String>, uint64_t> benchmark_marks_from;
	static inline HashMap<Pair<String, String>, double> benchmark_marks_final;

	static inline HasServerFeatureCallback has_server_feature_callback = nullptr;
	static inline bool _separate_thread_render = false;
	static inline bool _silent_crash_handler = false;

	static bool _check_internal_feature_support(const String& p_feature);

	friend class Main;
	friend int test_main(int p_argc, char* p_argv[]);

public:
	static void _set_logger(CompositeLogger* p_logger);
	static inline bool _initialized = false;
	static inline CompositeLogger* _logger = nullptr;

	// Lifecycle
	static void initialize();
	static bool is_initialized();
	static void initialize_core();
	static void initialize_joypads();
	static void finalize();
	static void finalize_core();

	static void set_cmdline(
		const char* p_execpath, const List<String>& p_args, const List<String>& p_user_args);
	static void set_main_loop(MainLoop* p_main_loop);
	static void delete_main_loop();
	static MainLoop* get_main_loop();

	static bool prefer_meta_over_ctrl();

	// Rendering Driver & Settings
	static void set_current_rendering_driver_name(
		const String& p_driver_name, RenderingSource p_source)
	{
		_current_rendering_driver_name = p_driver_name;
		_current_rendering_driver_name_source = p_source;
	}

	static void set_current_rendering_method(const String& p_name, RenderingSource p_source)
	{
		_current_rendering_method = p_name;
		_current_rendering_method_source = p_source;
	}

	static void set_gles_over_gl(bool p_enabled) { _is_gles_over_gl = p_enabled; }

	static String get_current_rendering_driver_name() { return _current_rendering_driver_name; }

	static String get_current_rendering_method() { return _current_rendering_method; }

	static RenderingSource get_current_rendering_driver_name_source()
	{
		return _current_rendering_driver_name_source;
	}

	static RenderingSource get_current_rendering_method_source()
	{
		return _current_rendering_method_source;
	}

	static bool get_gles_over_gl() { return _is_gles_over_gl; }

	static Vector<String> get_video_adapter_driver_info();
	static bool get_user_prefers_integrated_gpu();

	// Output & Logging
	static void print(const char* p_format, ...);
	static void print_rich(const char* p_format, ...);
	static void printerr(const char* p_format, ...);

	static String get_stdin_string(int64_t p_buffer_size = 1024);
	static Vector<uint8_t> get_stdin_buffer(int64_t p_buffer_size = 1024);

	static StdHandleType get_stdin_type();
	static StdHandleType get_stdout_type();
	static StdHandleType get_stderr_type();

	static Error get_entropy(uint8_t* r_buffer, int p_bytes);
	static String get_system_ca_certificates();

	static Vector<String> get_connected_midi_inputs();
	static void open_midi_inputs();
	static void close_midi_inputs();

	static Rect2 calculate_boot_screen_rect(
		const Size2& p_window_size, const Size2& p_imgrect_size);
	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	// Dynamic Libraries / GDExtension
	static Error open_dynamic_library(
		const String& p_path, void*& p_library_handle, GDExtensionData* p_data = nullptr);
	static Error close_dynamic_library(void* p_library_handle);
	static Error get_dynamic_library_symbol_handle(void* p_library_handle, const String& p_name,
		void*& p_symbol_handle, bool p_optional = false);

	// CPU / Power Management
	static void set_low_processor_usage_mode(bool p_enabled);
	static bool is_in_low_processor_usage_mode();
	static void set_low_processor_usage_mode_sleep_usec(int p_usec);
	static int get_low_processor_usage_mode_sleep_usec();

	static void set_delta_smoothing(bool p_enabled);
	static bool is_delta_smoothing_enabled();

	// System Fonts
	static Vector<String> get_system_fonts();
	static String get_system_font_path(
		const String& p_font_name, int p_weight = 400, int p_stretch = 100, bool p_italic = false);
	static Vector<String> get_system_font_path_for_text(const String& p_font_name,
		const String& p_text, const String& p_locale = String(), const String& p_script = String(),
		int p_weight = 400, int p_stretch = 100, bool p_italic = false);

	// Process & Execution
	static String get_executable_path();
	static Error execute(const String& p_path, const List<String>& p_arguments,
		String* r_pipe = nullptr, int* r_exitcode = nullptr, bool p_read_stderr = false,
		Mutex* p_pipe_mutex = nullptr, bool p_open_console = false);
	static Error create_process(const String& p_path, const List<String>& p_arguments,
		ProcessID* r_child_id = nullptr, bool p_open_console = false);

	static Error create_instance(const List<String>& p_arguments, ProcessID* r_child_id = nullptr)
	{
		return create_process(get_executable_path(), p_arguments, r_child_id);
	}

	static Error open_with_program(const String& p_program_path, const List<String>& p_paths)
	{
		return create_process(p_program_path, p_paths);
	}

	static Error kill(const ProcessID& p_pid);
	static int get_process_id();
	static bool is_process_running(const ProcessID& p_pid);
	static int get_process_exit_code(const ProcessID& p_pid);

	static void vibrate_handheld(int p_duration_ms = 500, float p_amplitude = -1.0);

	static Error shell_open(const String& p_uri);
	static Error shell_show_in_file_manager(String p_path, bool p_open_folder = true);
	static Error set_cwd(const String& p_cwd);
	static String get_cwd();

	// Environment
	static bool has_environment(const String& p_var);
	static String get_environment(const String& p_var);
	static void set_environment(const String& p_var, const String& p_value);
	static void unset_environment(const String& p_var);
	static void load_shell_environment();

	// Platform Metadata
	static String get_name();
	static String get_identifier();
	static String get_distribution_name();
	static String get_version();

	static String get_version_alias() { return get_version(); }

	static List<String> get_cmdline_args() { return List<String>(_cmdline); }

	static List<String> get_cmdline_user_args() { return List<String>(_user_args); }

	static List<String> get_cmdline_platform_args();

	static String get_model_name();

	static bool is_layered_allowed() { return _allow_layered; }

	static bool is_hidpi_allowed() { return _allow_hidpi; }

	static void ensure_user_data_dir();
	static void yield();

	// Time & Timing
	static DateTime get_datetime(bool p_utc = false);
	static TimeZoneInfo get_time_zone_info();
	static double get_unix_time();

	static void delay_usec(uint32_t p_usec);
	static void add_frame_delay(bool p_can_draw, bool p_wake_for_events);
	static uint64_t get_frame_delay(bool p_can_draw);

	static uint64_t get_ticks_usec();
	static uint64_t get_ticks_msec();

	static bool is_userfs_persistent() { return true; }

	// Diagnostics & Capabilities
	static bool is_stdout_verbose();
	static bool is_stdout_debug_enabled();
	static bool is_stdout_enabled();
	static bool is_stderr_enabled();
	static void set_stdout_enabled(bool p_enabled);
	static void set_stderr_enabled(bool p_enabled);

	static void set_crash_handler_silent() { _silent_crash_handler = true; }

	static bool is_crash_handler_silent() { return _silent_crash_handler; }

	static String multibyte_to_string(const String& p_encoding, const Vector<uint8_t>& p_array);
	static Vector<uint8_t> string_to_multibyte(const String& p_encoding, const String& p_string);

	static void disable_crash_handler();
	static bool is_disable_crash_handler();
	static void initialize_debugging();

	static uint64_t get_static_memory_usage();
	static uint64_t get_static_memory_peak_usage();

	static bool is_separate_thread_rendering_enabled() { return _separate_thread_render; }

	// Localization & Paths
	static String get_locale();

	static Vector<String> get_preferred_locales() { return Vector<String>{get_locale()}; }

	static String get_locale_language();

	static uint64_t get_embedded_pck_offset();
	static String get_safe_dir_name(const String& p_dir_name, bool p_allow_paths = false);
	static String get_godot_dir_name();

	static String get_data_path();
	static String get_config_path();
	static String get_cache_path();
	static String get_temp_path();
	static String get_bundle_resource_dir();
	static String get_bundle_icon_path();
	static String get_bundle_icon_name();

	static String get_user_data_dir(const String& p_user_dir);
	static String get_user_data_dir();
	static String get_resource_dir();

	static String get_system_dir(SystemDir p_dir, bool p_shared_storage = true);
	static String expand_path(const String& p_path);

	static Error move_to_trash(const String& p_path);

	static void create_lock_file();
	static void remove_lock_file();

	static int get_exit_code();
	static void set_exit_code(int p_code);

	static int get_processor_count();
	static String get_processor_name();

	static int get_default_thread_pool_size() { return get_processor_count(); }

	static String get_unique_id();
	static bool has_feature(const String& p_feature);
	static bool is_sandboxed();

	static void set_has_server_feature_callback(HasServerFeatureCallback p_callback);

	static void set_restart_on_exit(bool p_restart, const List<String>& p_restart_arguments);
	static bool is_restart_on_exit_set();
	static List<String> get_restart_on_exit_arguments();

	static bool request_permission(const String& p_name);
	static bool request_permissions();
	static Vector<String> get_granted_permissions();
	static void revoke_granted_permissions();

	// Benchmark
	static void set_use_benchmark(bool p_use_benchmark);
	static bool is_use_benchmark_set();
	static void set_benchmark_file(const String& p_benchmark_file);
	static String get_benchmark_file();
	static void benchmark_begin_measure(const String& p_context, const String& p_what);
	static void benchmark_end_measure(const String& p_context, const String& p_what);
	static void benchmark_dump();

	static Error setup_remote_filesystem(
		const String& p_server_host, int p_port, const String& p_password, String& r_project_path);

	static void add_logger(Logger* p_logger);

	static PreferredTextureFormat get_preferred_texture_format();
	static void load_platform_gdextensions();

	static String get_platform_string(PlatformString p_platform_string)
	{
		switch (p_platform_string) {
		case PlatformString::PLATFORM_STRING_FILE_MANAGER_OPEN:
			return ETR("Open in File Manager");
		case PlatformString::PLATFORM_STRING_FILE_MANAGER_SHOW:
			return ETR("Show in File Manager");
		default:
			return ETR("You shouldn't be here (os.h)");
		}
	}

#ifdef TOOLS_ENABLED
	static bool _test_create_rendering_device_and_gl(const String& p_display_driver);
	static bool _test_create_rendering_device(const String& p_display_driver);
#endif
};

