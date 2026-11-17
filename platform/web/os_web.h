/**************************************************************************/
/*  os_web.h                                                              */
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

#include <emscripten/html5.h>
#include "audio_driver_web.h"
#include "core/input/input_event.h"
#include "core/os/os.h"
#include "drivers/unix/os_unix.h"
#include "godot_js.h"
#include "webmidi_driver.h"

class OS_Web final
{
public:
	OS_Web() = delete;
	~OS_Web() = delete;
	OS_Web(const OS_Web&) = delete;
	OS_Web& operator=(const OS_Web&) = delete;

	struct Data
	{
		List<AudioDriverWeb*> audio_drivers;
		bool idb_is_syncing = false;
		bool idb_available = false;
		bool idb_needs_sync = false;
		bool pwa_is_waiting = false;
	};

	static inline Data* data = nullptr;

private:
	WASM_EXPORT static void main_loop_callback();

	WASM_EXPORT static void file_access_close_callback(const String& p_file, int p_flags);
	WASM_EXPORT static void dir_access_remove_callback(const String& p_file);
	WASM_EXPORT static void fs_sync_callback();
	WASM_EXPORT static void update_pwa_state_callback();

public:
	static void initialize_core();
	static void initialize();
	static void finalize();
	static void finalize_core();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static void delete_main_loop();
	static bool _check_internal_feature_support(const String& p_feature);

	static bool pwa_needs_update() { return data && data->pwa_is_waiting; }
	static Error pwa_update();
	static void force_fs_sync();

	static void initialize_joypads();

	static bool main_loop_iterate();

	static Error execute(const String& p_path, const List<String>& p_arguments,
		String* r_pipe = nullptr, int* r_exitcode = nullptr, bool read_stderr = false,
		Mutex* p_pipe_mutex = nullptr, bool p_open_console = false);
	static Dictionary execute_with_pipe(
		const String& p_path, const List<String>& p_arguments, bool p_blocking = true);
	static Error create_process(const String& p_path, const List<String>& p_arguments,
		OS::ProcessID* r_child_id = nullptr, bool p_open_console = false);
	static Error kill(const OS::ProcessID& p_pid);
	static int get_process_id();
	static bool is_process_running(const OS::ProcessID& p_pid);
	static int get_process_exit_code(const OS::ProcessID& p_pid);
	static int get_processor_count();
	static String get_unique_id();
	static int get_default_thread_pool_size();

	static String get_executable_path();
	static Error shell_open(const String& p_uri);
	static String get_name();

	static void add_frame_delay(bool p_can_draw, bool p_wake_for_events);

	static void vibrate_handheld(int p_duration_ms, float p_amplitude = -1.0);

	static String get_cache_path();
	static String get_config_path();
	static String get_data_path();
	static String get_user_data_dir(const String& p_user_dir);

	static bool is_userfs_persistent();

	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	static Error open_dynamic_library(
		const String& p_path, void*& p_library_handle, GDExtensionData* p_data = nullptr);

	static void resume_audio();
};
