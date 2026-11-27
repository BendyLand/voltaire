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
#include "core/io/ip.h"
#include "core/string/ustring.h"
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
	static uint64_t get_frames_drawn();

	static double get_frames_per_second() { return _fps; }

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

#include <cstdarg>
#include "core/io/file_access.h"

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

	enum Backend
	{
		BACKEND_STD,
		BACKEND_WINDOWS_TERMINAL,
		BACKEND_SYSLOG,
		BACKEND_OS_LOG,
		BACKEND_ANDROID,
	};

protected:
	Backend backend = BACKEND_STD;
#ifdef __APPLE__
	void* os_log_handle = nullptr;
#endif

public:
	static void set_flush_stdout_on_print(bool p_value);

	virtual void logv(
		const char* p_format, va_list p_list, bool p_err) _PRINTF_FORMAT_ATTRIBUTE_2_0;

	void logf(const char* p_format, ...) _PRINTF_FORMAT_ATTRIBUTE_2_3;
	void logf_error(const char* p_format, ...) _PRINTF_FORMAT_ATTRIBUTE_2_3;

	explicit Logger(Backend p_backend = BACKEND_STD) noexcept : backend(p_backend) {}
#ifdef __APPLE__
	explicit Logger(const char* p_subsystem);
#endif

	virtual ~Logger() {}
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

class RemoteFilesystemClient
{
	String cache_path;
	HashSet<String> validated_directories;

protected:
	String _get_cache_path() { return cache_path; }

	struct FileCache
	{
		String path;					   // Local path (as in "folder/to/file.png")
		uint64_t server_modified_time = 0; // MD5 checksum.
		uint64_t modified_time = 0;
	};

	virtual bool _is_configured() { return !cache_path.is_empty(); }

	// Can be re-implemented per platform. If so, feel free to ignore get_cache_path()
	virtual Vector<FileCache> _load_cache_file();
	virtual Error _store_file(
		const String& p_path, const LocalVector<uint8_t>& p_file, uint64_t& r_modified_time);
	virtual Error _remove_file(const String& p_path);
	virtual Error _store_cache_file(const Vector<FileCache>& p_cache);
	virtual Error _synchronize_with_server(
		const String& p_host, int p_port, const String& p_password, String& r_cache_path);

	virtual void _update_cache_path(String& r_cache_path);

public:
	Error synchronize_with_server(
		const String& p_host, int p_port, const String& p_password, String& r_cache_path);

	virtual ~RemoteFilesystemClient() {}
};

#include <cstdint>

typedef int64_t ProcessID;
#include <cstdint>

enum Month
{
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

enum Weekday : uint8_t
{
	WEEKDAY_SUNDAY,
	WEEKDAY_MONDAY,
	WEEKDAY_TUESDAY,
	WEEKDAY_WEDNESDAY,
	WEEKDAY_THURSDAY,
	WEEKDAY_FRIDAY,
	WEEKDAY_SATURDAY,
};

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

#include "core/typedefs.h"

enum class InputEventType
{
	INVALID = -1,
	KEY,
	MOUSE_BUTTON,
	MOUSE_MOTION,
	JOY_MOTION,
	JOY_BUTTON,
	SCREEN_TOUCH,
	SCREEN_DRAG,
	MAGNIFY_GESTURE,
	PAN_GESTURE,
	MIDI,
	SHORTCUT,
	ACTION,
	MAX,
};

enum class HatDir
{
	UP = 0,
	RIGHT = 1,
	DOWN = 2,
	LEFT = 3,
	MAX = 4,
};

enum class HatMask
{
	CENTER = 0,
	UP = 1,
	RIGHT = 2,
	DOWN = 4,
	LEFT = 8,
};

enum class JoyAxis
{
	INVALID = -1,
	LEFT_X = 0,
	LEFT_Y = 1,
	RIGHT_X = 2,
	RIGHT_Y = 3,
	TRIGGER_LEFT = 4,
	TRIGGER_RIGHT = 5,
	SDL_MAX = 6,
	MAX = 10, // OpenVR supports up to 5 Joysticks making a total of 10 axes.
};

enum class JoyButton
{
	INVALID = -1,
	A = 0,
	B = 1,
	X = 2,
	Y = 3,
	BACK = 4,
	GUIDE = 5,
	START = 6,
	LEFT_STICK = 7,
	RIGHT_STICK = 8,
	LEFT_SHOULDER = 9,
	RIGHT_SHOULDER = 10,
	DPAD_UP = 11,
	DPAD_DOWN = 12,
	DPAD_LEFT = 13,
	DPAD_RIGHT = 14,
	MISC1 = 15,
	PADDLE1 = 16,
	PADDLE2 = 17,
	PADDLE3 = 18,
	PADDLE4 = 19,
	TOUCHPAD = 20,
	MISC2 = 21,
	MISC3 = 22,
	MISC4 = 23,
	MISC5 = 24,
	MISC6 = 25,
	SDL_MAX = 26,
	MAX = 128, // Android supports up to 36 buttons. DirectInput supports up to 128 buttons.
};

enum class MIDIMessage
{
	NONE = 0,
	NOTE_OFF = 0x8,
	NOTE_ON = 0x9,
	AFTERTOUCH = 0xA,
	CONTROL_CHANGE = 0xB,
	PROGRAM_CHANGE = 0xC,
	CHANNEL_PRESSURE = 0xD,
	PITCH_BEND = 0xE,
	SYSTEM_EXCLUSIVE = 0xF0,
	QUARTER_FRAME = 0xF1,
	SONG_POSITION_POINTER = 0xF2,
	SONG_SELECT = 0xF3,
	TUNE_REQUEST = 0xF6,
	TIMING_CLOCK = 0xF8,
	START = 0xFA,
	CONTINUE = 0xFB,
	STOP = 0xFC,
	ACTIVE_SENSING = 0xFE,
	SYSTEM_RESET = 0xFF,
};

enum class MouseButton
{
	NONE = 0,
	LEFT = 1,
	RIGHT = 2,
	MIDDLE = 3,
	WHEEL_UP = 4,
	WHEEL_DOWN = 5,
	WHEEL_LEFT = 6,
	WHEEL_RIGHT = 7,
	MB_XBUTTON1 = 8, // "XBUTTON1" is a reserved word on Windows.
	MB_XBUTTON2 = 9, // "XBUTTON2" is a reserved word on Windows.
};

enum class MouseButtonMask
{
	NONE = 0,
	LEFT = (1 << (int(MouseButton::LEFT) - 1)),
	RIGHT = (1 << (int(MouseButton::RIGHT) - 1)),
	MIDDLE = (1 << (int(MouseButton::MIDDLE) - 1)),
	MB_XBUTTON1 = (1 << (int(MouseButton::MB_XBUTTON1) - 1)),
	MB_XBUTTON2 = (1 << (int(MouseButton::MB_XBUTTON2) - 1)),
};

inline MouseButtonMask mouse_button_to_mask(MouseButton p_button)
{
	ERR_FAIL_COND_V(p_button == MouseButton::NONE, MouseButtonMask::NONE);

	return MouseButtonMask(1 << ((int)p_button - 1));
}

constexpr MouseButtonMask operator|(MouseButtonMask p_a, MouseButtonMask p_b)
{
	return static_cast<MouseButtonMask>(static_cast<int>(p_a) | static_cast<int>(p_b));
}

constexpr MouseButtonMask& operator|=(MouseButtonMask& p_a, MouseButtonMask p_b)
{
	return p_a = p_a | p_b;
}

#include "core/math/vector2.h"
class String;

#include <cstdint>

// Keep the values in this enum in sync with `_keycodes` in `keyboard.cpp`,
// and the bindings in `core_constants.cpp`.
enum class Key
{
	NONE = 0,
	// Special key: The strategy here is similar to the one used by toolkits,
	// which consists in leaving the 21 bits unicode range for printable
	// characters, and use the upper 11 bits for special keys and modifiers.
	// This way everything (char/keycode) can fit nicely in one 32-bit
	// integer (the enum's underlying type is `int` by default).
	SPECIAL = (1 << 22),
	/* CURSOR/FUNCTION/BROWSER/MULTIMEDIA/MISC KEYS */
	ESCAPE = SPECIAL | 0x01,
	TAB = SPECIAL | 0x02,
	BACKTAB = SPECIAL | 0x03,
	BACKSPACE = SPECIAL | 0x04,
	ENTER = SPECIAL | 0x05,
	KP_ENTER = SPECIAL | 0x06,
	INSERT = SPECIAL | 0x07,
	KEY_DELETE = SPECIAL | 0x08, // "DELETE" is a reserved word on Windows.
	PAUSE = SPECIAL | 0x09,
	PRINT = SPECIAL | 0x0A,
	SYSREQ = SPECIAL | 0x0B,
	CLEAR = SPECIAL | 0x0C,
	HOME = SPECIAL | 0x0D,
	END = SPECIAL | 0x0E,
	LEFT = SPECIAL | 0x0F,
	UP = SPECIAL | 0x10,
	RIGHT = SPECIAL | 0x11,
	DOWN = SPECIAL | 0x12,
	PAGEUP = SPECIAL | 0x13,
	PAGEDOWN = SPECIAL | 0x14,
	SHIFT = SPECIAL | 0x15,
	CTRL = SPECIAL | 0x16,
	META = SPECIAL | 0x17,
#if defined(MACOS_ENABLED)
	CMD_OR_CTRL = META,
#else
	CMD_OR_CTRL = CTRL,
#endif
	ALT = SPECIAL | 0x18,
	CAPSLOCK = SPECIAL | 0x19,
	NUMLOCK = SPECIAL | 0x1A,
	SCROLLLOCK = SPECIAL | 0x1B,
	F1 = SPECIAL | 0x1C,
	F2 = SPECIAL | 0x1D,
	F3 = SPECIAL | 0x1E,
	F4 = SPECIAL | 0x1F,
	F5 = SPECIAL | 0x20,
	F6 = SPECIAL | 0x21,
	F7 = SPECIAL | 0x22,
	F8 = SPECIAL | 0x23,
	F9 = SPECIAL | 0x24,
	F10 = SPECIAL | 0x25,
	F11 = SPECIAL | 0x26,
	F12 = SPECIAL | 0x27,
	F13 = SPECIAL | 0x28,
	F14 = SPECIAL | 0x29,
	F15 = SPECIAL | 0x2A,
	F16 = SPECIAL | 0x2B,
	F17 = SPECIAL | 0x2C,
	F18 = SPECIAL | 0x2D,
	F19 = SPECIAL | 0x2E,
	F20 = SPECIAL | 0x2F,
	F21 = SPECIAL | 0x30,
	F22 = SPECIAL | 0x31,
	F23 = SPECIAL | 0x32,
	F24 = SPECIAL | 0x33,
	F25 = SPECIAL | 0x34,
	F26 = SPECIAL | 0x35,
	F27 = SPECIAL | 0x36,
	F28 = SPECIAL | 0x37,
	F29 = SPECIAL | 0x38,
	F30 = SPECIAL | 0x39,
	F31 = SPECIAL | 0x3A,
	F32 = SPECIAL | 0x3B,
	F33 = SPECIAL | 0x3C,
	F34 = SPECIAL | 0x3D,
	F35 = SPECIAL | 0x3E,
	KP_MULTIPLY = SPECIAL | 0x81,
	KP_DIVIDE = SPECIAL | 0x82,
	KP_SUBTRACT = SPECIAL | 0x83,
	KP_PERIOD = SPECIAL | 0x84,
	KP_ADD = SPECIAL | 0x85,
	KP_0 = SPECIAL | 0x86,
	KP_1 = SPECIAL | 0x87,
	KP_2 = SPECIAL | 0x88,
	KP_3 = SPECIAL | 0x89,
	KP_4 = SPECIAL | 0x8A,
	KP_5 = SPECIAL | 0x8B,
	KP_6 = SPECIAL | 0x8C,
	KP_7 = SPECIAL | 0x8D,
	KP_8 = SPECIAL | 0x8E,
	KP_9 = SPECIAL | 0x8F,
	MENU = SPECIAL | 0x42,
	HYPER = SPECIAL | 0x43,
	HELP = SPECIAL | 0x45,
	BACK = SPECIAL | 0x48,
	FORWARD = SPECIAL | 0x49,
	STOP = SPECIAL | 0x4A,
	REFRESH = SPECIAL | 0x4B,
	VOLUMEDOWN = SPECIAL | 0x4C,
	VOLUMEMUTE = SPECIAL | 0x4D,
	VOLUMEUP = SPECIAL | 0x4E,
	MEDIAPLAY = SPECIAL | 0x54,
	MEDIASTOP = SPECIAL | 0x55,
	MEDIAPREVIOUS = SPECIAL | 0x56,
	MEDIANEXT = SPECIAL | 0x57,
	MEDIARECORD = SPECIAL | 0x58,
	HOMEPAGE = SPECIAL | 0x59,
	FAVORITES = SPECIAL | 0x5A,
	SEARCH = SPECIAL | 0x5B,
	STANDBY = SPECIAL | 0x5C,
	OPENURL = SPECIAL | 0x5D,
	LAUNCHMAIL = SPECIAL | 0x5E,
	LAUNCHMEDIA = SPECIAL | 0x5F,
	LAUNCH0 = SPECIAL | 0x60,
	LAUNCH1 = SPECIAL | 0x61,
	LAUNCH2 = SPECIAL | 0x62,
	LAUNCH3 = SPECIAL | 0x63,
	LAUNCH4 = SPECIAL | 0x64,
	LAUNCH5 = SPECIAL | 0x65,
	LAUNCH6 = SPECIAL | 0x66,
	LAUNCH7 = SPECIAL | 0x67,
	LAUNCH8 = SPECIAL | 0x68,
	LAUNCH9 = SPECIAL | 0x69,
	LAUNCHA = SPECIAL | 0x6A,
	LAUNCHB = SPECIAL | 0x6B,
	LAUNCHC = SPECIAL | 0x6C,
	LAUNCHD = SPECIAL | 0x6D,
	LAUNCHE = SPECIAL | 0x6E,
	LAUNCHF = SPECIAL | 0x6F,

