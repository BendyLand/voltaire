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
#include "core/io/net_socket.h"

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

// --- Folded from drivers/coreaudio/audio_driver_coreaudio.h ---
#if defined(COREAUDIO_ENABLED)
#ifdef COREAUDIO_ENABLED

#include "servers/display/display_server.h"

#import <AudioUnit/AudioUnit.h>
#ifdef MACOS_ENABLED
#import <CoreAudio/AudioHardware.h>
#else
#import <AVFoundation/AVFoundation.h>
#endif

class AudioDriverCoreAudio : public AudioDriver {
	AudioComponentInstance audio_unit = nullptr;
	AudioComponentInstance input_unit = nullptr;

	bool active = false;
	Mutex mutex;

	String output_device_name = "Default";
	String input_device_name = "Default";

	int mix_rate = 0;
	int capture_mix_rate = 0;
	unsigned int channels = 2;
	unsigned int capture_channels = 2;
	unsigned int buffer_frames = 0;
	unsigned int capture_buffer_frames = 0;

	Vector<int32_t> samples_in;
	unsigned int buffer_size = 0;

#ifdef MACOS_ENABLED
	PackedStringArray _get_device_list(bool capture = false);
	void _set_device(const String &output_device, bool capture = false);

	static OSStatus input_device_address_cb(AudioObjectID inObjectID,
			UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,
			void *inClientData);

	static OSStatus output_device_address_cb(AudioObjectID inObjectID,
			UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses,
			void *inClientData);
#endif

	static OSStatus output_callback(void *inRefCon,
			AudioUnitRenderActionFlags *ioActionFlags,
			const AudioTimeStamp *inTimeStamp,
			UInt32 inBusNumber, UInt32 inNumberFrames,
			AudioBufferList *ioData);

	static OSStatus input_callback(void *inRefCon,
			AudioUnitRenderActionFlags *ioActionFlags,
			const AudioTimeStamp *inTimeStamp,
			UInt32 inBusNumber, UInt32 inNumberFrames,
			AudioBufferList *ioData);

	Error init_input_device();
	void finish_input_device();

public:
	virtual const char *get_name() const override {
		return "CoreAudio";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual int get_input_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

#ifdef MACOS_ENABLED
	virtual PackedStringArray get_output_device_list() override;
	virtual String get_output_device() override;
	virtual void set_output_device(const String &p_name) override;

	virtual PackedStringArray get_input_device_list() override;
	virtual String get_input_device() override;
	virtual void set_input_device(const String &p_name) override;
#endif

	virtual Error input_start() override;
	virtual Error input_stop() override;

	bool try_lock();
	void stop();

	AudioDriverCoreAudio();
	~AudioDriverCoreAudio() {}
};

#endif // COREAUDIO_ENABLED

// --- Folded from drivers/apple/os_log_logger.h ---
#include "core/os/os.h"

#include <os/log.h>

/**
 * @brief Apple unified logging system integration for Godot Engine.
 */
class OsLogLogger : public Logger {
	os_log_t log;
	os_log_t error_log;
	os_log_t warning_log;
	os_log_t script_log;
	os_log_t shader_log;

public:
	void logv(const char *p_format, va_list p_list, bool p_err) override _PRINTF_FORMAT_ATTRIBUTE_2_0;
	void log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify = false, ErrorType p_type = ERR_ERROR, const Vector<Ref<ScriptBacktrace>> &p_script_backtraces = {}) override;

	/**
	 * @brief Constructs an OsLogLogger with the specified subsystem identifier, which is normally the bundle identifier.
	 */
	OsLogLogger(const char *p_subsystem);
};
// --- End drivers/apple/os_log_logger.h ---

// --- Folded from drivers/apple/foundation_helpers.h ---
#ifdef __OBJC__
#import <Foundation/NSString.h>
#else
#include <Foundation/NSString.hpp>
#endif

class String;
template <typename T>
class CharStringT;

using CharString = CharStringT<char>;

template <typename T>
class Span;

namespace conv {

#ifdef __OBJC__
/**
 * Converts a Godot String to an NSString without allocating an intermediate UTF-8 buffer.
 * */
NSString *to_nsstring(const String &p_str);
/**
 * Converts a Godot String to an NSString without allocating an intermediate UTF-8 buffer.
 * */
NSString *to_nsstring(Span<char> p_str);
/**
 * Converts a Godot CharString to an NSString without allocating an intermediate UTF-8 buffer.
 * */
NSString *to_nsstring(const CharString &p_str);
/**
 * Converts an NSString to a Godot String without allocating intermediate buffers.
 * */
String to_string(NSString *p_str);
#else
/**
 * Converts a Godot String to an NSString without allocating an intermediate UTF-8 buffer.
 * */
NS::String *to_nsstring(const String &p_str);
/**
 * Converts a Godot String to an NSString without allocating an intermediate UTF-8 buffer.
 * */
NS::String *to_nsstring(Span<char> p_str);
/**
 * Converts a Godot CharString to an NSString without allocating an intermediate UTF-8 buffer.
 * */
NS::String *to_nsstring(const CharString &p_str);
/**
 * Converts an NSString to a Godot String without allocating intermediate buffers.
 * */
String to_string(NS::String *p_str);

#endif

} //namespace conv
// --- End drivers/apple/foundation_helpers.h ---
#endif // defined(COREAUDIO_ENABLED)
// --- End drivers/coreaudio/audio_driver_coreaudio.h ---

// --- Folded from platform/web/os_web.h ---
#ifdef WEB_ENABLED
#include <emscripten/html5.h>
// --- Folded from platform/web/audio_driver_web.h ---
#include "core/os/mutex.h"
// --- Inlined/Folded from platform/web/godot_js.h ---
#define WASM_EXPORT __attribute__((visibility("default")))

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Emscripten
extern char *godot_js_emscripten_get_version();

// Config
extern void godot_js_config_locale_get(char *p_ptr, int p_ptr_max);
extern void godot_js_config_canvas_id_get(char *p_ptr, int p_ptr_max);

// OS
extern void godot_js_os_finish_async(void (*p_callback)());
extern void godot_js_os_request_quit_cb(void (*p_callback)());
extern int godot_js_os_fs_is_persistent();
extern void godot_js_os_fs_sync(void (*p_callback)());
extern int godot_js_os_execute(const char *p_json);
extern void godot_js_os_shell_open(const char *p_uri);
extern int godot_js_os_hw_concurrency_get();
extern int godot_js_os_thread_pool_size_get();
extern int godot_js_os_has_feature(const char *p_ftr);
extern int godot_js_pwa_cb(void (*p_callback)());
extern int godot_js_pwa_update();

// Input
extern void godot_js_input_mouse_button_cb(int (*p_callback)(int p_pressed, int p_button, double p_x, double p_y, int p_modifiers));
extern void godot_js_input_mouse_move_cb(void (*p_callback)(double p_x, double p_y, double p_rel_x, double p_rel_y, int p_modifiers, double p_pressure));
extern void godot_js_input_mouse_wheel_cb(int (*p_callback)(int p_delta_mode, double p_delta_x, double p_delta_y));
extern void godot_js_input_touch_cb(void (*p_callback)(int p_type, int p_count), uint32_t *r_identifiers, double *r_coords);
extern void godot_js_input_key_cb(void (*p_callback)(int p_type, int p_repeat, int p_modifiers), char r_code[32], char r_key[32]);
extern void godot_js_input_vibrate_handheld(int p_duration_ms);

extern void godot_js_set_ime_active(int p_active);
extern void godot_js_set_ime_position(int p_x, int p_y);
extern void godot_js_set_ime_cb(void (*p_input)(int p_type, const char *p_text), void (*p_callback)(int p_type, int p_repeat, int p_modifiers), char r_code[32], char r_key[32]);
extern int godot_js_is_ime_focused();

// Input gamepad
extern void godot_js_input_gamepad_cb(void (*p_on_change)(int p_index, int p_connected, const char *p_id, const char *p_guid));
extern int godot_js_input_gamepad_sample();
extern int godot_js_input_gamepad_sample_count();
extern int godot_js_input_gamepad_sample_get(int p_idx, float r_btns[16], int32_t *r_btns_num, float r_axes[10], int32_t *r_axes_num, int32_t *r_standard);
extern void godot_js_input_paste_cb(void (*p_callback)(const char *p_text));
extern void godot_js_input_drop_files_cb(void (*p_callback)(const char **p_filev, int p_filec));

// TTS
extern int godot_js_tts_is_speaking();
extern int godot_js_tts_is_paused();
extern int godot_js_tts_get_voices(void (*p_callback)(int p_size, const char **p_voices));
extern void godot_js_tts_speak(const char *p_text, const char *p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, void (*p_callback)(int p_event, int64_t p_id, int p_pos));
extern void godot_js_tts_pause();
extern void godot_js_tts_resume();
extern void godot_js_tts_stop();

// Display
extern int godot_js_display_screen_dpi_get();
extern double godot_js_display_pixel_ratio_get();
extern void godot_js_display_alert(const char *p_text);
extern int godot_js_display_touchscreen_is_available();
extern int godot_js_display_is_swap_ok_cancel();
extern void godot_js_display_setup_canvas(int p_width, int p_height, int p_fullscreen, int p_hidpi);

// Display canvas
extern void godot_js_display_canvas_focus();
extern int godot_js_display_canvas_is_focused();

// Display window
extern void godot_js_display_desired_size_set(int p_width, int p_height);
extern int godot_js_display_size_update();
extern void godot_js_display_window_size_get(int32_t *p_x, int32_t *p_y);
extern void godot_js_display_screen_size_get(int32_t *p_x, int32_t *p_y);
extern int godot_js_display_fullscreen_request();
extern int godot_js_display_fullscreen_exit();
extern void godot_js_display_window_title_set(const char *p_text);
extern void godot_js_display_window_icon_set(const uint8_t *p_ptr, int p_len);
extern int godot_js_display_has_webgl(int p_version);

// Display clipboard
extern int godot_js_display_clipboard_set(const char *p_text);
extern int godot_js_display_clipboard_get(void (*p_callback)(const char *p_text));

// Display cursor
extern void godot_js_display_cursor_set_shape(const char *p_cursor);
extern int godot_js_display_cursor_is_hidden();
extern void godot_js_display_cursor_set_custom_shape(const char *p_shape, const uint8_t *p_ptr, int p_len, int p_hotspot_x, int p_hotspot_y);
extern void godot_js_display_cursor_set_visible(int p_visible);
extern void godot_js_display_cursor_lock_set(int p_lock);
extern int godot_js_display_cursor_is_locked();

// Display listeners
extern void godot_js_display_fullscreen_cb(void (*p_callback)(int p_fullscreen));
extern void godot_js_display_window_blur_cb(void (*p_callback)());
extern void godot_js_display_notification_cb(void (*p_callback)(int p_notification), int p_enter, int p_exit, int p_in, int p_out);

// Display Virtual Keyboard
extern int godot_js_display_vk_available();
extern int godot_js_display_tts_available();
extern void godot_js_display_vk_cb(void (*p_input)(const char *p_text, int p_cursor));
extern void godot_js_display_vk_show(const char *p_text, int p_type, int p_start, int p_end);
extern void godot_js_display_vk_hide();

#ifdef __cplusplus
}
#endif
#include "servers/display/display_server.h"

class AudioDriverWeb : public AudioDriver
{
private:
	struct AudioContext
	{
		bool inited = false;
		float output_latency = 0.0;
		int state = -1;
		int channel_count = 0;
		int mix_rate = 0;
	};

	static AudioContext audio_context;

	float* output_rb = nullptr;
	float* input_rb = nullptr;

	int buffer_length = 0;
	int mix_rate = 0;
	int channel_count = 0;

	WASM_EXPORT static void _state_change_callback(int p_state);
	WASM_EXPORT static void _latency_update_callback(float p_latency);
	WASM_EXPORT static void _sample_playback_finished_callback(const char* p_playback_object_id);

protected:
	void _audio_driver_process(int p_from = 0, int p_samples = 0);
	void _audio_driver_capture(int p_from = 0, int p_samples = 0);

	float* get_output_rb() const { return output_rb; }

	float* get_input_rb() const { return input_rb; }

	virtual Error create(int& p_buffer_samples, int p_channels) = 0;
	virtual void start(
		float* p_out_buf, int p_out_buf_size, float* p_in_buf, int p_in_buf_size) = 0;

	virtual void finish_driver() {}

public:
	static bool is_available();

	virtual Error init() final;
	virtual void start() final;
	virtual void finish() final;

