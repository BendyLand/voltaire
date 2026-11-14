/**************************************************************************/
/*  os_unix.h                                                             */
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

#ifdef UNIX_ENABLED

#include <sys/types.h>
#include "core/os/os.h"

#if defined(__GLIBC__) || defined(WEB_ENABLED)
#include <iconv.h>
#include <langinfo.h>
#define gd_iconv_t iconv_t
#define gd_iconv_open iconv_open
#define gd_iconv iconv
#define gd_iconv_close iconv_close
#else
typedef void* gd_iconv_t;
typedef gd_iconv_t (*PIConvOpen)(const char*, const char*);
typedef size_t (*PIConv)(gd_iconv_t, char**, size_t*, char**, size_t*);
typedef int (*PIConvClose)(gd_iconv_t);
typedef const char* (*PIConvLocaleCharset)();
#endif

class OS_Unix
{
public:
	OS_Unix() = delete;
	~OS_Unix() = delete;
	OS_Unix(const OS_Unix&) = delete;
	OS_Unix& operator=(const OS_Unix&) = delete;

	struct ProcessInfo
	{
		mutable bool is_running = true;
		mutable int exit_code = -1;
	};

private:
	static inline HashMap<ProcessID, ProcessInfo>* process_map = nullptr;
	static inline Mutex process_map_mutex;

#if defined(__GLIBC__) || defined(WEB_ENABLED)
	static inline bool _iconv_ok = true;
#else
	static inline bool _iconv_ok = false;
	static inline PIConvOpen gd_iconv_open = nullptr;
	static inline PIConv gd_iconv = nullptr;
	static inline PIConvClose gd_iconv_close = nullptr;
	static inline PIConvLocaleCharset gd_locale_charset = nullptr;

	static void _load_iconv();
#endif

	static int _wait_for_pid_completion(
		const pid_t p_pid, int* r_status, int p_options, pid_t* r_pid = nullptr);
	static bool _check_pid_is_running(const pid_t p_pid, int* r_status);

public:

	static void initialize();
	static void initialize_core();
	static int unix_initialize_audio(int p_audio_driver);
	static void finalize_core();

	static Vector<String> get_video_adapter_driver_info();

	static String get_stdin_string(int64_t p_buffer_size = 1024);
	static PackedByteArray get_stdin_buffer(int64_t p_buffer_size = 1024);
	static OS::StdHandleType get_stdin_type();
	static OS::StdHandleType get_stdout_type();
	static OS::StdHandleType get_stderr_type();

	static Error get_entropy(uint8_t* r_buffer, int p_bytes);

	static Error open_dynamic_library(
		const String& p_path, void*& p_library_handle, OS::GDExtensionData* p_data = nullptr);
	static Error close_dynamic_library(void* p_library_handle);
	static Error get_dynamic_library_symbol_handle(void* p_library_handle, const String& p_name,
		void*& p_symbol_handle, bool p_optional = false);

	static Error set_cwd(const String& p_cwd);
	static String get_cwd();

	static String get_name();
	static String get_distribution_name();
	static String get_version();

	static String get_temp_path();

	static OS::DateTime get_datetime(bool p_utc);
	static OS::TimeZoneInfo get_time_zone_info();

	static double get_unix_time();

	static void delay_usec(uint32_t p_usec);
	static uint64_t get_ticks_usec();

	static String multibyte_to_string(const String& p_encoding, const PackedByteArray& p_array);
	static PackedByteArray string_to_multibyte(const String& p_encoding, const String& p_string);

	static Error execute(const String& p_path, const List<String>& p_arguments,
		String* r_pipe = nullptr, int* r_exitcode = nullptr, bool read_stderr = false,
		Mutex* p_pipe_mutex = nullptr, bool p_open_console = false);
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

	static String get_locale();

	static void initialize_debugging();

	static String get_executable_path();
	static String get_user_data_dir(const String& p_user_dir);

	static String expand_path(const String& p_path);
};

class UnixTerminalLogger : public StdLogger
{
public:
	virtual ~UnixTerminalLogger();
};

#endif // UNIX_ENABLED

