/**************************************************************************/
/*  os.cpp                                                                */
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

#include <cstdarg>
#include <cstdio>
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/os/main_loop.h"
#include "core/os/midi_driver.h"
#include "core/version_generated.gen.h"
#include "os.h"

#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <thirdparty/mingw-std-threads/mingw.thread.h>
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <thread>
#define THREADING_NAMESPACE std
#endif

bool OS::prefer_meta_over_ctrl()
{
#if defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	return true;
#elif defined(WEB_ENABLED)
	return has_feature("web_macos") || has_feature("web_ios");
#else
	return false;
#endif
}

void OS::delete_main_loop() { MainLoop::clear(); }

uint64_t OS::get_ticks_msec() { return get_ticks_usec() / 1000ULL; }

double OS::get_unix_time() { return 0; }

void OS::_set_logger(CompositeLogger* p_logger)
{
	if (_logger) {
		memdelete(_logger);
	}
	_logger = p_logger;
}

void OS::add_logger(Logger* p_logger)
{
	if (!_logger) {
		Vector<Logger*> loggers;
		loggers.push_back(p_logger);
		_logger = memnew(CompositeLogger(loggers));
	}
	else {
		_logger->add_logger(p_logger);
	}
}

String OS::get_identifier() { return get_name().to_lower(); }

void OS::print(const char* p_format, ...)
{
	if (!_stdout_enabled) {
		return;
	}

	va_list argp;
	va_start(argp, p_format);

	if (_logger) {
		_logger->logv(p_format, argp, false);
	}

	va_end(argp);
}

void OS::print_rich(const char* p_format, ...)
{
	if (!_stdout_enabled) {
		return;
	}

	va_list argp;
	va_start(argp, p_format);

	if (_logger) {
		_logger->logv(p_format, argp, false);
	}

	va_end(argp);
}

void OS::printerr(const char* p_format, ...)
{
	if (!_stderr_enabled) {
		return;
	}

	va_list argp;
	va_start(argp, p_format);

	if (_logger) {
		_logger->logv(p_format, argp, true);
	}

	va_end(argp);
}

void OS::alert(const String& p_alert, const String& p_title)
{
	fprintf(stderr, "%s: %s\n", p_title.utf8().get_data(), p_alert.utf8().get_data());
}

void OS::set_low_processor_usage_mode(bool p_enabled) { low_processor_usage_mode = p_enabled; }

bool OS::is_in_low_processor_usage_mode() { return low_processor_usage_mode; }

void OS::set_low_processor_usage_mode_sleep_usec(int p_usec)
{
	low_processor_usage_mode_sleep_usec = p_usec;
}

int OS::get_low_processor_usage_mode_sleep_usec()
{
	return low_processor_usage_mode_sleep_usec;
}

void OS::set_delta_smoothing(bool p_enabled) { _delta_smoothing_enabled = p_enabled; }

bool OS::is_delta_smoothing_enabled() { return _delta_smoothing_enabled; }

String OS::get_executable_path() { return _execpath; }

int OS::get_process_id() { return -1; }

bool OS::is_stdout_verbose() { return _verbose_stdout; }

bool OS::is_stdout_debug_enabled() { return _debug_stdout; }

bool OS::is_stdout_enabled() { return _stdout_enabled; }

bool OS::is_stderr_enabled() { return _stderr_enabled; }

void OS::set_stdout_enabled(bool p_enabled) { _stdout_enabled = p_enabled; }

void OS::set_stderr_enabled(bool p_enabled) { _stderr_enabled = p_enabled; }

String OS::multibyte_to_string(const String& p_encoding, const PackedByteArray& p_array)
{
	return String();
}

PackedByteArray OS::string_to_multibyte(const String& p_encoding, const String& p_string)
{
	return PackedByteArray();
}

int OS::get_exit_code() { return _exit_code; }

void OS::set_exit_code(int p_code) { _exit_code = p_code; }

String OS::get_locale() { return "en"; }

// Non-virtual helper to extract the 2 or 3-letter language code from
// `get_locale()` in a way that's consistent for all platforms.
String OS::get_locale_language() { return get_locale().left(3).remove_char('_'); }

// Embedded PCK offset.
uint64_t OS::get_embedded_pck_offset() { return 0; }

// Default boot screen rect scale mode is "Keep Aspect Centered"
Rect2 OS::calculate_boot_screen_rect(const Size2& p_window_size, const Size2& p_imgrect_size)
{
	Rect2 screenrect;
	if (p_window_size.width > p_window_size.height) {
		// Scale horizontally.
		screenrect.size.y = p_window_size.height;
		screenrect.size.x = p_imgrect_size.x * p_window_size.height / p_imgrect_size.y;
		screenrect.position.x = (p_window_size.width - screenrect.size.x) / 2;
	}
	else {
		// Scale vertically.
		screenrect.size.x = p_window_size.width;
		screenrect.size.y = p_imgrect_size.y * p_window_size.width / p_imgrect_size.x;
		screenrect.position.y = (p_window_size.height - screenrect.size.y) / 2;
	}
	return screenrect;
}

// Helper function to ensure that a dir name/path will be valid on the OS
String OS::get_safe_dir_name(const String& p_dir_name, bool p_allow_paths)
{
	String safe_dir_name = p_dir_name;
	Vector<String> invalid_chars = String(": * ? \" < > |").split(" ");
	if (p_allow_paths) {
		// Dir separators are allowed, but disallow ".." to avoid going up the filesystem
		invalid_chars.push_back("..");
		safe_dir_name = safe_dir_name.replace_char('\\', '/').replace("//", "/").strip_edges();
	}
	else {
		invalid_chars.push_back("/");
		invalid_chars.push_back("\\");
		safe_dir_name = safe_dir_name.strip_edges();

		// These directory names are invalid.
		if (safe_dir_name == ".") {
			safe_dir_name = "dot";
		}
		else if (safe_dir_name == "..") {
			safe_dir_name = "twodots";
		}
	}

	for (int i = 0; i < invalid_chars.size(); i++) {
		safe_dir_name = safe_dir_name.replace(invalid_chars[i], "-");
	}

	// Trim trailing periods from the returned value as it's not valid for folder names on Windows.
	// This check is still applied on non-Windows platforms so the returned value is consistent
	// across platforms.
	return safe_dir_name.rstrip(".");
}

// Path to data, config, cache, etc. OS-specific folders

// Get properly capitalized engine name for system paths
String OS::get_godot_dir_name()
{
	// Default to lowercase, so only override when different case is needed
	return String(VLTR_VERSION_SHORT_NAME).to_lower();
}

// OS equivalent of XDG_DATA_HOME
String OS::get_data_path() { return "."; }

// OS equivalent of XDG_CONFIG_HOME
String OS::get_config_path() { return "."; }

// OS equivalent of XDG_CACHE_HOME
String OS::get_cache_path() { return "."; }

String OS::get_temp_path() { return "."; }

// Path to macOS .app bundle resources
String OS::get_bundle_resource_dir() { return "."; }

// Path to macOS .app bundle embedded icon (.icns file).
String OS::get_bundle_icon_path() { return String(); }

// Name of macOS .app bundle embedded icon (Liquid Glass asset name).
String OS::get_bundle_icon_name() { return String(); }

// OS specific path for user://
String OS::get_user_data_dir(const String& p_user_dir) { return "."; }

// Absolute path to res://
String OS::get_resource_dir()
{
	return ProjectSettings::get_resource_path();
}

// Access system-specific dirs like Documents, Downloads, etc.
String OS::get_system_dir(SystemDir p_dir, bool p_shared_storage) { return "."; }

String OS::expand_path(const String& p_path) { return p_path; }

void OS::create_lock_file()
{
	if (Engine::is_recovery_mode_hint()) {
		return;
	}

	String lock_file_path = get_user_data_dir().path_join(".recovery_mode_lock");
	Ref<FileAccess> lock_file = FileAccess::open(lock_file_path, FileAccess::WRITE);
	if (lock_file.is_valid()) {
		lock_file->close();
	}
}

void OS::remove_lock_file()
{
	String lock_file_path = get_user_data_dir().path_join(".recovery_mode_lock");
	DirAccess::remove_absolute(lock_file_path);
}

#if !defined(LINUXBSD_ENABLED)
Error OS::shell_open(const String& p_uri) { return ERR_UNAVAILABLE; }
#endif

Error OS::shell_show_in_file_manager(String p_path, bool p_open_folder)
{
	p_path = p_path.trim_prefix("file://");

	if (!DirAccess::dir_exists_absolute(p_path)) {
		p_path = p_path.get_base_dir();
	}

	p_path = String("file://") + p_path;

	return shell_open(p_path);
}

// implement these with the canvas?

uint64_t OS::get_static_memory_usage() { return Memory::get_mem_usage(); }

