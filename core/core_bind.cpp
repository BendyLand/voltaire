/**************************************************************************/
/*  core_bind.cpp                                                         */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/debugger/engine_debugger.h"
#include "core/debugger/script_debugger.h"
#include "core/io/file_access.h"
#include "core/io/marshalls.h"
#include "core/math/geometry_2d.h"
#include "core/math/geometry_3d.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/os/process_id.h"
#include "core/os/thread_safe.h"
#include "core/variant/typed_array.h"
#include "core_bind.compat.inc"
#include "core_bind.h"

namespace CoreBind
{

////// ResourceLoader //////

Error ResourceLoader::load_threaded_request(
	const String& p_path, const String& p_type_hint, bool p_use_sub_threads, CacheMode p_cache_mode)
{
	return ::ResourceLoader::load_threaded_request(
		p_path, p_type_hint, p_use_sub_threads, ResourceFormatLoader::CacheMode(p_cache_mode));
}

ResourceLoader::ThreadLoadStatus ResourceLoader::load_threaded_get_status(
	const String& p_path, Array r_progress)
{
	// Progress being the default array indicates the user hasn't requested for it to be computed.
	// Default array should never be modified, it causes the hash of the method to change.
	float progress = 0;
	::ResourceLoader::ThreadLoadStatus tls =
		::ResourceLoader::load_threaded_get_status(p_path, nullptr);
	return (ThreadLoadStatus)tls;
}

Ref<Resource> ResourceLoader::load_threaded_get(const String& p_path)
{
	Error error;
	Ref<Resource> res = ::ResourceLoader::load_threaded_get(p_path, &error);
	return res;
}

Ref<Resource> ResourceLoader::load(
	const String& p_path, const String& p_type_hint, CacheMode p_cache_mode)
{
	Error err = OK;
	Ref<Resource> ret = ::ResourceLoader::load(
		p_path, p_type_hint, ResourceFormatLoader::CacheMode(p_cache_mode), &err);

	ERR_FAIL_COND_V_MSG(err != OK, ret, vformat("Error loading resource: '%s'.", p_path));
	return ret;
}

Vector<String> ResourceLoader::get_recognized_extensions_for_type(const String& p_type)
{
	List<String> exts;
	::ResourceLoader::get_recognized_extensions_for_type(p_type, &exts);
	Vector<String> ret;
	for (const String& E : exts) {
		ret.push_back(E);
	}

	return ret;
}

void ResourceLoader::add_resource_format_loader(
	ResourceFormatLoader* rp_format_loader, bool p_at_front)
{
	::ResourceLoader::add_resource_format_loader(rp_format_loader, p_at_front);
}

void ResourceLoader::remove_resource_format_loader(ResourceFormatLoader* rp_format_loader)
{
	::ResourceLoader::remove_resource_format_loader(rp_format_loader);
}

void ResourceLoader::set_abort_on_missing_resources(bool p_abort)
{
	::ResourceLoader::set_abort_on_missing_resources(p_abort);
}

PackedStringArray ResourceLoader::get_dependencies(const String& p_path)
{
	List<String> deps;
	::ResourceLoader::get_dependencies(p_path, &deps);

	PackedStringArray ret;
	for (const String& E : deps) {
		ret.push_back(E);
	}

	return ret;
}

bool ResourceLoader::has_cached(const String& p_path)
{
	String local_path = ::ResourceLoader::_validate_local_path(p_path);
	return ResourceCache::has(local_path);
}

Ref<Resource> ResourceLoader::get_cached_ref(const String& p_path)
{
	String local_path = ::ResourceLoader::_validate_local_path(p_path);
	return ResourceCache::get_ref(local_path);
}

bool ResourceLoader::exists(const String& p_path, const String& p_type_hint)
{
	return ::ResourceLoader::exists(p_path, p_type_hint);
}

ResourceUID::ID ResourceLoader::get_resource_uid(const String& p_path)
{
	return ::ResourceLoader::get_resource_uid(p_path);
}

String ResourceLoader::get_resource_type(const String& p_path)
{
	return ::ResourceLoader::get_resource_type(p_path);
}

Vector<String> ResourceLoader::list_directory(const String& p_directory)
{
	return ::ResourceLoader::list_directory(p_directory);
}

void ResourceLoader::_bind_methods() {}

////// ResourceSaver //////

Error ResourceSaver::save(Resource* p_resource, const String& p_path, BitField<SaverFlags> p_flags)
{
	return ::ResourceSaver::save(p_resource, p_path, p_flags);
}

Error ResourceSaver::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	return ::ResourceSaver::set_uid(p_path, p_uid);
}

Vector<String> ResourceSaver::get_recognized_extensions(Resource* rp_resource)
{
	List<String> exts;
	::ResourceSaver::get_recognized_extensions(rp_resource, &exts);
	Vector<String> ret;
	for (const String& E : exts) {
		ret.push_back(E);
	}
	return ret;
}

void ResourceSaver::add_resource_format_saver(ResourceFormatSaver* rp_format_saver, bool p_at_front)
{
	::ResourceSaver::add_resource_format_saver(rp_format_saver, p_at_front);
}

void ResourceSaver::remove_resource_format_saver(ResourceFormatSaver* rp_format_saver)
{
	::ResourceSaver::remove_resource_format_saver(rp_format_saver);
}

ResourceUID::ID ResourceSaver::get_resource_id_for_path(const String& p_path, bool p_generate)
{
	return ::ResourceSaver::get_resource_id_for_path(p_path, p_generate);
}

void ResourceSaver::_bind_methods() {}

////// Logger ///////

void Logger::_bind_methods() {}

////// OS //////

void OS::LoggerBind::logv(const char* p_format, va_list p_list, bool p_err)
{
	if (!should_log(p_err)) {
		return;
	}

	constexpr int static_buf_size = 1024;
	char static_buf[static_buf_size] = {'\0'};
	char* buf = static_buf;
	va_list list_copy;
	va_copy(list_copy, p_list);
	int len = vsnprintf(buf, static_buf_size, p_format, p_list);
	if (len >= static_buf_size) {
		buf = (char*)Memory::alloc_static(len + 1);
		vsnprintf(buf, len + 1, p_format, list_copy);
	}
	va_end(list_copy);

	String str;
	str.append_utf8(buf, len);
	for (Ref<CoreBind::Logger>& logger : loggers) {
		logger->log_message(str, p_err);
	}

	if (len >= static_buf_size) {
		Memory::free_static(buf);
	}
}

void OS::LoggerBind::log_error(const char* p_function, const char* p_file, int p_line,
	const char* p_code, const char* p_rationale, bool p_editor_notify, ErrorType p_type,
	const Vector<Ref<ScriptBacktrace>>& p_script_backtraces)
{
	if (!should_log(true)) {
		return;
	}

	Array backtraces;
	backtraces.resize(p_script_backtraces.size());
	for (int i = 0; i < p_script_backtraces.size(); i++) {
		backtraces[i] = p_script_backtraces[i];
	}

	for (Ref<CoreBind::Logger>& logger : loggers) {
		logger->log_error(p_function, p_file, p_line, p_code, p_rationale, p_editor_notify,
			CoreBind::Logger::ErrorType(p_type), backtraces);
	}
}

void Logger::log_error(const char* p_function, const char* p_file, int p_line, const char* p_code,
	const char* p_rationale, bool p_editor_notify, Logger::ErrorType p_type,
	const Array& p_script_backtraces)
{
}