	virtual int get_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;
	virtual float get_latency() override;

	virtual Error input_start() override;
	virtual Error input_stop() override;

	static void resume();

	// Samples.
	virtual bool is_stream_registered_as_sample(const Ref<AudioStream>& p_stream) const override;
	virtual void register_sample(const Ref<AudioSample>& p_sample) override;
	virtual void unregister_sample(const Ref<AudioSample>& p_sample) override;
	virtual void start_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;
	virtual void stop_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;
	virtual void set_sample_playback_pause(
		const Ref<AudioSamplePlayback>& p_playback, bool p_paused) override;
	virtual bool is_sample_playback_active(const Ref<AudioSamplePlayback>& p_playback) override;
	virtual double get_sample_playback_position(
		const Ref<AudioSamplePlayback>& p_playback) override;
	virtual void update_sample_playback_pitch_scale(
		const Ref<AudioSamplePlayback>& p_playback, float p_pitch_scale = 0.0f) override;
	virtual void set_sample_playback_bus_volumes_linear(const Ref<AudioSamplePlayback>& p_playback,
		const HashMap<String, Vector<AudioFrame>>& p_bus_volumes) override;

	virtual void set_sample_bus_count(int p_count) override;
	virtual void remove_sample_bus(int p_index) override;
	virtual void add_sample_bus(int p_at_pos = -1) override;
	virtual void move_sample_bus(int p_bus, int p_to_pos) override;
	virtual void set_sample_bus_send(int p_bus, const String& p_send) override;
	virtual void set_sample_bus_volume_db(int p_bus, float p_volume_db) override;
	virtual void set_sample_bus_solo(int p_bus, bool p_enable) override;
	virtual void set_sample_bus_mute(int p_bus, bool p_enable) override;

	AudioDriverWeb() {}
};

#ifdef THREADS_ENABLED
class AudioDriverWorklet : public AudioDriverWeb
{
private:
	enum
	{
		STATE_LOCK,
		STATE_PROCESS,
		STATE_SAMPLES_IN,
		STATE_SAMPLES_OUT,
		STATE_MAX,
	};

	Mutex mutex;
	Thread thread;
	bool quit = false;
	int32_t state[STATE_MAX] = {0};

	static void _audio_thread_func(void* p_data);

protected:
	virtual Error create(int& p_buffer_size, int p_output_channels) override;
	virtual void start(
		float* p_out_buf, int p_out_buf_size, float* p_in_buf, int p_in_buf_size) override;
	virtual void finish_driver() override;

public:
	virtual const char* get_name() const override { return "AudioWorklet"; }

	virtual void lock() override;
	virtual void unlock() override;
};

#else

class AudioDriverWorklet : public AudioDriverWeb
{
private:
	static void _process_callback(int p_pos, int p_samples);
	static void _capture_callback(int p_pos, int p_samples);

protected:
	virtual Error create(int& p_buffer_size, int p_output_channels) override;
	virtual void start(
		float* p_out_buf, int p_out_buf_size, float* p_in_buf, int p_in_buf_size) override;

public:
	virtual const char* get_name() const override { return "AudioWorklet"; }

	virtual void lock() override {}

	virtual void unlock() override {}

	AudioDriverWorklet() {}
};

#endif // THREADS_ENABLED

class AudioDriverScriptProcessor : public AudioDriverWeb
{
private:
	static void _process_callback();

protected:
	virtual Error create(int& p_buffer_size, int p_output_channels) override;
	virtual void start(
		float* p_out_buf, int p_out_buf_size, float* p_in_buf, int p_in_buf_size) override;

public:
	virtual const char* get_name() const override { return "ScriptProcessor"; }

	virtual void lock() override {}

	virtual void unlock() override {}

	AudioDriverScriptProcessor() {}
};



// --- End platform/web/audio_driver_web.h ---
#include "core/os/os.h"


// --- Folded from platform/web/webmidi_driver.h ---


#include "core/os/os.h"

class MIDIDriverWebMidi final
{
private:
	static constexpr int MAX_EVENT_BUFFER_LENGTH = 2;
	static inline uint8_t _event_buffer[MAX_EVENT_BUFFER_LENGTH] = {0};

public:
	static void initialize();
	static void finalize();
	static Error open();
	static void close();

	MIDIDriverWebMidi() = delete;
	MIDIDriverWebMidi(const MIDIDriverWebMidi&) = delete;
	MIDIDriverWebMidi& operator=(const MIDIDriverWebMidi&) = delete;
	~MIDIDriverWebMidi() = delete;

	WASM_EXPORT static void set_input_names_callback(int p_size, const char** p_input_names);
	static void _set_input_names_callback(const Vector<String>& p_input_names);

	WASM_EXPORT static void on_midi_message(
		int p_device_index, int p_status, const uint8_t* p_data, int p_data_len);
	static void _on_midi_message(
		int p_device_index, int p_status, const PackedByteArray& p_data, int p_data_len);
};

// --- End platform/web/webmidi_driver.h ---

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

// --- Folded from platform/web/display_server_web.h ---


#include "core/os/os.h"
#include "servers/display/display_server.h"

#include <emscripten.h>
#include <emscripten/html5.h>

class InputEvent;
class InputEventWithModifiers;

class DisplayServerWeb : public DisplayServerDriver {

private:
	struct JSTouchEvent {
		uint32_t identifier[32] = { 0 };
		double coords[64] = { 0 };
	};
	JSTouchEvent touch_event;

	struct JSKeyEvent {
		char code[32] = { 0 };
		char key[32] = { 0 };
		uint8_t modifiers[4] = { 0 };
	};
	JSKeyEvent key_event;

#ifdef GLES3_ENABLED
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_ctx = 0;
#endif

	HashMap<int64_t, CharString> utterance_ids;

	DisplayServerEnums::WindowMode window_mode = DisplayServerEnums::WINDOW_MODE_WINDOWED;
	ObjectID window_attached_instance_id;

	Callable rect_changed_callback;
	Callable window_event_callback;
	Callable input_event_callback;
	Callable input_text_callback;
	Callable drop_files_callback;

	String clipboard;
	Point2 touches[32];

	Array voices;

	char canvas_id[256] = { 0 };
	bool cursor_inside_canvas = true;
	DisplayServerEnums::CursorShape cursor_shape = DisplayServerEnums::CURSOR_ARROW;
	Point2i last_click_pos = Point2(-100, -100); // TODO check this again.
	uint64_t last_click_ms = 0;
	MouseButton last_click_button_index = MouseButton::NONE;

	bool ime_active = false;
	bool ime_started = false;
	String ime_text;
	Vector2i ime_selection;

	struct KeyEvent {
		bool pressed = false;
		bool echo = false;
		bool raw = false;
		Key keycode = Key::NONE;
		Key physical_keycode = Key::NONE;
		Key key_label = Key::NONE;
		uint32_t unicode = 0;
		KeyLocation location = KeyLocation::UNSPECIFIED;
		int mod = 0;
	};

	Vector<KeyEvent> key_event_buffer;
	int key_event_pos = 0;

	bool swap_cancel_ok = false;

	int gamepad_count = 0;

	DisplayServerEnums::MouseMode mouse_mode_base = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	DisplayServerEnums::MouseMode mouse_mode_override = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	bool mouse_mode_override_enabled = false;
	void _mouse_update_mode();

	// utilities
	static void dom2godot_mod(Ref<InputEventWithModifiers> ev, int p_mod, Key p_keycode);
	static const char *godot2dom_cursor(DisplayServerEnums::CursorShape p_shape);

	// events
	WASM_EXPORT static void fullscreen_change_callback(int p_fullscreen);
	static void _fullscreen_change_callback(int p_fullscreen);
	WASM_EXPORT static int mouse_button_callback(int p_pressed, int p_button, double p_x, double p_y, int p_modifiers);
	static int _mouse_button_callback(int p_pressed, int p_button, double p_x, double p_y, int p_modifiers);
	WASM_EXPORT static void mouse_move_callback(double p_x, double p_y, double p_rel_x, double p_rel_y, int p_modifiers, double p_pressure);
	static void _mouse_move_callback(double p_x, double p_y, double p_rel_x, double p_rel_y, int p_modifiers, double p_pressure);
	WASM_EXPORT static int mouse_wheel_callback(int p_delta_mode, double p_delta_x, double p_delta_y);
	static int _mouse_wheel_callback(int p_delta_mode, double p_delta_x, double p_delta_y);
	WASM_EXPORT static void touch_callback(int p_type, int p_count);
	static void _touch_callback(int p_type, int p_count);
	WASM_EXPORT static void key_callback(int p_pressed, int p_repeat, int p_modifiers);
	static void _key_callback(const String &p_key_event_code, const String &p_key_event_key, int p_pressed, int p_repeat, int p_modifiers);
	WASM_EXPORT static void vk_input_text_callback(const char *p_text, int p_cursor);
	static void _vk_input_text_callback(const String &p_text, int p_cursor);
	WASM_EXPORT static void gamepad_callback(int p_index, int p_connected, const char *p_id, const char *p_guid);
	static void _gamepad_callback(int p_index, int p_connected, const String &p_id, const String &p_guid);
	WASM_EXPORT static void js_utterance_callback(int p_event, int64_t p_id, int p_pos);
	static void _js_utterance_callback(int p_event, int64_t p_id, int p_pos);
	WASM_EXPORT static void ime_callback(int p_type, const char *p_text);
	static void _ime_callback(int p_type, const String &p_text);
	WASM_EXPORT static void request_quit_callback();
	static void _request_quit_callback();
	WASM_EXPORT static void window_blur_callback();
	static void _window_blur_callback();
	WASM_EXPORT static void update_voices_callback(int p_size, const char **p_voice);
	static void _update_voices_callback(const Vector<String> &p_voices);
	WASM_EXPORT static void update_clipboard_callback(const char *p_text);
	static void _update_clipboard_callback(const String &p_text);
	WASM_EXPORT static void send_window_event_callback(int p_notification);
	static void _send_window_event_callback(int p_notification);
	WASM_EXPORT static void drop_files_js_callback(const char **p_filev, int p_filec);
	static void _drop_files_js_callback(const Vector<String> &p_files);

	void process_joypads();
	void process_keys();

	static Vector<String> get_rendering_drivers_func();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_window_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);

	static void _dispatch_input_event(const Ref<InputEvent> &p_event);

protected:
	int get_current_video_driver() const;