uint64_t OS::get_static_memory_peak_usage() { return Memory::get_mem_max_usage(); }

#if !defined(LINUXBSD_ENABLED)
Error OS::set_cwd(const String& p_cwd) { return ERR_CANT_OPEN; }

String OS::get_cwd() { return "."; }
#endif

void OS::yield() {}

void OS::ensure_user_data_dir()
{
	String dd = get_user_data_dir();
	if (DirAccess::exists(dd)) {
		return;
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	Error err = da->make_dir_recursive(dd);
	ERR_FAIL_COND_MSG(err != OK, vformat("Error attempting to create data dir: %s.", dd));
}

String OS::get_model_name() { return "GenericDevice"; }

void OS::set_cmdline(
	const char* p_execpath, const List<String>& p_args, const List<String>& p_user_args)
{
	_execpath = String::utf8(p_execpath);
	_cmdline = p_args;
	_user_args = p_user_args;
}

String OS::get_unique_id() { return ""; }

int OS::get_processor_count() { return THREADING_NAMESPACE::thread::hardware_concurrency(); }

String OS::get_processor_name() { return ""; }

void OS::set_has_server_feature_callback(HasServerFeatureCallback p_callback)
{
	has_server_feature_callback = p_callback;
}

bool OS::has_feature(const String& p_feature)
{
	// Feature tags are always lowercase for consistency.
	if (p_feature == get_identifier()) {
		return true;
	}

	if (p_feature == "movie") {
		return _writing_movie;
	}

#ifdef DEBUG_ENABLED
	if (p_feature == "debug") {
		return true;
	}
#endif // DEBUG_ENABLED

#ifdef TOOLS_ENABLED
	if (p_feature == "editor") {
		return true;
	}
	if (p_feature == "editor_hint") {
		return _in_editor;
	}
	else if (p_feature == "editor_runtime") {
		return !_in_editor;
	}
	else if (p_feature == "embedded_in_editor") {
		return _embedded_in_editor;
	}
#else
	if (p_feature == "template") {
		return true;
	}
#ifdef DEBUG_ENABLED
	if (p_feature == "template_debug") {
		return true;
	}
#else
	if (p_feature == "template_release" || p_feature == "release") {
		return true;
	}
#endif // DEBUG_ENABLED
#endif // TOOLS_ENABLED

#ifdef REAL_T_IS_DOUBLE
	if (p_feature == "double") {
		return true;
	}
#else
	if (p_feature == "single") {
		return true;
	}
#endif // REAL_T_IS_DOUBLE

	if (sizeof(void*) == 8 && p_feature == "64") {
		return true;
	}
	if (sizeof(void*) == 4 && p_feature == "32") {
		return true;
	}
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__i386) ||           \
	defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
#if defined(MACOS_ENABLED)
	if (p_feature == "universal") {
		return true;
	}
#endif
	if (p_feature == "x86_64") {
		return true;
	}
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
	if (p_feature == "x86_32") {
		return true;
	}
#endif
	if (p_feature == "x86") {
		return true;
	}
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#if defined(__aarch64__) || defined(_M_ARM64)
#if defined(MACOS_ENABLED)
	if (p_feature == "universal") {
		return true;
	}
#endif
	if (p_feature == "arm64") {
		return true;
	}
#elif defined(__arm__) || defined(_M_ARM)
	if (p_feature == "arm32") {
		return true;
	}
#endif
#if defined(__ARM_ARCH_7A__)
	if (p_feature == "armv7a" || p_feature == "armv7") {
		return true;
	}
#endif
#if defined(__ARM_ARCH_7S__)
	if (p_feature == "armv7s" || p_feature == "armv7") {
		return true;
	}
#endif
	if (p_feature == "arm") {
		return true;
	}
#elif defined(__riscv)
#if __riscv_xlen == 8
	if (p_feature == "rv64") {
		return true;
	}
#endif
	if (p_feature == "riscv") {
		return true;
	}
#elif defined(__powerpc__)
#if defined(__powerpc64__)
	if (p_feature == "ppc64") {
		return true;
	}
#endif
	if (p_feature == "ppc") {
		return true;
	}
#elif defined(__wasm__)
#if defined(__wasm64__)
	if (p_feature == "wasm64") {
		return true;
	}
#elif defined(__wasm32__)
	if (p_feature == "wasm32") {
		return true;
	}
#endif
	if (p_feature == "wasm") {
		return true;
	}
#elif defined(__loongarch64)
	if (p_feature == "loongarch64") {
		return true;
	}
#endif

#if defined(IOS_SIMULATOR) || defined(VISIONOS_SIMULATOR)
	if (p_feature == "simulator") {
		return true;
	}
#endif

	if (p_feature == "threads") {
#ifdef THREADS_ENABLED
		return true;
#else
		return false;
#endif
	}
	if (p_feature == "nothreads") {
#ifdef THREADS_ENABLED
		return false;
#else
		return true;
#endif
	}

	if (_check_internal_feature_support(p_feature)) {
		return true;
	}

	if (has_server_feature_callback && has_server_feature_callback(p_feature)) {
		return true;
	}

	if (ProjectSettings::has_custom_feature(p_feature)) {
		return true;
	}

	return false;
}

bool OS::is_sandboxed() { return false; }

void OS::set_restart_on_exit(bool p_restart, const List<String>& p_restart_arguments)
{
	restart_on_exit = p_restart;
	restart_commandline = p_restart_arguments;
}

bool OS::is_restart_on_exit_set() { return restart_on_exit; }

List<String> OS::get_restart_on_exit_arguments() { return List<String>(restart_commandline); }

PackedStringArray OS::get_connected_midi_inputs()
{
	if (MIDIDriver::is_bound()) {
		return MIDIDriver::get_connected_inputs();
	}

	PackedStringArray list;
	ERR_FAIL_V_MSG(
		list, vformat("MIDI input isn't supported on %s.", OS::get_name()));
}

void OS::open_midi_inputs()
{
	if (MIDIDriver::is_bound()) {
		MIDIDriver::open();
	}
	else {
		ERR_PRINT(vformat("MIDI input isn't supported on %s.", OS::get_name()));
	}
}

void OS::close_midi_inputs()
{
	if (MIDIDriver::is_bound()) {
		MIDIDriver::close();
	}
	else {
		ERR_PRINT(vformat("MIDI input isn't supported on %s.", OS::get_name()));
	}
}

uint64_t OS::get_frame_delay(bool p_can_draw)
{
	const uint32_t frame_delay = Engine::get_frame_delay();

	// Add a dynamic frame delay to decrease CPU/GPU usage. This takes the
	// previous frame time into account for a smoother result.
	uint64_t dynamic_delay = 0;
	if (is_in_low_processor_usage_mode() || !p_can_draw) {
		dynamic_delay = get_low_processor_usage_mode_sleep_usec();
	}
	const int max_fps = Engine::get_max_fps();
	if (max_fps > 0 && !Engine::is_editor_hint()) {
		// Override the low processor usage mode sleep delay if the target FPS is lower.
		dynamic_delay = MAX(dynamic_delay, (uint64_t)(1000000 / max_fps));
	}

	return frame_delay + dynamic_delay;
}

void OS::add_frame_delay(bool p_can_draw, bool p_wake_for_events)
{
	const uint32_t frame_delay = Engine::get_frame_delay();
	if (frame_delay) {
		// Add fixed frame delay to decrease CPU/GPU usage. This doesn't take
		// the actual frame time into account.
		// Due to the high fluctuation of the actual sleep duration, it's not recommended
		// to use this as a FPS limiter.
		delay_usec(frame_delay * 1000);
	}

	// Add a dynamic frame delay to decrease CPU/GPU usage. This takes the
	// previous frame time into account for a smoother result.
	uint64_t dynamic_delay = 0;
	if (is_in_low_processor_usage_mode() || !p_can_draw) {
		dynamic_delay = get_low_processor_usage_mode_sleep_usec();
	}
	const int max_fps = Engine::get_max_fps();
	if (max_fps > 0 && !Engine::is_editor_hint()) {
		// Override the low processor usage mode sleep delay if the target FPS is lower.
		dynamic_delay = MAX(dynamic_delay, (uint64_t)(1000000 / max_fps));
	}

	if (dynamic_delay > 0) {
		target_ticks += dynamic_delay;
		uint64_t current_ticks = get_ticks_usec();

		if (current_ticks < target_ticks) {
			delay_usec(target_ticks - current_ticks);
		}

		current_ticks = get_ticks_usec();
		target_ticks =
			MIN(MAX(target_ticks, current_ticks - dynamic_delay), current_ticks + dynamic_delay);
	}
}

Error OS::setup_remote_filesystem(
	const String& p_server_host, int p_port, const String& p_password, String& r_project_path)
{
	return default_rfs.synchronize_with_server(p_server_host, p_port, p_password, r_project_path);
}