void Logger::log_message(const String& p_text, bool p_error) {}

PackedByteArray OS::get_entropy(int p_bytes)
{
	PackedByteArray pba;
	pba.resize(p_bytes);
	Error err = ::OS::get_singleton()->get_entropy(pba.ptrw(), p_bytes);
	ERR_FAIL_COND_V(err != OK, PackedByteArray());
	return pba;
}

String OS::get_system_ca_certificates()
{
	return ::OS::get_singleton()->get_system_ca_certificates();
}

PackedStringArray OS::get_connected_midi_inputs()
{
	return ::OS::get_singleton()->get_connected_midi_inputs();
}

void OS::open_midi_inputs() { ::OS::get_singleton()->open_midi_inputs(); }

void OS::close_midi_inputs() { ::OS::get_singleton()->close_midi_inputs(); }

void OS::set_use_file_access_save_and_swap(bool p_enable) { FileAccess::set_backup_save(p_enable); }

void OS::set_low_processor_usage_mode(bool p_enabled)
{
	::OS::get_singleton()->set_low_processor_usage_mode(p_enabled);
}

bool OS::is_in_low_processor_usage_mode() const
{
	return ::OS::get_singleton()->is_in_low_processor_usage_mode();
}

void OS::set_low_processor_usage_mode_sleep_usec(int p_usec)
{
	::OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(p_usec);
}

int OS::get_low_processor_usage_mode_sleep_usec() const
{
	return ::OS::get_singleton()->get_low_processor_usage_mode_sleep_usec();
}

void OS::set_delta_smoothing(bool p_enabled)
{
	::OS::get_singleton()->set_delta_smoothing(p_enabled);
}

bool OS::is_delta_smoothing_enabled() const
{
	return ::OS::get_singleton()->is_delta_smoothing_enabled();
}

void OS::alert(const String& p_alert, const String& p_title)
{
	::OS::get_singleton()->alert(p_alert, p_title);
}

void OS::crash(const String& p_message) { CRASH_NOW_MSG(p_message); }

Vector<String> OS::get_system_fonts() const { return ::OS::get_singleton()->get_system_fonts(); }

String OS::get_system_font_path(
	const String& p_font_name, int p_weight, int p_stretch, bool p_italic) const
{
	return ::OS::get_singleton()->get_system_font_path(p_font_name, p_weight, p_stretch, p_italic);
}

Vector<String> OS::get_system_font_path_for_text(const String& p_font_name, const String& p_text,
	const String& p_locale, const String& p_script, int p_weight, int p_stretch,
	bool p_italic) const
{
	return ::OS::get_singleton()->get_system_font_path_for_text(
		p_font_name, p_text, p_locale, p_script, p_weight, p_stretch, p_italic);
}

String OS::get_executable_path() const { return ::OS::get_singleton()->get_executable_path(); }

Error OS::shell_open(const String& p_uri)
{
	if (p_uri.begins_with("res://")) {
		WARN_PRINT("Attempting to open an URL with the \"res://\" protocol. Use "
				   "`ProjectSettings.globalize_path()` to convert a Godot-specific path to a "
				   "system path before opening it with `OS.shell_open()`.");
	}
	else if (p_uri.begins_with("user://")) {
		WARN_PRINT("Attempting to open an URL with the \"user://\" protocol. Use "
				   "`ProjectSettings.globalize_path()` to convert a Godot-specific path to a "
				   "system path before opening it with `OS.shell_open()`.");
	}
	return ::OS::get_singleton()->shell_open(p_uri);
}

Error OS::shell_show_in_file_manager(const String& p_path, bool p_open_folder)
{
	if (p_path.begins_with("res://")) {
		WARN_PRINT("Attempting to explore file path with the \"res://\" protocol. Use "
				   "`ProjectSettings.globalize_path()` to convert a Godot-specific path to a "
				   "system path before opening it with `OS.shell_show_in_file_manager()`.");
	}
	else if (p_path.begins_with("user://")) {
		WARN_PRINT("Attempting to explore file path with the \"user://\" protocol. Use "
				   "`ProjectSettings.globalize_path()` to convert a Godot-specific path to a "
				   "system path before opening it with `OS.shell_show_in_file_manager()`.");
	}
	return ::OS::get_singleton()->shell_show_in_file_manager(p_path, p_open_folder);
}

String OS::read_string_from_stdin(int64_t p_buffer_size)
{
	return ::OS::get_singleton()->get_stdin_string(p_buffer_size);
}

PackedByteArray OS::read_buffer_from_stdin(int64_t p_buffer_size)
{
	return ::OS::get_singleton()->get_stdin_buffer(p_buffer_size);
}

OS::StdHandleType OS::get_stdin_type() const
{
	return (OS::StdHandleType)::OS::get_singleton()->get_stdin_type();
}

OS::StdHandleType OS::get_stdout_type() const
{
	return (OS::StdHandleType)::OS::get_singleton()->get_stdout_type();
}

OS::StdHandleType OS::get_stderr_type() const
{
	return (OS::StdHandleType)::OS::get_singleton()->get_stderr_type();
}

int OS::execute(const String& p_path, const Vector<String>& p_arguments, Array r_output,
	bool p_read_stderr, bool p_open_console)
{
	List<String> args;
	for (const String& arg : p_arguments) {
		args.push_back(arg);
	}
	String pipe;
	int exitcode = 0;
	Error err = ::OS::get_singleton()->execute(
		p_path, args, &pipe, &exitcode, p_read_stderr, nullptr, p_open_console);
	// Default array should never be modified, it causes the hash of the method to change.
	if (err != OK) {
		return -1;
	}
	return exitcode;
}

Dictionary OS::execute_with_pipe(
	const String& p_path, const Vector<String>& p_arguments, bool p_blocking)
{
	List<String> args;
	for (const String& arg : p_arguments) {
		args.push_back(arg);
	}
	return ::OS::get_singleton()->execute_with_pipe(p_path, args, p_blocking);
}

int OS::create_instance(const Vector<String>& p_arguments)
{
	List<String> args;
	for (const String& arg : p_arguments) {
		args.push_back(arg);
	}
	ProcessID pid = 0;
	Error err = ::OS::get_singleton()->create_instance(args, &pid);
	if (err != OK) {
		return -1;
	}
	return pid;
}

Error OS::open_with_program(const String& p_program_path, const Vector<String>& p_paths)
{
	List<String> paths;
	for (const String& path : p_paths) {
		paths.push_back(path);
	}
	return ::OS::get_singleton()->open_with_program(p_program_path, paths);
}

int OS::create_process(const String& p_path, const Vector<String>& p_arguments, bool p_open_console)
{
	List<String> args;
	for (const String& arg : p_arguments) {
		args.push_back(arg);
	}
	ProcessID pid = 0;
	Error err = ::OS::get_singleton()->create_process(p_path, args, &pid, p_open_console);
	if (err != OK) {
		return -1;
	}
	return pid;
}

Error OS::kill(int p_pid) { return ::OS::get_singleton()->kill(p_pid); }

bool OS::is_process_running(int p_pid) const
{
	return ::OS::get_singleton()->is_process_running(p_pid);
}

int OS::get_process_exit_code(int p_pid) const
{
	return ::OS::get_singleton()->get_process_exit_code(p_pid);
}

int OS::get_process_id() const { return ::OS::get_singleton()->get_process_id(); }

bool OS::has_environment(const String& p_var) const
{
	return ::OS::get_singleton()->has_environment(p_var);
}