public:
	// utilities
	bool check_size_force_redraw();

	// from DisplayServer
	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;
	virtual String get_name() const override;

	// tts
	virtual bool tts_is_speaking() const override;
	virtual bool tts_is_paused() const override;
	virtual TypedArray<Dictionary> tts_get_voices() const override;

	virtual void tts_speak(const String &p_text, const String &p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false) override;
	virtual void tts_pause() override;
	virtual void tts_resume() override;
	virtual void tts_stop() override;

	// cursor
	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override;
	virtual DisplayServerEnums::CursorShape cursor_get_shape() const override;
	virtual void cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW, const Vector2 &p_hotspot = Vector2()) override;

	// mouse
	virtual void mouse_set_mode(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode() const override;
	virtual void mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode_override() const override;
	virtual void mouse_set_mode_override_enabled(bool p_override_enabled) override;
	virtual bool mouse_is_mode_override_enabled() const override;

	virtual Point2i mouse_get_position() const override;

	// ime
	virtual void window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual Point2i ime_get_selection() const override;
	virtual String ime_get_text() const override;

	// touch
	virtual bool is_touchscreen_available() const override;

	// clipboard
	virtual void clipboard_set(const String &p_text) override;
	virtual String clipboard_get() const override;

	// screen
	virtual int get_screen_count() const override;
	virtual int get_primary_screen() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual void screen_set_keep_on(bool p_enable) override {}

	virtual void virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect = Rect2(), DisplayServerEnums::VirtualKeyboardType p_type = DisplayServerEnums::KEYBOARD_TYPE_DEFAULT, int p_max_input_length = -1, int p_cursor_start = -1, int p_cursor_end = -1) override;
	virtual void virtual_keyboard_hide() override;

	// windows
	virtual Vector<DisplayServerEnums::WindowID> get_window_list() const override;
	virtual DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i &p_position) const override;

	virtual void window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual ObjectID window_get_attached_instance_id(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual Point2i window_get_position(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Point2i window_get_position_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) override;

	virtual void window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_max_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_min_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::WindowMode window_get_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool window_is_maximize_allowed(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_request_attention(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_move_to_foreground(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_focused(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool can_any_window_draw() const override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override {}
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

	// events
	virtual void process_events() override;

	// icon
	virtual void set_icon(const Ref<Image> &p_icon) override;

	// others
	virtual bool get_swap_cancel_ok() override;
	virtual void swap_buffers() override;

	static void register_web_driver();
	DisplayServerWeb(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_window_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Point2i *p_position, const Size2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerWeb();
};

// --- End platform/web/display_server_web.h ---
#endif // WEB_ENABLED

// --- Folded from platform/web/api/javascript_bridge_singleton.h ---
#include "core/types.h"

class JavaScriptBridge final
{
public:
	static void download_buffer(Vector<uint8_t> p_arr, const String& p_name,
		const String& p_mime = "application/octet-stream");
	static bool pwa_needs_update();
	static Error pwa_update();
	static void force_fs_sync();

	JavaScriptBridge() = delete;
	JavaScriptBridge(const JavaScriptBridge&) = delete;
	JavaScriptBridge& operator=(const JavaScriptBridge&) = delete;
	~JavaScriptBridge() = delete;
};

// --- End platform/web/api/javascript_bridge_singleton.h ---
// --- End platform/web/os_web.h ---

// --- Folded from platform/macos/os_macos.h ---
#if defined(MACOS_ENABLED)
// --- Inlined/Folded from platform/macos/crash_handler_macos.h ---
class CrashHandler {
	bool disabled;

public:
	void initialize();

	void disable();
	bool is_disabled() const { return disabled; }

	CrashHandler();
	~CrashHandler();
};

#include "core/os/os.h"
#include "core/types.h"

// --- Inlined/Folded from drivers/coremidi/midi_driver_coremidi.h ---
#ifdef COREMIDI_ENABLED

#include "core/os/os.h"
#include "core/os/mutex.h"
#include "core/string/ustring.h"

#import <CoreMIDI/CoreMIDI.h>

#include <cstdio>

class MIDIDriverCoreMidi final
{
	struct InputConnection
	{
		InputConnection() = default;
		InputConnection(int p_device_index, MIDIEndpointRef p_source);
		MIDIDriver::Parser parser;
		MIDIEndpointRef source;
	};

public:
	struct Data
	{
		MIDIClientRef client = 0;
		MIDIPortRef port_in = 0;
		Vector<InputConnection*> connected_sources;
		Mutex mutex;
		bool core_midi_closed = false;
	};

	static inline Data* data = nullptr;

private:
	static void read(
		const MIDIPacketList* packet_list, void* read_proc_ref_con, void* src_conn_ref_con);

public:
	static void initialize();
	static void finalize();
	static Error open();
	static void close();

	MIDIDriverCoreMidi() = delete;
	MIDIDriverCoreMidi(const MIDIDriverCoreMidi&) = delete;
	MIDIDriverCoreMidi& operator=(const MIDIDriverCoreMidi&) = delete;
	~MIDIDriverCoreMidi() = delete;
};

#endif // COREMIDI_ENABLED

#include "servers/display/display_server.h"

class JoypadSDL;

class OS_MacOS : public OS_Unix {
#ifdef COREAUDIO_ENABLED
	AudioDriverCoreAudio audio_driver;
#endif

	CrashHandler crash_handler;

	List<String> launch_service_args;

	CGFloat _weight_to_ct(int p_weight) const;
	CGFloat _stretch_to_ct(int p_stretch) const;
	String _get_default_fontname(const String &p_font_name) const;

	static _FORCE_INLINE_ String get_framework_executable(const String &p_path);

protected:
	const char *execpath = nullptr;
	int argc = 0;
	char **argv = nullptr;

#ifdef SDL_ENABLED
	JoypadSDL *joypad_sdl = nullptr;
#endif
	CFRunLoopTimerRef wait_timer = nil;

	virtual void initialize_core() override;
	virtual void initialize() override;
	virtual void finalize() override;

	virtual void initialize_joypads() override;

	void delete_main_loop();

public:
	static inline const char *headless_args[] = {
		"--headless",
		"-h",
		"--help",
		"/?",
		"--version",
		"--dump-gdextension-interface",
		"--dump-extension-api",
		"--dump-gdextension-interface-json",
		"--dump-extension-api-with-docs",
		"--validate-extension-api",
		"--convert-3to4",
		"--validate-conversion-3to4",
		"--doctool",
		"--test",
	};

	virtual void add_frame_delay(bool p_can_draw, bool p_wake_for_events) override;

	virtual void set_cmdline_platform_args(const List<String> &p_args);
	virtual List<String> get_cmdline_platform_args() const override;

	virtual void load_shell_environment() const override;

	virtual String get_name() const override;
	virtual String get_distribution_name() const override;
	virtual String get_version() const override;
	virtual String get_version_alias() const override;

	virtual void alert(const String &p_alert, const String &p_title = "ALERT!") override;

	virtual Error open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data = nullptr) override;

	virtual String get_config_path() const override;
	virtual String get_data_path() const override;
	virtual String get_cache_path() const override;
	virtual String get_temp_path() const override;
	virtual String get_bundle_resource_dir() const override;
	virtual String get_bundle_icon_path() const override;
	virtual String get_bundle_icon_name() const override;
	virtual String get_godot_dir_name() const override;

	virtual String get_system_dir(SystemDir p_dir, bool p_shared_storage = true) const override;

	virtual Error shell_open(const String &p_uri) override;
	virtual Error shell_show_in_file_manager(String p_path, bool p_open_folder) override;

	virtual String get_locale() const override;
	virtual Vector<String> get_preferred_locales() const override;

	virtual Vector<String> get_system_fonts() const override;
	virtual String get_system_font_path(const String &p_font_name, int p_weight = 400, int p_stretch = 100, bool p_italic = false) const override;
	virtual Vector<String> get_system_font_path_for_text(const String &p_font_name, const String &p_text, const String &p_locale = String(), const String &p_script = String(), int p_weight = 400, int p_stretch = 100, bool p_italic = false) const override;
	virtual String get_executable_path() const override;
	virtual Error create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id = nullptr, bool p_open_console = false) override;
	virtual Error create_instance(const List<String> &p_arguments, ProcessID *r_child_id = nullptr) override;
	virtual Error open_with_program(const String &p_program_path, const List<String> &p_paths) override;
	virtual bool is_process_running(const ProcessID &p_pid) const override;

	virtual String get_unique_id() const override;
	virtual String get_processor_name() const override;

	virtual String get_model_name() const override;

	virtual bool is_sandboxed() const override;
	virtual bool request_permission(const String &p_name) override;
	virtual Vector<String> get_granted_permissions() const override;
	virtual void revoke_granted_permissions() override;

#ifdef TOOLS_ENABLED
	static bool is_debugger_attached();
	void wait_for_debugger(uint32_t p_msec);
#endif

	virtual bool _check_internal_feature_support(const String &p_feature) override;

	virtual void disable_crash_handler() override;
	virtual bool is_disable_crash_handler() const override;

	virtual Error move_to_trash(const String &p_path) override;

	virtual String get_system_ca_certificates() override;
	virtual OS::PreferredTextureFormat get_preferred_texture_format() const override;

	virtual void run() = 0;

	virtual String get_platform_string(PlatformString p_platform_string) const override {
		switch (p_platform_string) {
			case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_OPEN:
				return ETR("Open in Finder");
			case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_SHOW:
				return ETR("Show in Finder");
			default:
				return OS::get_platform_string(p_platform_string);
		}
	}

	OS_MacOS(const char *p_execpath, int p_argc, char **p_argv);
};

class OS_MacOS_NSApp : public OS_MacOS {
	id delegate = nullptr;
	bool should_terminate = false;
	bool main_started = false;

	CFRunLoopObserverRef pre_wait_observer = nil;

	void terminate();

public:
	void start_main(); // Initializes and runs Godot main loop.
	void cleanup();
	bool os_should_terminate() const { return should_terminate; }
	int get_cmd_argc() const { return argc; }

	virtual void run() override;

	OS_MacOS_NSApp(const char *p_execpath, int p_argc, char **p_argv);
};

class OS_MacOS_Headless : public OS_MacOS {
public:
	virtual void run() override;

	OS_MacOS_Headless(const char *p_execpath, int p_argc, char **p_argv);
};

#ifdef TOOLS_ENABLED

class OS_MacOS_Embedded : public OS_MacOS {
public:
	virtual void run() override;

	OS_MacOS_Embedded(const char *p_execpath, int p_argc, char **p_argv);
};

#endif

// --- Folded from platform/macos/dir_access_macos.h ---
#if defined(UNIX_ENABLED)



#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class DirAccessMacOS : public DirAccessUnix {

protected:
	virtual String fix_unicode_name(const char *p_name) const override;

	virtual int get_drive_count() override;
	virtual String get_drive(int p_drive) override;

	virtual bool is_hidden(const String &p_name) override;
	virtual bool is_case_sensitive(const String &p_path) const override;

	virtual String get_filesystem_type() const override;

	virtual bool is_bundle(const String &p_file) const override;
};

#endif // UNIX ENABLED

// --- Folded from platform/macos/godot_application.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <IOKit/hidsystem/ev_keymap.h>

@class GodotApplicationDelegate;

@interface GodotApplication : NSApplication

extern "C" GodotApplication *GodotApp;

@property(readonly, nonatomic) GodotApplicationDelegate *godotDelegate;

- (GodotApplication *)init;

- (void)activateApplication;
@end

// --- Folded from platform/macos/godot_application_delegate.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

class OS_MacOS_NSApp;

@interface GodotApplicationDelegate : NSObject <NSUserInterfaceItemSearching, NSApplicationDelegate, NSMenuItemValidation>

- (GodotApplicationDelegate *)initWithOS:(OS_MacOS_NSApp *)os;

- (bool)getHighContrast;
- (bool)getReduceMotion;
- (bool)getReduceTransparency;
- (bool)getVoiceOver;
@end

// --- Folded from platform/macos/display_server_macos.h ---
#if defined(MACOS_ENABLED)
// --- Inlined/Folded from platform/macos/display_server_macos_base.h ---
#include "core/os/os.h"
#include "core/typedefs.h" // IWYU pragma: keep. Prevent macOS `MAX`/`MIN` from being defined.
#include "servers/display/display_server.h"

#define FontVariation __FontVariation

#import <AppKit/AppKit.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

#undef FontVariation

class RenderingContextDriver;
class RenderingDevice;

class DisplayServerMacOSBase : public DisplayServerDriver {

	id tts = nullptr;
	IOPMAssertionID screen_keep_on_assertion = kIOPMNullAssertionID;

	struct LayoutInfo {
		String name;
		String code;
	};
	mutable Vector<LayoutInfo> kbd_layouts;
	mutable int current_layout = 0;
	mutable bool keyboard_layout_dirty = true;

	Callable system_theme_changed;

	constexpr static CGFloat HARDWARE_REFERENCE_LUMINANCE_NITS = 100.0f;

public:
	struct HDROutput {
		static constexpr float AUTO_MAX_LUMINANCE = -1.0f;

		bool requested = false;
		float max_luminance = AUTO_MAX_LUMINANCE;

		bool is_auto_max_luminance() const { return max_luminance < 0.0f; }
	};

protected:
	_THREAD_SAFE_CLASS_

	DisplayServerEnums::MouseMode mouse_mode = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	DisplayServerEnums::MouseMode mouse_mode_base = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	DisplayServerEnums::MouseMode mouse_mode_override = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	bool mouse_mode_override_enabled = false;

	void _mouse_update_mode();
	virtual void _mouse_apply_mode(DisplayServerEnums::MouseMode p_prev_mode, DisplayServerEnums::MouseMode p_new_mode) = 0;

	String im_text;
	Point2i im_selection;

	DisplayServerEnums::CursorShape cursor_shape = DisplayServerEnums::CURSOR_ARROW;

	void initialize_tts() const;

	static CGDirectDisplayID _get_display_id_for_screen(NSScreen *p_screen);

	void _update_keyboard_layouts() const;
	static void _keyboard_layout_changed(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef user_info);

#if defined(RD_ENABLED)
	RenderingContextDriver *rendering_context = nullptr;
	RenderingDevice *rendering_device = nullptr;
#endif

	virtual HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) = 0;
	virtual const HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) const = 0;

	constexpr float _calculate_current_reference_luminance(CGFloat p_max_potential_edr_value, CGFloat p_max_edr_value) const;
	void _update_hdr_output(DisplayServerEnums::WindowID p_window, const HDROutput &p_hdr);

public:
	virtual bool has_window(DisplayServerEnums::WindowID p_window) const { return p_window == DisplayServerEnums::MAIN_WINDOW_ID; }

	virtual bool tts_is_speaking() const override;
	virtual bool tts_is_paused() const override;
	virtual TypedArray<Dictionary> tts_get_voices() const override;

	virtual void tts_speak(const String &p_text, const String &p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false) override;
	virtual void tts_pause() override;
	virtual void tts_resume() override;
	virtual void tts_stop() override;

	void emit_system_theme_changed();
	virtual bool is_dark_mode_supported() const override;
	virtual bool is_dark_mode() const override;
	virtual Color get_accent_color() const override;
	virtual Color get_base_color() const override;
	virtual void set_system_theme_change_callback(const Callable &p_callable) override;

	virtual void send_window_event_by_id(DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual void mouse_set_mode(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode() const override;
	virtual void mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode_override() const override;
	virtual void mouse_set_mode_override_enabled(bool p_override_enabled) override;
	virtual bool mouse_is_mode_override_enabled() const override;

	virtual void clipboard_set(const String &p_text) override;
	virtual String clipboard_get() const override;
	virtual Ref<Image> clipboard_get_image() const override;
	virtual bool clipboard_has() const override;
	virtual bool clipboard_has_image() const override;

	virtual int get_primary_screen() const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual void screen_set_keep_on(bool p_enable) override;
	virtual bool screen_is_kept_on() const override;

	virtual int accessibility_should_increase_contrast() const override;
	virtual int accessibility_should_reduce_animation() const override;
	virtual int accessibility_should_reduce_transparency() const override;
	virtual int accessibility_screen_reader_active() const override;

	void update_im_text(const Point2i &p_selection, const String &p_text);
	virtual Point2i ime_get_selection() const override;
	virtual String ime_get_text() const override;

	virtual DisplayServerEnums::CursorShape cursor_get_shape() const override;

	virtual void beep() const override;

	virtual int keyboard_get_layout_count() const override;
	virtual int keyboard_get_current_layout() const override;
	virtual void keyboard_set_current_layout(int p_index) override;
	virtual String keyboard_get_layout_language(int p_index) const override;
	virtual String keyboard_get_layout_name(int p_index) const override;
	virtual Key keyboard_get_keycode_from_physical(Key p_keycode) const override;
	virtual Key keyboard_get_label_from_physical(Key p_keycode) const override;
	virtual void show_emoji_and_symbol_picker() const override;

	virtual void window_get_edr_values(DisplayServerEnums::WindowID p_window, CGFloat *r_max_potential_edr_value = nullptr, CGFloat *r_max_edr_value = nullptr) const = 0;
	virtual bool window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_request_hdr_output(const bool p_enable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual bool window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual float window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual float window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual float window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual float window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual float window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	DisplayServerMacOSBase();
	~DisplayServerMacOSBase();
};

// --- Folded from platform/macos/macos_quartz_core_spi.h ---
// Copyright 2014 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the OpenEmu Team nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import <Foundation/Foundation.h>
#import <Quartz/Quartz.h>

#include <cstdint>

// https://chromium.googlesource.com/chromium/src/+/refs/heads/main/ui/base/cocoa/remote_layer_api.h

// The CAContextID type identifies a CAContext across processes. This is the
// token that is passed from the process that is sharing the CALayer that it is
// rendering to the process that will be displaying that CALayer.
typedef uint32_t CAContextID;

// The CAContext has a static CAContextID which can be sent to another process.
// When a CALayerHost is created using that CAContextID in another process, the
// content displayed by that CALayerHost will be the content of the CALayer
// that is set as the |layer| property on the CAContext.
@interface CAContext : NSObject
+ (instancetype)contextWithCGSConnection:(CAContextID)contextId options:(NSDictionary *)optionsDict;
@property(readonly) CAContextID contextId;
@property(retain) CALayer *layer;
@end

// The CALayerHost is created in the process that will display the content
// being rendered by another process. Setting the |contextId| property on
// an object of this class will make this layer display the content of the
// CALayer that is set to the CAContext with that CAContextID in the layer
// sharing process.
@interface CALayerHost : CALayer
@property CAContextID contextId;
@end

// The CGSConnectionID is used to create the CAContext in the process that is
// going to share the CALayers that it is rendering to another process to
// display.
typedef uint32_t CGSConnectionID;
extern "C" CGSConnectionID CGSMainConnectionID(void);

extern "C" NSString *const kCAContextCIFilterBehavior;

// --- Folded from platform/macos/rendering_context_driver_vulkan_macos.h ---
#ifdef VULKAN_ENABLED

#include "drivers/vulkan/rendering_context_driver_vulkan.h"

#import <QuartzCore/CAMetalLayer.h>

class RenderingContextDriverVulkanMacOS : public RenderingContextDriverVulkan {
private:
	virtual const char *_get_platform_surface_extension() const override final;

protected:
	SurfaceID surface_create(const void *p_platform_data) override final;

public:
	struct WindowPlatformData {
		CAMetalLayer *const *layer_ptr;
	};

	RenderingContextDriverVulkanMacOS();
	~RenderingContextDriverVulkanMacOS();
};

#endif // VULKAN_ENABLED

// --- Folded from platform/macos/tts_macos.h ---
#include "core/string/ustring.h"
#include "core/types.h"
#include "core/templates/list.h"

#import <AppKit/AppKit.h>

#if __has_include(<AVFAudio/AVSpeechSynthesis.h>)
#import <AVFAudio/AVSpeechSynthesis.h>
#else
#import <AVFoundation/AVFoundation.h>
#endif

struct TTSUtterance;

@interface TTS_MacOS : NSObject <AVSpeechSynthesizerDelegate> {
	bool speaking;
	HashMap<id, int64_t> ids;

	id synth;
	List<TTSUtterance> queue;
}

- (void)pauseSpeaking;
- (void)resumeSpeaking;
- (void)stopSpeaking;
- (bool)isSpeaking;
- (bool)isPaused;
- (void)speak:(const String &)text voice:(const String &)voice volume:(int)volume pitch:(float)pitch rate:(float)rate utterance_id:(int64_t)utterance_id interrupt:(bool)interrupt;
- (Array)getVoices;
@end

#include "core/os/os.h"

#define FontVariation __FontVariation
#define BitMap _QDBitMap // Suppress deprecated QuickDraw definition.

#import <AppKit/AppKit.h>
#import <AppKit/NSCursor.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

#undef BitMap
#undef FontVariation

@class GodotWindow;
@class GodotContentView;
@class GodotWindowDelegate;
@class GodotButtonView;
@class GodotProgressView;

class InputEvent;
class InputEventWithModifiers;
class NativeMenuMacOS;

#ifdef TOOLS_ENABLED
class EmbeddedProcessMacOS;
#endif

#ifdef GLES3_ENABLED
class GLManagerLegacy_MacOS;
#ifdef ANGLE_ENABLED
class GLManagerANGLE_MacOS;
#endif
#endif

class DisplayServerMacOS : public DisplayServerMacOSBase {

public:
	struct KeyEvent {
		DisplayServerEnums::WindowID window_id = DisplayServerEnums::INVALID_WINDOW_ID;
		unsigned int macos_state = false;
		bool pressed = false;
		bool echo = false;
		bool raw = false;
		Key keycode = Key::NONE;
		Key physical_keycode = Key::NONE;
		Key key_label = Key::NONE;
		uint32_t unicode = 0;
		KeyLocation location = KeyLocation::UNSPECIFIED;
	};

	struct WindowData {
		GodotWindowDelegate *window_delegate;
		GodotWindow *window_object;
		GodotContentView *window_view;
		GodotButtonView *window_button_view;

		Vector<Vector2> mpath;

		CGDirectDisplayID display_id = -1;

		Point2i mouse_pos;
		DisplayServerEnums::WindowResizeEdge edge = DisplayServerEnums::WINDOW_EDGE_MAX;

		Size2i min_size;
		Size2i max_size;
		Size2i size;
		Vector2i wb_offset = Vector2i(14, 14);

		NSRect last_frame_rect = NSMakeRect(0, 0, 0, 0);
		NSRect pre_zoom_rect = NSMakeRect(0, 0, 0, 0);

		bool im_active = false;
		Size2i im_position;

		Callable rect_changed_callback;
		Callable event_callback;
		Callable input_event_callback;
		Callable input_text_callback;
		Callable drop_files_callback;

		ObjectID instance_id;
		bool fs_transition = false;
		bool initial_size = true;

		DisplayServerEnums::WindowID transient_parent = DisplayServerEnums::INVALID_WINDOW_ID;
		bool exclusive = false;
		HashSet<DisplayServerEnums::WindowID> transient_children;

		bool layered_window = false;
		bool fullscreen = false;
		bool exclusive_fullscreen = false;
		bool on_top = false;
		bool borderless = false;
		bool resize_disabled = false;
		bool no_min_btn = false;
		bool no_max_btn = false;
		bool no_focus = false;
		bool is_popup = false;
		bool mpass = false;
		bool focused = false;
		bool is_visible = true;
		bool extend_to_title = false;
		bool hide_from_capture = false;

		HDROutput hdr_output;

		Rect2i parent_safe_rect;
	};

	List<DisplayServerEnums::WindowID> popup_list;
	uint64_t time_since_popup = 0;

	GodotProgressView *dock_progress = nullptr;

private:
	id screen_observer = nil;

#if defined(GLES3_ENABLED)
	GLManagerLegacy_MacOS *gl_manager_legacy = nullptr;
#if defined(ANGLE_ENABLED)
	GLManagerANGLE_MacOS *gl_manager_angle = nullptr;
#endif
#endif
	String rendering_driver;

	struct WarpEvent {
		NSTimeInterval timestamp;
		NSPoint delta;
	};
	List<WarpEvent> warp_events;
	NSTimeInterval last_warp = 0;
	bool ignore_warp = false;

	Vector<KeyEvent> key_event_buffer;
	int key_event_pos = 0;

	id menu_delegate = nullptr;
	NativeMenuMacOS *native_menu = nullptr;

	CGEventSourceRef event_source;
	void _mouse_apply_mode(DisplayServerEnums::MouseMode p_prev_mode, DisplayServerEnums::MouseMode p_new_mode) override;

	HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) override;
	const HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) const override;

	bool drop_events = false;
	bool in_dispatch_input_event = false;

	DisplayServerEnums::WindowID window_mouseover_id = DisplayServerEnums::INVALID_WINDOW_ID;
	DisplayServerEnums::WindowID last_focused_window = DisplayServerEnums::INVALID_WINDOW_ID;
	DisplayServerEnums::WindowID window_id_counter = DisplayServerEnums::MAIN_WINDOW_ID;
	float display_max_scale = 1.f;
	mutable Point2i origin;
	mutable bool displays_arrangement_dirty = true;
	bool is_resizing = false;

	NSCursor *cursors[DisplayServerEnums::CURSOR_MAX];
	HashMap<DisplayServerEnums::CursorShape, Vector<Variant>> cursors_cache;

	HashMap<DisplayServerEnums::WindowID, WindowData> windows;

	struct IndicatorData {
		id delegate;
		id item;
	};

	DisplayServerEnums::IndicatorID indicator_id_counter = 0;
	HashMap<DisplayServerEnums::IndicatorID, IndicatorData> indicators;

	Callable help_search_callback;
	Callable help_action_callback;

	struct MenuCall {
		Variant tag;
		Callable callback;
	};
	List<MenuCall> deferred_menu_calls;

	DisplayServerEnums::WindowID _create_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, const Rect2i &p_rect);
	void _update_window_style(WindowData p_wd, DisplayServerEnums::WindowID p_window);

	void _update_displays_arrangement() const;
	Point2i _get_native_screen_position(int p_screen) const;
	static void _displays_arrangement_changed(CGDirectDisplayID display_id, CGDisplayChangeSummaryFlags flags, void *user_info);

	static void _dispatch_input_events(const Ref<InputEvent> &p_event);
	void _dispatch_input_event(const Ref<InputEvent> &p_event);
	void _push_input(const Ref<InputEvent> &p_event);
	void _process_key_events();

	static NSCursor *_cursor_from_selector(SEL p_selector, SEL p_fallback = nil);

	Error _file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, bool p_options_in_cb, DisplayServerEnums::WindowID p_window_id);

