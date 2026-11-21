/**************************************************************************/
/*  os_web.cpp                                                            */
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

#include "os_web.h"

#include "api/javascript_bridge_singleton.h"
#include "display_server_web.h"
#include "godot_js.h"
#include "ip_web.h"
#include "net_socket_web.h"

#include "core/io/file_access.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "drivers/unix/dir_access_unix.h"
#include "drivers/unix/os_unix.h"
#include "main/main.h"

#include <dlfcn.h>
#include <emscripten.h>

void OS_Web::alert(const String &p_alert, const String &p_title) {
	godot_js_display_alert(p_alert.utf8().get_data());
}

void OS_Web::initialize_core() {
	if (!data) {
		data = memnew(Data);
	}

	char locale_ptr[16];
	godot_js_config_locale_get(locale_ptr, 16);
	setenv("LANG", locale_ptr, true);

	godot_js_pwa_cb(&OS_Web::update_pwa_state_callback);

	data->idb_available = godot_js_os_fs_is_persistent();

	OS_Unix::initialize_core();
	FileAccessUnix::close_notification_func = file_access_close_callback;
	DirAccessUnix::remove_notification_func = dir_access_remove_callback;
}

void OS_Web::initialize() {
	if (!data) {
		initialize_core();
	}

	if (AudioDriverWeb::is_available()) {
		data->audio_drivers.push_back(memnew(AudioDriverWorklet));
		data->audio_drivers.push_back(memnew(AudioDriverScriptProcessor));
	}
	for (AudioDriverWeb *audio_driver : data->audio_drivers) {
		AudioDriverManager::add_driver(audio_driver);
	}

	IPWeb::make_default();
	NetSocketWeb::make_default();
	DisplayServerWeb::register_web_driver();
	MIDIDriver::bind<MIDIDriverWebMidi>();
	MIDIDriver::initialize();
}

void OS_Web::resume_audio() {
	AudioDriverWeb::resume();
}

void OS_Web::fs_sync_callback() {
	if (data) {
		data->idb_is_syncing = false;
	}
}

bool OS_Web::main_loop_iterate() {
	GodotProfileFrameMark;
	GodotProfileZone("OS_Web::main_loop_iterate");
	if (data && is_userfs_persistent() && data->idb_needs_sync && !data->idb_is_syncing) {
		data->idb_is_syncing = true;
		data->idb_needs_sync = false;
		godot_js_os_fs_sync(&fs_sync_callback);
	}

	DisplayServer::process_events();

	return Main::iteration();
}

void OS_Web::delete_main_loop() {
	MainLoop::clear();
}

void OS_Web::finalize() {
	delete_main_loop();
	MIDIDriver::finalize();
	MIDIDriver::reset_backend();
	if (data) {
		for (AudioDriverWeb *driver : data->audio_drivers) {
			memdelete(driver);
		}
		data->audio_drivers.clear();
		memdelete(data);
		data = nullptr;
	}
}

void OS_Web::finalize_core() {
	OS_Unix::finalize_core();
}

// Miscellaneous

Error OS_Web::execute(const String &p_path, const List<String> &p_arguments, String *r_pipe, int *r_exitcode, bool read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	return create_process(p_path, p_arguments);
}

Dictionary OS_Web::execute_with_pipe(const String &p_path, const List<String> &p_arguments, bool p_blocking) {
	ERR_FAIL_V_MSG(Dictionary(), "OS::execute_with_pipe is not available on the Web platform.");
}

Error OS_Web::create_process(const String &p_path, const List<String> &p_arguments, OS::ProcessID *r_child_id, bool p_open_console) {
	Array args;
	for (const String &E : p_arguments) {
		args.push_back(E);
	}
	String json_args = Variant(args).to_json_string();
	int failed = godot_js_os_execute(json_args.utf8().get_data());
	ERR_FAIL_COND_V_MSG(failed, ERR_UNAVAILABLE, "OS::execute() or create_process() must be implemented in Web via 'engine.setOnExecute' if required.");
	return OK;
}