String OS::get_environment(const String& p_var) const
{
	return ::OS::get_singleton()->get_environment(p_var);
}

void OS::set_environment(const String& p_var, const String& p_value) const
{
	::OS::get_singleton()->set_environment(p_var, p_value);
}

void OS::unset_environment(const String& p_var) const
{
	::OS::get_singleton()->unset_environment(p_var);
}

String OS::get_name() const { return ::OS::get_singleton()->get_name(); }

String OS::get_distribution_name() const { return ::OS::get_singleton()->get_distribution_name(); }

String OS::get_version() const { return ::OS::get_singleton()->get_version(); }

String OS::get_version_alias() const { return ::OS::get_singleton()->get_version_alias(); }

Vector<String> OS::get_video_adapter_driver_info() const
{
	return ::OS::get_singleton()->get_video_adapter_driver_info();
}

Vector<String> OS::get_cmdline_args()
{
	List<String> cmdline = ::OS::get_singleton()->get_cmdline_args();
	Vector<String> cmdlinev;
	for (const String& E : cmdline) {
		cmdlinev.push_back(E);
	}

	return cmdlinev;
}

Vector<String> OS::get_cmdline_user_args()
{
	List<String> cmdline = ::OS::get_singleton()->get_cmdline_user_args();
	Vector<String> cmdlinev;
	for (const String& E : cmdline) {
		cmdlinev.push_back(E);
	}

	return cmdlinev;
}

void OS::set_restart_on_exit(bool p_restart, const Vector<String>& p_restart_arguments)
{
	List<String> args_list;
	for (const String& restart_argument : p_restart_arguments) {
		args_list.push_back(restart_argument);
	}

	::OS::get_singleton()->set_restart_on_exit(p_restart, args_list);
}

bool OS::is_restart_on_exit_set() const { return ::OS::get_singleton()->is_restart_on_exit_set(); }

Vector<String> OS::get_restart_on_exit_arguments() const
{
	List<String> args = ::OS::get_singleton()->get_restart_on_exit_arguments();
	Vector<String> args_vector;
	for (const String& arg : args) {
		args_vector.push_back(arg);
	}

	return args_vector;
}

String OS::get_locale() const { return ::OS::get_singleton()->get_locale(); }

Vector<String> OS::get_preferred_locales() const
{
	return ::OS::get_singleton()->get_preferred_locales();
}

String OS::get_locale_language() const { return ::OS::get_singleton()->get_locale_language(); }

String OS::get_model_name() const { return ::OS::get_singleton()->get_model_name(); }

Error OS::set_thread_name(const String& p_name) { return ::Thread::set_name(p_name); }

::Thread::ID OS::get_thread_caller_id() const { return ::Thread::get_caller_id(); }

::Thread::ID OS::get_main_thread_id() const { return ::Thread::get_main_id(); }

bool OS::has_feature(const String& p_feature) const
{
	const bool* value_ptr = feature_cache.getptr(p_feature);
	if (value_ptr) {
		return *value_ptr;
	}
	else {
		const bool has = ::OS::get_singleton()->has_feature(p_feature);
		feature_cache[p_feature] = has;
		return has;
	}
}

bool OS::is_sandboxed() const { return ::OS::get_singleton()->is_sandboxed(); }

uint64_t OS::get_static_memory_usage() const
{
	return ::OS::get_singleton()->get_static_memory_usage();
}

uint64_t OS::get_static_memory_peak_usage() const
{
	return ::OS::get_singleton()->get_static_memory_peak_usage();
}

Dictionary OS::get_memory_info() const { return ::OS::get_singleton()->get_memory_info(); }

/** This method uses a signed argument for better error reporting as it's used from the scripting
 * API. */
void OS::delay_usec(int p_usec) const
{
	ERR_FAIL_COND_MSG(p_usec < 0, vformat("Can't sleep for %d microseconds. The delay provided "
										  "must be greater than or equal to 0 microseconds.",
									  p_usec));
	::OS::get_singleton()->delay_usec(p_usec);
}

/** This method uses a signed argument for better error reporting as it's used from the scripting
 * API. */
void OS::delay_msec(int p_msec) const
{
	ERR_FAIL_COND_MSG(p_msec < 0, vformat("Can't sleep for %d milliseconds. The delay provided "
										  "must be greater than or equal to 0 milliseconds.",
									  p_msec));
	::OS::get_singleton()->delay_usec(int64_t(p_msec) * 1000);
}

bool OS::is_userfs_persistent() const { return ::OS::get_singleton()->is_userfs_persistent(); }

int OS::get_processor_count() const { return ::OS::get_singleton()->get_processor_count(); }

String OS::get_processor_name() const { return ::OS::get_singleton()->get_processor_name(); }

bool OS::is_stdout_verbose() const { return ::OS::get_singleton()->is_stdout_verbose(); }

Error OS::move_to_trash(const String& p_path) const
{
	return ::OS::get_singleton()->move_to_trash(p_path);
}

String OS::get_user_data_dir() const { return ::OS::get_singleton()->get_user_data_dir(); }

String OS::get_config_dir() const
{
	// Exposed as `get_config_dir()` instead of `get_config_path()` for consistency with other
	// exposed OS methods.
	return ::OS::get_singleton()->get_config_path();
}

String OS::get_data_dir() const
{
	// Exposed as `get_data_dir()` instead of `get_data_path()` for consistency with other exposed
	// OS methods.
	return ::OS::get_singleton()->get_data_path();
}

String OS::get_cache_dir() const
{
	// Exposed as `get_cache_dir()` instead of `get_cache_path()` for consistency with other exposed
	// OS methods.
	return ::OS::get_singleton()->get_cache_path();
}

String OS::get_temp_dir() const
{
	// Exposed as `get_temp_dir()` instead of `get_temp_path()` for consistency with other exposed
	// OS methods.
	return ::OS::get_singleton()->get_temp_path();
}

bool OS::is_debug_build() const
{
#ifdef DEBUG_ENABLED
	return true;
#else
	return false;
#endif // DEBUG_ENABLED
}

String OS::get_system_dir(SystemDir p_dir, bool p_shared_storage) const
{
	return ::OS::get_singleton()->get_system_dir(::OS::SystemDir(p_dir), p_shared_storage);
}

String OS::get_keycode_string(Key p_code) const { return ::keycode_get_string(p_code); }

bool OS::is_keycode_unicode(char32_t p_unicode) const
{
	return ::keycode_has_unicode((Key)p_unicode);
}

Key OS::find_keycode_from_string(const String& p_code) const { return find_keycode(p_code); }

bool OS::request_permission(const String& p_name)
{
	return ::OS::get_singleton()->request_permission(p_name);
}

bool OS::request_permissions() { return ::OS::get_singleton()->request_permissions(); }

Vector<String> OS::get_granted_permissions() const
{
	return ::OS::get_singleton()->get_granted_permissions();
}

void OS::revoke_granted_permissions() { ::OS::get_singleton()->revoke_granted_permissions(); }

String OS::get_unique_id() const { return ::OS::get_singleton()->get_unique_id(); }

void OS::add_logger(Logger* rp_logger)
{
	if (!logger_bind) {
		logger_bind = memnew(LoggerBind);
		::OS::get_singleton()->add_logger(logger_bind);
	}

	ERR_FAIL_COND_MSG(logger_bind->loggers.find(rp_logger) != -1,
		"Could not add logger, as it has already been added.");
	logger_bind->loggers.push_back(rp_logger);
}