OS::PreferredTextureFormat OS::get_preferred_texture_format()
{
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
	return PREFERRED_TEXTURE_FORMAT_ETC2_ASTC; // By rule, ARM hardware uses ETC texture
											   // compression.
#elif defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) ||              \
	defined(__i386) || defined(_M_IX86)
	return PREFERRED_TEXTURE_FORMAT_S3TC_BPTC; // By rule, X86 hardware prefers S3TC and
											   // derivatives.
#else
	return PREFERRED_TEXTURE_FORMAT_S3TC_BPTC; // Override in platform if needed.
#endif
}

void OS::set_use_benchmark(bool p_use_benchmark) { use_benchmark = p_use_benchmark; }

bool OS::is_use_benchmark_set() { return use_benchmark; }

void OS::set_benchmark_file(const String& p_benchmark_file) { benchmark_file = p_benchmark_file; }

String OS::get_benchmark_file() { return benchmark_file; }

void OS::benchmark_begin_measure(const String& p_context, const String& p_what)
{
#ifdef TOOLS_ENABLED
	Pair<String, String> mark_key(p_context, p_what);
	ERR_FAIL_COND_MSG(benchmark_marks_from.has(mark_key),
		vformat("Benchmark key '%s:%s' already exists.", p_context, p_what));

	benchmark_marks_from[mark_key] = OS::get_ticks_usec();
#endif
}

void OS::benchmark_end_measure(const String& p_context, const String& p_what)
{
#ifdef TOOLS_ENABLED
	Pair<String, String> mark_key(p_context, p_what);
	ERR_FAIL_COND_MSG(!benchmark_marks_from.has(mark_key),
		vformat("Benchmark key '%s:%s' doesn't exist.", p_context, p_what));

	uint64_t total = OS::get_ticks_usec() - benchmark_marks_from[mark_key];
	double total_f = double(total) / double(1000000);
	benchmark_marks_final[mark_key] = total_f;
#endif
}

void OS::benchmark_dump()
{
#ifdef TOOLS_ENABLED
	if (!use_benchmark) {
		return;
	}

	if (!benchmark_file.is_empty()) {
		Ref<FileAccess> f = FileAccess::open(benchmark_file, FileAccess::WRITE);
		if (f.is_valid()) {
			Ref<JSON> json;
			json.instantiate();
		}
	}
	else {
		HashMap<String, String> results;
		for (const KeyValue<Pair<String, String>, double>& E : benchmark_marks_final) {
			if (E.key.first == "Startup" && !results.has(E.key.first)) {
				results.insert(
					E.key.first, "", true); // Hack to make sure "Startup" always comes first.
			}

			results[E.key.first] +=
				vformat("\t\t- %s: %.3f msec.\n", E.key.second, (E.value * 1000));
		}

		__print_line("BENCHMARK:");
		for (const KeyValue<String, String>& E : results) {
			__print_line(vformat("\t[%s]\n%s", E.key, E.value));
		}
	}
#endif
}

bool OS::is_initialized()
{
	return _initialized;
}

String OS::get_user_data_dir() { return String(); }

#ifdef MINGW_STDTHREAD_REDUNDANCY_WARNING
#undef MINGW_STDTHREAD_REDUNDANCY_WARNING
#endif


// ============================================================================
// Merged from core/os/keyboard.cpp
// ============================================================================

#include "keyboard.h"

#include "core/os/os.h"
#include "core/string/ustring.h"

struct _KeyCodeText {
	Key code;
	const char *text;
};

static const _KeyCodeText _keycodes[] = {
	/* clang-format off */
	{Key::ESCAPE                ,"Escape"},
	{Key::TAB                   ,"Tab"},
	{Key::BACKTAB               ,"Backtab"},
	{Key::BACKSPACE             ,"Backspace"},
	{Key::ENTER                 ,"Enter"},
	{Key::KP_ENTER              ,"Kp Enter"},
	{Key::INSERT                ,"Insert"},
	{Key::KEY_DELETE            ,"Delete"},
	{Key::PAUSE                 ,"Pause"},
	{Key::PRINT                 ,"Print"},
	{Key::SYSREQ                ,"SysReq"},
	{Key::CLEAR                 ,"Clear"},
	{Key::HOME                  ,"Home"},
	{Key::END                   ,"End"},
	{Key::LEFT                  ,"Left"},
	{Key::UP                    ,"Up"},
	{Key::RIGHT                 ,"Right"},
	{Key::DOWN                  ,"Down"},
	{Key::PAGEUP                ,"PageUp"},
	{Key::PAGEDOWN              ,"PageDown"},
	{Key::SHIFT                 ,"Shift"},
	{Key::CTRL                  ,"Ctrl"},
#if defined(MACOS_ENABLED)
	{Key::META                  ,"Command"},
	{Key::CMD_OR_CTRL           ,"Command"},
	{Key::ALT                   ,"Option"},
#elif defined(WINDOWS_ENABLED)
	{Key::META                  ,"Windows"},
	{Key::CMD_OR_CTRL           ,"Ctrl"},
	{Key::ALT                   ,"Alt"},
#else
	{Key::META                  ,"Meta"},
	{Key::CMD_OR_CTRL           ,"Ctrl"},
	{Key::ALT                   ,"Alt"},
#endif
	{Key::CAPSLOCK              ,"CapsLock"},
	{Key::NUMLOCK               ,"NumLock"},
	{Key::SCROLLLOCK            ,"ScrollLock"},
	{Key::F1                    ,"F1"},
	{Key::F2                    ,"F2"},
	{Key::F3                    ,"F3"},
	{Key::F4                    ,"F4"},
	{Key::F5                    ,"F5"},
	{Key::F6                    ,"F6"},
	{Key::F7                    ,"F7"},
	{Key::F8                    ,"F8"},
	{Key::F9                    ,"F9"},
	{Key::F10                   ,"F10"},
	{Key::F11                   ,"F11"},
	{Key::F12                   ,"F12"},
	{Key::F13                   ,"F13"},
	{Key::F14                   ,"F14"},
	{Key::F15                   ,"F15"},
	{Key::F16                   ,"F16"},
	{Key::F17                   ,"F17"},
	{Key::F18                   ,"F18"},
	{Key::F19                   ,"F19"},
	{Key::F20                   ,"F20"},
	{Key::F21                   ,"F21"},
	{Key::F22                   ,"F22"},
	{Key::F23                   ,"F23"},
	{Key::F24                   ,"F24"},
	{Key::F25                   ,"F25"},
	{Key::F26                   ,"F26"},
	{Key::F27                   ,"F27"},
	{Key::F28                   ,"F28"},
	{Key::F29                   ,"F29"},
	{Key::F30                   ,"F30"},
	{Key::F31                   ,"F31"},
	{Key::F32                   ,"F32"},
	{Key::F33                   ,"F33"},
	{Key::F34                   ,"F34"},
	{Key::F35                   ,"F35"},
	{Key::KP_MULTIPLY           ,"Kp Multiply"},
	{Key::KP_DIVIDE             ,"Kp Divide"},
	{Key::KP_SUBTRACT           ,"Kp Subtract"},
	{Key::KP_PERIOD             ,"Kp Period"},
	{Key::KP_ADD                ,"Kp Add"},
	{Key::KP_0                  ,"Kp 0"},
	{Key::KP_1                  ,"Kp 1"},
	{Key::KP_2                  ,"Kp 2"},
	{Key::KP_3                  ,"Kp 3"},
	{Key::KP_4                  ,"Kp 4"},
	{Key::KP_5                  ,"Kp 5"},
	{Key::KP_6                  ,"Kp 6"},
	{Key::KP_7                  ,"Kp 7"},
	{Key::KP_8                  ,"Kp 8"},
	{Key::KP_9                  ,"Kp 9"},
	{Key::MENU                  ,"Menu"},
	{Key::HYPER                 ,"Hyper"},
	{Key::HELP                  ,"Help"},
	{Key::BACK                  ,"Back"},
	{Key::FORWARD               ,"Forward"},
	{Key::STOP                  ,"Stop"},
	{Key::REFRESH               ,"Refresh"},
	{Key::VOLUMEDOWN            ,"VolumeDown"},
	{Key::VOLUMEMUTE            ,"VolumeMute"},
	{Key::VOLUMEUP              ,"VolumeUp"},
	{Key::MEDIAPLAY             ,"MediaPlay"},
	{Key::MEDIASTOP             ,"MediaStop"},
	{Key::MEDIAPREVIOUS         ,"MediaPrevious"},
	{Key::MEDIANEXT             ,"MediaNext"},
	{Key::MEDIARECORD           ,"MediaRecord"},
	{Key::HOMEPAGE              ,"HomePage"},
	{Key::FAVORITES             ,"Favorites"},
	{Key::SEARCH                ,"Search"},
	{Key::STANDBY               ,"StandBy"},
	{Key::OPENURL               ,"OpenURL"},
	{Key::LAUNCHMAIL            ,"LaunchMail"},
	{Key::LAUNCHMEDIA           ,"LaunchMedia"},
	{Key::LAUNCH0               ,"Launch0"},
	{Key::LAUNCH1               ,"Launch1"},
	{Key::LAUNCH2               ,"Launch2"},
	{Key::LAUNCH3               ,"Launch3"},
	{Key::LAUNCH4               ,"Launch4"},
	{Key::LAUNCH5               ,"Launch5"},
	{Key::LAUNCH6               ,"Launch6"},
	{Key::LAUNCH7               ,"Launch7"},
	{Key::LAUNCH8               ,"Launch8"},
	{Key::LAUNCH9               ,"Launch9"},
	{Key::LAUNCHA               ,"LaunchA"},
	{Key::LAUNCHB               ,"LaunchB"},
	{Key::LAUNCHC               ,"LaunchC"},
	{Key::LAUNCHD               ,"LaunchD"},
	{Key::LAUNCHE               ,"LaunchE"},
	{Key::LAUNCHF               ,"LaunchF"},
	{Key::GLOBE                 ,"Globe"},
	{Key::KEYBOARD              ,"On-screen keyboard"},
	{Key::JIS_EISU              ,"JIS Eisu"},
	{Key::JIS_KANA              ,"JIS Kana"},
	{Key::UNKNOWN               ,"Unknown"},
	{Key::SPACE                 ,"Space"},
	{Key::EXCLAM                ,"Exclam"},
	{Key::QUOTEDBL              ,"QuoteDbl"},
	{Key::NUMBERSIGN            ,"NumberSign"},
	{Key::DOLLAR                ,"Dollar"},
	{Key::PERCENT               ,"Percent"},
	{Key::AMPERSAND             ,"Ampersand"},
	{Key::APOSTROPHE            ,"Apostrophe"},
	{Key::PARENLEFT             ,"ParenLeft"},
	{Key::PARENRIGHT            ,"ParenRight"},
	{Key::ASTERISK              ,"Asterisk"},
	{Key::PLUS                  ,"Plus"},
	{Key::COMMA                 ,"Comma"},
	{Key::MINUS                 ,"Minus"},
	{Key::PERIOD                ,"Period"},
	{Key::SLASH                 ,"Slash"},
	{Key::KEY_0                 ,"0"},
	{Key::KEY_1                 ,"1"},
	{Key::KEY_2                 ,"2"},
	{Key::KEY_3                 ,"3"},
	{Key::KEY_4                 ,"4"},
	{Key::KEY_5                 ,"5"},
	{Key::KEY_6                 ,"6"},
	{Key::KEY_7                 ,"7"},
	{Key::KEY_8                 ,"8"},
	{Key::KEY_9                 ,"9"},
	{Key::COLON                 ,"Colon"},
	{Key::SEMICOLON             ,"Semicolon"},
	{Key::LESS                  ,"Less"},
	{Key::EQUAL                 ,"Equal"},
	{Key::GREATER               ,"Greater"},
	{Key::QUESTION              ,"Question"},
	{Key::AT                    ,"At"},
	{Key::A                     ,"A"},
	{Key::B                     ,"B"},
	{Key::C                     ,"C"},
	{Key::D                     ,"D"},
	{Key::E                     ,"E"},
	{Key::F                     ,"F"},
	{Key::G                     ,"G"},
	{Key::H                     ,"H"},
	{Key::I                     ,"I"},
	{Key::J                     ,"J"},
	{Key::K                     ,"K"},
	{Key::L                     ,"L"},
	{Key::M                     ,"M"},
	{Key::N                     ,"N"},
	{Key::O                     ,"O"},
	{Key::P                     ,"P"},
	{Key::Q                     ,"Q"},
	{Key::R                     ,"R"},
	{Key::S                     ,"S"},
	{Key::T                     ,"T"},
	{Key::U                     ,"U"},
	{Key::V                     ,"V"},
	{Key::W                     ,"W"},
	{Key::X                     ,"X"},
	{Key::Y                     ,"Y"},
	{Key::Z                     ,"Z"},
	{Key::BRACKETLEFT           ,"BracketLeft"},
	{Key::BACKSLASH             ,"BackSlash"},
	{Key::BRACKETRIGHT          ,"BracketRight"},
	{Key::ASCIICIRCUM           ,"AsciiCircum"},
	{Key::UNDERSCORE            ,"UnderScore"},
	{Key::QUOTELEFT             ,"QuoteLeft"},
	{Key::BRACELEFT             ,"BraceLeft"},
	{Key::BAR                   ,"Bar"},
	{Key::BRACERIGHT            ,"BraceRight"},
	{Key::ASCIITILDE            ,"AsciiTilde"},
	{Key::YEN                   ,"Yen"},
	{Key::SECTION               ,"Section"},
	{Key::NONE                  ,nullptr}
	/* clang-format on */
};