#ifdef TOOLS_ENABLED
	struct EmbeddedProcessData {
		EmbeddedProcessMacOS *process = nullptr;
		WindowData *wd = nullptr;
		CALayer *layer_host = nil;
	};
	HashMap<ProcessID, EmbeddedProcessData> embedded_processes;
#endif
	void _window_update_display_id(WindowData *p_wd);

public:
	void menu_callback(id p_sender);

	virtual bool has_window(DisplayServerEnums::WindowID p_window) const override;
	WindowData &get_window(DisplayServerEnums::WindowID p_window);

	NSImage *_convert_to_nsimg(Ref<Image> &p_image) const;
	Point2i _get_screens_origin() const;

	void set_menu_delegate(NSMenu *p_menu);

	void send_event(NSEvent *p_event);
	void send_window_event(const WindowData &p_wd, DisplayServerEnums::WindowEvent p_event) const;
	virtual void send_window_event_by_id(DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	void release_pressed_events();
	void sync_mouse_state();
	void get_key_modifier_state(unsigned int p_macos_state, Ref<InputEventWithModifiers> r_state) const;
	void update_mouse_pos(WindowData &p_wd, NSPoint p_location_in_window);
	void push_to_key_event_buffer(const KeyEvent &p_event);
	void pop_last_key_event();
	void set_last_focused_window(DisplayServerEnums::WindowID p_window);
	bool mouse_process_popups(bool p_close = false);
	void popup_open(DisplayServerEnums::WindowID p_window);
	void popup_close(DisplayServerEnums::WindowID p_window);
	void set_is_resizing(bool p_is_resizing);
	bool get_is_resizing() const;
	void reparent_check(DisplayServerEnums::WindowID p_window);
	DisplayServerEnums::WindowID _get_focused_window_or_popup() const;
	void mouse_enter_window(DisplayServerEnums::WindowID p_window);
	void mouse_exit_window(DisplayServerEnums::WindowID p_window);
	void update_presentation_mode();

	bool is_always_on_top_recursive(DisplayServerEnums::WindowID p_window) const;

	/**
	 * Get the display ID of a window.
	 */
	uint32_t window_get_display_id(DisplayServerEnums::WindowID p_window) const;
	void window_destroy(DisplayServerEnums::WindowID p_window);
	void window_resize(DisplayServerEnums::WindowID p_window, int p_width, int p_height);
	void window_set_custom_window_buttons(WindowData &p_wd, bool p_enabled);
	void set_window_per_pixel_transparency_enabled(bool p_enabled, DisplayServerEnums::WindowID p_window);

	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;
	virtual String get_name() const override;

	virtual void help_set_search_callbacks(const Callable &p_search_callback = Callable(), const Callable &p_action_callback = Callable()) override;
	Callable _help_get_search_callback() const;
	Callable _help_get_action_callback() const;

	virtual Error dialog_show(String p_title, String p_description, Vector<String> p_buttons, const Callable &p_callback) override;
	virtual Error dialog_input_text(String p_title, String p_description, String p_partial, const Callable &p_callback) override;

	virtual Error file_dialog_show(const String &p_title, const String &p_current_directory, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) override;
	virtual Error file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) override;

	bool update_mouse_wrap(WindowData &p_wd, NSPoint &r_delta, NSPoint &r_mpos, NSTimeInterval p_timestamp);
	virtual void warp_mouse(const Point2i &p_position) override;
	virtual Point2i mouse_get_position() const override;
	virtual uint32_t mouse_get_button_state() const override;

	virtual TypedArray<Rect2> get_display_cutouts(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i get_display_safe_area(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

	virtual int get_screen_count() const override;
	virtual int get_keyboard_focus_screen() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_max_scale() const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Color screen_get_pixel(const Point2i &p_position) const override;
	virtual Ref<Image> screen_get_image(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Ref<Image> screen_get_image_rect(const Rect2i &p_rect) const override;
	virtual Vector<int> get_window_list() const override;

	virtual DisplayServerEnums::WindowID create_sub_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i &p_rect = Rect2i(), bool p_exclusive = false, DisplayServerEnums::WindowID p_transient_parent = DisplayServerEnums::INVALID_WINDOW_ID) override;
	virtual void show_window(DisplayServerEnums::WindowID p_id) override;
	virtual void delete_sub_window(DisplayServerEnums::WindowID p_id) override;

	virtual DisplayServerEnums::WindowID window_get_active_popup() const override;
	virtual void window_set_popup_safe_rect(DisplayServerEnums::WindowID p_window, const Rect2i &p_rect) override;
	virtual Rect2i window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window) const override;

	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_title_size(const String &p_title, DisplayServerEnums::WindowID p_window) const override;
	virtual void window_set_mouse_passthrough(const Vector<Vector2> &p_region, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual Point2i window_get_position(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Point2i window_get_position_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) override;
	virtual void window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive) override;

	virtual void window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_max_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_min_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::WindowMode window_get_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool window_is_maximize_allowed(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_request_attention(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_move_to_foreground(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_focused(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual DisplayServerEnums::WindowID get_focused_window() const override;

	virtual bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool can_any_window_draw() const override;

	virtual void window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i &p_position) const override;

	virtual int64_t window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual ObjectID window_get_attached_instance_id(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void gl_window_make_current(DisplayServerEnums::WindowID p_window_id) override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

public:
	void update_screen_parameters();

	virtual bool window_maximize_on_title_dbl_click() const override;
	virtual bool window_minimize_on_title_dbl_click() const override;

	virtual void window_start_drag(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_window_buttons_offset(const Vector2i &p_offset, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Vector3i window_get_safe_title_margins(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	void cursor_update_shape();
	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override;
	virtual void cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW, const Vector2 &p_hotspot = Vector2()) override;

	virtual bool get_swap_cancel_ok() override;

	virtual void enable_for_stealing_focus(ProcessID pid) override;
#ifdef TOOLS_ENABLED
	Error embed_process_update(DisplayServerEnums::WindowID p_window, EmbeddedProcessMacOS *p_process);
	virtual Error request_close_embedded_process(ProcessID p_pid) override;
	virtual Error remove_embedded_process(ProcessID p_pid) override;
#endif

	void _process_events(bool p_pump);
	virtual void process_events() override;
	virtual void force_process_and_drop_events() override;

	virtual void release_rendering_thread() override;
	virtual void swap_buffers() override;

	virtual void set_native_icon(const String &p_filename) override;
	virtual void set_icon(const Ref<Image> &p_icon) override;

	virtual DisplayServerEnums::IndicatorID create_status_indicator(const Ref<Texture2D> &p_icon, const String &p_tooltip, const Callable &p_callback) override;
	virtual void status_indicator_set_icon(DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D> &p_icon) override;
	virtual void status_indicator_set_tooltip(DisplayServerEnums::IndicatorID p_id, const String &p_tooltip) override;
	virtual void status_indicator_set_menu(DisplayServerEnums::IndicatorID p_id, const RID &p_menu_rid) override;
	virtual void status_indicator_set_callback(DisplayServerEnums::IndicatorID p_id, const Callable &p_callback) override;
	virtual Rect2 status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const override;
	virtual void delete_status_indicator(DisplayServerEnums::IndicatorID p_id) override;

	virtual bool is_window_transparency_available() const override;

	void window_get_edr_values(DisplayServerEnums::WindowID p_window, CGFloat *r_max_potential_edr_value, CGFloat *r_max_edr_value) const override;

	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();

	static void register_macos_driver();

	DisplayServerMacOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerMacOS();
};

// --- Folded from platform/macos/gl_manager_macos_angle.h ---
#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#include "core/string/ustring.h"
#include "drivers/gles3/shader_gles3.h"
#include "servers/display/display_server.h"

// Suppress redefinition conflicts
#define FontVariation __FontVariation
#define BitMap __BitMap

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreVideo/CoreVideo.h>

#undef BitMap
#undef FontVariation

class GLManagerANGLE_MacOS : public EGLManager {
private:
	virtual const char *_get_platform_extension_name() const override;
	virtual EGLenum _get_platform_extension_enum() const override;
	virtual EGLenum _get_platform_api_enum() const override;
	virtual Vector<EGLAttrib> _get_platform_display_attributes() const override;
	virtual Vector<EGLint> _get_platform_context_attribs() const override;

public:
	void window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height) {}

	GLManagerANGLE_MacOS() {}
	~GLManagerANGLE_MacOS() {}
};

#endif // MACOS_ENABLED && GLES3_ENABLED

// --- Folded from platform/macos/gl_manager_macos_legacy.h ---
#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#include "drivers/gles3/shader_gles3.h"

#include "core/typedefs.h"
#include "core/types.h"
#include "servers/display/display_server.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreVideo/CoreVideo.h>

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in macOS 10.14.

typedef CGLError (*CGLEnablePtr)(CGLContextObj ctx, CGLContextEnable pname);
typedef CGLError (*CGLSetParameterPtr)(CGLContextObj ctx, CGLContextParameter pname, const GLint *params);
typedef CGLContextObj (*CGLGetCurrentContextPtr)();

class GLManagerLegacy_MacOS {
	struct GLWindow {
		id window_view = nullptr;
		NSOpenGLContext *context = nullptr;
	};

	RBMap<DisplayServerEnums::WindowID, GLWindow> windows;

	NSOpenGLContext *shared_context = nullptr;
	DisplayServerEnums::WindowID current_window = DisplayServerEnums::INVALID_WINDOW_ID;

	Error create_context(GLWindow &win);

	bool framework_loaded = false;
	bool use_vsync = false;
	CGLEnablePtr CGLEnable = nullptr;
	CGLSetParameterPtr CGLSetParameter = nullptr;
	CGLGetCurrentContextPtr CGLGetCurrentContext = nullptr;

public:
	Error window_create(DisplayServerEnums::WindowID p_window_id, id p_view, int p_width, int p_height);
	void window_destroy(DisplayServerEnums::WindowID p_window_id);
	void window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height);

	void release_current();
	void swap_buffers();

	void window_make_current(DisplayServerEnums::WindowID p_window_id);

	void window_set_per_pixel_transparency_enabled(DisplayServerEnums::WindowID p_window_id, bool p_enabled);

	Error initialize();

	void set_use_vsync(bool p_use);
	bool is_using_vsync() const;

	NSOpenGLContext *get_context(DisplayServerEnums::WindowID p_window_id);

	GLManagerLegacy_MacOS();
	~GLManagerLegacy_MacOS();
};

GODOT_CLANG_WARNING_PUSH

#endif // MACOS_ENABLED && GLES3_ENABLED

// --- Folded from platform/macos/godot_core_cursor.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

constexpr int32_t GDCoreCursorWindowMove = 39;

// Expose private interface for CoreCursor.
@interface GodotCoreCursor : NSCursor

@property(readonly, nonatomic) int32_t _coreCursorType;

- (id)initWithType:(int32_t)type;

@end

// --- Folded from platform/macos/godot_open_save_delegate.h ---
#include "core/types.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

@interface GodotOpenSaveDelegate : NSObject <NSOpenSavePanelDelegate> {
	NSSavePanel *dialog;
	NSMutableArray *allowed_types;
	Vector<Vector<String>> preferred_types;

	HashMap<int, String> ctr_ids;
	Dictionary options;
	int cur_index;
	int ctr_id;

	String root;
}

- (void)makeAccessoryView:(NSSavePanel *)p_panel filters:(const Vector<String> &)p_filters options:(const TypedArray<Dictionary> &)p_options;
- (void)setFileTypes:(NSMutableArray *)p_allowed_types pref:(const Vector<Vector<String>> &)p_preftypes;
- (void)popupOptionAction:(id)p_sender;
- (void)popupCheckAction:(id)p_sender;
- (void)popupFileAction:(id)p_sender;
- (int)getIndex;
- (Dictionary)getSelection;
- (int)setDefaultInt:(const String &)p_name value:(int)p_value;
- (int)setDefaultBool:(const String &)p_name value:(bool)p_value;
- (void)setRootPath:(const String &)p_root_path;
- (String)validateFilename:(const String &)p_path;

@end

// --- Folded from platform/macos/godot_progress_view.h ---


#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotProgressView : NSView {
	DisplayServerEnums::ProgressState pr_state;
	float pr_value;
	float pr_offset;
}
- (void)setValue:(float)value;
- (void)setState:(DisplayServerEnums::ProgressState)state;
@end

// --- Folded from platform/macos/godot_status_item.h ---
#include "core/os/os.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotStatusItemDelegate : NSObject {
	Callable cb;
}

- (IBAction)click:(id)sender;

- (void)setCallback:(const Callable &)callback;

@end

// --- Folded from platform/macos/godot_window_delegate.h ---
#include "servers/display/display_server.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

class DisplayServerMacOS;

@interface GodotWindowDelegate : NSObject <NSWindowDelegate>

- (void)setWindowID:(DisplayServerEnums::WindowID)wid;

- (instancetype)initWithDisplayServer:(DisplayServerMacOS *)p_ds;

@end

// --- Folded from platform/macos/godot_window.h ---
#include "servers/display/display_server.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotWindow : NSWindow {
	DisplayServerEnums::WindowID window_id;
	NSTimeInterval anim_duration;
}

- (void)setWindowID:(DisplayServerEnums::WindowID)wid;
- (void)setAnimDuration:(NSTimeInterval)duration;

@end

// --- Folded from platform/macos/godot_button_view.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotButtonView : NSView {
	NSTrackingArea *tracking_area;
	NSPoint offset;
	CGFloat spacing;
	bool mouse_in_group;
	bool rtl;
	NSButton *close_button;
	NSButton *miniaturize_button;
	NSButton *zoom_button;
}