void OS::remove_logger(Logger* rp_logger)
{
	ERR_FAIL_COND_MSG(!logger_bind || logger_bind->loggers.find(rp_logger) == -1,
		"Could not remove logger, as it hasn't been added.");
	logger_bind->loggers.erase(rp_logger);
}

void OS::remove_script_loggers(const ScriptLanguage* p_script)
{
	if (logger_bind) {
		LocalVector<Ref<CoreBind::Logger>> to_remove;
		for (const Ref<CoreBind::Logger>& logger : logger_bind->loggers) {
			if (logger.is_null()) {
				continue;
			}
			ScriptInstance* si = logger->obj->get_script_instance();
			if (!si) {
				continue;
			}
			if (si->get_language() == p_script) {
				to_remove.push_back(logger);
			}
		}
		for (const Ref<CoreBind::Logger>& logger : to_remove) {
			logger_bind->loggers.erase(logger);
		}
	}
}

void OS::_bind_methods() {}

OS::OS() { singleton = this; }

OS::~OS()
{
	if (singleton == this) {
		singleton = nullptr;
	}

	if (logger_bind) {
		logger_bind->clear();
	}
}

////// Geometry2D //////

Geometry2D* Geometry2D::get_singleton() { return singleton; }

bool Geometry2D::is_point_in_circle(
	const Vector2& p_point, const Vector2& p_circle_pos, real_t p_circle_radius)
{
	return ::Geometry2D::is_point_in_circle(p_point, p_circle_pos, p_circle_radius);
}

real_t Geometry2D::segment_intersects_circle(
	const Vector2& p_from, const Vector2& p_to, const Vector2& p_circle_pos, real_t p_circle_radius)
{
	return ::Geometry2D::segment_intersects_circle(p_from, p_to, p_circle_pos, p_circle_radius);
}

Variant Geometry2D::segment_intersects_segment(
	const Vector2& p_from_a, const Vector2& p_to_a, const Vector2& p_from_b, const Vector2& p_to_b)
{
	Vector2 result;
	if (::Geometry2D::segment_intersects_segment(p_from_a, p_to_a, p_from_b, p_to_b, &result)) {
		return result;
	}
	else {
		return Variant();
	}
}

Variant Geometry2D::line_intersects_line(const Vector2& p_from_a, const Vector2& p_dir_a,
	const Vector2& p_from_b, const Vector2& p_dir_b)
{
	Vector2 result;
	if (::Geometry2D::line_intersects_line(p_from_a, p_dir_a, p_from_b, p_dir_b, result)) {
		return result;
	}
	else {
		return Variant();
	}
}

Vector<Vector2> Geometry2D::get_closest_points_between_segments(
	const Vector2& p_p1, const Vector2& p_q1, const Vector2& p_p2, const Vector2& p_q2)
{
	Vector2 r1, r2;
	::Geometry2D::get_closest_points_between_segments(p_p1, p_q1, p_p2, p_q2, r1, r2);
	Vector<Vector2> r = {r1, r2};
	return r;
}

Vector2 Geometry2D::get_closest_point_to_segment(
	const Vector2& p_point, const Vector2& p_a, const Vector2& p_b)
{
	return ::Geometry2D::get_closest_point_to_segment(p_point, p_a, p_b);
}

Vector2 Geometry2D::get_closest_point_to_segment_uncapped(
	const Vector2& p_point, const Vector2& p_a, const Vector2& p_b)
{
	return ::Geometry2D::get_closest_point_to_segment_uncapped(p_point, p_a, p_b);
}

bool Geometry2D::point_is_inside_triangle(
	const Vector2& p_s, const Vector2& p_a, const Vector2& p_b, const Vector2& p_c) const
{
	return ::Geometry2D::is_point_in_triangle(p_s, p_a, p_b, p_c);
}

bool Geometry2D::is_polygon_clockwise(const Vector<Vector2>& p_polygon)
{
	return ::Geometry2D::is_polygon_clockwise(p_polygon);
}

bool Geometry2D::is_point_in_polygon(const Point2& p_point, const Vector<Vector2>& p_polygon)
{
	return ::Geometry2D::is_point_in_polygon(p_point, p_polygon);
}

Vector<int> Geometry2D::triangulate_polygon(const Vector<Vector2>& p_polygon)
{
	return ::Geometry2D::triangulate_polygon(p_polygon);
}

Vector<int> Geometry2D::triangulate_delaunay(const Vector<Vector2>& p_points)
{
	return ::Geometry2D::triangulate_delaunay(p_points);
}

Vector<Point2> Geometry2D::convex_hull(const Vector<Point2>& p_points)
{
	return ::Geometry2D::convex_hull(p_points);
}

TypedArray<PackedVector2Array> Geometry2D::decompose_polygon_in_convex(
	const Vector<Vector2>& p_polygon)
{
	Vector<Vector<Point2>> decomp = ::Geometry2D::decompose_polygon_in_convex(p_polygon);

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < decomp.size(); ++i) {
		ret.push_back(decomp[i]);
	}
	return ret;
}

TypedArray<PackedVector2Array> Geometry2D::merge_polygons(
	const Vector<Vector2>& p_polygon_a, const Vector<Vector2>& p_polygon_b)
{
	Vector<Vector<Point2>> polys = ::Geometry2D::merge_polygons(p_polygon_a, p_polygon_b);

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < polys.size(); ++i) {
		ret.push_back(polys[i]);
	}
	return ret;
}

TypedArray<PackedVector2Array> Geometry2D::clip_polygons(
	const Vector<Vector2>& p_polygon_a, const Vector<Vector2>& p_polygon_b)
{
	Vector<Vector<Point2>> polys = ::Geometry2D::clip_polygons(p_polygon_a, p_polygon_b);

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < polys.size(); ++i) {
		ret.push_back(polys[i]);
	}
	return ret;
}

TypedArray<PackedVector2Array> Geometry2D::intersect_polygons(
	const Vector<Vector2>& p_polygon_a, const Vector<Vector2>& p_polygon_b)
{
	Vector<Vector<Point2>> polys = ::Geometry2D::intersect_polygons(p_polygon_a, p_polygon_b);

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < polys.size(); ++i) {
		ret.push_back(polys[i]);
	}
	return ret;
}

TypedArray<PackedVector2Array> Geometry2D::exclude_polygons(
	const Vector<Vector2>& p_polygon_a, const Vector<Vector2>& p_polygon_b)
{
	Vector<Vector<Point2>> polys = ::Geometry2D::exclude_polygons(p_polygon_a, p_polygon_b);

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < polys.size(); ++i) {
		ret.push_back(polys[i]);
	}
	return ret;
}

TypedArray<PackedVector2Array> Geometry2D::clip_polyline_with_polygon(
	const Vector<Vector2>& p_polyline, const Vector<Vector2>& p_polygon)
{
	Vector<Vector<Point2>> polys = ::Geometry2D::clip_polyline_with_polygon(p_polyline, p_polygon);

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < polys.size(); ++i) {
		ret.push_back(polys[i]);
	}
	return ret;
}

TypedArray<PackedVector2Array> Geometry2D::intersect_polyline_with_polygon(
	const Vector<Vector2>& p_polyline, const Vector<Vector2>& p_polygon)
{
	Vector<Vector<Point2>> polys =
		::Geometry2D::intersect_polyline_with_polygon(p_polyline, p_polygon);

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < polys.size(); ++i) {
		ret.push_back(polys[i]);
	}
	return ret;
}