bool keycode_has_unicode(Key p_keycode) {
	switch (p_keycode) {
		case Key::ESCAPE:
		case Key::TAB:
		case Key::BACKTAB:
		case Key::BACKSPACE:
		case Key::ENTER:
		case Key::KP_ENTER:
		case Key::INSERT:
		case Key::KEY_DELETE:
		case Key::PAUSE:
		case Key::PRINT:
		case Key::SYSREQ:
		case Key::CLEAR:
		case Key::HOME:
		case Key::END:
		case Key::LEFT:
		case Key::UP:
		case Key::RIGHT:
		case Key::DOWN:
		case Key::PAGEUP:
		case Key::PAGEDOWN:
		case Key::SHIFT:
		case Key::CTRL:
		case Key::META:
		case Key::ALT:
		case Key::CAPSLOCK:
		case Key::NUMLOCK:
		case Key::SCROLLLOCK:
		case Key::F1:
		case Key::F2:
		case Key::F3:
		case Key::F4:
		case Key::F5:
		case Key::F6:
		case Key::F7:
		case Key::F8:
		case Key::F9:
		case Key::F10:
		case Key::F11:
		case Key::F12:
		case Key::F13:
		case Key::F14:
		case Key::F15:
		case Key::F16:
		case Key::F17:
		case Key::F18:
		case Key::F19:
		case Key::F20:
		case Key::F21:
		case Key::F22:
		case Key::F23:
		case Key::F24:
		case Key::F25:
		case Key::F26:
		case Key::F27:
		case Key::F28:
		case Key::F29:
		case Key::F30:
		case Key::F31:
		case Key::F32:
		case Key::F33:
		case Key::F34:
		case Key::F35:
		case Key::MENU:
		case Key::HYPER:
		case Key::HELP:
		case Key::BACK:
		case Key::FORWARD:
		case Key::STOP:
		case Key::REFRESH:
		case Key::VOLUMEDOWN:
		case Key::VOLUMEMUTE:
		case Key::VOLUMEUP:
		case Key::MEDIAPLAY:
		case Key::MEDIASTOP:
		case Key::MEDIAPREVIOUS:
		case Key::MEDIANEXT:
		case Key::MEDIARECORD:
		case Key::HOMEPAGE:
		case Key::FAVORITES:
		case Key::SEARCH:
		case Key::STANDBY:
		case Key::OPENURL:
		case Key::LAUNCHMAIL:
		case Key::LAUNCHMEDIA:
		case Key::LAUNCH0:
		case Key::LAUNCH1:
		case Key::LAUNCH2:
		case Key::LAUNCH3:
		case Key::LAUNCH4:
		case Key::LAUNCH5:
		case Key::LAUNCH6:
		case Key::LAUNCH7:
		case Key::LAUNCH8:
		case Key::LAUNCH9:
		case Key::LAUNCHA:
		case Key::LAUNCHB:
		case Key::LAUNCHC:
		case Key::LAUNCHD:
		case Key::LAUNCHE:
		case Key::LAUNCHF:
		case Key::GLOBE:
		case Key::KEYBOARD:
		case Key::JIS_EISU:
		case Key::JIS_KANA:
			return false;
		default: {
		}
	}

	return true;
}