	GLOBE = SPECIAL | 0x70,
	KEYBOARD = SPECIAL | 0x71,
	JIS_EISU = SPECIAL | 0x72,
	JIS_KANA = SPECIAL | 0x73,

	UNKNOWN = SPECIAL | 0x7FFFFF,

	/* PRINTABLE LATIN 1 CODES */

	SPACE = 0x0020,
	EXCLAM = 0x0021,
	QUOTEDBL = 0x0022,
	NUMBERSIGN = 0x0023,
	DOLLAR = 0x0024,
	PERCENT = 0x0025,
	AMPERSAND = 0x0026,
	APOSTROPHE = 0x0027,
	PARENLEFT = 0x0028,
	PARENRIGHT = 0x0029,
	ASTERISK = 0x002A,
	PLUS = 0x002B,
	COMMA = 0x002C,
	MINUS = 0x002D,
	PERIOD = 0x002E,
	SLASH = 0x002F,
	KEY_0 = 0x0030,
	KEY_1 = 0x0031,
	KEY_2 = 0x0032,
	KEY_3 = 0x0033,
	KEY_4 = 0x0034,
	KEY_5 = 0x0035,
	KEY_6 = 0x0036,
	KEY_7 = 0x0037,
	KEY_8 = 0x0038,
	KEY_9 = 0x0039,
	COLON = 0x003A,
	SEMICOLON = 0x003B,
	LESS = 0x003C,
	EQUAL = 0x003D,
	GREATER = 0x003E,
	QUESTION = 0x003F,
	AT = 0x0040,
	A = 0x0041,
	B = 0x0042,
	C = 0x0043,
	D = 0x0044,
	E = 0x0045,
	F = 0x0046,
	G = 0x0047,
	H = 0x0048,
	I = 0x0049,
	J = 0x004A,
	K = 0x004B,
	L = 0x004C,
	M = 0x004D,
	N = 0x004E,
	O = 0x004F,
	P = 0x0050,
	Q = 0x0051,
	R = 0x0052,
	S = 0x0053,
	T = 0x0054,
	U = 0x0055,
	V = 0x0056,
	W = 0x0057,
	X = 0x0058,
	Y = 0x0059,
	Z = 0x005A,
	BRACKETLEFT = 0x005B,
	BACKSLASH = 0x005C,
	BRACKETRIGHT = 0x005D,
	ASCIICIRCUM = 0x005E,
	UNDERSCORE = 0x005F,
	QUOTELEFT = 0x0060,
	BRACELEFT = 0x007B,
	BAR = 0x007C,
	BRACERIGHT = 0x007D,
	ASCIITILDE = 0x007E,
	YEN = 0x00A5,
	SECTION = 0x00A7,
};

enum class KeyModifierMask
{
	CODE_MASK = ((1 << 23) - 1),  ///< Apply this mask to any keycode to remove modifiers.
	MODIFIER_MASK = (0x7F << 24), ///< Apply this mask to isolate modifiers.
	// RESERVED = (1 << 23),
	CMD_OR_CTRL = (1 << 24),
	SHIFT = (1 << 25),
	ALT = (1 << 26),
	META = (1 << 27),
	CTRL = (1 << 28),
	KPAD = (1 << 29),
	GROUP_SWITCH = (1 << 30)
};

enum class KeyLocation
{
	UNSPECIFIED,
	LEFT,
	RIGHT
};

// To avoid having unnecessary operators, only define the ones that are needed.

constexpr Key operator-(uint32_t p_left, Key p_right) { return (Key)(p_left - (uint32_t)p_right); }

constexpr Key& operator-=(Key& p_left, int p_right)
{
	p_left = static_cast<Key>(static_cast<int>(p_left) - static_cast<int>(p_right));
	return p_left;
}

constexpr Key operator+(Key p_left, int p_right) { return (Key)((int)p_left + (int)p_right); }

constexpr Key operator+(Key p_left, Key p_right) { return (Key)((int)p_left + (int)p_right); }

constexpr Key operator-(Key p_left, Key p_right) { return (Key)((int)p_left - (int)p_right); }

constexpr Key operator&(Key p_left, Key p_right) { return (Key)((int)p_left & (int)p_right); }

constexpr Key operator|(Key p_left, Key p_right) { return (Key)((int)p_left | (int)p_right); }

constexpr Key& operator|=(Key& p_left, Key p_right)
{
	p_left = static_cast<Key>(static_cast<int>(p_left) | static_cast<int>(p_right));
	return p_left;
}

constexpr Key& operator|=(Key& p_left, KeyModifierMask p_right)
{
	p_left = static_cast<Key>(static_cast<int>(p_left) | static_cast<int>(p_right));
	return p_left;
}

constexpr Key& operator&=(Key& p_left, KeyModifierMask p_right)
{
	p_left = static_cast<Key>(static_cast<int>(p_left) & static_cast<int>(p_right));
	return p_left;
}

constexpr Key operator|(Key p_left, KeyModifierMask p_right)
{
	return (Key)((int)p_left | (int)p_right);
}

constexpr Key operator&(Key p_left, KeyModifierMask p_right)
{
	return (Key)((int)p_left & (int)p_right);
}

constexpr Key operator+(KeyModifierMask p_left, Key p_right)
{
	return (Key)((int)p_left + (int)p_right);
}

constexpr Key operator|(KeyModifierMask p_left, Key p_right)
{
	return (Key)((int)p_left | (int)p_right);
}

constexpr KeyModifierMask operator+(KeyModifierMask p_left, KeyModifierMask p_right)
{
	return (KeyModifierMask)((int)p_left + (int)p_right);
}

constexpr KeyModifierMask operator|(KeyModifierMask p_left, KeyModifierMask p_right)
{
	return (KeyModifierMask)((int)p_left | (int)p_right);
}

constexpr Key operator|(Key p_left, uint32_t p_right)
{
	return static_cast<Key>(static_cast<uint32_t>(p_left) | p_right);
}

constexpr Key operator|(uint32_t p_left, Key p_right)
{
	return static_cast<Key>(p_left | static_cast<uint32_t>(p_right));
}

String keycode_get_string(Key p_code);
bool keycode_has_unicode(Key p_keycode);
Key find_keycode(const String& p_codestr);
const char* find_keycode_name(Key p_keycode);
int keycode_get_count();
int keycode_get_value_by_index(int p_index);
const char* keycode_get_name_by_index(int p_index);

char32_t fix_unicode(char32_t p_char);
Key fix_keycode(char32_t p_char, Key p_key);
Key fix_key_label(char32_t p_char, Key p_key);
#include <type_traits>

// TODO: Replace `typename` with enum concept once C++20 concepts/constraints are allowed.

template <typename T> class BitField
{
	static_assert(std::is_enum_v<T>);
	uint64_t value;

public:
	_ALWAYS_INLINE_ constexpr void set_flag(BitField p_flag) { value |= p_flag.value; }

	_ALWAYS_INLINE_ constexpr bool has_flag(BitField p_flag) const { return value & p_flag.value; }

	_ALWAYS_INLINE_ constexpr bool is_empty() const { return value == 0; }

	_ALWAYS_INLINE_ constexpr void clear_flag(BitField p_flag) { value &= ~p_flag.value; }

	_ALWAYS_INLINE_ constexpr void clear() { value = 0; }

	[[nodiscard]] _ALWAYS_INLINE_ constexpr BitField get_combined(BitField p_other) const
	{
		return BitField(value | p_other.value);
	}

	[[nodiscard]] _ALWAYS_INLINE_ constexpr BitField get_shared(BitField p_other) const
	{
		return BitField(value & p_other.value);
	}

	[[nodiscard]] _ALWAYS_INLINE_ constexpr BitField get_different(BitField p_other) const
	{
		return BitField(value ^ p_other.value);
	}

	_ALWAYS_INLINE_ constexpr BitField() = default;

	_ALWAYS_INLINE_ constexpr BitField(T p_value) : value(static_cast<uint64_t>(p_value)) {}

	_ALWAYS_INLINE_ constexpr operator T() const { return static_cast<T>(value); }

	// TODO: Unify as single constructor once C++20 `explicit` conditionals are allowed.

	template <typename V,
		std::enable_if_t<std::is_arithmetic_v<V> && std::is_convertible_v<T, int>, int> = 0>
	_ALWAYS_INLINE_ constexpr BitField(V p_value) : value(static_cast<uint64_t>(p_value))
	{
	}

	template <typename V,
		std::enable_if_t<std::is_arithmetic_v<V> && !std::is_convertible_v<T, int>, int> = 0>
	_ALWAYS_INLINE_ constexpr explicit BitField(V p_value) : value(static_cast<uint64_t>(p_value))
	{
	}

	template <typename V, std::enable_if_t<std::is_arithmetic_v<V>, int> = 0>
	_ALWAYS_INLINE_ constexpr explicit operator V() const
	{
		return static_cast<V>(value);
	}
};

// Implicitly zero-constructible as a trivially-constructible type.
static_assert(is_zero_constructible_v<BitField<Error>>);

class Shortcut;
struct Transform2D;

/**
 * Plain value-type input event with composed per-event payload structs.
 * Replaces the former 6-level, 16-class RefCounted Resource hierarchy.
 */
struct InputEvent
{
	static constexpr int DEVICE_ID_EMULATION = -1;
	static constexpr int DEVICE_ID_INTERNAL = -2;
	static constexpr int DEVICE_ID_KEYBOARD = 16; // IDs 0-15 are reserved for joypads.
	static constexpr int DEVICE_ID_MOUSE = 32;	  // IDs 17-31 are reserved for multiple keyboards.