TypedArray<PackedVector2Array> Geometry2D::offset_polygon(
	const Vector<Vector2>& p_polygon, real_t p_delta, PolyJoinType p_join_type)
{
	Vector<Vector<Point2>> polys =
		::Geometry2D::offset_polygon(p_polygon, p_delta, ::Geometry2D::PolyJoinType(p_join_type));

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < polys.size(); ++i) {
		ret.push_back(polys[i]);
	}
	return ret;
}

TypedArray<PackedVector2Array> Geometry2D::offset_polyline(const Vector<Vector2>& p_polygon,
	real_t p_delta, PolyJoinType p_join_type, PolyEndType p_end_type)
{
	Vector<Vector<Point2>> polys = ::Geometry2D::offset_polyline(p_polygon, p_delta,
		::Geometry2D::PolyJoinType(p_join_type), ::Geometry2D::PolyEndType(p_end_type));

	TypedArray<PackedVector2Array> ret;

	for (int i = 0; i < polys.size(); ++i) {
		ret.push_back(polys[i]);
	}
	return ret;
}

Dictionary Geometry2D::make_atlas(const Vector<Size2>& p_rects)
{
	Dictionary ret;

	Vector<Size2i> rects;
	for (int i = 0; i < p_rects.size(); i++) {
		rects.push_back(p_rects[i]);
	}

	Vector<Point2i> result;
	Size2i size;

	::Geometry2D::make_atlas(rects, result, size);

	Vector<Point2> r_result;
	for (int i = 0; i < result.size(); i++) {
		r_result.push_back(result[i]);
	}

	ret["points"] = r_result;
	ret["size"] = size;

	return ret;
}

TypedArray<Point2i> Geometry2D::bresenham_line(const Point2i& p_from, const Point2i& p_to)
{
	Vector<Point2i> points = ::Geometry2D::bresenham_line(p_from, p_to);

	TypedArray<Point2i> result;
	result.resize(points.size());

	for (int i = 0; i < points.size(); i++) {
		result[i] = points[i];
	}

	return result;
}

void Geometry2D::_bind_methods() {}

////// Geometry3D //////

Geometry3D* Geometry3D::get_singleton() { return singleton; }

Vector<Vector3> Geometry3D::compute_convex_mesh_points(const TypedArray<Plane>& p_planes)
{
	Vector<Plane> planes_vec;
	int size = p_planes.size();
	planes_vec.resize(size);
	for (int i = 0; i < size; ++i) {
		planes_vec.set(i, p_planes[i]);
	}
	Variant ret = ::Geometry3D::compute_convex_mesh_points(planes_vec.ptr(), size);
	return ret;
}

TypedArray<Plane> Geometry3D::build_box_planes(const Vector3& p_extents)
{
	Variant ret = ::Geometry3D::build_box_planes(p_extents);
	return ret;
}

TypedArray<Plane> Geometry3D::build_cylinder_planes(
	float p_radius, float p_height, int p_sides, Vector3::Axis p_axis)
{
	Variant ret = ::Geometry3D::build_cylinder_planes(p_radius, p_height, p_sides, p_axis);
	return ret;
}

TypedArray<Plane> Geometry3D::build_capsule_planes(
	float p_radius, float p_height, int p_sides, int p_lats, Vector3::Axis p_axis)
{
	Variant ret = ::Geometry3D::build_capsule_planes(p_radius, p_height, p_sides, p_lats, p_axis);
	return ret;
}

Vector<Vector3> Geometry3D::get_closest_points_between_segments(
	const Vector3& p_p1, const Vector3& p_p2, const Vector3& p_q1, const Vector3& p_q2)
{
	Vector3 r1, r2;
	::Geometry3D::get_closest_points_between_segments(p_p1, p_p2, p_q1, p_q2, r1, r2);
	Vector<Vector3> r = {r1, r2};
	return r;
}

Vector3 Geometry3D::get_closest_point_to_segment(
	const Vector3& p_point, const Vector3& p_a, const Vector3& p_b)
{
	return ::Geometry3D::get_closest_point_to_segment(p_point, p_a, p_b);
}

Vector3 Geometry3D::get_closest_point_to_segment_uncapped(
	const Vector3& p_point, const Vector3& p_a, const Vector3& p_b)
{
	return ::Geometry3D::get_closest_point_to_segment_uncapped(p_point, p_a, p_b);
}

Vector3 Geometry3D::get_triangle_barycentric_coords(
	const Vector3& p_point, const Vector3& p_v0, const Vector3& p_v1, const Vector3& p_v2)
{
	Vector3 res = ::Geometry3D::triangle_get_barycentric_coords(p_v0, p_v1, p_v2, p_point);
	return res;
}

Variant Geometry3D::ray_intersects_triangle(const Vector3& p_from, const Vector3& p_dir,
	const Vector3& p_v0, const Vector3& p_v1, const Vector3& p_v2)
{
	Vector3 res;
	if (::Geometry3D::ray_intersects_triangle(p_from, p_dir, p_v0, p_v1, p_v2, &res)) {
		return res;
	}
	else {
		return Variant();
	}
}

Variant Geometry3D::segment_intersects_triangle(const Vector3& p_from, const Vector3& p_to,
	const Vector3& p_v0, const Vector3& p_v1, const Vector3& p_v2)
{
	Vector3 res;
	if (::Geometry3D::segment_intersects_triangle(p_from, p_to, p_v0, p_v1, p_v2, &res)) {
		return res;
	}
	else {
		return Variant();
	}
}

Vector<Vector3> Geometry3D::segment_intersects_sphere(
	const Vector3& p_from, const Vector3& p_to, const Vector3& p_sphere_pos, real_t p_sphere_radius)
{
	Vector<Vector3> r;
	Vector3 res, norm;
	if (!::Geometry3D::segment_intersects_sphere(
			p_from, p_to, p_sphere_pos, p_sphere_radius, &res, &norm)) {
		return r;
	}

	r.resize(2);
	r.set(0, res);
	r.set(1, norm);
	return r;
}

Vector<Vector3> Geometry3D::segment_intersects_cylinder(
	const Vector3& p_from, const Vector3& p_to, float p_height, float p_radius)
{
	Vector<Vector3> r;
	Vector3 res, norm;
	if (!::Geometry3D::segment_intersects_cylinder(p_from, p_to, p_height, p_radius, &res, &norm)) {
		return r;
	}

	r.resize(2);
	r.set(0, res);
	r.set(1, norm);
	return r;
}

Vector<Vector3> Geometry3D::segment_intersects_convex(
	const Vector3& p_from, const Vector3& p_to, const TypedArray<Plane>& p_planes)
{
	Vector<Vector3> r;
	Vector3 res, norm;
	Vector<Plane> planes = Variant(p_planes);
	if (!::Geometry3D::segment_intersects_convex(
			p_from, p_to, planes.ptr(), planes.size(), &res, &norm)) {
		return r;
	}

	r.resize(2);
	r.set(0, res);
	r.set(1, norm);
	return r;
}

Vector<Vector3> Geometry3D::clip_polygon(const Vector<Vector3>& p_points, const Plane& p_plane)
{
	return ::Geometry3D::clip_polygon(p_points, p_plane);
}

Vector<int32_t> Geometry3D::tetrahedralize_delaunay(const Vector<Vector3>& p_points)
{
	return ::Geometry3D::tetrahedralize_delaunay(p_points);
}

