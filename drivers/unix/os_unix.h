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

// --- Folded from drivers/unix/file_access_unix.h ---
#if defined(UNIX_ENABLED)

#include <cstdio>
#include "core/io/file_access.h"

class FileAccessUnix : public FileAccess
{
	FILE* f = nullptr;
	int flags = 0;
	void check_errors(bool p_write = false) const;
	mutable Error last_error = OK;
	String save_path;
	String path;
	String path_src;

	void _close();

#if defined(TOOLS_ENABLED)
	String get_real_path() const; // Returns the resolved real path for the current open file.
#endif

public:
	typedef void (*CloseNotificationFunc)(const String& p_file, int p_flags);
	static CloseNotificationFunc close_notification_func;

	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String
	get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override;		///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;			///< get position in the file
	virtual uint64_t get_length() const override;			///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override;
	virtual void flush() override;
	virtual bool store_buffer(
		const uint8_t* p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String& p_path) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override;
	virtual uint64_t _get_access_time(const String& p_file) override;
	virtual int64_t _get_size(const String& p_file) override;
	virtual uint32_t _get_unix_permissions(
		const String& p_file) override;
	virtual Error _set_unix_permissions(
		const String& p_file, uint32_t p_permissions) override;

	virtual bool _get_hidden_attribute(const String& p_file) override;
	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override;
	virtual bool _get_read_only_attribute(const String& p_file) override;
	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override;

	virtual PackedByteArray _get_extended_attribute(
		const String& p_file, const String& p_attribute_name) override;
	virtual Error _set_extended_attribute(const String& p_file, const String& p_attribute_name,
		const PackedByteArray& p_data) override;
	virtual Error _remove_extended_attribute(
		const String& p_file, const String& p_attribute_name) override;
	virtual PackedStringArray _get_extended_attributes_list(const String& p_file) override;

	virtual void close() override;

	FileAccessUnix() {}

	virtual ~FileAccessUnix();
};

#endif // UNIX_ENABLED



// --- End drivers/unix/file_access_unix.h ---

// --- Folded from drivers/unix/net_socket_unix.h ---
#if defined(UNIX_ENABLED) && !defined(UNIX_SOCKET_UNAVAILABLE)

#include <sys/socket.h>
#include <sys/un.h>
#include "core/io/net_socket.h"

class NetSocketUnix : public NetSocket
{
private:
	int _sock = -1;
	Family _family = Family::NONE;
	IP::Type _ip_type = IP::TYPE_NONE;
	bool _is_stream = false;
	CharString _unix_path;
	// If this is Family::UNIX,
	bool _unlink_on_close = false;

	enum NetError
	{
		ERR_NET_WOULD_BLOCK,
		ERR_NET_IS_CONNECTED,
		ERR_NET_IN_PROGRESS,
		ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE,
		ERR_NET_UNAUTHORIZED,
		ERR_NET_BUFFER_TOO_SMALL,
		ERR_NET_OTHER,
	};

	NetError _get_socket_error() const;
	void _set_socket(int p_sock, IP::Type p_ip_type, bool p_is_stream);
	_FORCE_INLINE_ Error _change_multicast_group(IPAddress p_ip, String p_if_name, bool p_add);
	_FORCE_INLINE_ void _set_close_exec_enabled(bool p_enabled);

protected:
	static Ref<NetSocket> _create_func();

	bool _can_use_ip(const IPAddress& p_ip, const bool p_for_bind) const;
	bool _can_use_path(const CharString& p_path) const;

	Error _inet_open(Type p_sock_type, IP::Type& r_ip_type);
	Error _inet_bind(IPAddress p_addr, uint16_t p_port);
	Error _inet_connect_to_host(IPAddress p_addr, uint16_t p_port);
	Error _inet_get_socket_address(IPAddress* r_ip, uint16_t* r_port) const;
	Ref<NetSocket> _inet_accept(IPAddress& r_ip, uint16_t& r_port);

	static socklen_t _unix_set_sockaddr(struct sockaddr_un* p_addr, const CharString& p_path);
	Error _unix_open();
	Error _unix_bind(const CharString& p_path);
	Error _unix_connect_to_host(const CharString& p_path);
	Ref<NetSocket> _unix_accept();

public:
	static void make_default();
	static void cleanup();
	static void _set_ip_port(struct sockaddr_storage* p_addr, IPAddress* r_ip, uint16_t* r_port);
	static size_t _set_addr_storage(struct sockaddr_storage* p_addr, const IPAddress& p_ip,
		uint16_t p_port, IP::Type p_ip_type);

