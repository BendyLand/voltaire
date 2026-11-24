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
// --- Folded from core/config/engine.h ---
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"

class Engine
{
private:
	friend class Main;

	static inline uint64_t frames_drawn = 0;
	static inline uint32_t _frame_delay = 0;
	static inline uint64_t _frame_ticks = 0;
	static inline double _process_step = 0;

	static inline int ips = 60;
	static inline int user_ips = 60;
	static inline double physics_jitter_fix = 0.5;
	static inline double _fps = 1;
	static inline int _max_fps = 0;
	static inline int _audio_output_latency = 0;
	static inline double _time_scale = 1.0;
	static inline double _game_time_scale = 1.0;
	static inline double _user_time_scale = 1.0;
	static inline uint64_t _physics_frames = 0;
	static inline int max_physics_steps_per_frame = 8;
	static inline int max_user_physics_steps_per_frame = 8;
	static inline double _physics_interpolation_fraction = 0.0f;
	static inline bool abort_on_gpu_errors = false;
	static inline bool use_validation_layers = false;
	static inline bool generate_spirv_debug_info = false;
	static inline bool extra_gpu_memory_tracking = false;
#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
	static inline bool accurate_breadcrumbs = false;
#endif
	static inline int32_t gpu_idx = -1;

	static inline uint64_t _process_frames = 0;
	static inline bool _in_physics = false;

	static inline bool editor_hint = false;
	static inline bool project_manager_hint = false;
	static inline bool extension_reloading = false;
	static inline bool embedded_in_editor = false;
	static inline bool recovery_mode_hint = false;

	static inline bool _print_header = true;

	static inline String write_movie_path;
	static inline String shader_cache_path;

	static constexpr int SERVER_SYNC_FRAME_COUNT_WARNING = 5;
	static inline int server_syncs = 0;
	static inline bool frame_server_synced = false;

	static inline bool freeze_time_scale = false;

protected:
	static void _update_time_scale();

public:
	static void set_physics_ticks_per_second(int p_ips);
	static int get_physics_ticks_per_second();
	static int get_user_physics_ticks_per_second();

	static void set_max_physics_steps_per_frame(int p_max_physics_steps);
	static int get_max_physics_steps_per_frame();
	static int get_user_max_physics_steps_per_frame();

	static void set_physics_jitter_fix(double p_threshold);
	static double get_physics_jitter_fix();

	static void set_max_fps(int p_fps);
	static int get_max_fps();

	static void set_audio_output_latency(int p_msec);
	static int get_audio_output_latency();

	static double get_frames_per_second() { return _fps; }

	static uint64_t get_frames_drawn();

	static uint64_t get_physics_frames() { return _physics_frames; }

	static uint64_t get_process_frames() { return _process_frames; }

	static bool is_in_physics_frame() { return _in_physics; }

	static uint64_t get_frame_ticks() { return _frame_ticks; }

	static double get_process_step() { return _process_step; }

	static double get_physics_interpolation_fraction() { return _physics_interpolation_fraction; }

	static void set_time_scale(double p_scale);
	static double get_time_scale();
	static void set_user_time_scale(double p_scale);
	static double get_effective_time_scale();
	static double get_unfrozen_time_scale();

	static void set_print_to_stdout(bool p_enabled);
	static bool is_printing_to_stdout();

	static void set_print_error_messages(bool p_enabled);
	static bool is_printing_error_messages();
	static void print_header(const String& p_string);
	static void print_header_rich(const String& p_string);

	static void set_frame_delay(uint32_t p_msec);
	static uint32_t get_frame_delay();

#ifdef TOOLS_ENABLED
	_FORCE_INLINE_ static void set_editor_hint(bool p_enabled) { editor_hint = p_enabled; }

	_FORCE_INLINE_ static bool is_editor_hint() { return editor_hint; }

	_FORCE_INLINE_ static void set_project_manager_hint(bool p_enabled)
	{
		project_manager_hint = p_enabled;
	}

	_FORCE_INLINE_ static bool is_project_manager_hint() { return project_manager_hint; }

	_FORCE_INLINE_ static void set_extension_reloading_enabled(bool p_enabled)
	{
		extension_reloading = p_enabled;
	}

	_FORCE_INLINE_ static bool is_extension_reloading_enabled() { return extension_reloading; }

	_FORCE_INLINE_ static void set_recovery_mode_hint(bool p_enabled)
	{
		recovery_mode_hint = p_enabled;
	}

	_FORCE_INLINE_ static bool is_recovery_mode_hint() { return recovery_mode_hint; }
#else
	_FORCE_INLINE_ static void set_editor_hint(bool p_enabled) {}

	_FORCE_INLINE_ static bool is_editor_hint() { return false; }

	_FORCE_INLINE_ static void set_project_manager_hint(bool p_enabled) {}

	_FORCE_INLINE_ static bool is_project_manager_hint() { return false; }

	_FORCE_INLINE_ static void set_extension_reloading_enabled(bool p_enabled) {}

	_FORCE_INLINE_ static bool is_extension_reloading_enabled() { return false; }

	_FORCE_INLINE_ static void set_recovery_mode_hint(bool p_enabled) {}