String keycode_get_string(Key p_code) {
	Vector<String> keycode_string;
	if ((p_code & KeyModifierMask::CMD_OR_CTRL) != Key::NONE && !OS::prefer_meta_over_ctrl()) {
		keycode_string.push_back(find_keycode_name(Key::CTRL));
	}
	if ((p_code & KeyModifierMask::CTRL) != Key::NONE) {
		keycode_string.push_back(find_keycode_name(Key::CTRL));
	}
	if ((p_code & KeyModifierMask::ALT) != Key::NONE) {
		keycode_string.push_back(find_keycode_name(Key::ALT));
	}
	if ((p_code & KeyModifierMask::SHIFT) != Key::NONE) {
		keycode_string.push_back(find_keycode_name(Key::SHIFT));
	}
	if ((p_code & KeyModifierMask::CMD_OR_CTRL) != Key::NONE && OS::prefer_meta_over_ctrl()) {
		keycode_string.push_back(find_keycode_name(Key::META));
	}
	if ((p_code & KeyModifierMask::META) != Key::NONE) {
		keycode_string.push_back(find_keycode_name(Key::META));
	}

	p_code &= KeyModifierMask::CODE_MASK;
	if ((char32_t)p_code == 0) {
		// The key was just a modifier without any code.
		return String("+").join(keycode_string);
	}

	// The key is a named keycode.
	const _KeyCodeText *kct = &_keycodes[0];
	while (kct->text) {
		if (kct->code == p_code) {
			keycode_string.push_back(kct->text);
			return String("+").join(keycode_string);
		}
		kct++;
	}

	// The key is a single character.
	keycode_string.push_back(String::chr((char32_t)p_code));
	return String("+").join(keycode_string);
}

Key find_keycode(const String &p_codestr) {
	Key keycode = Key::NONE;
	Vector<String> code_parts = p_codestr.split("+");
	if (code_parts.is_empty()) {
		return keycode;
	}

	const String &last_part = code_parts[code_parts.size() - 1];
	const _KeyCodeText *kct = &_keycodes[0];

	while (kct->text) {
		if (last_part.nocasecmp_to(kct->text) == 0) {
			keycode = kct->code;
			break;
		}
		kct++;
	}

	for (int part = 0; part < code_parts.size() - 1; part++) {
		const String &code_part = code_parts[part];
		if (code_part.nocasecmp_to(find_keycode_name(Key::SHIFT)) == 0) {
			keycode |= KeyModifierMask::SHIFT;
		} else if (code_part.nocasecmp_to(find_keycode_name(Key::CTRL)) == 0) {
			keycode |= KeyModifierMask::CTRL;
		} else if (code_part.nocasecmp_to(find_keycode_name(Key::META)) == 0) {
			keycode |= KeyModifierMask::META;
		} else if (code_part.nocasecmp_to(find_keycode_name(Key::ALT)) == 0) {
			keycode |= KeyModifierMask::ALT;
		}
	}

	return keycode;
}

const char *find_keycode_name(Key p_keycode) {
	const _KeyCodeText *kct = &_keycodes[0];

	while (kct->text) {
		if (kct->code == p_keycode) {
			return kct->text;
		}
		kct++;
	}

	return "";
}

int keycode_get_count() {
	const _KeyCodeText *kct = &_keycodes[0];

	int count = 0;
	while (kct->text) {
		count++;
		kct++;
	}
	return count;
}

int keycode_get_value_by_index(int p_index) {
	return (int)_keycodes[p_index].code;
}

const char *keycode_get_name_by_index(int p_index) {
	return _keycodes[p_index].text;
}

char32_t fix_unicode(char32_t p_char) {
	if (p_char >= 0x20 && p_char != 0x7F) {
		return p_char;
	}
	return 0;
}

Key fix_keycode(char32_t p_char, Key p_key) {
	if (p_char >= 0x20 && p_char <= 0x7E) {
		return (Key)String::char_uppercase(p_char);
	}
	return p_key;
}

Key fix_key_label(char32_t p_char, Key p_key) {
	if (p_char >= 0x20 && p_char != 0x7F) {
		return (Key)String::char_uppercase(p_char);
	}
	return p_key;
}

// ============================================================================
// Merged from core/os/main_loop.cpp
// ============================================================================

#include "main_loop.h"

bool s_quit_requested = false;

void MainLoop::reset_backend()
{
	initialize = &_default_initialize;
	iteration_prepare = &_default_iteration_prepare;
	physics_process = &_default_physics_process;
	iteration_end = &_default_iteration_end;
	process = &_default_process;
	finalize = &_default_finalize;
	notification = &_default_notification;
	destroy = &_default_destroy;
	_active = false;
}

void MainLoop::clear()
{
	void (*destroy_fn)() = destroy;
	reset_backend();
	if (destroy_fn) {
		destroy_fn();
	}
}

// ============================================================================
// Merged from core/os/midi_driver.cpp
// ============================================================================

#include "core/input/input.h"
#include "midi_driver.h"

uint8_t MIDIDriver::last_received_message = 0x00;

void MIDIDriver::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);
	_initialize_backend();
}

void MIDIDriver::finalize()
{
	if (data == nullptr) {
		return;
	}
	close();
	_finalize_backend();
	memdelete(data);
	data = nullptr;
}

void MIDIDriver::reset_backend()
{
	_initialize_backend = &_default_initialize_backend;
	_finalize_backend = &_default_finalize_backend;
	open = &_default_open;
	close = &_default_close;
	_is_bound = false;
}

MIDIDriver::MessageCategory MIDIDriver::Parser::category(uint8_t p_midi_fragment)
{
	if (p_midi_fragment >= 0xf8) {
		return MessageCategory::RealTime;
	}
	else if (p_midi_fragment >= 0xf0) {
		// System Exclusive begin/end are specified as System Common Category
		// messages, but we separate them here and give them their own categories
		// as their behavior is significantly different.
		if (p_midi_fragment == 0xf0) {
			return MessageCategory::SysExBegin;
		}
		else if (p_midi_fragment == 0xf7) {
			return MessageCategory::SysExEnd;
		}
		return MessageCategory::SystemCommon;
	}
	else if (p_midi_fragment >= 0x80) {
		return MessageCategory::Voice;
	}
	return MessageCategory::Data;
}

MIDIMessage MIDIDriver::Parser::status_to_msg_enum(uint8_t p_status_byte)
{
	if (p_status_byte & 0x80) {
		if (p_status_byte < 0xf0) {
			return MIDIMessage(p_status_byte >> 4);
		}
		else {
			return MIDIMessage(p_status_byte);
		}
	}
	return MIDIMessage::NONE;
}

size_t MIDIDriver::Parser::expected_data(uint8_t p_status_byte)
{
	return expected_data(status_to_msg_enum(p_status_byte));
}

size_t MIDIDriver::Parser::expected_data(MIDIMessage p_msg_type)
{
	switch (p_msg_type) {
	case MIDIMessage::NOTE_OFF:
	case MIDIMessage::NOTE_ON:
	case MIDIMessage::AFTERTOUCH:
	case MIDIMessage::CONTROL_CHANGE:
	case MIDIMessage::PITCH_BEND:
	case MIDIMessage::SONG_POSITION_POINTER:
		return 2;
	case MIDIMessage::PROGRAM_CHANGE:
	case MIDIMessage::CHANNEL_PRESSURE:
	case MIDIMessage::QUARTER_FRAME:
	case MIDIMessage::SONG_SELECT:
		return 1;
	default:
		return 0;
	}
}

uint8_t MIDIDriver::Parser::channel(uint8_t p_status_byte)
{
	if (category(p_status_byte) == MessageCategory::Voice) {
		return p_status_byte & 0x0f;
	}
	return 0;
}

void MIDIDriver::send_event(
	int p_device_index, uint8_t p_status, const uint8_t* p_data, size_t p_data_len)
{
	const MIDIMessage msg = Parser::status_to_msg_enum(p_status);
	ERR_FAIL_COND(p_data_len < Parser::expected_data(msg));

	InputEvent event = InputEvent::make_midi();
	event.device = p_device_index;
	event.midi.channel = Parser::channel(p_status);
	event.midi.message = msg;
	switch (msg) {
	case MIDIMessage::NOTE_OFF:
	case MIDIMessage::NOTE_ON:
		event.midi.pitch = p_data[0];
		event.midi.velocity = p_data[1];
		break;
	case MIDIMessage::AFTERTOUCH:
		event.midi.pitch = p_data[0];
		event.midi.pressure = p_data[1];
		break;
	case MIDIMessage::CONTROL_CHANGE:
		event.midi.controller_number = p_data[0];
		event.midi.controller_value = p_data[1];
		break;
	case MIDIMessage::PROGRAM_CHANGE:
		event.midi.instrument = p_data[0];
		break;
	case MIDIMessage::CHANNEL_PRESSURE:
		event.midi.pressure = p_data[0];
		break;
	case MIDIMessage::PITCH_BEND:
		event.midi.pitch = (p_data[1] << 7) | p_data[0];
		break;
	// QUARTER_FRAME, SONG_POSITION_POINTER, and SONG_SELECT not yet implemented.
	default:
		break;
	}
	Input::parse_input_event(event);
}