	struct KeyData
	{
		Key keycode = Key::NONE;
		Key physical_keycode = Key::NONE;
		Key key_label = Key::NONE;
		char32_t unicode = 0;
		KeyLocation location = KeyLocation::UNSPECIFIED;
		bool echo = false;
	};

	struct MouseButtonData
	{
		Vector2 position;
		Vector2 global_position;
		uint32_t button_mask = 0;
		float factor = 1.0f;
		MouseButton button_index = MouseButton::NONE;
		bool double_click = false;
	};

	struct MouseMotionData
	{
		Vector2 position;
		Vector2 global_position;
		uint32_t button_mask = 0;
		Vector2 tilt;
		float pressure = 0.0f;
		Vector2 relative;
		Vector2 screen_relative;
		Vector2 velocity;
		Vector2 screen_velocity;
		bool pen_inverted = false;
	};

	struct JoypadButtonData
	{
		JoyButton button_index = (JoyButton)0;
		float pressure = 0.0f;
	};

	struct JoypadMotionData
	{
		JoyAxis axis = (JoyAxis)0;
		float axis_value = 0.0f;
	};

	struct TouchData
	{
		int index = 0;
		Vector2 position;
		bool double_tap = false;
	};

	struct ScreenDragData
	{
		int index = 0;
		Vector2 position;
		Vector2 relative;
		Vector2 screen_relative;
		Vector2 velocity;
		Vector2 screen_velocity;
		Vector2 tilt;
		float pressure = 0.0f;
		bool pen_inverted = false;
	};