Error OS_Web::kill(const OS::ProcessID &p_pid) {
	ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "OS::kill() is not available on the Web platform.");
}

int OS_Web::get_process_id() {
	return 0;
}

bool OS_Web::is_process_running(const OS::ProcessID &p_pid) {
	return false;
}

int OS_Web::get_process_exit_code(const OS::ProcessID &p_pid) {
	return -1;
}

int OS_Web::get_processor_count() {
	return godot_js_os_hw_concurrency_get();
}

String OS_Web::get_unique_id() {
	ERR_FAIL_V_MSG("", "OS::get_unique_id() is not available on the Web platform.");
}

int OS_Web::get_default_thread_pool_size() {
#ifdef THREADS_ENABLED
	return godot_js_os_thread_pool_size_get();
#else // No threads.
	return 1;
#endif
}

bool OS_Web::_check_internal_feature_support(const String &p_feature) {
	if (p_feature == "web") {
		return true;
	}

	if (p_feature == "web_extensions") {
#ifdef WEB_DLINK_ENABLED
		return true;
#else
		return false;
#endif
	}
	if (p_feature == "web_noextensions") {
#ifdef WEB_DLINK_ENABLED
		return false;
#else
		return true;
#endif
	}

	if (godot_js_os_has_feature(p_feature.utf8().get_data())) {
		return true;
	}
	return false;
}

String OS_Web::get_executable_path() {
	return OS::get_executable_path();
}

Error OS_Web::shell_open(const String &p_uri) {
	godot_js_os_shell_open(p_uri.utf8().get_data());
	return OK;
}

String OS_Web::get_name() {
	return "Web";
}

void OS_Web::add_frame_delay(bool p_can_draw, bool p_wake_for_events) {
#ifndef PROXY_TO_PTHREAD_ENABLED
	OS::add_frame_delay(p_can_draw, p_wake_for_events);
#endif
}

void OS_Web::vibrate_handheld(int p_duration_ms, float p_amplitude) {
	godot_js_input_vibrate_handheld(p_duration_ms);
}

String OS_Web::get_user_data_dir(const String &p_user_dir) {
	String userfs = "/userfs";
	return userfs.path_join(p_user_dir).replace_char('\\', '/');
}

String OS_Web::get_cache_path() {
	return "/home/web_user/.cache";
}

String OS_Web::get_config_path() {
	return "/home/web_user/.config";
}

String OS_Web::get_data_path() {
	return "/home/web_user/.local/share";
}

void OS_Web::file_access_close_callback(const String &p_file, int p_flags) {
	if (!data || !(is_userfs_persistent() && (p_flags & FileAccess::WRITE))) {
		return; // FS persistence is not working or we are not writing.
	}
	bool is_file_persistent = p_file.begins_with("/userfs");
#ifdef TOOLS_ENABLED
	// Hack for editor persistence (can we track).
	is_file_persistent = is_file_persistent || p_file.begins_with("/home/web_user/");
#endif
	if (is_file_persistent) {
		data->idb_needs_sync = true;
	}
}

void OS_Web::dir_access_remove_callback(const String &p_file) {
	if (!data) {
		return;
	}
	bool is_file_persistent = p_file.begins_with("/userfs");
#ifdef TOOLS_ENABLED
	// Hack for editor persistence (can we track).
	is_file_persistent = is_file_persistent || p_file.begins_with("/home/web_user/");
#endif
	if (is_file_persistent) {
		data->idb_needs_sync = true;
	}
}

void OS_Web::update_pwa_state_callback() {
	if (data) {
		data->pwa_is_waiting = true;
	}
}

void OS_Web::force_fs_sync() {
	if (data && is_userfs_persistent()) {
		data->idb_needs_sync = true;
	}
}

Error OS_Web::pwa_update() {
	return godot_js_pwa_update() ? FAILED : OK;
}