	virtual Error open(Family p_family, Type p_sock_type, IP::Type& r_ip_type) override;
	virtual void close() override;
	virtual Error bind(Address p_addr) override;
	virtual Error listen(int p_max_pending) override;
	virtual Error connect_to_host(Address p_addr) override;
	virtual Error poll(PollType p_type, int timeout) const override;
	virtual Error recv(uint8_t* p_buffer, int p_len, int& r_read) override;
	virtual Error recvfrom(uint8_t* p_buffer, int p_len, int& r_read, IPAddress& r_ip,
		uint16_t& r_port, bool p_peek = false) override;
	virtual Error send(const uint8_t* p_buffer, int p_len, int& r_sent) override;
	virtual Error sendto(
		const uint8_t* p_buffer, int p_len, int& r_sent, IPAddress p_ip, uint16_t p_port) override;
	virtual Ref<NetSocket> accept(Address& r_addr) override;

	virtual bool is_open() const override;
	virtual int get_available_bytes() const override;
	virtual Error get_socket_address(Address* r_addr) const override;

	virtual Error set_broadcasting_enabled(bool p_enabled) override;
	virtual void set_blocking_enabled(bool p_enabled) override;
	virtual void set_ipv6_only_enabled(bool p_enabled) override;
	virtual void set_tcp_no_delay_enabled(bool p_enabled) override;
	virtual void set_reuse_address_enabled(bool p_enabled) override;
	virtual Error join_multicast_group(
		const IPAddress& p_multi_address, const String& p_if_name) override;
	virtual Error leave_multicast_group(
		const IPAddress& p_multi_address, const String& p_if_name) override;

	NetSocketUnix();
	~NetSocketUnix() override;
};

#endif // UNIX_ENABLED && !UNIX_SOCKET_UNAVAILABLE



// --- End drivers/unix/net_socket_unix.h ---

// --- Folded from drivers/unix/dir_access_unix.h ---
#if defined(UNIX_ENABLED)

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "core/io/dir_access.h"

class DirAccessUnix : public DirAccess
{
	DIR* dir_stream = nullptr;

	bool _cisdir = false;
	bool _cishidden = false;

protected:
	String current_dir;

	virtual String fix_unicode_name(const char* p_name) const { return String::utf8(p_name); }

	virtual bool is_hidden(const String& p_name);
	virtual String fix_path(const String& p_path) const override;

public:
	typedef void (*RemoveNotificationFunc)(const String& p_file);
	static RemoveNotificationFunc remove_notification_func;

	virtual Error list_dir_begin() override; ///< This starts dir listing
	virtual String get_next() override;
	virtual bool current_is_dir() const override;
	virtual bool current_is_hidden() const override;

	virtual void list_dir_end() override; ///<

	virtual int get_drive_count() override;
	virtual String get_drive(int p_drive) override;
	virtual int get_current_drive() override;
	virtual bool drives_are_shortcuts() override;

	virtual Error change_dir(
		String p_dir) override; ///< can be relative or absolute, return false on success
	virtual String get_current_dir(
		bool p_include_drive = true) const override; ///< return current dir location
	virtual Error make_dir(String p_dir) override;

	virtual bool file_exists(String p_file) override;
	virtual bool dir_exists(String p_dir) override;
	virtual bool is_readable(String p_dir) override;
	virtual bool is_writable(String p_dir) override;

	virtual uint64_t get_modified_time(String p_file);

	virtual Error rename(String p_path, String p_new_path) override;
	virtual Error remove(String p_path) override;

	virtual bool is_link(String p_file) override;
	virtual String read_link(String p_file) override;
	virtual Error create_link(String p_source, String p_target) override;

	virtual bool is_case_sensitive(const String& p_path) const override;
	virtual bool is_equivalent(const String& p_path_a, const String& p_path_b) const override;

	virtual uint64_t get_space_left() override;

	virtual String get_filesystem_type() const override;

	DirAccessUnix();
	~DirAccessUnix();
};

#endif // UNIX_ENABLED
// --- End drivers/unix/dir_access_unix.h ---