	struct GestureData
	{
		Vector2 position;
		Vector2 delta;
		real_t factor = 1.0;
	};

	struct MIDIData
	{
		int channel = 0;
		MIDIMessage message = MIDIMessage::NONE;
		int pitch = 0;
		int velocity = 0;
		int instrument = 0;
		int pressure = 0;
		int controller_number = 0;
		int controller_value = 0;
	};

	struct ActionData
	{
		String action;
		float strength = 1.0f;
		int event_index = -1;
	};

	struct ShortcutData
	{
		const Shortcut* shortcut = nullptr;
	};

	// Common header fields
	InputEventType type = InputEventType::INVALID;
	int device = 0;
	int64_t window_id = 0;
	BitField<KeyModifierMask> modifiers = KeyModifierMask(0);
	bool pressed = false;
	bool canceled = false;

	// Composed POD union payload + action metadata
	union
	{
		KeyData key;
		MouseButtonData mouse_button;
		MouseMotionData mouse_motion = {};
		JoypadButtonData joy_button;
		JoypadMotionData joy_motion;
		TouchData touch;
		ScreenDragData screen_drag;
		GestureData gesture;
		MIDIData midi;
		ShortcutData shortcut;
	};

	ActionData action;

	InputEvent() = default;

	// Factory constructors
	static InputEvent make_key(
		Key p_keycode_with_modifier_masks = Key::NONE, bool p_physical = false);
	static InputEvent make_mouse_button();
	static InputEvent make_mouse_motion();
	static InputEvent make_joypad_button(JoyButton p_btn_index = (JoyButton)0, int p_device = -1);
	static InputEvent make_joypad_motion(
		JoyAxis p_axis = (JoyAxis)0, float p_value = 0.0f, int p_device = -1);
	static InputEvent make_screen_touch();
	static InputEvent make_screen_drag();
	static InputEvent make_magnify_gesture();
	static InputEvent make_pan_gesture();
	static InputEvent make_midi();
	static InputEvent make_action(
		const String& p_action = String(), bool p_pressed = false, float p_strength = 1.0f);
	static InputEvent make_shortcut(const Shortcut* p_shortcut = nullptr);

	// Type & state queries
	bool is_valid() const { return type != InputEventType::INVALID; }

	bool is_null() const { return type == InputEventType::INVALID; }

	InputEventType get_type() const { return type; }

	bool is_mouse() const
	{
		return type == InputEventType::MOUSE_BUTTON || type == InputEventType::MOUSE_MOTION;
	}

	bool is_gesture() const
	{
		return type == InputEventType::MAGNIFY_GESTURE || type == InputEventType::PAN_GESTURE;
	}

	bool is_from_window() const
	{
		return type == InputEventType::KEY || type == InputEventType::MOUSE_BUTTON ||
			   type == InputEventType::MOUSE_MOTION || type == InputEventType::SCREEN_TOUCH ||
			   type == InputEventType::SCREEN_DRAG || type == InputEventType::MAGNIFY_GESTURE ||
			   type == InputEventType::PAN_GESTURE;
	}

	void set_device(int p_device) { device = p_device; }

	int get_device() const { return device; }

	void set_window_id(int64_t p_id) { window_id = p_id; }

	int64_t get_window_id() const { return window_id; }

	void set_pressed(bool p_pressed) { pressed = p_pressed; }

	void set_canceled(bool p_canceled) { canceled = p_canceled; }

	bool is_canceled() const { return canceled; }

	bool is_pressed() const { return pressed && !canceled; }

	bool is_released() const { return !pressed && !canceled; }

	bool is_echo() const { return type == InputEventType::KEY && key.echo; }

	// Shared mouse helpers (MOUSE_BUTTON & MOUSE_MOTION)
	Vector2 get_mouse_position() const;
	void set_mouse_position(const Vector2& p_pos);
	Vector2 get_mouse_global_position() const;
	void set_mouse_global_position(const Vector2& p_global_pos);
	uint32_t get_mouse_button_mask() const;
	void set_mouse_button_mask(uint32_t p_mask);

	// Joypad motion helper (updates `pressed` threshold as well)
	void set_joy_axis(JoyAxis p_axis);
	void set_joy_axis_value(float p_value);

	// Modifier helpers
	uint32_t get_modifiers_mask() const
	{
		return static_cast<uint32_t>(modifiers) & (static_cast<uint32_t>(KeyModifierMask::SHIFT) |
													  static_cast<uint32_t>(KeyModifierMask::ALT) |
													  static_cast<uint32_t>(KeyModifierMask::CTRL) |
													  static_cast<uint32_t>(KeyModifierMask::META));
	}

	void set_command_or_control_autoremap(bool p_enabled);

	bool is_command_or_control_autoremap() const
	{
		return modifiers.has_flag(KeyModifierMask::CMD_OR_CTRL);
	}

	bool is_command_or_control_pressed() const;

	void set_shift_pressed(bool p_pressed);

	bool is_shift_pressed() const { return modifiers.has_flag(KeyModifierMask::SHIFT); }

	void set_alt_pressed(bool p_pressed);

	bool is_alt_pressed() const { return modifiers.has_flag(KeyModifierMask::ALT); }

	void set_ctrl_pressed(bool p_pressed);

	bool is_ctrl_pressed() const { return modifiers.has_flag(KeyModifierMask::CTRL); }

	void set_meta_pressed(bool p_pressed);

	bool is_meta_pressed() const { return modifiers.has_flag(KeyModifierMask::META); }

	void copy_modifiers_from(const InputEvent& p_event);
	String modifiers_as_text() const;

	// Key helpers (InputEventType::KEY)
	Key get_keycode_with_modifiers() const;
	Key get_physical_keycode_with_modifiers() const;
	Key get_key_label_with_modifiers() const;
	String as_text_physical_keycode() const;
	String as_text_keycode() const;
	String as_text_key_label() const;
	String as_text_location() const;

	// Action queries & matching
	bool is_action(const String& p_action, bool p_exact_match = false) const;
	bool is_action_pressed(
		const String& p_action, bool p_allow_echo = false, bool p_exact_match = false) const;
	bool is_action_released(const String& p_action, bool p_exact_match = false) const;
	bool is_action_just_pressed_or_echo(const String& p_action, bool p_exact_match = false) const;
	float get_action_strength(const String& p_action, bool p_exact_match = false) const;
	float get_action_raw_strength(const String& p_action, bool p_exact_match = false) const;

	bool is_action_type() const;
	bool action_match(const InputEvent& p_event, bool p_exact_match, float p_deadzone,
		bool* r_pressed, float* r_strength, float* r_raw_strength) const;
	bool is_match(const InputEvent& p_event, bool p_exact_match = true) const;

	// Transform, accumulation, and formatting
	InputEvent xformed_by(const Transform2D& p_xform, const Vector2& p_local_ofs = Vector2()) const;
	bool accumulate(const InputEvent& p_event);

	String as_text() const;
	String to_string() const;
};

class Shortcut : public Resource
{
	Vector<InputEvent> events;

public:
	void set_events(const Vector<InputEvent>& p_events);
	const Vector<InputEvent>& get_events() const;

	void set_events_list(const List<InputEvent>* p_events);

	bool matches_event(const InputEvent& p_event) const;
	bool has_valid_event() const;

	String get_as_text() const;

	static Ref<Shortcut> make_from_action(const String& p_action);

	static bool is_event_array_equal(
		const Vector<InputEvent>& p_event_array1, const Vector<InputEvent>& p_event_array2);
};

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

#include "core/os/mutex.h" // IWYU pragma: keep // Used in macro.

#define _THREAD_SAFE_CLASS_ mutable Mutex _thread_safe_;
#define _THREAD_SAFE_METHOD_ MutexLock _thread_safe_method_(_thread_safe_);
#define _THREAD_SAFE_LOCK_ _thread_safe_.lock();
#define _THREAD_SAFE_UNLOCK_ _thread_safe_.unlock();

bool is_current_thread_safe_for_nodes();
void set_current_thread_safe_for_nodes(bool p_safe);

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

	static Vector2 get_joy_touchpad_finger_position(int p_device, int p_finger, int p_touchpad = 0);
	static float get_joy_touchpad_finger_pressure(int p_device, int p_finger, int p_touchpad = 0);

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

class DefaultControllerMappings
{
public:
	static const char* mappings[];
};

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

#if defined(SDL_ENABLED)

typedef uint32_t SDL_JoystickID;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_Gamepad SDL_Gamepad;

class JoypadSDL
{
public:
	~JoypadSDL();