void Geometry3D::_bind_methods() {}

////// Marshalls //////

Marshalls* Marshalls::get_singleton() { return singleton; }

String Marshalls::variant_to_base64(const Variant& p_var, bool p_full_objects)
{
	int len;
	Error err = encode_variant(p_var, nullptr, len, p_full_objects);
	ERR_FAIL_COND_V_MSG(err != OK, "", "Error when trying to encode Variant.");

	Vector<uint8_t> buff;
	buff.resize(len);
	uint8_t* w = buff.ptrw();

	err = encode_variant(p_var, &w[0], len, p_full_objects);
	ERR_FAIL_COND_V_MSG(err != OK, "", "Error when trying to encode Variant.");

	String ret = CryptoCore::b64_encode_str(&w[0], len);
	ERR_FAIL_COND_V(ret.is_empty(), ret);

	return ret;
}

Variant Marshalls::base64_to_variant(const String& p_str, bool p_allow_objects)
{
	int strlen = p_str.length();
	CharString cstr = p_str.ascii();

	Vector<uint8_t> buf;
	buf.resize(strlen / 4 * 3 + 1);
	uint8_t* w = buf.ptrw();

	size_t len = 0;
	ERR_FAIL_COND_V(CryptoCore::b64_decode(
						&w[0], buf.size(), &len, (unsigned char*)cstr.get_data(), strlen) != OK,
		Variant());

	Variant v;
	Error err = decode_variant(v, &w[0], len, nullptr, p_allow_objects);
	ERR_FAIL_COND_V_MSG(err != OK, Variant(), "Error when trying to decode Variant.");

	return v;
}

String Marshalls::raw_to_base64(const Vector<uint8_t>& p_arr)
{
	String ret = CryptoCore::b64_encode_str(p_arr.ptr(), p_arr.size());
	ERR_FAIL_COND_V(ret.is_empty(), ret);
	return ret;
}

Vector<uint8_t> Marshalls::base64_to_raw(const String& p_str)
{
	int strlen = p_str.length();
	CharString cstr = p_str.ascii();

	size_t arr_len = 0;
	Vector<uint8_t> buf;
	{
		buf.resize(strlen / 4 * 3 + 1);
		uint8_t* w = buf.ptrw();

		ERR_FAIL_COND_V(CryptoCore::b64_decode(&w[0], buf.size(), &arr_len,
							(unsigned char*)cstr.get_data(), strlen) != OK,
			Vector<uint8_t>());
	}
	buf.resize(arr_len);

	return buf;
}

String Marshalls::utf8_to_base64(const String& p_str)
{
	if (p_str.is_empty()) {
		return String();
	}
	CharString cstr = p_str.utf8();
	String ret = CryptoCore::b64_encode_str((unsigned char*)cstr.get_data(), cstr.length());
	ERR_FAIL_COND_V(ret.is_empty(), ret);
	return ret;
}

String Marshalls::base64_to_utf8(const String& p_str)
{
	int strlen = p_str.length();
	CharString cstr = p_str.ascii();

	Vector<uint8_t> buf;
	buf.resize(strlen / 4 * 3 + 1 + 1);
	uint8_t* w = buf.ptrw();

	size_t len = 0;
	ERR_FAIL_COND_V(CryptoCore::b64_decode(
						&w[0], buf.size(), &len, (unsigned char*)cstr.get_data(), strlen) != OK,
		String());

	w[len] = 0;
	String ret = String::utf8((char*)&w[0]);

	return ret;
}

void Marshalls::_bind_methods() {}

////// Semaphore //////

void Semaphore::wait() { semaphore.wait(); }

bool Semaphore::try_wait() { return semaphore.try_wait(); }

void Semaphore::post(int p_count)
{
	ERR_FAIL_COND(p_count <= 0);
	semaphore.post(p_count);
}

void Semaphore::_bind_methods() {}

////// Mutex //////

void Mutex::lock() { mutex.lock(); }

bool Mutex::try_lock() { return mutex.try_lock(); }

void Mutex::unlock() { mutex.unlock(); }

void Mutex::_bind_methods() {}

////// Thread //////

void Thread::_start_func(void* p_ud)
{
	Ref<Thread>* tud = (Ref<Thread>*)p_ud;
	Ref<Thread> t = *tud;
	memdelete(tud);

	if (!t->target_callable.is_valid()) {
		t->running.clear();
		ERR_FAIL_MSG(
			vformat("Could not call function '%s' on previously freed instance to start thread %s.",
				t->target_callable.get_method(), t->get_id()));
	}

	// Finding out a suitable name for the thread can involve querying a node, if the target is one.
	// We know this is safe (unless the user is causing life cycle race conditions, which would be a
	// bug on their part).
	set_current_thread_safe_for_nodes(true);
	String func_name = t->target_callable.is_custom()
						   ? t->target_callable.get_custom()->get_as_text()
						   : String(t->target_callable.get_method());
	set_current_thread_safe_for_nodes(false);
	::Thread::set_name(func_name);

	// To avoid a circular reference between the thread and the script which can possibly contain a
	// reference to the thread, we will do the call (keeping a reference up to that point) and then
	// break chains with it. When the call returns, we will reference the thread again if possible.
	ObjectID th_instance_id = t->obj->get_instance_id();
	Callable target_callable = t->target_callable;
	String id = t->get_id();
	t = Ref<Thread>();

	Callable::CallError ce;
	Variant ret;
	target_callable.callp(nullptr, 0, ret, ce);
	// If script properly kept a reference to the thread, we should be able to re-reference it now
	// (well, or if the call failed, since we had to break chains anyway because the outcome isn't
	// known upfront).
	t = ObjectDB::get_ref<Thread>(th_instance_id);
	if (t.is_valid()) {
		t->ret = ret;
		t->running.clear();
	}
	else {
		// We could print a warning here, but the Thread object will be eventually destroyed
		// noticing wait_to_finish() hasn't been called on it, and it will print a warning itself.
	}

	if (ce.error != Callable::CallError::CALL_OK) {
		ERR_FAIL_MSG(vformat("Could not call function '%s' to start thread %s: %s.", func_name, id,
			Variant::get_callable_error_text(target_callable, nullptr, 0, ce)));
	}
}

Error Thread::start(const Callable& p_callable, Priority p_priority)
{
	ERR_FAIL_COND_V_MSG(is_started(), ERR_ALREADY_IN_USE, "Thread already started.");
	ERR_FAIL_COND_V(!p_callable.is_valid(), ERR_INVALID_PARAMETER);
	ERR_FAIL_INDEX_V(p_priority, PRIORITY_MAX, ERR_INVALID_PARAMETER);

	ret = Variant();
	target_callable = p_callable;
	running.set();

	Ref<Thread>* ud = memnew(Ref<Thread>(this));

	::Thread::Settings s;
	s.priority = (::Thread::Priority)p_priority;
	thread.start(_start_func, ud, s);

	return OK;
}

String Thread::get_id() const { return itos(thread.get_id()); }

bool Thread::is_started() const { return thread.is_started(); }

bool Thread::is_alive() const { return running.is_set(); }

Variant Thread::wait_to_finish()
{
	ERR_FAIL_COND_V_MSG(
		!is_started(), Variant(), "Thread must have been started to wait for its completion.");
	thread.wait_to_finish();
	Variant r = ret;
	target_callable = Callable();

	return r;
}