void MIDIDriver::Parser::parse_fragment(uint8_t p_fragment)
{
	switch (category(p_fragment)) {
	case MessageCategory::RealTime:
		// Real-Time messages are single byte messages that can
		// occur at any point and do not interrupt other messages.
		// We pass them straight through.
		MIDIDriver::send_event(device_index, p_fragment);
		break;

	case MessageCategory::SysExBegin:
		status_byte = p_fragment;
		skipping_sys_ex = true;
		break;

	case MessageCategory::SysExEnd:
		status_byte = 0;
		skipping_sys_ex = false;
		break;

	case MessageCategory::Voice:
	case MessageCategory::SystemCommon:
		skipping_sys_ex = false; // If we were in SysEx, assume it was aborted.
		received_data_len = 0;
		status_byte = 0;
		ERR_FAIL_COND(expected_data(p_fragment) > DATA_BUFFER_SIZE);
		if (expected_data(p_fragment) == 0) {
			// No data bytes needed, post it now.
			MIDIDriver::send_event(device_index, p_fragment);
		}
		else {
			status_byte = p_fragment;
		}
		break;

	case MessageCategory::Data:
		// We don't currently process SysEx messages, so ignore their data.
		if (!skipping_sys_ex) {
			const size_t expected = expected_data(status_byte);
			if (received_data_len < expected) {
				data_buffer[received_data_len] = p_fragment;
				received_data_len++;
				if (received_data_len == expected) {
					MIDIDriver::send_event(device_index, status_byte, data_buffer, expected);
					received_data_len = 0;
					// Voice messages can use 'running status', sending further
					// messages without resending their status byte.
					// For other messages types we clear the cached status byte.
					if (category(status_byte) != MessageCategory::Voice) {
						status_byte = 0;
					}
				}
			}
		}
		break;
	}
}

void MIDIDriver::add_connected_input(const String& p_name)
{
	if (data != nullptr) {
		data->connected_input_names.push_back(p_name);
	}
}

void MIDIDriver::clear_connected_inputs()
{
	if (data != nullptr) {
		data->connected_input_names.clear();
	}
}

PackedStringArray MIDIDriver::get_connected_inputs()
{
	return data != nullptr ? data->connected_input_names : PackedStringArray();
}



// ============================================================================
// Merged from core/os/mutex.cpp
// ============================================================================

#include "mutex.h"

static Mutex _global_mutex;

void _global_lock() {
	_global_mutex.lock();
}

void _global_unlock() {
	_global_mutex.unlock();
}

#ifdef THREADS_ENABLED

template class MutexImpl<THREADING_NAMESPACE::recursive_mutex>;
template class MutexImpl<THREADING_NAMESPACE::mutex>;
template class MutexLock<MutexImpl<THREADING_NAMESPACE::recursive_mutex>>;
template class MutexLock<MutexImpl<THREADING_NAMESPACE::mutex>>;

#endif

// ============================================================================
// Merged from core/os/thread.cpp
// ============================================================================

#include "core/types.h"
#include "platform_config.h" // IWYU pragma: keep. Can override the implementation.

#ifndef PLATFORM_THREAD_OVERRIDE // See details in thread.h.

#include "thread.h"

#ifdef THREADS_ENABLED

SafeNumeric<uint64_t> Thread::id_counter(
	1); // The first value after .increment() is 2, hence by default the main thread ID should be 1.
thread_local Thread::ID Thread::caller_id = Thread::id_counter.increment();

#endif

Thread::PlatformFunctions Thread::platform_functions;

void Thread::_set_platform_functions(const PlatformFunctions& p_functions)
{
	platform_functions = p_functions;
}

#ifdef THREADS_ENABLED
void Thread::callback(
	ID p_caller_id, const Settings& p_settings, Callback p_callback, void* p_userdata)
{
	Thread::caller_id = p_caller_id;
	if (platform_functions.set_priority) {
		platform_functions.set_priority(p_settings.priority);
	}
	if (platform_functions.init) {
		platform_functions.init();
	}
	if (platform_functions.wrapper) {
		platform_functions.wrapper(p_callback, p_userdata);
	}
	else {
		p_callback(p_userdata);
	}
	if (platform_functions.term) {
		platform_functions.term();
	}
}

Thread::ID Thread::start(Thread::Callback p_callback, void* p_user, const Settings& p_settings)
{
	ERR_FAIL_COND_V_MSG(id != UNASSIGNED_ID, UNASSIGNED_ID,
		"A Thread object has been re-started without wait_to_finish() having been called on it.");
	id = id_counter.increment();
	thread = THREADING_NAMESPACE::thread(&Thread::callback, id, p_settings, p_callback, p_user);
	return id;
}

bool Thread::is_started() const { return id != UNASSIGNED_ID; }

void Thread::wait_to_finish()
{
	ERR_FAIL_COND_MSG(
		id == UNASSIGNED_ID, "Attempt of waiting to finish on a thread that was never started.");
	ERR_FAIL_COND_MSG(id == get_caller_id(),
		"Threads can't wait to finish on themselves, another thread must wait.");
	thread.join();
	thread = THREADING_NAMESPACE::thread();
	id = UNASSIGNED_ID;
}

void Thread::make_main_thread()
{
	if (caller_id == MAIN_ID) {
		return; // We're already the main thread
	}
	CRASH_COND_MSG(!is_main_thread_assigned.set_if_clear(),
		"A second thread attempted to become the main thread.");
	caller_id = MAIN_ID;
}

void Thread::release_main_thread()
{
	CRASH_COND_MSG(
		caller_id != MAIN_ID, "Trying to release main thread from a thread that isn't main.");
	CRASH_COND(!is_main_thread_assigned.clear_if_set());
	caller_id = id_counter.increment();
}

Error Thread::set_name(const String& p_name)
{
	if (platform_functions.set_name) {
		return platform_functions.set_name(p_name);
	}

	return ERR_UNAVAILABLE;
}

Thread::~Thread()
{
	if (id != UNASSIGNED_ID) {
#ifdef DEBUG_ENABLED
		WARN_PRINT(
			"A Thread object is being destroyed without its completion having been realized.\n"
			"Please call wait_to_finish() on it to ensure correct cleanup.");
#endif
		thread.detach();
	}
}

#endif // THREADS_ENABLED

#endif // PLATFORM_THREAD_OVERRIDE



// ============================================================================
// Merged from core/os/thread_safe.cpp
// ============================================================================

#ifndef THREAD_SAFE_CPP
#define THREAD_SAFE_CPP

#include "thread_safe.h"

static thread_local bool current_thread_safe_for_nodes = false;

bool is_current_thread_safe_for_nodes() {
	return current_thread_safe_for_nodes;
}

void set_current_thread_safe_for_nodes(bool p_safe) {
	current_thread_safe_for_nodes = p_safe;
}

#endif // THREAD_SAFE_CPP

#ifdef THREAD_SAFE_CPP
#undef THREAD_SAFE_CPP
#endif

// ============================================================================
// Merged from core/os/time.cpp
// ============================================================================

#include "core/os/os.h"
#include "time.h" // NOLINT(modernize-deprecated-headers) False positive with C-Header of the same name.

#define UNIX_EPOCH_YEAR_AD 1970		   // 1970
#define SECONDS_PER_DAY (24 * 60 * 60) // 86400
#define IS_LEAP_YEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEAR_SIZE(year) (IS_LEAP_YEAR(year) ? 366 : 365)

static constexpr int64_t total_leap_days(int64_t p_year)
{
	if (p_year > 0) {
		--p_year;
		return 1 + (p_year / 4 - p_year / 100 + p_year / 400);
	}

	return p_year / 4 - p_year / 100 + p_year / 400;
}

static constexpr int64_t year_to_days(int64_t p_year)
{
	return p_year * 365 + total_leap_days(p_year);
}

static constexpr int64_t days_to_year(int64_t p_days)
{
	int64_t year = 400 * p_days / year_to_days(400);
	if (year < 0) {
		--year;
	}
	if (year_to_days(year) > p_days) {
		--year;
	}
	if (year_to_days(year + 1) <= p_days) {
		++year;
	}
	return year;
}

#define YEAR_KEY "year"
#define MONTH_KEY "month"
#define DAY_KEY "day"
#define WEEKDAY_KEY "weekday"
#define HOUR_KEY "hour"
#define MINUTE_KEY "minute"
#define SECOND_KEY "second"
#define DST_KEY "dst"