	Error initialize();
	void process_events();

private:
	class Joypad : public Input::JoypadFeatures
	{
	public:
		bool attached = false;
		String guid;

		SDL_JoystickID sdl_instance_idx;

		bool supports_force_feedback = false;
		bool supports_motion_sensors = false;
		uint64_t ff_effect_timestamp = 0;

		virtual bool has_joy_light() const override;
		virtual void set_joy_light(const Color& p_color) override;

		virtual bool has_joy_motion_sensors() const override;
		virtual void set_joy_motion_sensors_enabled(bool p_enable) override;

		virtual bool has_joy_vibration() const override;

		virtual int get_joy_num_touchpads() const override;

		SDL_Joystick* get_sdl_joystick() const;
		SDL_Gamepad* get_sdl_gamepad() const;
	};

	Joypad joypads[Input::JOYPADS_MAX];
	HashMap<SDL_JoystickID, int> sdl_instance_id_to_joypad_id;

	void close_joypad(int p_pad_idx);
};
#endif // defined(SDL_ENABLED)

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
	static void get_argument_options(const String& p_function, int p_idx, List<String>* r_options);
#endif

	static String get_builtin_display_name(const String& p_name);
	static const HashMap<String, List<InputEvent>>& get_builtins();
	static const HashMap<String, List<InputEvent>>& get_builtins_with_feature_overrides_applied();

	InputMap() = delete;
	InputMap(const InputMap&) = delete;
	InputMap& operator=(const InputMap&) = delete;
	~InputMap() = delete;
};

#include <cstddef>
#include <cstdint>
#include <cstring>

#define VLTR_PROJECT_MAGIC_0 'V'
#define VLTR_PROJECT_MAGIC_1 'L'
#define VLTR_PROJECT_MAGIC_2 'T'
#define VLTR_PROJECT_MAGIC_3 'R'
#define VLTR_PROJECT_FORMAT_VERSION 1u

enum VoltaireProjectFlags
{
	PROJECT_FLAG_NONE = 0,
	PROJECT_FLAG_USE_3D = 1u << 0,
	PROJECT_FLAG_VSYNC = 1u << 1,
	PROJECT_FLAG_BORDERLESS = 1u << 2,
	PROJECT_FLAG_RESIZABLE = 1u << 3,
};

/* 24 bytes: exact byte-for-byte layout of the start of the file */
typedef struct VoltaireProjectHeader
{
	char magic[4];
	uint32_t format_version;
	uint32_t header_size;
	uint32_t config_size;
	uint32_t flags;
	uint32_t reserved;
} VoltaireProjectHeader;

/* 672 bytes: exact byte-for-byte layout of the project settings payload */
typedef struct VoltaireProjectConfig
{
	char project_name[128];
	char main_scene[256];
	char icon_path[256];

	uint32_t window_width;
	uint32_t window_height;
	uint32_t physics_ticks_per_second;
	uint32_t max_fps;

	float clear_color[4];
} VoltaireProjectConfig;

/* 696 bytes: the entire file on disk is literally just these two structs back-to-back */
typedef struct VoltaireProjectFile
{
	VoltaireProjectHeader header;
	VoltaireProjectConfig config;
} VoltaireProjectFile;

/* Compile-time layout locks so compiler padding never silently shifts offsets */
static_assert(sizeof(VoltaireProjectHeader) == 24, "Header must be 24 bytes");
static_assert(sizeof(VoltaireProjectConfig) == 672, "Config must be 672 bytes");
static_assert(sizeof(VoltaireProjectFile) == 696, "File must be 696 bytes");
static_assert(offsetof(VoltaireProjectFile, config) == 24, "Config must immediately follow header");

static inline void vltr_copy_cstr(char* p_dst, size_t p_dst_cap, const char* p_src)
{
	if (!p_dst || p_dst_cap == 0) {
		return;
	}
	memset(p_dst, 0, p_dst_cap);
	if (!p_src) {
		return;
	}
	size_t len = strlen(p_src);
	size_t copy_len = len < (p_dst_cap - 1) ? len : (p_dst_cap - 1);
	memcpy(p_dst, p_src, copy_len);
}

static inline void vltr_project_init(
	VoltaireProjectFile* p_file, const char* p_name, const char* p_main_scene)
{
	memset(p_file, 0, sizeof(*p_file));

	p_file->header.magic[0] = VLTR_PROJECT_MAGIC_0;
	p_file->header.magic[1] = VLTR_PROJECT_MAGIC_1;
	p_file->header.magic[2] = VLTR_PROJECT_MAGIC_2;
	p_file->header.magic[3] = VLTR_PROJECT_MAGIC_3;
	p_file->header.format_version = VLTR_PROJECT_FORMAT_VERSION;
	p_file->header.header_size = sizeof(VoltaireProjectHeader);
	p_file->header.config_size = sizeof(VoltaireProjectConfig);
	p_file->header.flags = PROJECT_FLAG_VSYNC | PROJECT_FLAG_RESIZABLE;

	vltr_copy_cstr(p_file->config.project_name, sizeof(p_file->config.project_name), p_name);
	vltr_copy_cstr(p_file->config.main_scene, sizeof(p_file->config.main_scene), p_main_scene);

	p_file->config.window_width = 1152;
	p_file->config.window_height = 648;
	p_file->config.physics_ticks_per_second = 60;
	p_file->config.clear_color[0] = 0.12f;
	p_file->config.clear_color[1] = 0.14f;
	p_file->config.clear_color[2] = 0.17f;
	p_file->config.clear_color[3] = 1.00f;
}

static inline bool vltr_header_valid(const VoltaireProjectHeader* p_hdr)
{
	return p_hdr->magic[0] == VLTR_PROJECT_MAGIC_0 && p_hdr->magic[1] == VLTR_PROJECT_MAGIC_1 &&
		   p_hdr->magic[2] == VLTR_PROJECT_MAGIC_2 && p_hdr->magic[3] == VLTR_PROJECT_MAGIC_3 &&
		   p_hdr->format_version == VLTR_PROJECT_FORMAT_VERSION &&
		   p_hdr->header_size == sizeof(VoltaireProjectHeader) &&
		   p_hdr->config_size == sizeof(VoltaireProjectConfig);
}

static inline bool vltr_project_valid(const VoltaireProjectFile* p_file)
{
	return vltr_header_valid(&p_file->header) &&
		   p_file->config.project_name[sizeof(p_file->config.project_name) - 1] == '\0' &&
		   p_file->config.main_scene[sizeof(p_file->config.main_scene) - 1] == '\0' &&
		   p_file->config.icon_path[sizeof(p_file->config.icon_path) - 1] == '\0';
}

template <typename T> class TypedArray;

class ProjectSettings final
{
	static inline BinaryMutex _thread_safe_mutex;
	friend class TestProjectSettingsInternalsAccessor;

public:
	typedef HashMap<String, String> StringMap;
	// This constant is used to make the ".voltaire" folder and paths like "res://.voltaire/editor".
	static inline const String PROJECT_DATA_DIR_NAME_SUFFIX = "voltaire";
	static inline const String EDITOR_SETTING_OVERRIDE_PREFIX =
		PNAME("editor_overrides") + String("/");

	// Properties that are not for built in values begin from this value, so builtin ones are
	// displayed first.
	constexpr static const int32_t NO_BUILTIN_ORDER_BASE = 1 << 16;

#ifdef TOOLS_ENABLED
	const static Vector<String> get_required_features();
	const static Vector<String> get_unsupported_features(const Vector<String>& p_project_features);
#endif // TOOLS_ENABLED

	struct AutoloadInfo
	{
		String name;
		String path;
		bool is_singleton = false;
	};

private:
	static bool _property_can_revert(const String& p_name);

	static void _queue_changed(const String& p_name);
	static void _emit_changed();

	static Error _load_settings_text(const String& p_path);
	static Error _load_settings_binary(const String& p_path);
	static Error _load_settings_text_or_binary(const String& p_text_path, const String& p_bin_path);

	static Error _save_settings_text(const String& p_file,
		const RBMap<String, List<String>>& p_props, const StringMap& p_custom = StringMap(),
		const String& p_custom_features = String());
	static Error _save_settings_binary(const String& p_file,
		const RBMap<String, List<String>>& p_props, const StringMap& p_custom = StringMap(),
		const String& p_custom_features = String());

	static Error _save_custom_bnd(const String& p_file);

#ifdef TOOLS_ENABLED
	const static Vector<String> _get_supported_features();
	const static Vector<String> _trim_to_supported_features(
		const Vector<String>& p_project_features);
#endif // TOOLS_ENABLED