- (void)initButtons:(CGFloat)button_spacing offset:(NSPoint)button_offset rtl:(bool)is_rtl;
- (void)displayButtons;
- (void)setOffset:(NSPoint)button_offset;
- (NSPoint)getOffset;

@end

// --- Folded from platform/macos/godot_content_view.h ---
#include "core/os/os.h"
#include "core/typedefs.h"
#include "servers/display/display_server.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#if defined(GLES3_ENABLED)
#import <AppKit/NSOpenGLView.h>
#define RootView NSOpenGLView
#else
#define RootView NSView
#endif

#import <QuartzCore/CAMetalLayer.h>

@interface GodotContentLayerDelegate : NSObject <CALayerDelegate> {
	DisplayServerEnums::WindowID window_id;
	bool need_redraw;
}

- (void)setWindowID:(DisplayServerEnums::WindowID)wid;
- (void)setNeedRedraw:(bool)redraw;

@end

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in macOS 10.14.

@interface GodotContentView : RootView <NSTextInputClient> {
	DisplayServerEnums::WindowID window_id;
	NSTrackingArea *tracking_area;
	NSMutableAttributedString *marked_text;
	bool ime_input_event_in_progress;
	bool mouse_down_control;
	bool ignore_momentum_scroll;
	bool last_pen_inverted;
	bool ime_suppress_next_keyup;
	id layer_delegate;
	NSMutableSet<NSString *> *registered_observers;
}