// Table of number of days in each month (for regular year and leap year).
static const uint8_t MONTH_DAYS_TABLE[2][12] = {{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

#define UNIX_TIME_TO_HMS                                                                           \
	uint8_t hour, minute, second;                                                                  \
	{                                                                                              \
		/* The time of the day (in seconds since start of day). */                                 \
		uint32_t day_clock = Math::posmod(p_unix_time_val, SECONDS_PER_DAY);                       \
		/* On x86 these 4 lines can be optimized to only 2 divisions. */                           \
		second = day_clock % 60;                                                                   \
		day_clock /= 60;                                                                           \
		minute = day_clock % 60;                                                                   \
		hour = day_clock / 60;                                                                     \
	}

#define UNIX_TIME_TO_YMD                                                                           \
	int64_t year;                                                                                  \
	Month month;                                                                                   \
	uint8_t day;                                                                                   \
	/* The day number since Unix epoch (0-index). Days before 1970 are negative. */                \
	int64_t day_number = Math::floor(p_unix_time_val / (double)SECONDS_PER_DAY);                   \
	{                                                                                              \
		int64_t day_number_copy = day_number;                                                      \
		day_number_copy += year_to_days(UNIX_EPOCH_YEAR_AD);                                       \
		year = days_to_year(day_number_copy);                                                      \
		day_number_copy -= year_to_days(year);                                                     \
		uint8_t month_zero_index = 0;                                                              \
		/* After the above, day_number now represents the day of the year (0-index). */            \
		while (day_number_copy >= MONTH_DAYS_TABLE[IS_LEAP_YEAR(year)][month_zero_index]) {        \
			day_number_copy -= MONTH_DAYS_TABLE[IS_LEAP_YEAR(year)][month_zero_index];             \
			month_zero_index++;                                                                    \
		}                                                                                          \
		/* After the above, day_number now represents the day of the month (0-index). */           \
		month = (Month)(month_zero_index + 1);                                                     \
		day = day_number_copy + 1;                                                                 \
	}

#define VALIDATE_YMDHMS(ret)                                                                       \
	ERR_FAIL_COND_V_MSG(month == 0, ret,                                                           \
		"Invalid month value of: " + itos(month) +                                                 \
			", months are 1-indexed and cannot be 0. See the Time.Month enum for valid values.");  \
	ERR_FAIL_COND_V_MSG(month < 0, ret, "Invalid month value of: " + itos(month) + ".");           \
	ERR_FAIL_COND_V_MSG(month > 12, ret,                                                           \
		"Invalid month value of: " + itos(month) + ". See the Time.Month enum for valid values."); \
	ERR_FAIL_COND_V_MSG(hour > 23, ret, "Invalid hour value of: " + itos(hour) + ".");             \
	ERR_FAIL_COND_V_MSG(hour < 0, ret, "Invalid hour value of: " + itos(hour) + ".");              \
	ERR_FAIL_COND_V_MSG(minute > 59, ret, "Invalid minute value of: " + itos(minute) + ".");       \
	ERR_FAIL_COND_V_MSG(minute < 0, ret, "Invalid minute value of: " + itos(minute) + ".");        \
	ERR_FAIL_COND_V_MSG(second > 59, ret,                                                          \
		"Invalid second value of: " + itos(second) + " (leap seconds are not supported).");        \
	ERR_FAIL_COND_V_MSG(second < 0, ret, "Invalid second value of: " + itos(second) + ".");        \
	ERR_FAIL_COND_V_MSG(day == 0, ret,                                                             \
		"Invalid day value of: " + itos(day) + ", days are 1-indexed and cannot be 0.");           \
	ERR_FAIL_COND_V_MSG(day < 0, ret, "Invalid day value of: " + itos(day) + ".");                 \
	/* Do this check after month is tested as valid. */                                            \
	uint8_t days_in_this_month = MONTH_DAYS_TABLE[IS_LEAP_YEAR(year)][month - 1];                  \
	ERR_FAIL_COND_V_MSG(day > days_in_this_month, ret,                                             \
		"Invalid day value of: " + itos(day) +                                                     \
			" which is larger than the maximum for this month, " + itos(days_in_this_month) +      \
			".");

#define YMD_TO_DAY_NUMBER                                                                          \
	/* The day number since Unix epoch (0-index). Days before 1970 are negative. */                \
	int64_t day_number = day - 1;                                                                  \
	/* Add the days in the months to day_number. */                                                \
	for (int i = 0; i < month - 1; i++) {                                                          \
		day_number += MONTH_DAYS_TABLE[IS_LEAP_YEAR(year)][i];                                     \
	}                                                                                              \
	/* Add the days in the years to day_number. */                                                 \
	day_number += year_to_days(year);                                                              \
	day_number -= year_to_days(UNIX_EPOCH_YEAR_AD);

#define PARSE_ISO8601_STRING(ret)                                                                  \
	int64_t year = UNIX_EPOCH_YEAR_AD;                                                             \
	Month month = MONTH_JANUARY;                                                                   \
	int day = 1;                                                                                   \
	int hour = 0;                                                                                  \
	int minute = 0;                                                                                \
	int second = 0;                                                                                \
	{                                                                                              \
		bool has_date = false, has_time = false;                                                   \
		String date, time;                                                                         \
		if (p_datetime.find_char('T') > 0) {                                                       \
			has_date = has_time = true;                                                            \
			PackedStringArray array = p_datetime.split("T");                                       \
			ERR_FAIL_COND_V_MSG(array.size() < 2, ret, "Invalid ISO 8601 date/time string.");      \
			date = array[0];                                                                       \
			time = array[1];                                                                       \
		} \
		else if (p_datetime.find_char(' ') > 0) {                                                \
			has_date = has_time = true;                                                            \
			PackedStringArray array = p_datetime.split(" ");                                       \
			ERR_FAIL_COND_V_MSG(array.size() < 2, ret, "Invalid ISO 8601 date/time string.");      \
			date = array[0];                                                                       \
			time = array[1];                                                                       \
		} \
		else if (p_datetime.find_char('-', 1) > 0) {                                             \
			has_date = true;                                                                       \
			date = p_datetime;                                                                     \
		} \
		else if (p_datetime.find_char(':') > 0) {                                                \
			has_time = true;                                                                       \
			time = p_datetime;                                                                     \
		}                                                                                          \
		/* Set the variables from the contents of the string. */                                   \
		if (has_date) {                                                                            \
			PackedInt32Array array = date.split_ints("-", false);                                  \
			ERR_FAIL_COND_V_MSG(array.size() < 3, ret, "Invalid ISO 8601 date string.");           \
			year = array[0];                                                                       \
			month = (Month)array[1];                                                               \
			day = array[2];                                                                        \
			/* Handle negative years. */                                                           \
			if (p_datetime.find_char('-') == 0) {                                                  \
				year *= -1;                                                                        \
			}                                                                                      \
		}                                                                                          \
		if (has_time) {                                                                            \
			PackedInt32Array array = time.split_ints(":", false);                                  \
			ERR_FAIL_COND_V_MSG(array.size() < 3, ret, "Invalid ISO 8601 time string.");           \
			hour = array[0];                                                                       \
			minute = array[1];                                                                     \
			second = array[2];                                                                     \
		}                                                                                          \
	}

#define EXTRACT_FROM_DICTIONARY                                                                    \
	/* Get all time values from the dictionary. If it doesn't exist, set the */                    \
	/* values to the default values for Unix epoch (1970-01-01 00:00:00). */                       \
	int64_t year = p_datetime.has(YEAR_KEY) ? int64_t(p_datetime[YEAR_KEY]) : UNIX_EPOCH_YEAR_AD;  \
	Month month = Month((p_datetime.has(MONTH_KEY)) ? int(p_datetime[MONTH_KEY]) : 1);             \
	int day = p_datetime.has(DAY_KEY) ? int(p_datetime[DAY_KEY]) : 1;                              \
	int hour = p_datetime.has(HOUR_KEY) ? int(p_datetime[HOUR_KEY]) : 0;                           \
	int minute = p_datetime.has(MINUTE_KEY) ? int(p_datetime[MINUTE_KEY]) : 0;                     \
	int second = p_datetime.has(SECOND_KEY) ? int(p_datetime[SECOND_KEY]) : 0;

String Time::get_datetime_string_from_unix_time(int64_t p_unix_time_val, bool p_use_space)
{
	UNIX_TIME_TO_HMS
	UNIX_TIME_TO_YMD
	const String format_string =
		p_use_space ? "%04d-%02d-%02d %02d:%02d:%02d" : "%04d-%02d-%02dT%02d:%02d:%02d";
	return vformat(format_string, year, (uint8_t)month, day, hour, minute, second);
}

String Time::get_date_string_from_unix_time(int64_t p_unix_time_val)
{
	UNIX_TIME_TO_YMD
	// Android is picky about the types passed to make Variant, so we need a cast.
	return vformat("%04d-%02d-%02d", year, (uint8_t)month, day);
}

String Time::get_time_string_from_unix_time(int64_t p_unix_time_val)
{
	UNIX_TIME_TO_HMS
	return vformat("%02d:%02d:%02d", hour, minute, second);
}

int64_t Time::get_unix_time_from_datetime_string(const String& p_datetime)
{
	PARSE_ISO8601_STRING(-1)
	VALIDATE_YMDHMS(0)
	YMD_TO_DAY_NUMBER
	return day_number * SECONDS_PER_DAY + hour * 3600 + minute * 60 + second;
}

String Time::get_offset_string_from_offset_minutes(int64_t p_offset_minutes)
{
	String sign;
	if (p_offset_minutes < 0) {
		sign = "-";
		p_offset_minutes = -p_offset_minutes;
	}
	else {
		sign = "+";
	}
	// These two lines can be optimized to one instruction on x86 and others.
	// Note that % is acceptable here only because we ensure it's positive.
	int64_t offset_hours = p_offset_minutes / 60;
	int64_t offset_minutes = p_offset_minutes % 60;
	return vformat("%s%02d:%02d", sign, offset_hours, offset_minutes);
}

String Time::get_datetime_string_from_system(bool p_utc, bool p_use_space)
{
	OS::DateTime dt = OS::get_datetime(p_utc);
	const String format_string =
		p_use_space ? "%04d-%02d-%02d %02d:%02d:%02d" : "%04d-%02d-%02dT%02d:%02d:%02d";
	return vformat(
		format_string, dt.year, (uint8_t)dt.month, dt.day, dt.hour, dt.minute, dt.second);
}

String Time::get_date_string_from_system(bool p_utc)
{
	OS::DateTime dt = OS::get_datetime(p_utc);
	// Android is picky about the types passed to make Variant, so we need a cast.
	return vformat("%04d-%02d-%02d", dt.year, (uint8_t)dt.month, dt.day);
}

String Time::get_time_string_from_system(bool p_utc)
{
	OS::DateTime dt = OS::get_datetime(p_utc);
	return vformat("%02d:%02d:%02d", dt.hour, dt.minute, dt.second);
}

double Time::get_unix_time_from_system() { return OS::get_unix_time(); }

uint64_t Time::get_ticks_msec() { return OS::get_ticks_msec(); }

uint64_t Time::get_ticks_usec() { return OS::get_ticks_usec(); }

#ifdef UNIX_EPOCH_YEAR_AD
#undef UNIX_EPOCH_YEAR_AD
#endif
#ifdef SECONDS_PER_DAY
#undef SECONDS_PER_DAY
#endif
#ifdef IS_LEAP_YEAR
#undef IS_LEAP_YEAR
#endif
#ifdef YEAR_SIZE
#undef YEAR_SIZE
#endif
#ifdef YEAR_KEY
#undef YEAR_KEY
#endif
#ifdef MONTH_KEY
#undef MONTH_KEY
#endif
#ifdef DAY_KEY
#undef DAY_KEY
#endif
#ifdef WEEKDAY_KEY
#undef WEEKDAY_KEY
#endif
#ifdef HOUR_KEY
#undef HOUR_KEY
#endif
#ifdef MINUTE_KEY
#undef MINUTE_KEY
#endif
#ifdef SECOND_KEY
#undef SECOND_KEY
#endif
#ifdef DST_KEY
#undef DST_KEY
#endif
#ifdef UNIX_TIME_TO_HMS
#undef UNIX_TIME_TO_HMS
#endif
#ifdef UNIX_TIME_TO_YMD
#undef UNIX_TIME_TO_YMD
#endif
#ifdef VALIDATE_YMDHMS
#undef VALIDATE_YMDHMS
#endif
#ifdef YMD_TO_DAY_NUMBER
#undef YMD_TO_DAY_NUMBER
#endif
#ifdef PARSE_ISO8601_STRING
#undef PARSE_ISO8601_STRING
#endif
#ifdef EXTRACT_FROM_DICTIONARY
#undef EXTRACT_FROM_DICTIONARY
#endif

// ============================================================================
// Merged from core/os/memory.cpp
// ============================================================================

#include "core/templates/safe_refcount.h"
#include "memory.h"

#ifdef DEV_ENABLED
#include "core/math/math_funcs_binary.h"
#endif

#include <cstdlib>

#ifdef DEBUG_ENABLED
static SafeNumeric<uint64_t> _current_mem_usage;
static SafeNumeric<uint64_t> _max_mem_usage;
#endif

void* operator new(size_t p_size, DefaultAllocator p_allocator)
{
	return Memory::alloc_static(p_size);
}

void* operator new(size_t p_size, void* (*p_allocfunc)(size_t p_size))
{
	return p_allocfunc(p_size);
}

void* Memory::alloc_aligned_static(size_t p_bytes, size_t p_alignment)
{
	DEV_ASSERT(Math::is_power_of_2(p_alignment));

	void *p1, *p2;
	if ((p1 = (void*)malloc(p_bytes + p_alignment - 1 + sizeof(uint32_t))) == nullptr) {
		return nullptr;
	}
	p2 = (void*)(((uintptr_t)p1 + sizeof(uint32_t) + p_alignment - 1) & ~((p_alignment)-1));
	*((uint32_t*)p2 - 1) = (uint32_t)((uintptr_t)p2 - (uintptr_t)p1);
	return p2;
}

void* Memory::realloc_aligned_static(
	void* p_memory, size_t p_bytes, size_t p_prev_bytes, size_t p_alignment)
{
	if (p_memory == nullptr) {
		return alloc_aligned_static(p_bytes, p_alignment);
	}

	void* ret = alloc_aligned_static(p_bytes, p_alignment);
	if (ret) {
		memcpy(ret, p_memory, p_prev_bytes);
	}
	free_aligned_static(p_memory);
	return ret;
}

void Memory::free_aligned_static(void* p_memory)
{
	uint32_t offset = *((uint32_t*)p_memory - 1);
	void* p = (void*)((uint8_t*)p_memory - offset);
	free(p);
}

template <bool p_ensure_zero> void* Memory::alloc_static(size_t p_bytes, bool p_pad_align)
{
#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	void* mem;
	if constexpr (p_ensure_zero) {
		mem = calloc(1, p_bytes + (prepad ? DATA_OFFSET : 0));
	}
	else {
		mem = malloc(p_bytes + (prepad ? DATA_OFFSET : 0));
	}

	ERR_FAIL_NULL_V(mem, nullptr);

	if (prepad) {
		uint8_t* s8 = (uint8_t*)mem;

		uint64_t* s = (uint64_t*)(s8 + SIZE_OFFSET);
		*s = p_bytes;

#ifdef DEBUG_ENABLED
		uint64_t new_mem_usage = _current_mem_usage.add(p_bytes);
		_max_mem_usage.exchange_if_greater(new_mem_usage);
#endif
		return s8 + DATA_OFFSET;
	}
	else {
		return mem;
	}
}

template void* Memory::alloc_static<true>(size_t p_bytes, bool p_pad_align);
template void* Memory::alloc_static<false>(size_t p_bytes, bool p_pad_align);

void* Memory::realloc_static(void* p_memory, size_t p_bytes, bool p_pad_align)
{
	if (p_memory == nullptr) {
		return alloc_static(p_bytes, p_pad_align);
	}

	uint8_t* mem = (uint8_t*)p_memory;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= DATA_OFFSET;
		uint64_t* s = (uint64_t*)(mem + SIZE_OFFSET);

#ifdef DEBUG_ENABLED
		if (p_bytes > *s) {
			uint64_t new_mem_usage = _current_mem_usage.add(p_bytes - *s);
			_max_mem_usage.exchange_if_greater(new_mem_usage);
		}
		else {
			_current_mem_usage.sub(*s - p_bytes);
		}
#endif

		if (p_bytes == 0) {
			free(mem);
			return nullptr;
		}
		else {
			*s = p_bytes;

			mem = (uint8_t*)realloc(mem, p_bytes + DATA_OFFSET);
			ERR_FAIL_NULL_V(mem, nullptr);
			s = (uint64_t*)(mem + SIZE_OFFSET);
			*s = p_bytes;

			return mem + DATA_OFFSET;
		}
	}
	else {
		mem = (uint8_t*)realloc(mem, p_bytes);
		ERR_FAIL_COND_V(mem == nullptr && p_bytes > 0, nullptr);
		return mem;
	}
}

void Memory::free_static(void* p_ptr, bool p_pad_align)
{
	ERR_FAIL_NULL(p_ptr);

	uint8_t* mem = (uint8_t*)p_ptr;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= DATA_OFFSET;
#ifdef DEBUG_ENABLED
		uint64_t* s = (uint64_t*)(mem + SIZE_OFFSET);
		_current_mem_usage.sub(*s);
#endif
		free(mem);
	}
	else {
		free(mem);
	}
}

uint64_t Memory::get_mem_available()
{
	return -1; // 0xFFFF...
}

uint64_t Memory::get_mem_usage()
{
#ifdef DEBUG_ENABLED
	return _current_mem_usage.get();
#else
	return 0;
#endif
}

uint64_t Memory::get_mem_max_usage()
{
#ifdef DEBUG_ENABLED
	return _max_mem_usage.get();
#else
	return 0;
#endif
}

_GlobalNil::_GlobalNil()
{
	left = this;
	right = this;
	parent = this;
}

_GlobalNil _GlobalNilClass::_nil;