	static void _convert_to_last_version(int p_from_version);

	static bool load_resource_pack(const String& p_pack, bool p_replace_files, int p_offset);
	static bool _load_resource_pack(const String& p_pack, bool p_replace_files = true,
		int p_offset = 0, bool p_main_pack = false);

	static Error _setup(const String& p_path, const String& p_main_pack, bool p_upwards = false,
		bool p_ignore_override = false);

	static void _add_builtin_input_map();

private:
	struct Data
	{
		VoltaireProjectFile project;

		bool is_changed = false;

		// Starting version from 1 ensures that all callers can reset their tested version to 0,
		// and will always detect the initial project settings as a "change".
		uint32_t _version = 1;

		// Track changed settings for get_changed_settings functionality
		HashSet<String> changed_settings;

		int last_order = NO_BUILTIN_ORDER_BASE;
		int last_builtin_order = 0;
		uint64_t last_save_time = 0;

		String resource_path = "";
		bool using_datapack = false;
		bool project_loaded = false;
		List<String> input_presets;

		HashSet<String> custom_features;
		HashMap<String, LocalVector<Pair<String, String>>> feature_overrides;
		LocalVector<String> hidden_prefixes;
		HashMap<String, AutoloadInfo> autoloads;
		HashMap<String, String> global_groups;
		HashMap<String, HashSet<String>> scene_groups_cache;

		Vector<String> global_class_list;
		bool is_global_class_list_loaded = false;
		String project_data_dir_name;
	};

public:
	static const int CONFIG_VERSION = 5;
	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();

	static const VoltaireProjectFile& get_project() { return data->project; }

	static VoltaireProjectFile& get_project_mut() { return data->project; }

	static void refresh_global_class_list();
	static void store_global_class_list(const Vector<String>& p_classes);
	static String get_global_class_list_path();

	static bool is_initialized();
	static bool has_setting(const String& p_var);
	static String localize_path(const String& p_path);
	static String globalize_path(const String& p_path);

	static void set_as_basic(const String& p_name, bool p_basic);
	static void set_as_internal(const String& p_name, bool p_internal);
	static void set_restart_if_changed(const String& p_name, bool p_restart);
	static void set_ignore_value_in_docs(const String& p_name, bool p_ignore);
	static bool get_ignore_value_in_docs(const String& p_name);
	static void add_hidden_prefix(const String& p_prefix);

	static String get_project_data_dir_name();
	static String get_project_data_path();
	static String get_resource_path();
	static String get_imported_files_path();

	static void clear(const String& p_name);
	static int get_order(const String& p_name);
	static void set_order(const String& p_name, int p_order);
	static void set_builtin_order(const String& p_name);
	static bool is_builtin_setting(const String& p_name);

	static Error setup(const String& p_path, const String& p_main_pack, bool p_upwards = false,
		bool p_ignore_override = false);

	static Error load_custom(const String& p_path);
	static Error save_custom(const String& p_path = "", const StringMap& p_custom = StringMap(),
		const Vector<String>& p_custom_features = Vector<String>(),
		bool p_merge_with_current = true);
	static Error save();

	static uint64_t get_last_saved_time() { return data->last_save_time; }

	static List<String> get_input_presets() { return List<String>(data->input_presets); }

	static bool is_using_datapack();
	static bool is_project_loaded();

	static bool has_custom_feature(const String& p_feature);

	// Change tracking methods
	static Vector<String> get_changed_settings();
	static bool check_changed_settings_in_group(const String& p_setting_prefix);

	static const HashMap<String, AutoloadInfo>& get_autoload_list();
	static void add_autoload(const AutoloadInfo& p_autoload, bool p_front_insert = false);
	static void remove_autoload(const String& p_autoload);
	static bool has_autoload(const String& p_autoload);
	static AutoloadInfo get_autoload(const String& p_name);
	static void fix_autoload_paths();

	static const HashMap<String, String>& get_global_groups_list();
	static void add_global_group(const String& p_name, const String& p_description);
	static void remove_global_group(const String& p_name);
	static bool has_global_group(const String& p_name);

	static const HashMap<String, HashSet<String>>& get_scene_groups_cache();
	static void add_scene_groups_cache(const String& p_path, const HashSet<String>& p_cache);
	static void remove_scene_groups_cache(const String& p_path);
	static void save_scene_groups_cache();
	static String get_scene_groups_cache_path();
	static void load_scene_groups_cache();

	// Testing a version allows fast cached GET_GLOBAL macros.
	static uint32_t get_version() { return data->_version; }

#ifdef TOOLS_ENABLED
	static void get_argument_options(const String& p_function, int p_idx, List<String>* r_options);
#endif

	static bool has_editor_setting_override(const String& p_setting);

	ProjectSettings() = delete;
	ProjectSettings(const String& p_path) = delete;
	~ProjectSettings() = delete;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Cached versions of GLOBAL_GET.
// Cached but uses a typed variable for storage, this can be more efficient.
// Variables prefixed with _ggc_ to avoid shadowing warnings.
#define GLOBAL_GET_CACHED(m_type, m_setting_name)                                                  \
	([](const char* p_name) -> m_type {                                                            \
		static_assert(std::is_trivially_destructible<m_type>::value,                               \
			"GLOBAL_GET_CACHED must use a trivial type that allows static lifetime.");             \
		static m_type _ggc_local_var;                                                              \
		static uint32_t _ggc_local_version = 0;                                                    \
		static SpinLock _ggc_spin;                                                                 \
		uint32_t _ggc_new_version = ProjectSettings::get_version();                                \
		if (_ggc_local_version != _ggc_new_version) {                                              \
			_ggc_spin.lock();                                                                      \
			_ggc_local_version = _ggc_new_version;                                                 \
			m_type _ggc_temp = _ggc_local_var;                                                     \
			_ggc_spin.unlock();                                                                    \
			return _ggc_temp;                                                                      \
		}                                                                                          \
		_ggc_spin.lock();                                                                          \
		m_type _ggc_temp2 = _ggc_local_var;                                                        \
		_ggc_spin.unlock();                                                                        \
		return _ggc_temp2;                                                                         \
	})(m_setting_name)

class CoreConstants
{
public:
	static int get_global_constant_count();
	static String get_global_constant_enum(int p_idx);
	static bool is_global_constant_bitfield(int p_idx);
	static bool get_ignore_value_in_docs(int p_idx);
	static const char* get_global_constant_name(int p_idx);
	static int64_t get_global_constant_value(int p_idx);
	static bool is_global_constant(const String& p_name);
	static int get_global_constant_index(const String& p_name);
	static bool is_global_enum(const String& p_enum);
	static void get_enum_values(const String& p_enum, HashMap<String, int64_t>* r_values);
#ifdef TOOLS_ENABLED
	static void get_global_enums(List<String>* r_values);
#endif
};

class PluralRules;

class Translation : public Resource
{
public:
	struct MessageKey
	{
		String msgctxt;
		String msgid;

		// Required to use this struct as a key in HashMap.
		static uint32_t hash(const MessageKey& p_key)
		{
			uint32_t h = hash_murmur3_one_32(HashMapHasherDefault::hash(p_key.msgctxt));
			return hash_fmix32(hash_murmur3_one_32(HashMapHasherDefault::hash(p_key.msgid), h));
		}

		bool operator==(const MessageKey& p_key) const
		{
			return msgctxt == p_key.msgctxt && msgid == p_key.msgid;
		}
	};

private:
	String locale = "en";

	HashMap<MessageKey, Vector<String>, MessageKey> translation_map;

	// Compressed perfect-hash lookup tables (populated by `generate()`).
	// `hash_table[hash(0, text)]` produces a `bucket_table` index or 0xFFFFFFFF if not found.
	Vector<int> hash_table;

	// Continuous `Bucket`s in a flat layout.
	Vector<int> bucket_table;

	// Data for translated strings, UTF-8 encoded, either compressed or uncompressed.
	Vector<uint8_t> strings;

	struct Bucket
	{
		// Number of `Elem` objects at `elem`.
		int size;

		// Use `hash(func, text)` to generate the unique `Elem::key` in this bucket.
		uint32_t func;

		struct Elem
		{
			// Unique key for the text.
			uint32_t key;

			// Used to index into `strings`.
			uint32_t str_offset;

			// The string is not compressed if `comp_size` equals `uncomp_size`.
			uint32_t comp_size;
			uint32_t uncomp_size;
		};