- (void)processScrollEvent:(NSEvent *)event button:(MouseButton)button factor:(double)factor;
- (void)processPanEvent:(NSEvent *)event dx:(double)dx dy:(double)dy;
- (void)processMouseEvent:(NSEvent *)event index:(MouseButton)index pressed:(bool)pressed outofstream:(bool)outofstream;
- (void)setWindowID:(DisplayServerEnums::WindowID)wid;
- (void)updateLayerDelegate;
- (void)cancelComposition;

@end

GODOT_CLANG_WARNING_POP

// --- Folded from platform/macos/display_server_macos_embedded.h ---
@class CAContext;
@class CALayer;
class InputEvent;
class NativeMenu;

#ifdef GLES3_ENABLED
class GLManagerEmbedded;
#endif

struct DisplayServerMacOSEmbeddedState {
	/*! Default to a scale of 2.0, which is the most common. */
	float screen_max_scale = 2.0f;
	float screen_dpi = 96.0f;
	/*! Scale for window displaying embedded content */
	float screen_window_scale = 2.0f;
	/*! The display ID of the window which is displaying the embedded process content. */
	uint32_t display_id = -1;

	/*! The current maximum EDR value for the display. */
	double screen_max_edr = 1.0;
	/*! The maximum possible EDR value for the display. */
	double screen_max_potential_edr = 1.0;

	void serialize(PackedByteArray &r_data);
	Error deserialize(const PackedByteArray &p_data);

	_FORCE_INLINE_ bool operator==(const DisplayServerMacOSEmbeddedState &p_other) const {
		return screen_max_scale == p_other.screen_max_scale && screen_dpi == p_other.screen_dpi && screen_window_scale == p_other.screen_window_scale && display_id == p_other.display_id && screen_max_edr == p_other.screen_max_edr && screen_max_potential_edr == p_other.screen_max_potential_edr;
	}
};

/// "Embedded" as in "Embedded in the Godot editor window".
class DisplayServerMacOSEmbedded : public DisplayServerMacOSBase {

	DisplayServerMacOSEmbeddedState state;

	HashMap<DisplayServerEnums::WindowID, ObjectID> window_attached_instance_id;

	HashMap<DisplayServerEnums::WindowID, Callable> window_event_callbacks;
	HashMap<DisplayServerEnums::WindowID, Callable> window_resize_callbacks;
	HashMap<DisplayServerEnums::WindowID, Callable> input_event_callbacks;
	HashMap<DisplayServerEnums::WindowID, Callable> input_text_callbacks;

	DisplayServerEnums::WindowID window_id_counter = DisplayServerEnums::MAIN_WINDOW_ID;

	bool transparent = false;

	CAContext *ca_context = nullptr;
	// Either be a CAMetalLayer or a CALayer depending on the rendering driver.
	CALayer *layer = nullptr;
#ifdef GLES3_ENABLED
	GLManagerEmbedded *gl_manager = nullptr;
#endif

	String rendering_driver;

	HDROutput hdr_output;

	HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) override;
	const HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) const override;

	Point2i ime_last_position;

	// Convert source pixel size to render pixel size, accounting for the
	// difference between display scale and render scale when hiDPI is disabled.
	Size2i _source_to_render_size(const Size2i &p_source_size) const;

	void _mouse_apply_mode(DisplayServerEnums::MouseMode p_prev_mode, DisplayServerEnums::MouseMode p_new_mode) override;

	struct Joy {
		String name;
		uint64_t timestamp = 0;

		Joy() = default;
		Joy(const String &p_name) :
				name(p_name) {}
	};
	HashMap<int, Joy> joysticks;