	_FORCE_INLINE_ static bool is_recovery_mode_hint() { return false; }
#endif

	static String get_license_text();

	static void set_write_movie_path(const String& p_path);
	static String get_write_movie_path();

	static String get_architecture_name();

	static void set_shader_cache_path(const String& p_path);
	static String get_shader_cache_path();

	static bool is_abort_on_gpu_errors_enabled();
	static bool is_validation_layers_enabled();
	static bool is_generate_spirv_debug_info_enabled();
	static bool is_extra_gpu_memory_tracking_enabled();
#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
	static bool is_accurate_breadcrumbs_enabled();
#endif
	static int32_t get_gpu_index();

	static void increment_frames_drawn();
	static bool notify_frame_server_synced();

	static void set_freeze_time_scale(bool p_frozen);
	static void set_embedded_in_editor(bool p_enabled);
	static bool is_embedded_in_editor();

	Engine() = delete;
	~Engine() = delete;
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
};
// --- End core/config/engine.h ---
// --- Inlined/Folded from core/io/logger.h ---
#include <cstdarg>
#include "core/io/file_access.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "core/types.h"

class FileAccess;
class RegEx;

class Logger
{
protected:
	bool should_log(bool p_err);

	static inline bool _flush_stdout_on_print = true;

public:
	enum ErrorType
	{
		ERR_ERROR,
		ERR_WARNING,
		ERR_SCRIPT,
		ERR_SHADER
	};

	static constexpr const char* error_type_string(ErrorType p_type)
	{
		switch (p_type) {
		case ERR_ERROR:
			return "ERROR";
		case ERR_WARNING:
			return "WARNING";
		case ERR_SCRIPT:
			return "SCRIPT ERROR";
		case ERR_SHADER:
			return "SHADER ERROR";
		}
		return "UNKNOWN ERROR";
	}

	static constexpr const char* error_type_indent(ErrorType p_type)
	{
		switch (p_type) {
		case ERR_ERROR:
			return "   ";
		case ERR_WARNING:
			return "     ";
		case ERR_SCRIPT:
			return "          ";
		case ERR_SHADER:
			return "          ";
		}
		return "           ";
	}

	static void set_flush_stdout_on_print(bool p_value);

	virtual void logv(
		const char* p_format, va_list p_list, bool p_err) _PRINTF_FORMAT_ATTRIBUTE_2_0 = 0;

	void logf(const char* p_format, ...) _PRINTF_FORMAT_ATTRIBUTE_2_3;
	void logf_error(const char* p_format, ...) _PRINTF_FORMAT_ATTRIBUTE_2_3;

	Logger() noexcept {}

	virtual ~Logger() {}
};

/**
 * Writes messages to stdout/stderr.
 */
class StdLogger : public Logger
{
public:
	virtual void logv(
		const char* p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;

	virtual ~StdLogger() {}
};

/**
 * Writes messages to the specified file.
If the file already exists, creates a copy (backup)
 * of it with timestamp appended to the file name. Maximum number of backups is configurable.
 * When maximum is reached, the oldest backups are erased. With the maximum being equal to 1,
 * it acts as a simple file logger.
 */
class RotatedFileLogger : public Logger
{
	String base_path;
	int max_files;

	Ref<FileAccess> file;

	void clear_old_backups();
	void rotate_file();

	Ref<RegEx> strip_ansi_regex;

public:
	explicit RotatedFileLogger(const String& p_base_path, int p_max_files = 10);

	virtual void logv(
		const char* p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;
};

class CompositeLogger : public Logger
{
	Vector<Logger*> loggers;

public:
	explicit CompositeLogger(const Vector<Logger*>& p_loggers);

	virtual void logv(
		const char* p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;

	void add_logger(Logger* p_logger);

	virtual ~CompositeLogger();
};
// --- Folded from core/io/remote_filesystem_client.h ---
#include "core/string/ustring.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"

class RemoteFilesystemClient {
	String cache_path;
	HashSet<String> validated_directories;

protected:
	String _get_cache_path() { return cache_path; }
	struct FileCache {
		String path; // Local path (as in "folder/to/file.png")
		uint64_t server_modified_time = 0; // MD5 checksum.
		uint64_t modified_time = 0;
	};
	virtual bool _is_configured() { return !cache_path.is_empty(); }
	// Can be re-implemented per platform. If so, feel free to ignore get_cache_path()
	virtual Vector<FileCache> _load_cache_file();
	virtual Error _store_file(const String &p_path, const LocalVector<uint8_t> &p_file, uint64_t &r_modified_time);
	virtual Error _remove_file(const String &p_path);
	virtual Error _store_cache_file(const Vector<FileCache> &p_cache);
	virtual Error _synchronize_with_server(const String &p_host, int p_port, const String &p_password, String &r_cache_path);