		Elem elem[1];
	};

	_FORCE_INLINE_ uint32_t hash(uint32_t p_d, const char* p_str) const
	{
		if (p_d == 0) {
			p_d = 0x1000193;
		}
		while (*p_str) {
			p_d = (p_d * 0x1000193) ^ static_cast<uint8_t>(*p_str);
			p_str++;
		}

		return p_d;
	}

	mutable PluralRules* plural_rules_cache = nullptr;
	String plural_rules_override;

	Vector<String> _get_message_list() const;
	PluralRules* _get_plural_rules() const;

public:
	void set_locale(const String& p_locale);

	_FORCE_INLINE_ String get_locale() const { return locale; }

	void add_message(
		const String& p_src_text, const String& p_xlated_text, const String& p_context = "");
	void add_plural_message(const String& p_src_text, const Vector<String>& p_plural_xlated_texts,
		const String& p_context = "");
	String get_message(const String& p_src_text, const String& p_context = "") const;
	String get_plural_message(const String& p_src_text, const String& p_plural_text, int p_n,
		const String& p_context = "") const;
	void erase_message(const String& p_src_text, const String& p_context = "");
	void get_message_list(List<String>* r_messages) const;
	int get_message_count() const;
	Vector<String> get_translated_message_list() const;

	bool generate(const Ref<Translation>& p_from);

	void set_plural_rules_override(const String& p_rules);
	String get_plural_rules_override() const;

	// This method is not exposed to scripting intentionally. It is only used by TranslationLoaderPO
	// and tests.
	int get_nplurals() const;

	~Translation();
};

class Translation;

class TranslationDomain : public RefCounted
{
	struct PseudolocalizationConfig
	{
		bool enabled = false;
		bool accents_enabled = true;
		bool double_vowels_enabled = false;
		bool fake_bidi_enabled = false;
		bool override_enabled = false;
		bool skip_placeholders_enabled = true;
		float expansion_ratio = 0.0;
		String prefix = "[";
		String suffix = "]";
	};

	bool enabled = true;

	String locale_override;
	HashSet<Ref<Translation>> translations;
	PseudolocalizationConfig pseudolocalization;

	String _get_override_string(const String& p_message) const;
	String _double_vowels(const String& p_message) const;
	String _replace_with_accented_string(const String& p_message) const;
	String _wrap_with_fakebidi_characters(const String& p_message) const;
	String _add_padding(const String& p_message, int p_length) const;
	const char32_t* _get_accented_version(char32_t p_character) const;
	bool _is_placeholder(const String& p_message, int p_index) const;

protected:
public:
	// Methods in this section are not intended for scripting.
	String get_message_from_translations(
		const String& p_locale, const String& p_message, const String& p_context) const;
	String get_message_from_translations(const String& p_locale, const String& p_message,
		const String& p_message_plural, int p_n, const String& p_context) const;
	PackedStringArray get_loaded_locales() const;

public:
#ifndef DISABLE_DEPRECATED
	Ref<Translation> get_translation_object(const String& p_locale) const;
#endif

	void add_translation(const Ref<Translation>& p_translation);
	void remove_translation(const Ref<Translation>& p_translation);
	void clear();

	bool has_translation(const Ref<Translation>& p_translation) const;
	const HashSet<Ref<Translation>> get_translations() const;
	HashSet<Ref<Translation>> find_translations(const String& p_locale, bool p_exact) const;
	bool has_translation_for_locale(const String& p_locale, bool p_exact) const;

	String translate(const String& p_message, const String& p_context) const;
	String translate_plural(const String& p_message, const String& p_message_plural, int p_n,
		const String& p_context) const;

	String get_locale_override() const;
	void set_locale_override(const String& p_locale);

	bool is_enabled() const;
	void set_enabled(bool p_enabled);

	bool is_pseudolocalization_enabled() const;
	void set_pseudolocalization_enabled(bool p_enabled);
	bool is_pseudolocalization_accents_enabled() const;
	void set_pseudolocalization_accents_enabled(bool p_enabled);
	bool is_pseudolocalization_double_vowels_enabled() const;
	void set_pseudolocalization_double_vowels_enabled(bool p_enabled);
	bool is_pseudolocalization_fake_bidi_enabled() const;
	void set_pseudolocalization_fake_bidi_enabled(bool p_enabled);
	bool is_pseudolocalization_override_enabled() const;
	void set_pseudolocalization_override_enabled(bool p_enabled);
	bool is_pseudolocalization_skip_placeholders_enabled() const;
	void set_pseudolocalization_skip_placeholders_enabled(bool p_enabled);
	float get_pseudolocalization_expansion_ratio() const;
	void set_pseudolocalization_expansion_ratio(float p_ratio);
	String get_pseudolocalization_prefix() const;
	void set_pseudolocalization_prefix(const String& p_prefix);
	String get_pseudolocalization_suffix() const;
	void set_pseudolocalization_suffix(const String& p_suffix);

	String pseudolocalize(const String& p_message) const;
};

class TranslationServer final
{
	struct Data
	{
		String locale = "en";
		bool allow_fallback = true;
		String fallback;

		Ref<TranslationDomain> main_domain;
#ifdef TOOLS_ENABLED
		Ref<TranslationDomain> editor_domain;
		Ref<TranslationDomain> property_domain;
		Ref<TranslationDomain> doc_domain;
#endif // TOOLS_ENABLED
		HashMap<String, Ref<TranslationDomain>> custom_domains;

		HashMap<String, int> locale_compare_cache;
	};

	static inline Data* data = nullptr;

#ifndef DISABLE_DEPRECATED
	static String _standardize_locale_bind_compat_98972(const String& p_locale);
	static void _bind_compatibility_methods();
#endif

	struct LocaleScriptInfo
	{
		String name;
		String script;
		String default_country;
		HashSet<String> supported_countries;
	};

	static inline Vector<LocaleScriptInfo> locale_script_info;

	struct Locale
	{
		String language;
		String script;
		String country;
		String variant;

		bool operator==(const Locale& p_locale) const
		{
			return (p_locale.language == language) && (p_locale.script == script) &&
				   (p_locale.country == country) && (p_locale.variant == variant);
		}

		explicit operator String() const;

		Locale(const String& p_locale, bool p_add_defaults);
	};

	static inline HashMap<String, String> language_map;
	static inline HashMap<String, String> script_map;
	static inline HashMap<String, String> locale_rename_map;
	static inline HashMap<String, String> country_name_map;
	static inline HashMap<String, String> country_rename_map;
	static inline HashMap<String, String> variant_map;
	static inline HashMap<String, String> plural_rules_map;
	static inline HashMap<String, int> num_system_map;
	static inline HashMap<String, HashSet<String>> language_script_map;

	static void init_locale_info();

public:
	static void initialize();
	static void finalize();

	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	// Built-in domain accessors. For engine code only, user code should use `get_or_add_domain()`
	// instead.
	static Ref<TranslationDomain> get_main_domain()
	{
		return data ? data->main_domain : Ref<TranslationDomain>();
	}
#ifdef TOOLS_ENABLED
	static Ref<TranslationDomain> get_editor_domain()
	{
		return data ? data->editor_domain : Ref<TranslationDomain>();
	}

	static Ref<TranslationDomain> get_property_domain()
	{
		return data ? data->property_domain : Ref<TranslationDomain>();
	}

	static Ref<TranslationDomain> get_doc_domain()
	{
		return data ? data->doc_domain : Ref<TranslationDomain>();
	}
#endif // TOOLS_ENABLED

	static void set_locale(const String& p_locale);
	static String get_locale();
	static void set_fallback_allowed(const bool& p_allow);
	static bool is_fallback_allowed();
	static void set_fallback_locale(const String& p_locale);
	static String get_fallback_locale();

#ifndef DISABLE_DEPRECATED
	static Ref<Translation> get_translation_object(const String& p_locale);
#endif

	static bool has_translation(const Ref<Translation>& p_translation);
	static bool has_translation_for_locale(const String& p_locale, bool p_exact);

	static Vector<String> get_all_languages();
	static String get_language_name(const String& p_language);

	static Vector<String> get_all_scripts();
	static String get_script_name(const String& p_script);

	static Vector<String> get_all_countries();
	static String get_country_name(const String& p_country);

	static String get_locale_name(const String& p_locale);
	static String get_plural_rules(const String& p_locale);

	static bool is_script_suppored_by_locale(const String& p_locale, const String& p_script);

	static PackedStringArray get_loaded_locales();