bool OS_Web::is_userfs_persistent() {
	return data && data->idb_available;
}

Error OS_Web::open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data) {
	String path = p_path.get_file();
	p_library_handle = dlopen(path.utf8().get_data(), RTLD_NOW);
	ERR_FAIL_NULL_V_MSG(p_library_handle, ERR_CANT_OPEN, vformat("Can't open dynamic library: %s. Error: %s.", p_path, dlerror()));

	if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
		*p_data->r_resolved_path = path;
	}

	return OK;
}

void OS_Web::initialize_joypads() {
}

/* -------------------------------------------------------------------------- */
/*                        OS Static Platform Dispatch                         */
/* -------------------------------------------------------------------------- */

void OS::initialize_core() {
	OS_Web::initialize_core();
	_initialized = true;
}

void OS::initialize() {
	OS_Web::initialize();
	Vector<Logger *> loggers;
	loggers.push_back(memnew(StdLogger));
	_set_logger(memnew(CompositeLogger(loggers)));
	_initialized = true;
}

void OS::finalize() {
	_initialized = false;
	OS_Web::finalize();
	if (_logger) {
		memdelete(_logger);
		_logger = nullptr;
	}
}

void OS::finalize_core() {
	OS_Web::finalize_core();
}

void OS::initialize_joypads() {
	OS_Web::initialize_joypads();
}

uint64_t OS::get_ticks_usec() {
	return OS_Unix::get_ticks_usec();
}

void OS::delay_usec(uint32_t p_usec) {
	OS_Unix::delay_usec(p_usec);
}

String OS::get_environment(const String &p_var) {
	return OS_Unix::get_environment(p_var);
}

bool OS::has_environment(const String &p_var) {
	return OS_Unix::has_environment(p_var);
}

void OS::set_environment(const String &p_var, const String &p_value) {
	OS_Unix::set_environment(p_var, p_value);
}

void OS::unset_environment(const String &p_var) {
	OS_Unix::unset_environment(p_var);
}

void OS::load_shell_environment() {
}

OS::DateTime OS::get_datetime(bool p_utc) {
	return OS_Unix::get_datetime(p_utc);
}

OS::TimeZoneInfo OS::get_time_zone_info() {
	return OS_Unix::get_time_zone_info();
}

Error OS::execute(const String &p_path, const List<String> &p_arguments, String *r_pipe, int *r_exitcode, bool p_read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	return OS_Web::execute(p_path, p_arguments, r_pipe, r_exitcode, p_read_stderr, p_pipe_mutex, p_open_console);
}

Error OS::create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id, bool p_open_console) {
	return OS_Web::create_process(p_path, p_arguments, r_child_id, p_open_console);
}

Error OS::kill(const ProcessID &p_pid) {
	return OS_Web::kill(p_pid);
}

bool OS::is_process_running(const ProcessID &p_pid) {
	return OS_Web::is_process_running(p_pid);
}

Error OS::get_entropy(uint8_t *r_buffer, int p_bytes) {
	return OS_Unix::get_entropy(r_buffer, p_bytes);
}

String OS::get_system_ca_certificates() {
	return String();
}

String OS::get_name() {
	return OS_Web::get_name();
}

List<String> OS::get_cmdline_platform_args() {
	return List<String>();
}

bool OS::_check_internal_feature_support(const String &p_feature) {
	return OS_Web::_check_internal_feature_support(p_feature);
}

void OS::disable_crash_handler() {
}

void OS::vibrate_handheld(int p_duration_ms, float p_amplitude) {
	OS_Web::vibrate_handheld(p_duration_ms, p_amplitude);
}

Error OS::move_to_trash(const String &p_path) {
	return FAILED;
}

#ifdef TOOLS_ENABLED
bool OS::_test_create_rendering_device(const String &p_display_driver) {
	return false;
}

bool OS::_test_create_rendering_device_and_gl(const String &p_display_driver) {
	return false;
}
#endif