	virtual void _update_cache_path(String &r_cache_path);

public:
	Error synchronize_with_server(const String &p_host, int p_port, const String &p_password, String &r_cache_path);
	virtual ~RemoteFilesystemClient() {}
};

// --- End core/io/remote_filesystem_client.h ---
// --- Folded from core/os/process_id.h ---
#include <cstdint>

typedef int64_t ProcessID;
// --- End core/os/process_id.h ---
// --- Folded from core/os/time_enums.h ---
#include <cstdint>

enum Month {
	/// Start at 1 to follow Windows SYSTEMTIME structure
	/// https://msdn.microsoft.com/en-us/library/windows/desktop/ms724950(v=vs.85).aspx
	MONTH_JANUARY = 1,
	MONTH_FEBRUARY,
	MONTH_MARCH,
	MONTH_APRIL,
	MONTH_MAY,
	MONTH_JUNE,
	MONTH_JULY,
	MONTH_AUGUST,
	MONTH_SEPTEMBER,
	MONTH_OCTOBER,
	MONTH_NOVEMBER,
	MONTH_DECEMBER,
};

enum Weekday : uint8_t {
	WEEKDAY_SUNDAY,
	WEEKDAY_MONDAY,
	WEEKDAY_TUESDAY,
	WEEKDAY_WEDNESDAY,
	WEEKDAY_THURSDAY,
	WEEKDAY_FRIDAY,
	WEEKDAY_SATURDAY,
};

// --- End core/os/time_enums.h ---
#include "core/string/ustring.h"
#include "core/templates/list.h"
#include "core/templates/vector.h"
#include "core/types.h"

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
	static void delete_main_loop();

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

// --- Folded from core/os/midi_driver.h ---
#include "core/input/input_event.h"
#include "core/typedefs.h"
#include "core/types.h"

/**
 * Multi-Platform abstraction for accessing to MIDI.
 */

class MIDIDriver final
{
	static uint8_t last_received_message;
	static inline bool _is_bound = false;

	static void _default_initialize_backend() {}
	static void _default_finalize_backend() {}
	static Error _default_open() { return ERR_UNAVAILABLE; }
	static void _default_close() {}

	inline static void (*_initialize_backend)() = &_default_initialize_backend;
	inline static void (*_finalize_backend)() = &_default_finalize_backend;

public:
	struct Data
	{
		PackedStringArray connected_input_names;
	};

	static inline Data* data = nullptr;

	// Categories of message for parser logic.
	enum class MessageCategory
	{
		Data,
		Voice,
		SysExBegin,
		SystemCommon, // excluding System Exclusive Begin/End
		SysExEnd,
		RealTime,
	};

	// Convert midi data to InputEventMIDI and send it to Input.
	// p_data_len is the length of the buffer passed at p_data, this must be
	// at least equal to the data required by the passed message type, but
	// may be larger. Only the required data will be read.
	static void send_event(int p_device_index, uint8_t p_status, const uint8_t* p_data = nullptr,
		size_t p_data_len = 0);

	class Parser
	{
	public:
		Parser() = default;

		Parser(int p_device_index) : device_index{p_device_index} {}

		virtual ~Parser() = default;

		// Push a byte of MIDI stream. Any completed messages will be
		// forwarded to MIDIDriver::send_event.
		void parse_fragment(uint8_t p_fragment);

		static MessageCategory category(uint8_t p_midi_fragment);

		// If the byte is a Voice Message status byte return the contained
		// channel number, otherwise zero.
		static uint8_t channel(uint8_t p_status_byte);

		// If the byte is a status byte for a message with a fixed number of
		// additional data bytes, return the number expected, otherwise zero.
		static size_t expected_data(uint8_t p_status_byte);
		static size_t expected_data(MIDIMessage p_msg_type);

		// If the fragment is a status byte return the message type
		// represented, otherwise MIDIMessage::NONE.
		static MIDIMessage status_to_msg_enum(uint8_t p_status_byte);

	private:
		int device_index = 0;

		static constexpr size_t DATA_BUFFER_SIZE = 2;

		uint8_t status_byte = 0;
		uint8_t data_buffer[DATA_BUFFER_SIZE] = {0};
		size_t received_data_len = 0;
		bool skipping_sys_ex = false;
	};

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }
	_FORCE_INLINE_ static bool is_bound() { return _is_bound; }

	inline static Error (*open)() = &_default_open;
	inline static void (*close)() = &_default_close;

	template <typename Impl> static void bind()
	{
		_initialize_backend = &Impl::initialize;
		_finalize_backend = &Impl::finalize;
		open = &Impl::open;
		close = &Impl::close;
		_is_bound = true;
	}

	static void reset_backend();

	static void add_connected_input(const String& p_name);
	static void clear_connected_inputs();
	static PackedStringArray get_connected_inputs();

	MIDIDriver() = delete;
	MIDIDriver(const MIDIDriver&) = delete;
	MIDIDriver& operator=(const MIDIDriver&) = delete;
	~MIDIDriver() = delete;
};

// --- Folded from core/os/thread_safe.h ---
#include "core/os/mutex.h" // IWYU pragma: keep // Used in macro.

#define _THREAD_SAFE_CLASS_ mutable Mutex _thread_safe_;
#define _THREAD_SAFE_METHOD_ MutexLock _thread_safe_method_(_thread_safe_);
#define _THREAD_SAFE_LOCK_ _thread_safe_.lock();
#define _THREAD_SAFE_UNLOCK_ _thread_safe_.unlock();

bool is_current_thread_safe_for_nodes();
void set_current_thread_safe_for_nodes(bool p_safe);
// --- End core/os/thread_safe.h ---

// --- Folded from core/os/main_loop.h ---
#include "core/typedefs.h"

class MainLoop final
{
private:
	static inline bool _active = false;