void Thread::set_thread_safety_checks_enabled(bool p_enabled)
{
	ERR_FAIL_COND_MSG(::Thread::is_main_thread(), "This call is forbidden on the main thread.");
	set_current_thread_safe_for_nodes(!p_enabled);
}

bool Thread::is_main_thread() { return ::Thread::is_main_thread(); }

void Thread::_bind_methods() {}

////// Engine //////

void Engine::set_physics_ticks_per_second(int p_ips)
{
	::Engine::get_singleton()->set_physics_ticks_per_second(p_ips);
}

int Engine::get_physics_ticks_per_second() const
{
	return ::Engine::get_singleton()->get_physics_ticks_per_second();
}

void Engine::set_max_physics_steps_per_frame(int p_max_physics_steps)
{
	::Engine::get_singleton()->set_max_physics_steps_per_frame(p_max_physics_steps);
}

int Engine::get_max_physics_steps_per_frame() const
{
	return ::Engine::get_singleton()->get_max_physics_steps_per_frame();
}

void Engine::set_physics_jitter_fix(double p_threshold)
{
	::Engine::get_singleton()->set_physics_jitter_fix(p_threshold);
}

double Engine::get_physics_jitter_fix() const
{
	return ::Engine::get_singleton()->get_physics_jitter_fix();
}

double Engine::get_physics_interpolation_fraction() const
{
	return ::Engine::get_singleton()->get_physics_interpolation_fraction();
}

void Engine::set_max_fps(int p_fps) { ::Engine::get_singleton()->set_max_fps(p_fps); }

int Engine::get_max_fps() const { return ::Engine::get_singleton()->get_max_fps(); }

double Engine::get_frames_per_second() const
{
	return ::Engine::get_singleton()->get_frames_per_second();
}

uint64_t Engine::get_physics_frames() const
{
	return ::Engine::get_singleton()->get_physics_frames();
}

uint64_t Engine::get_process_frames() const
{
	return ::Engine::get_singleton()->get_process_frames();
}

void Engine::set_time_scale(double p_scale) { ::Engine::get_singleton()->set_time_scale(p_scale); }

double Engine::get_time_scale() { return ::Engine::get_singleton()->get_time_scale(); }

int Engine::get_frames_drawn() { return ::Engine::get_singleton()->get_frames_drawn(); }

MainLoop* Engine::get_main_loop() const
{
	// Needs to remain in OS, since it's actually OS that interacts with it, but it's better exposed
	// here
	return ::OS::get_singleton()->get_main_loop();
}

Dictionary Engine::get_version_info() const
{
	return ::Engine::get_singleton()->get_version_info();
}

Dictionary Engine::get_author_info() const { return ::Engine::get_singleton()->get_author_info(); }

TypedArray<Dictionary> Engine::get_copyright_info() const
{
	return ::Engine::get_singleton()->get_copyright_info();
}

Dictionary Engine::get_donor_info() const { return ::Engine::get_singleton()->get_donor_info(); }

Dictionary Engine::get_license_info() const
{
	return ::Engine::get_singleton()->get_license_info();
}

String Engine::get_license_text() const { return ::Engine::get_singleton()->get_license_text(); }

String Engine::get_architecture_name() const
{
	return ::Engine::get_singleton()->get_architecture_name();
}

bool Engine::is_in_physics_frame() const
{
	return ::Engine::get_singleton()->is_in_physics_frame();
}

bool Engine::has_singleton(const StringName& p_name) const
{
	return ::Engine::get_singleton()->has_singleton(p_name);
}

Object* Engine::get_singleton_object(const StringName& p_name) const
{
	return ::Engine::get_singleton()->get_singleton_object(p_name);
}

void Engine::register_singleton(const StringName& p_name, Object* rp_instance)
{
	ERR_FAIL_COND_MSG(
		has_singleton(p_name), vformat("Singleton already registered: '%s'.", String(p_name)));
	ERR_FAIL_COND_MSG(!String(p_name).is_valid_ascii_identifier(),
		vformat("Singleton name is not a valid identifier: '%s'.", p_name));
	::Engine::Singleton s;
	s.class_name = p_name;
	s.name = p_name;
	s.ptr = rp_instance;
	s.user_created = true;
	::Engine::get_singleton()->add_singleton(s);
}

void Engine::unregister_singleton(const StringName& p_name)
{
	ERR_FAIL_COND_MSG(!has_singleton(p_name),
		vformat("Attempt to remove unregistered singleton: '%s'.", String(p_name)));
	ERR_FAIL_COND_MSG(!::Engine::get_singleton()->is_singleton_user_created(p_name),
		vformat("Attempt to remove non-user created singleton: '%s'.", String(p_name)));
	::Engine::get_singleton()->remove_singleton(p_name);
}

Vector<String> Engine::get_singleton_list() const
{
	List<::Engine::Singleton> singletons;
	::Engine::get_singleton()->get_singletons(&singletons);
	Vector<String> ret;
	for (const ::Engine::Singleton& E : singletons) {
		ret.push_back(E.name);
	}
	return ret;
}

Error Engine::register_script_language(ScriptLanguage* rp_language)
{
	return ScriptServer::register_language(rp_language);
}

Error Engine::unregister_script_language(const ScriptLanguage* rp_language)
{
	return ScriptServer::unregister_language(rp_language);
}

int Engine::get_script_language_count() { return ScriptServer::get_language_count(); }

ScriptLanguage* Engine::get_script_language(int p_index) const
{
	return ScriptServer::get_language(p_index);
}

Array Engine::capture_script_backtraces(bool p_include_variables) const
{
	Vector<Ref<ScriptBacktrace>> backtraces =
		ScriptServer::capture_script_backtraces(p_include_variables);
	Array result;
	result.resize(backtraces.size());
	for (int i = 0; i < backtraces.size(); i++) {
		result[i] = backtraces[i];
	}
	return result;
}

void Engine::set_editor_hint(bool p_enabled)
{
	::Engine::get_singleton()->set_editor_hint(p_enabled);
}

bool Engine::is_editor_hint() const { return ::Engine::get_singleton()->is_editor_hint(); }

bool Engine::is_embedded_in_editor() const
{
	return ::Engine::get_singleton()->is_embedded_in_editor();
}

String Engine::get_write_movie_path() const
{
	return ::Engine::get_singleton()->get_write_movie_path();
}

void Engine::set_print_to_stdout(bool p_enabled)
{
	::Engine::get_singleton()->set_print_to_stdout(p_enabled);
}

bool Engine::is_printing_to_stdout() const
{
	return ::Engine::get_singleton()->is_printing_to_stdout();
}

void Engine::set_print_error_messages(bool p_enabled)
{
	::Engine::get_singleton()->set_print_error_messages(p_enabled);
}

bool Engine::is_printing_error_messages() const
{
	return ::Engine::get_singleton()->is_printing_error_messages();
}

#ifdef TOOLS_ENABLED
void Engine::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 &&
		(pf == "has_singleton" || pf == "get_singleton" || pf == "unregister_singleton")) {
		for (const String& E : get_singleton_list()) {
			r_options->push_back(E.quote());
		}
	}
	Object::get_argument_options(p_function, p_idx, r_options);
}
#endif

void Engine::_bind_methods() {}

////// EngineDebugger //////

bool EngineDebugger::is_active() { return ::EngineDebugger::is_active(); }