	static void add_translation(const Ref<Translation>& p_translation);
	static void remove_translation(const Ref<Translation>& p_translation);

	static String translate(const String& p_message, const String& p_context = "");
	static String translate_plural(const String& p_message, const String& p_message_plural, int p_n,
		const String& p_context = "");

	static String pseudolocalize(const String& p_message);

	static bool is_pseudolocalization_enabled();
	static void set_pseudolocalization_enabled(bool p_enabled);
	static void reload_pseudolocalization();

	static String format_number(const String& p_string, const String& p_locale);
	static String parse_number(const String& p_string, const String& p_locale);
	static String get_percent_sign(const String& p_locale);

	static String standardize_locale(const String& p_locale, bool p_add_defaults = false);

	static int compare_locales(const String& p_locale_a, const String& p_locale_b);

	static String get_tool_locale();

	static bool has_domain(const String& p_domain);
	static Ref<TranslationDomain> get_or_add_domain(const String& p_domain);
	static void remove_domain(const String& p_domain);

	static void setup();

	static void clear();

	static void load_project_translations(Ref<TranslationDomain> p_domain);

#ifdef TOOLS_ENABLED
	static void get_argument_options(const String& p_function, int p_idx, List<String>* r_options);
#endif // TOOLS_ENABLED

	TranslationServer() = delete;
	TranslationServer(const TranslationServer&) = delete;
	TranslationServer& operator=(const TranslationServer&) = delete;
	~TranslationServer() = delete;
};

class Expression;

class PluralRules
{
	mutable LRUCache<int, int> cache;

	// These two fields are initialized in the constructor.
	const int nplurals;
	const String plural;

	// Cache temporary variables related to `evaluate()` to make it faster.
	class EQNode : public RefCounted
	{
	public:
		String regex;
		Ref<EQNode> left;
		Ref<EQNode> right;
	};

	Ref<EQNode> equi_tests;
	Ref<Expression> expr;

	int _find_unquoted(const String& p_src, char32_t p_chr) const;
	void _cache_plural_tests(const String& p_plural_rule, Ref<EQNode>& p_node);

	PluralRules(int p_nplurals, const String& p_plural);

public:
	int evaluate(int p_n) const;

	int get_nplurals() const { return nplurals; }

	String get_plural() const { return plural; }

	static PluralRules* parse(const String& p_rules);
};

class FuzzyTokenMatch;

struct FuzzySearchToken
{
	int idx = -1;
	String string;

	bool try_exact_match(FuzzyTokenMatch& p_match, const String& p_target, int p_offset) const;
	bool try_fuzzy_match(
		FuzzyTokenMatch& p_match, const String& p_target, int p_offset, int p_miss_budget) const;
};

class FuzzyTokenMatch
{
	friend struct FuzzySearchToken;
	friend class FuzzySearchMatch;
	friend class FuzzySearch;

	int matched_length = 0;
	int token_length = 0;
	int token_idx = -1;
	Vector2i interval = Vector2i(-1, -1); // x and y are both inclusive indices.

	void add_substring(int p_substring_start, int p_substring_length);
	bool intersects(const Vector2i& p_other_interval) const;
	bool is_case_insensitive(const String& p_original, const String& p_adjusted) const;

	int get_miss_count() const { return token_length - matched_length; }

public:
	int score = 0;
	Vector<Vector2i> substrings; // x is start index, y is length.
};

class FuzzySearchMatch : public RefCounted
{
	friend class FuzzySearch;

	String target;
	int score = 0;
	int original_index = -1;
	int dir_index = -1;
	Vector<FuzzyTokenMatch> token_matches;
	int miss_budget = 0;
	Vector2i match_interval = Vector2i(-1, -1);

	bool _can_add_token_match(const FuzzyTokenMatch& p_match) const;
	void _score_token_match(FuzzyTokenMatch& p_match, bool p_case_insensitive) const;
	void _add_token_match(const FuzzyTokenMatch& p_match);
	void _maybe_apply_token_order_score_bonus();

protected:
public:
	void set_target(const String& p_target) { target = p_target; }

	String get_target() const { return target; }

	void set_score(int p_score) { score = p_score; }

	int get_score() const { return score; }

	void set_original_index(int p_original_index) { original_index = p_original_index; }

	int get_original_index() const { return original_index; }

	void set_dir_index(int p_dir_index) { dir_index = p_dir_index; }

	int get_dir_index() const { return dir_index; }

	Vector<FuzzyTokenMatch> get_token_matches() { return token_matches; }

	Vector<Vector2i> get_matched_substrings() const;
};

class FuzzySearch : public RefCounted
{
	int start_offset = 0;
	int max_results = 100;
	int max_misses = 2;
	bool exact_token_matches = false;
	bool case_sensitive = false;
	bool filter_low_scores = true;
	float filter_factor = 0.1f;
	float filter_cutoff = 30.0f;

	Vector<FuzzySearchToken> _get_tokens(const String& p_query) const;
	void _sort_and_filter(Vector<Ref<FuzzySearchMatch>>& p_results) const;
	bool _search_tokens(const Vector<FuzzySearchToken>& p_tokens, const String& p_target,
		Ref<FuzzySearchMatch>& r_result) const;

protected:
public:
	void set_start_offset(int p_offset)
	{
		ERR_FAIL_COND(p_offset < 0);
		start_offset = p_offset;
	}

	int get_start_offset() const { return start_offset; }

	void set_max_results(int p_max_results)
	{
		ERR_FAIL_COND(p_max_results <= 0);
		max_results = p_max_results;
	}

	int get_max_results() const { return max_results; }

	void set_max_misses(int p_max_misses)
	{
		ERR_FAIL_COND(p_max_misses < 0);
		max_misses = p_max_misses;
	}

	int get_max_misses() const { return max_misses; }

	void set_use_exact_tokens(bool p_use_exact_tokens) { exact_token_matches = p_use_exact_tokens; }

	bool get_use_exact_tokens() const { return exact_token_matches; }

	void set_case_sensitive(bool p_case_sensitive);

	bool get_case_sensitive() const { return case_sensitive; }

	void set_filter_low_scores(bool p_filter_low_scores)
	{
		filter_low_scores = p_filter_low_scores;
	}

	bool get_filter_low_scores() const { return filter_low_scores; }

	void set_filter_factor(float p_filter_factor)
	{
		ERR_FAIL_COND_MSG(p_filter_factor < 0.0f || p_filter_factor > 1.0f,
			"filter_factor should be in the range [0, 1]");
		filter_factor = p_filter_factor;
	}

	float get_filter_factor() const { return filter_factor; }

	void set_filter_cutoff(float p_filter_cutoff) { filter_cutoff = p_filter_cutoff; }

	float get_filter_cutoff() const { return filter_cutoff; }

	Ref<FuzzySearchMatch> search(const String& p_query, const String& p_target) const;
	Vector<Ref<FuzzySearchMatch>> search_all(
		const String& p_query, const PackedStringArray& p_targets) const;
};

class RegExMatch final : public RefCounted
{
	struct Range
	{
		int start = 0;
		int end = 0;
	};

	String subject;
	Vector<Range> data;
	HashMap<String, int> names;

	friend class RegEx;

protected:
public:
	String get_subject() const;
	int get_group_count() const;

	Vector<String> get_strings() const;
};

class RegEx final : public RefCounted
{
	void* general_ctx = nullptr;
	void* code = nullptr;
	String pattern;

	void _pattern_info(uint32_t p_what, void* p_where) const;

	int _sub(const String& p_subject, const String& p_replacement, int p_offset, int p_end,
		uint32_t p_flags, String& r_output) const;

protected:
#ifndef DISABLE_DEPRECATED
	static Ref<RegEx> _create_from_string_bind_compat_95212(const String& p_pattern);
	Error _compile_bind_compat_95212(const String& p_pattern);
	static void _bind_compatibility_methods();
#endif

public:
	static Ref<RegEx> create_from_string(const String& p_pattern, bool p_show_error = true);
	void clear();
	Error compile(const String& p_pattern, bool p_show_error = true);
	Ref<RegExMatch> search(const String& p_subject, int p_offset = 0, int p_end = -1) const;
	String sub(const String& p_subject, const String& p_replacement, bool p_all = false,
		int p_offset = 0, int p_end = -1) const;
	bool is_valid() const;
	String get_pattern() const;
	int get_group_count() const;
	Vector<String> get_names() const;

	RegEx();
	RegEx(const String& p_pattern);
	~RegEx();
};