	static void _default_initialize() {}
	static void _default_iteration_prepare() {}
	static bool _default_physics_process(double p_time)
	{
		(void)p_time;
		return false;
	}
	static void _default_iteration_end() {}
	static bool _default_process(double p_time)
	{
		(void)p_time;
		return false;
	}
	static void _default_finalize() {}
	static void _default_notification(int p_notification) { (void)p_notification; }
	static void _default_destroy() {}

public:
	MainLoop() = delete;

	enum
	{
		// make sure these are replicated in Node
		NOTIFICATION_OS_MEMORY_WARNING = 2009,
		NOTIFICATION_TRANSLATION_CHANGED = 2010,
		NOTIFICATION_WM_ABOUT = 2011,
		NOTIFICATION_CRASH = 2012,
		NOTIFICATION_OS_IME_UPDATE = 2013,
		NOTIFICATION_APPLICATION_RESUMED = 2014,
		NOTIFICATION_APPLICATION_PAUSED = 2015,
		NOTIFICATION_APPLICATION_FOCUS_IN = 2016,
		NOTIFICATION_APPLICATION_FOCUS_OUT = 2017,
		NOTIFICATION_TEXT_SERVER_CHANGED = 2018,
		NOTIFICATION_APPLICATION_PIP_MODE_ENTERED = 2019,
		NOTIFICATION_APPLICATION_PIP_MODE_EXITED = 2020,
	};

	inline static void (*initialize)() = &_default_initialize;
	inline static void (*iteration_prepare)() = &_default_iteration_prepare;
	inline static bool (*physics_process)(double p_time) = &_default_physics_process;
	inline static void (*iteration_end)() = &_default_iteration_end;
	inline static bool (*process)(double p_time) = &_default_process;
	inline static void (*finalize)() = &_default_finalize;
	inline static void (*notification)(int p_notification) = &_default_notification;
	inline static void (*destroy)() = &_default_destroy;

	template <typename Impl> static void bind()
	{
		initialize = &Impl::initialize_tree;
		iteration_prepare = &Impl::iteration_prepare;
		physics_process = &Impl::physics_process;
		iteration_end = &Impl::iteration_end;
		process = &Impl::process;
		finalize = &Impl::finalize_tree;
		notification = &Impl::notification;
		destroy = &Impl::finalize;
		_active = true;
	}

	static void reset_backend();
	static void clear();
	_FORCE_INLINE_ static bool is_active() { return _active; }
};
// --- End core/os/main_loop.h ---

// --- Folded from core/input/input.h ---
#include "core/input/input_event.h"
#include "core/io/resource.h"
#include "core/templates/hashfuncs.h"
#include "core/input/input_event.h"

#include "core/templates/list.h"
#include "core/templates/hash_map.h"
#include "core/templates/rb_set.h"

class GamepadMotion;

namespace InputClassEnums
{
// Keep synced with DisplayServerEnums::MouseMode enum.
enum MouseMode : int
{
	MOUSE_MODE_VISIBLE,
	MOUSE_MODE_HIDDEN,
	MOUSE_MODE_CAPTURED,
	MOUSE_MODE_CONFINED,
	MOUSE_MODE_CONFINED_HIDDEN,
	MOUSE_MODE_MAX,
};

// Keep synced with DisplayServerEnums and Control enums.
#undef CursorShape

enum CursorShape : int
{
	CURSOR_ARROW,
	CURSOR_IBEAM,
	CURSOR_POINTING_HAND,
	CURSOR_CROSS,
	CURSOR_WAIT,
	CURSOR_BUSY,
	CURSOR_DRAG,
	CURSOR_CAN_DROP,
	CURSOR_FORBIDDEN,
	CURSOR_VSIZE,
	CURSOR_HSIZE,
	CURSOR_BDIAGSIZE,
	CURSOR_FDIAGSIZE,
	CURSOR_MOVE,
	CURSOR_VSPLIT,
	CURSOR_HSPLIT,
	CURSOR_HELP,
	CURSOR_MAX
};
} // namespace InputClassEnums

class Input final
{
	static inline Mutex _thread_safe_;

	static constexpr uint64_t MAX_EVENT = 32;

public:
	// TODO: When we migrate to C++20, replace these with "using enum" and skip prefixing MouseMode
	// and CursorShape in other files.
	using MouseMode = InputClassEnums::MouseMode;
	using CursorShape = InputClassEnums::CursorShape;
	static Vector<uint32_t> get_joy_touchpad_fingers(int p_device, int p_touchpad);

	class JoypadFeatures
	{
	public:
		virtual ~JoypadFeatures() {}

		virtual bool has_joy_vibration() const { return false; }

		virtual bool has_joy_light() const { return false; }

		virtual void set_joy_light(const Color& p_color) {}