void EngineDebugger::register_profiler(const StringName& p_name, Ref<EngineProfiler> p_profiler)
{
	ERR_FAIL_COND(p_profiler.is_null());
	ERR_FAIL_COND_MSG(p_profiler->is_bound(), "Profiler already registered.");
	ERR_FAIL_COND_MSG(profilers.has(p_name) || has_profiler(p_name),
		vformat("Profiler name already in use: '%s'.", p_name));
	Error err = p_profiler->bind(p_name);
	ERR_FAIL_COND_MSG(err != OK, vformat("Profiler failed to register with error: %d.", err));
	profilers.insert(p_name, p_profiler);
}

void EngineDebugger::unregister_profiler(const StringName& p_name)
{
	ERR_FAIL_COND_MSG(!profilers.has(p_name), vformat("Profiler not registered: '%s'.", p_name));
	profilers[p_name]->unbind();
	profilers.erase(p_name);
}

bool EngineDebugger::is_profiling(const StringName& p_name)
{
	return ::EngineDebugger::is_profiling(p_name);
}

bool EngineDebugger::has_profiler(const StringName& p_name)
{
	return ::EngineDebugger::has_profiler(p_name);
}

void EngineDebugger::profiler_add_frame_data(const StringName& p_name, const Array& p_data)
{
	::EngineDebugger::profiler_add_frame_data(p_name, p_data);
}

void EngineDebugger::profiler_enable(const StringName& p_name, bool p_enabled, const Array& p_opts)
{
	if (::EngineDebugger::get_singleton()) {
		::EngineDebugger::get_singleton()->profiler_enable(p_name, p_enabled, p_opts);
	}
}

void EngineDebugger::register_message_capture(const StringName& p_name, const Callable& p_callable)
{
	ERR_FAIL_COND_MSG(captures.has(p_name) || has_capture(p_name),
		vformat("Capture already registered: '%s'.", p_name));
	captures.insert(p_name, p_callable);
	Callable& c = captures[p_name];
	::EngineDebugger::Capture capture(&c, &EngineDebugger::call_capture);
	::EngineDebugger::register_message_capture(p_name, capture);
}

void EngineDebugger::unregister_message_capture(const StringName& p_name)
{
	ERR_FAIL_COND_MSG(!captures.has(p_name), vformat("Capture not registered: '%s'.", p_name));
	::EngineDebugger::unregister_message_capture(p_name);
	captures.erase(p_name);
}

bool EngineDebugger::has_capture(const StringName& p_name)
{
	return ::EngineDebugger::has_capture(p_name);
}

void EngineDebugger::send_message(const String& p_msg, const Array& p_data)
{
	ERR_FAIL_COND_MSG(!::EngineDebugger::is_active(), "Can't send message. No active debugger");
	::EngineDebugger::get_singleton()->send_message(p_msg, p_data);
}

void EngineDebugger::debug(bool p_can_continue, bool p_is_error_breakpoint)
{
	ERR_FAIL_COND_MSG(!::EngineDebugger::is_active(), "Can't send debug. No active debugger");
	::EngineDebugger::get_singleton()->debug(p_can_continue, p_is_error_breakpoint);
}

void EngineDebugger::script_debug(
	ScriptLanguage* p_lang, bool p_can_continue, bool p_is_error_breakpoint)
{
	ERR_FAIL_COND_MSG(
		!::EngineDebugger::get_script_debugger(), "Can't send debug. No active debugger");
	::EngineDebugger::get_script_debugger()->debug(p_lang, p_can_continue, p_is_error_breakpoint);
}

Error EngineDebugger::call_capture(
	void* p_user, const String& p_cmd, const Array& p_data, bool& r_captured)
{
	Callable& capture = *(Callable*)p_user;
	if (!capture.is_valid()) {
		return FAILED;
	}
	Variant cmd = p_cmd, data = p_data;
	const Variant* args[2] = {&cmd, &data};
	Variant retval;
	Callable::CallError err;
	capture.callp(args, 2, retval, err);
	ERR_FAIL_COND_V_MSG(err.error != Callable::CallError::CALL_OK, FAILED,
		vformat("Error calling 'capture' to callable: %s.",
			Variant::get_callable_error_text(capture, args, 2, err)));
	ERR_FAIL_COND_V_MSG(retval.get_type() != Variant::BOOL, FAILED,
		vformat("Error calling 'capture' to callable: '%s'. Return type is not bool.",
			String(capture)));
	r_captured = retval;
	return OK;
}

void EngineDebugger::line_poll()
{
	ERR_FAIL_COND_MSG(!::EngineDebugger::is_active(), "Can't poll. No active debugger");
	::EngineDebugger::get_singleton()->line_poll();
}

void EngineDebugger::set_lines_left(int p_lines)
{
	ERR_FAIL_COND_MSG(
		!::EngineDebugger::get_script_debugger(), "Can't set lines left. No active debugger");
	::EngineDebugger::get_script_debugger()->set_lines_left(p_lines);
}

int EngineDebugger::get_lines_left() const
{
	ERR_FAIL_COND_V_MSG(
		!::EngineDebugger::get_script_debugger(), 0, "Can't get lines left. No active debugger");
	return ::EngineDebugger::get_script_debugger()->get_lines_left();
}

void EngineDebugger::set_depth(int p_depth)
{
	ERR_FAIL_COND_MSG(
		!::EngineDebugger::get_script_debugger(), "Can't set depth. No active debugger");
	::EngineDebugger::get_script_debugger()->set_depth(p_depth);
}

int EngineDebugger::get_depth() const
{
	ERR_FAIL_COND_V_MSG(
		!::EngineDebugger::get_script_debugger(), 0, "Can't get depth. No active debugger");
	return ::EngineDebugger::get_script_debugger()->get_depth();
}

bool EngineDebugger::is_breakpoint(int p_line, const StringName& p_source) const
{
	ERR_FAIL_COND_V_MSG(!::EngineDebugger::get_script_debugger(), false,
		"Can't check breakpoint. No active debugger");
	return ::EngineDebugger::get_script_debugger()->is_breakpoint(p_line, p_source);
}

bool EngineDebugger::is_skipping_breakpoints() const
{
	ERR_FAIL_COND_V_MSG(!::EngineDebugger::get_script_debugger(), false,
		"Can't check skipping breakpoint. No active debugger");
	return ::EngineDebugger::get_script_debugger()->is_skipping_breakpoints();
}

void EngineDebugger::insert_breakpoint(int p_line, const StringName& p_source)
{
	ERR_FAIL_COND_MSG(
		!::EngineDebugger::get_script_debugger(), "Can't insert breakpoint. No active debugger");
	::EngineDebugger::get_script_debugger()->insert_breakpoint(p_line, p_source);
}

void EngineDebugger::remove_breakpoint(int p_line, const StringName& p_source)
{
	ERR_FAIL_COND_MSG(
		!::EngineDebugger::get_script_debugger(), "Can't remove breakpoint. No active debugger");
	::EngineDebugger::get_script_debugger()->remove_breakpoint(p_line, p_source);
}

void EngineDebugger::clear_breakpoints()
{
	ERR_FAIL_COND_MSG(
		!::EngineDebugger::get_script_debugger(), "Can't clear breakpoints. No active debugger");
	::EngineDebugger::get_script_debugger()->clear_breakpoints();
}

EngineDebugger::~EngineDebugger()
{
	for (const KeyValue<StringName, Callable>& E : captures) {
		::EngineDebugger::unregister_message_capture(E.key);
	}
	captures.clear();
}

void EngineDebugger::_bind_methods() {}

} // namespace CoreBind