public:
	static void register_embedded_driver();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();

	void _window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);

	// MARK: - Events

	virtual void process_events() override;

	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	static void _dispatch_input_events(const Ref<InputEvent> &p_event);
	void send_input_event(const Ref<InputEvent> &p_event, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const;
	void send_input_text(const String &p_text, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const;
	virtual void send_window_event_by_id(DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	void _window_callback(const Callable &p_callable, const Variant &p_arg) const;

	// MARK: - Mouse
	virtual void warp_mouse(const Point2i &p_position) override;
	virtual Point2i mouse_get_position() const override;
	virtual uint32_t mouse_get_button_state() const override;

	// MARK: - Window

	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;
	virtual String get_name() const override;

	virtual int get_screen_count() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Vector<DisplayServerEnums::WindowID> get_window_list() const override;

	virtual DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i &p_position) const override;

	virtual void window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual ObjectID window_get_attached_instance_id(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual Point2i window_get_position(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Point2i window_get_position_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) override;

	virtual void window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_max_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_min_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::WindowMode window_get_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool window_is_maximize_allowed(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_request_attention(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_move_to_foreground(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_focused(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual float screen_get_max_scale() const override;

	virtual bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool can_any_window_draw() const override;

	virtual void window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override;
	virtual void cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW, const Vector2 &p_hotspot = Vector2()) override;

	void window_get_edr_values(DisplayServerEnums::WindowID p_window, CGFloat *r_max_potential_edr_value, CGFloat *r_max_edr_value) const override;
	void update_screen_parameters();

	void set_state(const DisplayServerMacOSEmbeddedState &p_state);
	virtual void swap_buffers() override;

	DisplayServerMacOSEmbedded(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, Error &r_error);
	~DisplayServerMacOSEmbedded();
};

// --- Folded from platform/macos/embedded_debugger.h ---
#include "core/types.h"

class DisplayServerMacOSEmbedded;

class EmbeddedDebugger final
{
#ifdef DEBUG_ENABLED
	typedef Error (*ParseMessageFunc)(const Array& p_args);

	struct Data
	{
		DisplayServerMacOSEmbedded* ds = nullptr;
		HashMap<String, ParseMessageFunc> parse_message_handlers;
	};

	static inline Data* data = nullptr;

	static void _init_parse_message_handlers();

	static Error _msg_window_size(const Array& p_args);
	static Error _msg_mouse_set_mode(const Array& p_args);
	static Error _msg_event(const Array& p_args);
	static Error _msg_win_event(const Array& p_args);
	static Error _msg_notification(const Array& p_args);
	static Error _msg_ime_update(const Array& p_args);
	static Error _msg_ds_state(const Array& p_args);
#endif

public:
	EmbeddedDebugger() = delete;

	static void initialize(DisplayServerMacOSEmbedded* p_ds);
	static void deinitialize();

#ifdef DEBUG_ENABLED
	static Error parse_message(
		void* p_user, const String& p_msg, const Array& p_args, bool& r_captured);
#endif
};

// --- Folded from platform/macos/embedded_gl_manager.h ---
#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#include "core/typedefs.h"
#include "core/math/vector2.h"
#include "core/types.h"
#include "servers/display/display_server.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreVideo/CoreVideo.h>

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in macOS 10.14.

typedef CGLContextObj (*CGLGetCurrentContextPtr)();
typedef CGLError (*CGLTexImageIOSurface2DPtr)(CGLContextObj ctx, GLenum target, GLenum internal_format,
		GLsizei width, GLsizei height, GLenum format, GLenum type, IOSurfaceRef ioSurface, GLuint plane);
typedef const char *(*CGLErrorStringPtr)(CGLError);

class GLManagerEmbedded {
	/// @brief The number of framebuffers to create for each window.
	///
	/// Triple-buffering is used to avoid stuttering.
	static constexpr uint32_t BUFFER_COUNT = 3;

	// The display ID for which vsync is used. If this value is -1, vsync is disabled.
	constexpr static uint32_t INVALID_DISPLAY_ID = static_cast<uint32_t>(-1);

	struct FrameBuffer {
		IOSurfaceRef surface = nullptr;
		unsigned int tex = 0;
		unsigned int fbo = 0;
	};

	struct GLWindow {
		uint32_t width = 0;
		uint32_t height = 0;
		CALayer *layer = nullptr;
		NSOpenGLContext *context = nullptr;
		FrameBuffer framebuffers[BUFFER_COUNT];
		uint32_t current_fb = 0;
		bool is_valid = false;

		void destroy_framebuffers();

		~GLWindow() { destroy_framebuffers(); }
	};

	RBMap<DisplayServerEnums::WindowID, GLWindow> windows;
	typedef RBMap<DisplayServerEnums::WindowID, GLWindow>::Element GLWindowElement;

	NSOpenGLContext *shared_context = nullptr;
	DisplayServerEnums::WindowID current_window = DisplayServerEnums::INVALID_WINDOW_ID;

	Error create_context(GLWindow &p_win);

	bool framework_loaded = false;
	CGLGetCurrentContextPtr CGLGetCurrentContext = nullptr;
	CGLTexImageIOSurface2DPtr CGLTexImageIOSurface2D = nullptr;
	CGLErrorStringPtr CGLErrorString = nullptr;

	uint32_t display_id = INVALID_DISPLAY_ID;
	CVDisplayLinkRef display_link = nullptr;
	bool vsync_enabled = false;
	bool display_link_running = false;
	dispatch_semaphore_t display_semaphore = nullptr;

	void create_display_link();
	void release_display_link();

public:
	Error window_create(DisplayServerEnums::WindowID p_window_id, CALayer *p_layer, int p_width, int p_height);
	void window_destroy(DisplayServerEnums::WindowID p_window_id);
	void window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height);
	Size2i window_get_size(DisplayServerEnums::WindowID p_window_id) const;

	void set_display_id(uint32_t p_display_id);
	void set_vsync_enabled(bool p_enabled);
	bool is_vsync_enabled() const { return vsync_enabled; }

	void release_current();
	void swap_buffers();

	void window_make_current(DisplayServerEnums::WindowID p_window_id);

	Error initialize();

	GLManagerEmbedded();
	~GLManagerEmbedded();
};

GODOT_CLANG_WARNING_PUSH

#endif // MACOS_ENABLED && GLES3_ENABLED

// --- Folded from platform/macos/key_mapping_macos.h ---
#include "core/os/os.h"

class KeyMappingMacOS {
	KeyMappingMacOS() {}

	static bool is_numpad_key(unsigned int p_key);

public:
	static void initialize();

	// Mappings input.
	static Key translate_key(unsigned int p_key);
	static unsigned int unmap_key(Key p_key);
	static Key remap_key(unsigned int p_key, unsigned int p_state, bool p_unicode);
	static KeyLocation translate_location(unsigned int p_key);

	// Mapping for menu shortcuts.
	static String keycode_get_native_string(Key p_keycode);
	static unsigned int keycode_get_native_mask(Key p_keycode);
};

// --- Folded from platform/macos/native_menu_macos.h ---
#include "core/types.h"
#include "servers/display/display_server.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

class NativeMenuMacOS : public NativeMenuDriver {

	struct MenuData {
		NSMenu *menu = nullptr;

		Callable open_cb;
		Callable close_cb;
		bool is_open = false;
		bool is_system = false;
	};

	mutable RID_PtrOwner<MenuData> menus;
	HashMap<NSMenu *, RID> menu_lookup;

	NSMenu *main_menu_ns = nullptr;
	NSMenu *application_menu_ns = nullptr;
	NSMenu *window_menu_ns = nullptr;
	NSMenu *help_menu_ns = nullptr;
	NSMenu *dock_menu_ns = nullptr;

	RID main_menu;
	RID application_menu;
	RID window_menu;
	RID help_menu;
	RID dock_menu;

	void _update_align(MenuData *p_md);

	int _get_system_menu_start(const NSMenu *p_menu) const;
	int _get_system_menu_count(const NSMenu *p_menu) const;
	bool _is_menu_opened(NSMenu *p_menu) const;
	NSMenuItem *_menu_add_item(NSMenu *p_menu, const String &p_label, Key p_accel, int p_index, int *r_out);

public:
	void _register_system_menus(NSMenu *p_main_menu, NSMenu *p_application_menu, NSMenu *p_window_menu, NSMenu *p_help_menu, NSMenu *p_dock_menu);
	NSMenu *_get_dock_menu();

	void _menu_need_update(NSMenu *p_menu);
	void _menu_open(NSMenu *p_menu);
	void _menu_close(NSMenu *p_menu);
	void _menu_close_cb(const RID &p_rid);

	virtual bool has_feature(Feature p_feature) const override;

	virtual bool has_system_menu(SystemMenus p_menu_id) const override;
	virtual RID get_system_menu(SystemMenus p_menu_id) const override;

	virtual RID create_menu() override;
	virtual bool has_menu(const RID &p_rid) const override;
	virtual void free_menu(const RID &p_rid) override;

	NSMenu *get_native_menu_handle(const RID &p_rid);

	virtual String get_system_menu_text(SystemMenus p_menu_id) const override;
	virtual void set_system_menu_text(SystemMenus p_menu_id, const String &p_name) override;

	virtual Size2 get_size(const RID &p_rid) const override;
	virtual void popup(const RID &p_rid, const Vector2i &p_position) override;

	virtual void set_interface_direction(const RID &p_rid, bool p_is_rtl) override;
	virtual void set_popup_open_callback(const RID &p_rid, const Callable &p_callback) override;
	virtual Callable get_popup_open_callback(const RID &p_rid) const override;
	virtual void set_popup_close_callback(const RID &p_rid, const Callable &p_callback) override;
	virtual Callable get_popup_close_callback(const RID &p_rid) const override;
	virtual void set_minimum_width(const RID &p_rid, float p_width) override;
	virtual float get_minimum_width(const RID &p_rid) const override;

	virtual bool is_opened(const RID &p_rid) const override;

	virtual int add_submenu_item(const RID &p_rid, const String &p_label, const RID &p_submenu_rid, const Variant &p_tag = Variant(), int p_index = -1) override;
	virtual int add_item(const RID &p_rid, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_check_item(const RID &p_rid, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_icon_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_icon_check_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_radio_check_item(const RID &p_rid, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_icon_radio_check_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_multistate_item(const RID &p_rid, const String &p_label, int p_max_states, int p_default_state, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_separator(const RID &p_rid, int p_index = -1) override;

	virtual int find_item_index_with_text(const RID &p_rid, const String &p_text) const override;
	virtual int find_item_index_with_tag(const RID &p_rid, const Variant &p_tag) const override;

	virtual bool is_item_checked(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_indeterminate(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_checkable(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_radio_checkable(const RID &p_rid, int p_idx) const override;
	virtual Callable get_item_callback(const RID &p_rid, int p_idx) const override;
	virtual Callable get_item_key_callback(const RID &p_rid, int p_idx) const override;
	virtual Variant get_item_tag(const RID &p_rid, int p_idx) const override;
	virtual String get_item_text(const RID &p_rid, int p_idx) const override;
	virtual RID get_item_submenu(const RID &p_rid, int p_idx) const override;
	virtual Key get_item_accelerator(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_disabled(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_hidden(const RID &p_rid, int p_idx) const override;
	virtual String get_item_tooltip(const RID &p_rid, int p_idx) const override;
	virtual int get_item_state(const RID &p_rid, int p_idx) const override;
	virtual int get_item_max_states(const RID &p_rid, int p_idx) const override;
	virtual Ref<Texture2D> get_item_icon(const RID &p_rid, int p_idx) const override;
	virtual int get_item_indentation_level(const RID &p_rid, int p_idx) const override;

	virtual void set_item_checked(const RID &p_rid, int p_idx, bool p_checked) override;
	virtual void set_item_indeterminate(const RID &p_rid, int p_idx, bool p_cindeterminate) override;
	virtual void set_item_checkable(const RID &p_rid, int p_idx, bool p_checkable) override;
	virtual void set_item_radio_checkable(const RID &p_rid, int p_idx, bool p_checkable) override;
	virtual void set_item_callback(const RID &p_rid, int p_idx, const Callable &p_callback) override;
	virtual void set_item_key_callback(const RID &p_rid, int p_idx, const Callable &p_key_callback) override;
	virtual void set_item_hover_callbacks(const RID &p_rid, int p_idx, const Callable &p_callback) override;
	virtual void set_item_tag(const RID &p_rid, int p_idx, const Variant &p_tag) override;
	virtual void set_item_text(const RID &p_rid, int p_idx, const String &p_text) override;
	virtual void set_item_submenu(const RID &p_rid, int p_idx, const RID &p_submenu_rid) override;
	virtual void set_item_accelerator(const RID &p_rid, int p_idx, Key p_keycode) override;
	virtual void set_item_disabled(const RID &p_rid, int p_idx, bool p_disabled) override;
	virtual void set_item_hidden(const RID &p_rid, int p_idx, bool p_hidden) override;
	virtual void set_item_tooltip(const RID &p_rid, int p_idx, const String &p_tooltip) override;
	virtual void set_item_state(const RID &p_rid, int p_idx, int p_state) override;
	virtual void set_item_max_states(const RID &p_rid, int p_idx, int p_max_states) override;
	virtual void set_item_icon(const RID &p_rid, int p_idx, const Ref<Texture2D> &p_icon) override;
	virtual void set_item_indentation_level(const RID &p_rid, int p_idx, int p_level) override;
	virtual int set_item_index(const RID &p_rid, int p_idx, int p_target_idx) override;

	virtual int get_item_count(const RID &p_rid) const override;
	virtual bool is_system_menu(const RID &p_rid) const override;

	virtual void remove_item(const RID &p_rid, int p_idx) override;
	virtual void clear(const RID &p_rid) override;

	NativeMenuMacOS();
	~NativeMenuMacOS();
};

// --- Folded from platform/macos/godot_menu_delegate.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotMenuDelegate : NSObject <NSMenuDelegate> {
}

- (void)doNothing:(id)sender;

@end

// --- Folded from platform/macos/godot_menu_item.h ---
#include "core/io/resource_loader.h"
#include "core/os/os.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#define MENU_TAG_START 0x00004447
#define MENU_TAG_END 0xFFFF4447

enum GlobalMenuCheckType {
	CHECKABLE_TYPE_NONE,
	CHECKABLE_TYPE_CHECK_BOX,
	CHECKABLE_TYPE_RADIO_BUTTON,
};

@interface GodotMenuItem : NSObject {
@public
	Callable callback;
	Callable key_callback;
	Callable hover_callback;
	Variant meta;
	Key accel;
	GlobalMenuCheckType checkable_type;
	bool checked;
	bool enabled;
	bool indeterminate;
	int max_states;
	int state;
	int indent;
	Ref<Image> img;
}

@end
#endif // defined(MACOS_ENABLED)
// --- End platform/macos/display_server_macos.h ---
#endif // defined(MACOS_ENABLED)
// --- End platform/macos/os_macos.h ---

// --- Folded from drivers/apple_embedded/os_apple_embedded.h ---
#if defined(APPLE_EMBEDDED_ENABLED)
#ifdef APPLE_EMBEDDED_ENABLED

#include "core/os/os.h"
// --- Folded from drivers/apple_embedded/apple_embedded.h ---
#import <CoreHaptics/CoreHaptics.h>

class AppleEmbedded
{

private:
	CHHapticEngine* haptic_engine API_AVAILABLE(ios(13)) = nullptr;

	CHHapticEngine* get_haptic_engine_instance() API_AVAILABLE(ios(13));
	void start_haptic_engine();
	void stop_haptic_engine();

public:
	mem_unique_ptr<Object> obj;
	static void alert(const char* p_alert, const char* p_title);

	bool supports_haptic_engine();
	void vibrate_haptic_engine(float p_duration_seconds, float p_amplitude);

	String get_model() const;
	String get_rate_url(int p_app_id) const;

	AppleEmbedded();
};

// --- Folded from drivers/apple_embedded/display_layer_apple_embedded.h ---
#import <QuartzCore/CAMetalLayer.h>

@protocol GDTDisplayLayer <NSObject>

- (void)startRenderDisplayLayer;
- (void)stopRenderDisplayLayer;
- (void)initializeDisplayLayer;
- (void)layoutDisplayLayer;

@end
// --- End drivers/apple_embedded/apple_embedded.h ---


#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

#if defined(RD_ENABLED)
#include "servers/rendering/renderer.h"

#if defined(VULKAN_ENABLED)

#endif
#endif

class JoypadSDL;

class OS_AppleEmbedded final
{
public:
	OS_AppleEmbedded() = delete;
	~OS_AppleEmbedded() = delete;
	OS_AppleEmbedded(const OS_AppleEmbedded&) = delete;
	OS_AppleEmbedded& operator=(const OS_AppleEmbedded&) = delete;

	struct Data
	{
		AudioDriverCoreAudio audio_driver;
		AppleEmbedded* apple_embedded = nullptr;
#ifdef SDL_ENABLED
		JoypadSDL* joypad_sdl = nullptr;
#endif
		bool is_focused = false;
		String remote_fs_dir;
	};

	static inline Data* data = nullptr;

private:
	static HashMap<String, void*> dynamic_symbol_lookup_table;
	friend void register_dynamic_symbol(char* name, void* address);

	static CGFloat _weight_to_ct(int p_weight);
	static CGFloat _stretch_to_ct(int p_stretch);
	static String _get_default_fontname(const String& p_font_name);

	static _FORCE_INLINE_ String get_framework_executable(const String& p_path);

	static void deinitialize_modules();

public:
	static void setup();
	static void initialize_core();
	static void initialize();
	static void initialize_joypads();
	static void delete_main_loop();
	static void finalize();
	static void finalize_core();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static void initialize_modules();

	static bool iterate();

	static void start();

	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	static Vector<String> get_system_fonts();
	static Vector<String> get_system_font_path_for_text(const String& p_font_name,
		const String& p_text, const String& p_locale = String(), const String& p_script = String(),
		int p_weight = 400, int p_stretch = 100, bool p_italic = false);
	static String get_system_font_path(
		const String& p_font_name, int p_weight = 400, int p_stretch = 100, bool p_italic = false);

	static Error open_dynamic_library(
		const String& p_path, void*& p_library_handle, GDExtensionData* p_data = nullptr);
	static Error close_dynamic_library(void* p_library_handle);
	static Error get_dynamic_library_symbol_handle(void* p_library_handle, const String& p_name,
		void*& p_symbol_handle, bool p_optional = false);

	static String get_distribution_name();
	static String get_version();
	static String get_model_name();

	static Error shell_open(const String& p_uri);

	static String get_user_data_dir(const String& p_user_dir);

	static String get_cache_path();
	static String get_temp_path();
	static String get_resource_dir();
	static String get_bundle_resource_dir();

	static String get_locale();
	static Vector<String> get_preferred_locales();

	static String get_unique_id();
	static String get_processor_name();

	static void vibrate_handheld(int p_duration_ms = 500, float p_amplitude = -1.0);

	static bool _check_internal_feature_support(const String& p_feature);

	static Error setup_remote_filesystem(const String& p_server_host, int p_port,
		const String& p_password, String& r_project_path);

	static void on_focus_out();
	static void on_focus_in();

	static void on_enter_background();
	static void on_exit_background();

	static Rect2 calculate_boot_screen_rect(
		const Size2& p_window_size, const Size2& p_imgrect_size);

	static bool request_permission(const String& p_name);
	static Vector<String> get_granted_permissions();

	static String get_platform_string(OS::PlatformString p_platform_string)
	{
		switch (p_platform_string) {
		case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_OPEN:
			return ETR("Open in Files");
		case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_SHOW:
			return ETR("Show in Files");
		default:
			return String();
		}
	}
};


// --- Folded from drivers/apple_embedded/godot_view_apple_embedded.h ---
#import <UIKit/UIKit.h>

class String;

@class GDTView;
@protocol GDTDisplayLayer;
@protocol GDTViewRendererProtocol;

@protocol GDTViewDelegate

- (BOOL)godotViewFinishedSetup:(GDTView *)view;

@end

@interface GDTView : UIView

@property(assign, nonatomic) id<GDTViewRendererProtocol> renderer;
@property(assign, nonatomic) id<GDTViewDelegate> delegate;

@property(assign, readonly, nonatomic) BOOL isActive;

@property(assign, nonatomic) BOOL useCADisplayLink;
@property(strong, readonly, nonatomic) CALayer<GDTDisplayLayer> *renderingLayer;
@property(assign, readonly, nonatomic) BOOL canRender;

@property(assign, nonatomic) float preferredFrameRate;

// Can be extended by subclasses
- (void)godot_commonInit;

// Implemented in subclasses
- (CALayer<GDTDisplayLayer> *)initializeRenderingForDriver:(NSString *)driverName;

- (void)startRendering;
- (void)stopRendering;

@end

// Implemented in subclasses
extern GDTView *GDTViewCreate();

// --- Folded from drivers/apple_embedded/godot_view_renderer.h ---
#import <UIKit/UIKit.h>

@protocol GDTViewRendererProtocol <NSObject>

@property(assign, readonly, nonatomic) BOOL hasFinishedSetup;

- (BOOL)setupView:(UIView *)view;
- (void)renderOnView:(UIView *)view;

@end

@interface GDTViewRenderer : NSObject <GDTViewRendererProtocol>

@end
// --- End drivers/apple_embedded/godot_view_apple_embedded.h ---


// --- Folded from drivers/apple_embedded/godot_view_controller.h ---
#import <UIKit/UIKit.h>

@class GDTView;
@class GDTKeyboardInputView;

@interface GDTViewController : UIViewController

@property(nonatomic, readonly, strong) GDTView *godotView;
@property(nonatomic, readonly, strong) GDTKeyboardInputView *keyboardView;

#ifdef IOS_ENABLED
- (void)propagateUIPreferencesToRootViewController;
#endif

@end
// --- End drivers/apple_embedded/godot_view_controller.h ---


// --- Folded from drivers/apple_embedded/app_delegate_service.h ---
#import <UIKit/UIKit.h>

@class GDTViewController;

@interface GDTAppDelegateService : NSObject <UIApplicationDelegate, UIWindowSceneDelegate>

@property(strong, class, nonatomic) GDTViewController *viewController;

@end
// --- End drivers/apple_embedded/app_delegate_service.h ---


// --- Folded from drivers/apple_embedded/godot_app_delegate.h ---
#import <UIKit/UIKit.h>

typedef NSObject<UIApplicationDelegate, UIWindowSceneDelegate> GDTAppDelegateServiceProtocol;

@interface GDTApplicationDelegate : NSObject <UIApplicationDelegate, UIWindowSceneDelegate>

@property(class, readonly, strong) NSArray<GDTAppDelegateServiceProtocol *> *services;

+ (void)addService:(GDTAppDelegateServiceProtocol *)service;

@end

// --- Folded from drivers/apple_embedded/main_utilities.h ---
void change_to_launch_dir(char **p_args);

int process_args(int p_argc, char **p_args, char **r_args);
// --- End drivers/apple_embedded/godot_app_delegate.h ---


// --- Folded from drivers/apple_embedded/display_server_apple_embedded.h ---
#include "core/os/os.h"
#include "servers/display/display_server.h"

#if defined(RD_ENABLED)
#include "servers/rendering/renderer.h"

#if defined(VULKAN_ENABLED)
// --- Inlined/Folded from drivers/apple_embedded/rendering_context_driver_vulkan_apple_embedded.h ---
#ifdef VULKAN_ENABLED

#include "drivers/vulkan/rendering_context_driver_vulkan.h"

#import <QuartzCore/CAMetalLayer.h>

class RenderingContextDriverVulkanAppleEmbedded : public RenderingContextDriverVulkan {
private:
	virtual const char *_get_platform_surface_extension() const override final;

protected:
	SurfaceID surface_create(const void *p_platform_data) override final;

public:
	struct WindowPlatformData {
		CAMetalLayer *const *layer_ptr;
	};

	RenderingContextDriverVulkanAppleEmbedded();
	~RenderingContextDriverVulkanAppleEmbedded();
};

#endif // VULKAN_ENABLED

#include <drivers/vulkan/godot_vulkan.h>
#endif // VULKAN_ENABLED

#if defined(METAL_ENABLED)
#import "drivers/metal/rendering_device_driver_metal3.h"
#endif // METAL_ENABLED
#endif // RD_ENABLED

#if defined(GLES3_ENABLED)
#include "drivers/gles3/shader_gles3.h"
#endif // GLES3_ENABLED

#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>

class InputEvent;
/// "Embedded" as in "Embedded Device".
class DisplayServerAppleEmbedded : public DisplayServerDriver {

	_THREAD_SAFE_CLASS_

#if defined(RD_ENABLED)
	RenderingContextDriver *rendering_context = nullptr;
	RenderingDevice *rendering_device = nullptr;
#endif

	id tts = nullptr;

	DisplayServerEnums::ScreenOrientation screen_orientation;

	ObjectID window_attached_instance_id;

	Callable window_event_callback;
	Callable window_resize_callback;
	Callable input_event_callback;
	Callable input_text_callback;

	Callable system_theme_changed;

	int virtual_keyboard_height = 0;

	void perform_event(const Ref<InputEvent> &p_event);

	void initialize_tts() const;

	bool edr_requested = false;
	void _update_hdr_output(bool edr_headroom_changed);
	float _calculate_current_reference_luminance() const;

protected:
	virtual bool _screen_hdr_is_supported() const { return false; }
	virtual float _screen_potential_edr_headroom() const { return 1.0f; }
	virtual float _screen_current_edr_headroom() const { return 1.0f; }
	float hardware_reference_luminance_nits = 100.0f;

	DisplayServerAppleEmbedded(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerAppleEmbedded();

public:
	String rendering_driver;

	static Vector<String> get_rendering_drivers_func();

	// MARK: - Events

	virtual void process_events() override;

	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	static void _dispatch_input_events(const Ref<InputEvent> &p_event);
	void send_input_event(const Ref<InputEvent> &p_event) const;
	void send_input_text(const String &p_text) const;
	void send_window_event(DisplayServerEnums::WindowEvent p_event) const;
	void _window_callback(const Callable &p_callable, const Variant &p_arg) const;

	void emit_system_theme_changed();

	// MARK: - Input

	// MARK: Touches and Apple Pencil

	void touch_press(int p_idx, int p_x, int p_y, bool p_pressed, bool p_double_click);
	void touch_drag(int p_idx, int p_prev_x, int p_prev_y, int p_x, int p_y, float p_pressure, Vector2 p_tilt);
	void touches_canceled(int p_idx);

	// MARK: Keyboard

	void key(Key p_key, char32_t p_char, Key p_unshifted, Key p_physical, NSInteger p_modifier, bool p_pressed, KeyLocation p_location);
	bool is_keyboard_active() const;

	// MARK: Motion

	void update_gravity(const Vector3 &p_gravity);
	void update_accelerometer(const Vector3 &p_accelerometer);
	void update_magnetometer(const Vector3 &p_magnetometer);
	void update_gyroscope(const Vector3 &p_gyroscope);

	// MARK: -

	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;

	virtual bool tts_is_speaking() const override;
	virtual bool tts_is_paused() const override;
	virtual TypedArray<Dictionary> tts_get_voices() const override;

	virtual void tts_speak(const String &p_text, const String &p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false) override;
	virtual void tts_pause() override;
	virtual void tts_resume() override;
	virtual void tts_stop() override;

	virtual bool is_dark_mode_supported() const override;
	virtual bool is_dark_mode() const override;
	virtual void set_system_theme_change_callback(const Callable &p_callable) override;

	virtual Rect2i get_display_safe_area(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

	virtual int get_screen_count() const override;
	virtual int get_primary_screen() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

	virtual Vector<DisplayServerEnums::WindowID> get_window_list() const override;

	virtual DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i &p_position) const override;

	virtual int64_t window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual ObjectID window_get_attached_instance_id(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual Point2i window_get_position(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Point2i window_get_position_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) override;

	virtual void window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_max_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_min_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::WindowMode window_get_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool window_is_maximize_allowed(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_request_attention(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_move_to_foreground(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_focused(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual float screen_get_max_scale() const override;

	virtual void screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen) override;
	virtual DisplayServerEnums::ScreenOrientation screen_get_orientation(int p_screen) const override;

	virtual bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool can_any_window_draw() const override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

	virtual bool is_touchscreen_available() const override;

	virtual void virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect, DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start, int p_cursor_end) override;
	virtual void virtual_keyboard_hide() override;

	void virtual_keyboard_set_height(int height);
	virtual int virtual_keyboard_get_height() const override;
	virtual bool has_hardware_keyboard() const override;

	virtual void clipboard_set(const String &p_text) override;
	virtual String clipboard_get() const override;

	virtual void screen_set_keep_on(bool p_enable) override;
	virtual bool screen_is_kept_on() const override;

	// MARK: - HDR / EDR

	void current_edr_headroom_changed();
	virtual bool window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_request_hdr_output(const bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual bool window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual float window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual float window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual float window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual float window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual float window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	void resize_window(CGSize size);
	virtual void swap_buffers() override {}

	virtual void set_native_icon(const String &p_filename) override;
	virtual void set_icon(const Ref<Image> &p_icon) override;
};

// --- Folded from drivers/apple_embedded/godot_keyboard_input_view.h ---
#import <UIKit/UIKit.h>

@interface GDTKeyboardInputView : UITextView

- (BOOL)becomeFirstResponderWithString:(NSString *)existingString cursorStart:(NSInteger)start cursorEnd:(NSInteger)end;

@end

// --- Folded from drivers/apple_embedded/key_mapping_apple_embedded.h ---
#include "core/os/os.h"

#import <UIKit/UIKit.h>

class KeyMappingAppleEmbedded {
	KeyMappingAppleEmbedded() {}

public:
	static void initialize();
	static Key remap_key(CFIndex p_keycode);
	static KeyLocation key_location(CFIndex p_keycode);
};

// --- Folded from drivers/apple_embedded/tts_apple_embedded.h ---
#include "core/string/ustring.h"
#include "core/types.h"
#include "core/templates/list.h"

#if __has_include(<AVFAudio/AVSpeechSynthesis.h>)
#import <AVFAudio/AVSpeechSynthesis.h>
#else
#import <AVFoundation/AVFoundation.h>
#endif

struct TTSUtterance;

@interface GDTTTS : NSObject <AVSpeechSynthesizerDelegate> {
	bool speaking;
	HashMap<id, int64_t> ids;

	AVSpeechSynthesizer *av_synth;
	List<TTSUtterance> queue;
}

- (void)pauseSpeaking;
- (void)resumeSpeaking;
- (void)stopSpeaking;
- (bool)isSpeaking;
- (bool)isPaused;
- (void)speak:(const String &)text voice:(const String &)voice volume:(int)volume pitch:(float)pitch rate:(float)rate utterance_id:(int64_t)utterance_id interrupt:(bool)interrupt;
- (Array)getVoices;
@end
// --- End drivers/apple_embedded/display_server_apple_embedded.h ---

#endif // APPLE_EMBEDDED_ENABLED
#endif // defined(APPLE_EMBEDDED_ENABLED)
// --- End drivers/apple_embedded/os_apple_embedded.h ---