		virtual bool has_joy_motion_sensors() const { return false; }

		virtual void set_joy_motion_sensors_enabled(bool p_enable) {}

		virtual int get_joy_num_touchpads() const { return 0; }
	};

	static constexpr int32_t JOYPADS_MAX = 16;

	typedef void (*EventDispatchFunc)(const InputEvent& p_event);

	struct ActionState
	{
		uint64_t pressed_physics_frame = UINT64_MAX;
		uint64_t pressed_process_frame = UINT64_MAX;
		uint64_t released_physics_frame = UINT64_MAX;
		uint64_t released_process_frame = UINT64_MAX;
		bool exact = true;

		struct DeviceState
		{
			bool pressed[MAX_EVENT] = {false};
			float strength[MAX_EVENT] = {0.0};
			float raw_strength[MAX_EVENT] = {0.0};
			InputEventType event_type[MAX_EVENT] = {InputEventType::INVALID};
		};

		bool api_pressed = false;
		float api_strength = 0.0;
		HashMap<int, DeviceState> device_states;

		// Cache.
		struct ActionStateCache
		{
			bool pressed = false;
			float strength = false;
			float raw_strength = false;
		} cache;
	};

	struct VibrationInfo
	{
		float weak_magnitude;
		float strong_magnitude;
		float duration; // Duration in seconds
		uint64_t timestamp;
	};

	struct MotionInfo
	{
		bool sensors_enabled : 1;
		bool calibrating : 1;
		bool calibrated : 1;
		float sensor_data_rate = 0.0f;
		uint64_t last_timestamp = 0;
		GamepadMotion* gamepad_motion = nullptr;

		MotionInfo()
		{
			sensors_enabled = false;
			calibrating = false;
			calibrated = false;
		}
	};

	struct TouchpadFingerInfo
	{
		Vector2 position;
		float pressure;
	};

	struct TouchpadInfo
	{
		int num_touchpads = 0;
		HashMap<uint16_t, TouchpadFingerInfo> finger_info;
	};

	struct VelocityTrack
	{
		uint64_t last_tick = 0;
		Vector2 velocity;
		Vector2 screen_velocity;
		Vector2 accum;
		Vector2 screen_accum;
		float accum_t = 0.0f;
		float min_ref_frame;
		float max_ref_frame;

		void update(const Vector2& p_delta_p, const Vector2& p_screen_delta_p);
		void reset();
		VelocityTrack();
	};

	struct Joypad
	{
		String name;
		String uid;
		bool connected = false;
		bool is_known = false;
		bool last_buttons[(size_t)JoyButton::MAX] = {false};
		float last_axis[(size_t)JoyAxis::MAX] = {0.0f};
		HatMask last_hat = HatMask::CENTER;
		int mapping = -1;
		int hat_current = 0;
		bool has_light = false;
		bool has_vibration = false;
		Input::JoypadFeatures* features = nullptr;
	};

	enum JoyType
	{
		TYPE_BUTTON,
		TYPE_AXIS,
		TYPE_HAT,
		TYPE_MAX,
	};

	enum JoyAxisRange
	{
		NEGATIVE_HALF_AXIS = -1,
		FULL_AXIS = 0,
		POSITIVE_HALF_AXIS = 1
	};

	struct JoyEvent
	{
		int type = TYPE_MAX;
		int index = -1; // Can be either JoyAxis or JoyButton.
		float value = 0.f;
	};

	struct JoyBinding
	{
		JoyType inputType;

		union
		{
			JoyButton button;

			struct
			{
				JoyAxis axis;
				JoyAxisRange range;
				bool invert;
			} axis;

			struct
			{
				HatDir hat;
				HatMask hat_mask;
			} hat;

		} input;

		JoyType outputType;

		union
		{
			JoyButton button;

			struct
			{
				JoyAxis axis;
				JoyAxisRange range;
			} axis;

		} output;
	};

	struct JoyDeviceMapping
	{
		String uid;
		String name;
		Vector<JoyBinding> bindings;
	};

	struct Data
	{
		uint32_t mouse_button_mask = 0;

		RBSet<Key> key_label_pressed;
		RBSet<Key> physical_keys_pressed;
		RBSet<Key> keys_pressed;
		RBSet<JoyButton> joy_buttons_pressed;
		RBMap<JoyAxis, float> _joy_axis;

		bool gravity_enabled = false;
		Vector3 gravity;
		bool accelerometer_enabled = false;
		Vector3 accelerometer;
		bool magnetometer_enabled = false;
		Vector3 magnetometer;
		bool gyroscope_enabled = false;
		Vector3 gyroscope;
		Vector2 mouse_pos;
		int64_t mouse_window = 0;
		bool legacy_just_pressed_behavior = false;
		bool disable_input = false;
		bool ignore_joypad_on_unfocused_application = false;
		bool application_focused = true;
		bool embedder_focused = false;

		HashMap<String, ActionState> action_states;

		bool emulate_touch_from_mouse = false;
		bool emulate_mouse_from_touch = false;
		bool agile_input_event_flushing = false;
		bool use_accumulated_input = true;

		int mouse_from_touch_index = -1;

		HashMap<int, VibrationInfo> joy_vibration;
		HashMap<int, MotionInfo> joy_motion;
		HashMap<int, TouchpadInfo> joy_touch;

		VelocityTrack mouse_velocity_track;
		HashMap<int, VelocityTrack> touch_velocity_track;
		HashMap<int, Joypad> joy_names;

		HashSet<uint32_t> ignored_device_ids;

		int fallback_mapping = -1; // Index of the guid in map_db.

		CursorShape default_shape = CursorShape::CURSOR_ARROW;

		Vector<JoyDeviceMapping> map_db;

		List<InputEvent> buffered_events;

		EventDispatchFunc event_dispatch_function = nullptr;
	};

	static inline Data* data = nullptr;

private:
	static void _set_joypad_mapping(Joypad& p_js, int p_map_index);

	static JoyEvent _get_mapped_button_event(const JoyDeviceMapping& p_mapping, JoyButton p_button);
	static JoyEvent _get_mapped_axis_event(
		const JoyDeviceMapping& p_mapping, JoyAxis p_axis, float p_value, JoyAxisRange& r_range);
	static void _get_mapped_hat_events(
		const JoyDeviceMapping& p_mapping, HatDir p_hat, JoyEvent r_events[(size_t)HatDir::MAX]);
	static JoyButton _get_output_button(const String& p_output);
	static JoyAxis _get_output_axis(const String& p_output);
	static void _button_event(int p_device, JoyButton p_index, bool p_pressed);
	static void _axis_event(int p_device, JoyAxis p_axis, float p_value);
	static void _update_action_cache(const String& p_action_name, ActionState& r_action_state);
	static void _update_joypad_features(int p_device);

	static void _parse_input_event_impl(const InputEvent& p_event, bool p_is_emulated);

	friend class DisplayServer;
	friend class SceneTree;
	friend class SceneDebugger;

	static void (*set_mouse_mode_func)(MouseMode);
	static MouseMode (*get_mouse_mode_func)();
	static void (*set_mouse_mode_override_func)(MouseMode);
	static MouseMode (*get_mouse_mode_override_func)();
	static void (*set_mouse_mode_override_enabled_func)(bool);
	static bool (*is_mouse_mode_override_enabled_func)();
	static void (*warp_mouse_func)(const Vector2& p_position);

	static CursorShape (*get_current_cursor_shape_func)();
	static void (*set_custom_mouse_cursor_func)(const Ref<Resource>&, CursorShape, const Vector2&);

	static bool _should_ignore_joypad_events();

#ifndef DISABLE_DEPRECATED
	static void _vibrate_handheld_bind_compat_91143(int p_duration_ms = 500);
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void set_mouse_mode(MouseMode p_mode);
	static MouseMode get_mouse_mode();
	static void set_mouse_mode_override(MouseMode p_mode);
	static MouseMode get_mouse_mode_override();
	static void set_mouse_mode_override_enabled(bool p_override_enabled);
	static bool is_mouse_mode_override_enabled();

	static bool is_anything_pressed();
	static bool is_any_key_pressed();
	static bool is_key_pressed(Key p_keycode);
	static bool is_physical_key_pressed(Key p_keycode);
	static bool is_key_label_pressed(Key p_keycode);
	static bool is_mouse_button_pressed(MouseButton p_button);
	static bool is_joy_button_pressed(int p_device, JoyButton p_button);
	static bool is_action_pressed(const String& p_action, bool p_exact = false);
	static bool is_action_just_pressed(const String& p_action, bool p_exact = false);
	static bool is_action_just_released(const String& p_action, bool p_exact = false);
	static bool is_action_just_pressed_by_event(
		const String& p_action, const InputEvent& p_event, bool p_exact = false);
	static bool is_action_just_released_by_event(
		const String& p_action, const InputEvent& p_event, bool p_exact = false);
	static float get_action_strength(const String& p_action, bool p_exact = false);
	static float get_action_raw_strength(const String& p_action, bool p_exact = false);

	static float get_axis(const String& p_negative_action, const String& p_positive_action);
	static Vector2 get_vector(const String& p_negative_x, const String& p_positive_x,
		const String& p_negative_y, const String& p_positive_y, float p_deadzone = -1.0f);

	static float get_joy_axis(int p_device, JoyAxis p_axis);
	static String get_joy_name(int p_idx);
	static Vector2 get_joy_vibration_strength(int p_device);
	static float get_joy_vibration_duration(int p_device);
	static float get_joy_vibration_remaining_duration(int p_device);
	static uint64_t get_joy_vibration_timestamp(int p_device);
	static bool is_joy_vibrating(int p_device);
	static bool has_joy_vibration(int p_device);

	static Vector3 get_gravity();
	static Vector3 get_accelerometer();
	static Vector3 get_magnetometer();
	static Vector3 get_gyroscope();

	static int get_joy_num_touchpads(int p_device);

	static Point2 get_mouse_position();
	static Vector2 get_last_mouse_velocity();
	static Vector2 get_last_mouse_screen_velocity();
	static uint32_t get_mouse_button_mask();

	static void warp_mouse(const Vector2& p_position);
	static Point2 warp_mouse_motion(const InputEvent& p_motion, const Rect2& p_rect);

	static void parse_input_event(const InputEvent& p_event);

	static void set_gravity(const Vector3& p_gravity);
	static void set_accelerometer(const Vector3& p_accel);
	static void set_magnetometer(const Vector3& p_magnetometer);
	static void set_gyroscope(const Vector3& p_gyroscope);
	static void set_joy_axis(int p_device, JoyAxis p_axis, float p_value);

	static void set_joy_features(int p_device, JoypadFeatures* p_features);

	static void set_joy_light(int p_device, const Color& p_color);
	static bool has_joy_light(int p_device);

	static Vector3 get_joy_accelerometer(int p_device);
	static Vector3 get_joy_gravity(int p_device);
	static Vector3 get_joy_gyroscope(int p_device);

	static void set_joy_motion_sensors_enabled(int p_device, bool p_enable);
	static bool is_joy_motion_sensors_enabled(int p_device);

	static bool has_joy_motion_sensors(int p_device);
	static float get_joy_motion_sensors_rate(int p_device);

	static void start_joy_motion_sensors_calibration(int p_device);
	static void stop_joy_motion_sensors_calibration(int p_device);
	static void clear_joy_motion_sensors_calibration(int p_device);

	static bool is_joy_motion_sensors_calibrating(int p_device);
	static bool is_joy_motion_sensors_calibrated(int p_device);

	static void set_joy_motion_sensors_rate(int p_device, float p_rate);

	static Vector2 get_joy_touchpad_finger_position(
		int p_device, int p_finger, int p_touchpad = 0);
	static float get_joy_touchpad_finger_pressure(
		int p_device, int p_finger, int p_touchpad = 0);

	static void start_joy_vibration(
		int p_device, float p_weak_magnitude, float p_strong_magnitude, float p_duration = 0);
	static void stop_joy_vibration(int p_device);
	static void vibrate_handheld(int p_duration_ms = 500, float p_amplitude = -1.0);

	static void set_ignore_joypad_on_unfocused_application(bool p_ignore);
	static bool is_ignoring_joypad_on_unfocused_application();
	static void set_application_focused(bool p_focused);

	static void set_mouse_position(const Point2& p_posf);

	static void action_press(const String& p_action, float p_strength = 1.f);
	static void action_release(const String& p_action);

	static void set_emulate_touch_from_mouse(bool p_emulate);
	static bool is_emulating_touch_from_mouse();
	static void ensure_touch_mouse_raised();

	static void set_emulate_mouse_from_touch(bool p_emulate);
	static bool is_emulating_mouse_from_touch();

	static CursorShape get_default_cursor_shape();
	static void set_default_cursor_shape(CursorShape p_shape);
	static CursorShape get_current_cursor_shape();
	static void set_custom_mouse_cursor(const Ref<Resource>& p_cursor,
		CursorShape p_shape = Input::CursorShape::CURSOR_ARROW,
		const Vector2& p_hotspot = Vector2());

	static void parse_mapping(const String& p_mapping);
	static void joy_button(int p_device, JoyButton p_button, bool p_pressed);
	static void joy_axis(int p_device, JoyAxis p_axis, float p_value);
	static void joy_hat(int p_device, uint32_t p_val);
	static void joy_motion_sensors(
		int p_device, const Vector3& p_accelerometer, const Vector3& p_gyroscope);
	static void joy_touchpad(int p_device, int p_touchpad, int p_finger, const Vector2& p_position,
		float p_pressure, bool p_pressed);

	static void add_joy_mapping(const String& p_mapping, bool p_update_existing = false);
	static void remove_joy_mapping(const String& p_guid);

	static int get_unused_joy_id();

	static bool is_joy_known(int p_device);
	static String get_joy_guid(int p_device);
	static bool should_ignore_device(int p_vendor_id, int p_product_id);
	static void set_fallback_mapping(const String& p_guid);

	static void flush_buffered_events();
	static bool is_agile_input_event_flushing();
	static void set_agile_input_event_flushing(bool p_enable);
	static void set_use_accumulated_input(bool p_enable);
	static bool is_using_accumulated_input();

	static void release_pressed_events();

	static void set_event_dispatch_function(EventDispatchFunc p_function);

	static void set_disable_input(bool p_disable);
	static bool is_input_disabled();

	Input() = delete;
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;
	~Input() = delete;
};

// --- Folded from core/input/default_controller_mappings.h ---
class DefaultControllerMappings {
public:
	static const char *mappings[];
};

// --- End core/input/default_controller_mappings.h ---

// --- Folded from core/input/input_event_codec.h ---
#include "core/input/input_event.h"

/**
 * Encodes the input event as a byte array.
 *
 * Returns `true` if the event was successfully encoded, `false` otherwise.
 */
bool encode_input_event(const InputEvent& p_event, Vector<uint8_t>& r_data);
void decode_input_event(const Vector<uint8_t>& p_data, InputEvent& r_event);

void encode_input_event_key(const InputEvent& p_event, Vector<uint8_t>& r_data);
void encode_input_event_mouse_button(const InputEvent& p_event, Vector<uint8_t>& r_data);
void encode_input_event_mouse_motion(const InputEvent& p_event, Vector<uint8_t>& r_data);
void encode_input_event_joypad_button(const InputEvent& p_event, Vector<uint8_t>& r_data);
void encode_input_event_joypad_motion(const InputEvent& p_event, Vector<uint8_t>& r_data);
void encode_input_event_gesture_pan(const InputEvent& p_event, Vector<uint8_t>& r_data);
void encode_input_event_gesture_magnify(const InputEvent& p_event, Vector<uint8_t>& r_data);

// --- End core/input/input_event_codec.h ---

// --- Folded from drivers/sdl/joypad_sdl.h ---
#if defined(SDL_ENABLED)


typedef uint32_t SDL_JoystickID;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_Gamepad SDL_Gamepad;

class JoypadSDL {
public:
	~JoypadSDL();

	Error initialize();
	void process_events();

private:
	class Joypad : public Input::JoypadFeatures {
	public:
		bool attached = false;
		String guid;

		SDL_JoystickID sdl_instance_idx;

		bool supports_force_feedback = false;
		bool supports_motion_sensors = false;
		uint64_t ff_effect_timestamp = 0;

		virtual bool has_joy_light() const override;
		virtual void set_joy_light(const Color &p_color) override;

		virtual bool has_joy_motion_sensors() const override;
		virtual void set_joy_motion_sensors_enabled(bool p_enable) override;

		virtual bool has_joy_vibration() const override;

		virtual int get_joy_num_touchpads() const override;

		SDL_Joystick *get_sdl_joystick() const;
		SDL_Gamepad *get_sdl_gamepad() const;
	};

	Joypad joypads[Input::JOYPADS_MAX];
	HashMap<SDL_JoystickID, int> sdl_instance_id_to_joypad_id;

	void close_joypad(int p_pad_idx);
};
#endif // defined(SDL_ENABLED)
// --- End drivers/sdl/joypad_sdl.h ---

// --- Folded from core/input/input_map.h ---
#include "core/input/input_event.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"

class InputMap final
{
public:
	/**
	 * A special value used to signify that a given Action can be triggered by any device
	 */
	static constexpr int ALL_DEVICES = -1;

	struct Action
	{
		int id = 0;
		float deadzone = DEFAULT_DEADZONE;
		List<InputEvent> inputs;
	};

	static constexpr float DEFAULT_DEADZONE = 0.2f;
	// Keep bigger deadzone for toggle actions (default `ui_*` actions, axis `pressed`) (GH-103360).
	static constexpr float DEFAULT_TOGGLE_DEADZONE = 0.5f;

	struct Data
	{
		HashMap<String, Action> input_map;
		HashMap<String, List<InputEvent>> default_builtin_cache;
		HashMap<String, List<InputEvent>> default_builtin_with_overrides_cache;
		int last_id = 1;
	};

	static inline Data* data = nullptr;

private:
	static List<InputEvent>::Element* _find_event(Action& p_action, const InputEvent& p_event,
		bool p_exact_match = false, bool* r_pressed = nullptr, float* r_strength = nullptr,
		float* r_raw_strength = nullptr, int* r_event_index = nullptr);

#ifndef DISABLE_DEPRECATED
	static void _add_action_bind_compat_97281(const String& p_action, float p_deadzone = 0.5);
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static bool has_action(const String& p_action);
	static void add_action(const String& p_action, float p_deadzone = DEFAULT_DEADZONE);
	static void erase_action(const String& p_action);

	static String get_action_description(const String& p_action);

	static float action_get_deadzone(const String& p_action);
	static void action_set_deadzone(const String& p_action, float p_deadzone);
	static void action_add_event(const String& p_action, const InputEvent& p_event);
	static bool action_has_event(const String& p_action, const InputEvent& p_event);
	static void action_erase_event(const String& p_action, const InputEvent& p_event);
	static void action_erase_events(const String& p_action);

	static const List<InputEvent>* action_get_events(const String& p_action);
	static bool event_is_action(
		const InputEvent& p_event, const String& p_action, bool p_exact_match = false);
	static int event_get_index(
		const InputEvent& p_event, const String& p_action, bool p_exact_match = false);
	static bool event_get_action_status(const InputEvent& p_event, const String& p_action,
		bool p_exact_match = false, bool* r_pressed = nullptr, float* r_strength = nullptr,
		float* r_raw_strength = nullptr, int* r_event_index = nullptr);

	static const HashMap<String, Action>& get_action_map();
	static void load_from_project_settings();
	static void load_default();

	static String suggest_actions(const String& p_action);

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);
#endif

	static String get_builtin_display_name(const String& p_name);
	static const HashMap<String, List<InputEvent>>& get_builtins();
	static const HashMap<String, List<InputEvent>>& get_builtins_with_feature_overrides_applied();

	InputMap() = delete;
	InputMap(const InputMap&) = delete;
	InputMap& operator=(const InputMap&) = delete;
	~InputMap() = delete;
};
// --- End core/input/input_map.h ---
// --- End core/input/input.h ---
