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
#include "core/io/ip.h"

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
#include "core/io/ip.h"

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
#include "core/io/ip.h"
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
#include "servers/rendering/renderer.h"
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

#include "servers/rendering/renderer.h"

#include "core/typedefs.h"
#include "core/io/ip.h"
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
#include "core/io/ip.h"

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
#include "core/io/ip.h"

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
#include "core/io/ip.h"
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
#include "core/io/ip.h"
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
#include "core/io/net_socket.h"
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
#import "servers/rendering/renderer.h"
#endif // METAL_ENABLED
#endif // RD_ENABLED

#if defined(GLES3_ENABLED)
#include "servers/rendering/renderer.h"
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
#include "core/io/ip.h"
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

// --- Folded from platform/linuxbsd/os_linuxbsd.h ---
#if defined(LINUXBSD_ENABLED)
#include "core/os/os.h"
// --- Folded from platform/linuxbsd/crash_handler_linuxbsd.h ---
class CrashHandler {
	bool disabled;

public:
	void initialize();

	void disable();
	bool is_disabled() const { return disabled; }

	CrashHandler();
	~CrashHandler();
};

// --- End platform/linuxbsd/crash_handler_linuxbsd.h ---
// --- Folded from drivers/alsa/audio_driver_alsa.h ---
#ifdef ALSA_ENABLED

#include "core/os/mutex.h"
#include "core/os/memory.h"
#include "servers/display/display_server.h"

typedef struct _snd_pcm snd_pcm_t;
typedef unsigned long snd_pcm_uframes_t;

class AudioDriverALSA : public AudioDriver {
	Thread thread;
	Mutex mutex;

	snd_pcm_t *pcm_handle = nullptr;

	String output_device_name = "Default";
	String new_output_device = "Default";

	Vector<int32_t> samples_in;
	Vector<int16_t> samples_out;

	Error init_output_device();
	void finish_output_device();

	static void thread_func(void *p_udata);

	unsigned int mix_rate = 0;
	SpeakerMode speaker_mode;

	snd_pcm_uframes_t buffer_frames;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t period_size;
	int channels = 0;

	SafeFlag active;
	SafeFlag exit_thread;

public:
	virtual const char *get_name() const override {
		return "ALSA";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

	virtual PackedStringArray get_output_device_list() override;
	virtual String get_output_device() override;
	virtual void set_output_device(const String &p_name) override;

	AudioDriverALSA() {}
	~AudioDriverALSA() {}
};

#endif // ALSA_ENABLED

// --- End drivers/alsa/audio_driver_alsa.h ---
// --- Folded from drivers/alsamidi/midi_driver_alsamidi.h ---
#ifdef ALSAMIDI_ENABLED

#include "core/os/os.h"
#include "core/os/mutex.h"
#include "core/os/memory.h"
#include "core/string/ustring.h"

typedef struct _snd_rawmidi snd_rawmidi_t;

class MIDIDriverALSAMidi final
{
	struct InputConnection
	{
		InputConnection() = default;
		InputConnection(int p_device_index, snd_rawmidi_t* p_rawmidi);

		MIDIDriver::Parser parser;
		snd_rawmidi_t* rawmidi_ptr = nullptr;

		// Read in and parse available data, forwarding complete events to Input.
		void read();
	};

public:
	struct Data
	{
		Thread thread;
		Mutex mutex;
		Vector<InputConnection> connected_inputs;
		SafeFlag exit_thread;
	};

	static inline Data* data = nullptr;

private:
	static void thread_func(void* p_udata);

	static void lock();
	static void unlock();

public:
	static void initialize();
	static void finalize();
	static Error open();
	static void close();

	MIDIDriverALSAMidi() = delete;
	MIDIDriverALSAMidi(const MIDIDriverALSAMidi&) = delete;
	MIDIDriverALSAMidi& operator=(const MIDIDriverALSAMidi&) = delete;
	~MIDIDriverALSAMidi() = delete;
};

#endif // ALSAMIDI_ENABLED

// --- End drivers/alsamidi/midi_driver_alsamidi.h ---
// --- Folded from drivers/pulseaudio/audio_driver_pulseaudio.h ---
#ifdef PULSEAUDIO_ENABLED

#include "core/os/mutex.h"
#include "core/os/memory.h"
#include "servers/display/display_server.h"

#ifdef SOWRAP_ENABLED
// --- Inlined/Folded from drivers/pulseaudio/pulse-so_wrap.h ---
#ifndef DYLIBLOAD_WRAPPER_PULSE
#define DYLIBLOAD_WRAPPER_PULSE
// This file is generated. Do not edit!
// see https://github.com/hpvb/dynload-wrapper for details
// generated by generate-wrapper.py 0.3 on 2023-01-12 10:26:29
// flags: generate-wrapper.py --include ./thirdparty/linuxbsd_headers/pulse/pulseaudio.h --sys-include "thirdparty/linuxbsd_headers/pulse/pulseaudio.h" --soname libpulse.so.0 --omit-prefix _pa_ --init-name pulse --output-header ./drivers/pulseaudio/pulse-so_wrap.h --output-implementation ./drivers/pulseaudio/pulse-so_wrap.c
//
#include <stdint.h>

#define pa_get_library_version pa_get_library_version_dylibloader_orig_pulse
#define pa_bytes_per_second pa_bytes_per_second_dylibloader_orig_pulse
#define pa_frame_size pa_frame_size_dylibloader_orig_pulse
#define pa_sample_size pa_sample_size_dylibloader_orig_pulse
#define pa_sample_size_of_format pa_sample_size_of_format_dylibloader_orig_pulse
#define pa_bytes_to_usec pa_bytes_to_usec_dylibloader_orig_pulse
#define pa_usec_to_bytes pa_usec_to_bytes_dylibloader_orig_pulse
#define pa_sample_spec_init pa_sample_spec_init_dylibloader_orig_pulse
#define pa_sample_format_valid pa_sample_format_valid_dylibloader_orig_pulse
#define pa_sample_rate_valid pa_sample_rate_valid_dylibloader_orig_pulse
#define pa_channels_valid pa_channels_valid_dylibloader_orig_pulse
#define pa_sample_spec_valid pa_sample_spec_valid_dylibloader_orig_pulse
#define pa_sample_spec_equal pa_sample_spec_equal_dylibloader_orig_pulse
#define pa_sample_format_to_string pa_sample_format_to_string_dylibloader_orig_pulse
#define pa_parse_sample_format pa_parse_sample_format_dylibloader_orig_pulse
#define pa_sample_spec_snprint pa_sample_spec_snprint_dylibloader_orig_pulse
#define pa_bytes_snprint pa_bytes_snprint_dylibloader_orig_pulse
#define pa_sample_format_is_le pa_sample_format_is_le_dylibloader_orig_pulse
#define pa_sample_format_is_be pa_sample_format_is_be_dylibloader_orig_pulse
#define pa_direction_valid pa_direction_valid_dylibloader_orig_pulse
#define pa_direction_to_string pa_direction_to_string_dylibloader_orig_pulse
#define pa_mainloop_api_once pa_mainloop_api_once_dylibloader_orig_pulse
#define pa_proplist_new pa_proplist_new_dylibloader_orig_pulse
#define pa_proplist_free pa_proplist_free_dylibloader_orig_pulse
#define pa_proplist_key_valid pa_proplist_key_valid_dylibloader_orig_pulse
#define pa_proplist_sets pa_proplist_sets_dylibloader_orig_pulse
#define pa_proplist_setp pa_proplist_setp_dylibloader_orig_pulse
#define pa_proplist_setf pa_proplist_setf_dylibloader_orig_pulse
#define pa_proplist_set pa_proplist_set_dylibloader_orig_pulse
#define pa_proplist_gets pa_proplist_gets_dylibloader_orig_pulse
#define pa_proplist_get pa_proplist_get_dylibloader_orig_pulse
#define pa_proplist_update pa_proplist_update_dylibloader_orig_pulse
#define pa_proplist_unset pa_proplist_unset_dylibloader_orig_pulse
#define pa_proplist_unset_many pa_proplist_unset_many_dylibloader_orig_pulse
#define pa_proplist_iterate pa_proplist_iterate_dylibloader_orig_pulse
#define pa_proplist_to_string pa_proplist_to_string_dylibloader_orig_pulse
#define pa_proplist_to_string_sep pa_proplist_to_string_sep_dylibloader_orig_pulse
#define pa_proplist_from_string pa_proplist_from_string_dylibloader_orig_pulse
#define pa_proplist_contains pa_proplist_contains_dylibloader_orig_pulse
#define pa_proplist_clear pa_proplist_clear_dylibloader_orig_pulse
#define pa_proplist_copy pa_proplist_copy_dylibloader_orig_pulse
#define pa_proplist_size pa_proplist_size_dylibloader_orig_pulse
#define pa_proplist_isempty pa_proplist_isempty_dylibloader_orig_pulse
#define pa_proplist_equal pa_proplist_equal_dylibloader_orig_pulse
#define pa_channel_map_init pa_channel_map_init_dylibloader_orig_pulse
#define pa_channel_map_init_mono pa_channel_map_init_mono_dylibloader_orig_pulse
#define pa_channel_map_init_stereo pa_channel_map_init_stereo_dylibloader_orig_pulse
#define pa_channel_map_init_auto pa_channel_map_init_auto_dylibloader_orig_pulse
#define pa_channel_map_init_extend pa_channel_map_init_extend_dylibloader_orig_pulse
#define pa_channel_position_to_string pa_channel_position_to_string_dylibloader_orig_pulse
#define pa_channel_position_from_string pa_channel_position_from_string_dylibloader_orig_pulse
#define pa_channel_position_to_pretty_string pa_channel_position_to_pretty_string_dylibloader_orig_pulse
#define pa_channel_map_snprint pa_channel_map_snprint_dylibloader_orig_pulse
#define pa_channel_map_parse pa_channel_map_parse_dylibloader_orig_pulse
#define pa_channel_map_equal pa_channel_map_equal_dylibloader_orig_pulse
#define pa_channel_map_valid pa_channel_map_valid_dylibloader_orig_pulse
#define pa_channel_map_compatible pa_channel_map_compatible_dylibloader_orig_pulse
#define pa_channel_map_superset pa_channel_map_superset_dylibloader_orig_pulse
#define pa_channel_map_can_balance pa_channel_map_can_balance_dylibloader_orig_pulse
#define pa_channel_map_can_fade pa_channel_map_can_fade_dylibloader_orig_pulse
#define pa_channel_map_can_lfe_balance pa_channel_map_can_lfe_balance_dylibloader_orig_pulse
#define pa_channel_map_to_name pa_channel_map_to_name_dylibloader_orig_pulse
#define pa_channel_map_to_pretty_name pa_channel_map_to_pretty_name_dylibloader_orig_pulse
#define pa_channel_map_has_position pa_channel_map_has_position_dylibloader_orig_pulse
#define pa_channel_map_mask pa_channel_map_mask_dylibloader_orig_pulse
#define pa_encoding_to_string pa_encoding_to_string_dylibloader_orig_pulse
#define pa_encoding_from_string pa_encoding_from_string_dylibloader_orig_pulse
#define pa_format_info_new pa_format_info_new_dylibloader_orig_pulse
#define pa_format_info_copy pa_format_info_copy_dylibloader_orig_pulse
#define pa_format_info_free pa_format_info_free_dylibloader_orig_pulse
#define pa_format_info_valid pa_format_info_valid_dylibloader_orig_pulse
#define pa_format_info_is_pcm pa_format_info_is_pcm_dylibloader_orig_pulse
#define pa_format_info_is_compatible pa_format_info_is_compatible_dylibloader_orig_pulse
#define pa_format_info_snprint pa_format_info_snprint_dylibloader_orig_pulse
#define pa_format_info_from_string pa_format_info_from_string_dylibloader_orig_pulse
#define pa_format_info_from_sample_spec pa_format_info_from_sample_spec_dylibloader_orig_pulse
#define pa_format_info_to_sample_spec pa_format_info_to_sample_spec_dylibloader_orig_pulse
#define pa_format_info_get_prop_type pa_format_info_get_prop_type_dylibloader_orig_pulse
#define pa_format_info_get_prop_int pa_format_info_get_prop_int_dylibloader_orig_pulse
#define pa_format_info_get_prop_int_range pa_format_info_get_prop_int_range_dylibloader_orig_pulse
#define pa_format_info_get_prop_int_array pa_format_info_get_prop_int_array_dylibloader_orig_pulse
#define pa_format_info_get_prop_string pa_format_info_get_prop_string_dylibloader_orig_pulse
#define pa_format_info_get_prop_string_array pa_format_info_get_prop_string_array_dylibloader_orig_pulse
#define pa_format_info_free_string_array pa_format_info_free_string_array_dylibloader_orig_pulse
#define pa_format_info_set_prop_int pa_format_info_set_prop_int_dylibloader_orig_pulse
#define pa_format_info_set_prop_int_array pa_format_info_set_prop_int_array_dylibloader_orig_pulse
#define pa_format_info_set_prop_int_range pa_format_info_set_prop_int_range_dylibloader_orig_pulse
#define pa_format_info_set_prop_string pa_format_info_set_prop_string_dylibloader_orig_pulse
#define pa_format_info_set_prop_string_array pa_format_info_set_prop_string_array_dylibloader_orig_pulse
#define pa_format_info_set_sample_format pa_format_info_set_sample_format_dylibloader_orig_pulse
#define pa_format_info_set_rate pa_format_info_set_rate_dylibloader_orig_pulse
#define pa_format_info_set_channels pa_format_info_set_channels_dylibloader_orig_pulse
#define pa_format_info_set_channel_map pa_format_info_set_channel_map_dylibloader_orig_pulse
#define pa_operation_ref pa_operation_ref_dylibloader_orig_pulse
#define pa_operation_unref pa_operation_unref_dylibloader_orig_pulse
#define pa_operation_cancel pa_operation_cancel_dylibloader_orig_pulse
#define pa_operation_get_state pa_operation_get_state_dylibloader_orig_pulse
#define pa_operation_set_state_callback pa_operation_set_state_callback_dylibloader_orig_pulse
#define pa_context_new pa_context_new_dylibloader_orig_pulse
#define pa_context_new_with_proplist pa_context_new_with_proplist_dylibloader_orig_pulse
#define pa_context_unref pa_context_unref_dylibloader_orig_pulse
#define pa_context_ref pa_context_ref_dylibloader_orig_pulse
#define pa_context_set_state_callback pa_context_set_state_callback_dylibloader_orig_pulse
#define pa_context_set_event_callback pa_context_set_event_callback_dylibloader_orig_pulse
#define pa_context_errno pa_context_errno_dylibloader_orig_pulse
#define pa_context_is_pending pa_context_is_pending_dylibloader_orig_pulse
#define pa_context_get_state pa_context_get_state_dylibloader_orig_pulse
#define pa_context_connect pa_context_connect_dylibloader_orig_pulse
#define pa_context_disconnect pa_context_disconnect_dylibloader_orig_pulse
#define pa_context_drain pa_context_drain_dylibloader_orig_pulse
#define pa_context_exit_daemon pa_context_exit_daemon_dylibloader_orig_pulse
#define pa_context_set_default_sink pa_context_set_default_sink_dylibloader_orig_pulse
#define pa_context_set_default_source pa_context_set_default_source_dylibloader_orig_pulse
#define pa_context_is_local pa_context_is_local_dylibloader_orig_pulse
#define pa_context_set_name pa_context_set_name_dylibloader_orig_pulse
#define pa_context_get_server pa_context_get_server_dylibloader_orig_pulse
#define pa_context_get_protocol_version pa_context_get_protocol_version_dylibloader_orig_pulse
#define pa_context_get_server_protocol_version pa_context_get_server_protocol_version_dylibloader_orig_pulse
#define pa_context_proplist_update pa_context_proplist_update_dylibloader_orig_pulse
#define pa_context_proplist_remove pa_context_proplist_remove_dylibloader_orig_pulse
#define pa_context_get_index pa_context_get_index_dylibloader_orig_pulse
#define pa_context_rttime_new pa_context_rttime_new_dylibloader_orig_pulse
#define pa_context_rttime_restart pa_context_rttime_restart_dylibloader_orig_pulse
#define pa_context_get_tile_size pa_context_get_tile_size_dylibloader_orig_pulse
#define pa_context_load_cookie_from_file pa_context_load_cookie_from_file_dylibloader_orig_pulse
#define pa_cvolume_equal pa_cvolume_equal_dylibloader_orig_pulse
#define pa_cvolume_init pa_cvolume_init_dylibloader_orig_pulse
#define pa_cvolume_set pa_cvolume_set_dylibloader_orig_pulse
#define pa_cvolume_snprint pa_cvolume_snprint_dylibloader_orig_pulse
#define pa_sw_cvolume_snprint_dB pa_sw_cvolume_snprint_dB_dylibloader_orig_pulse
#define pa_cvolume_snprint_verbose pa_cvolume_snprint_verbose_dylibloader_orig_pulse
#define pa_volume_snprint pa_volume_snprint_dylibloader_orig_pulse
#define pa_sw_volume_snprint_dB pa_sw_volume_snprint_dB_dylibloader_orig_pulse
#define pa_volume_snprint_verbose pa_volume_snprint_verbose_dylibloader_orig_pulse
#define pa_cvolume_avg pa_cvolume_avg_dylibloader_orig_pulse
#define pa_cvolume_avg_mask pa_cvolume_avg_mask_dylibloader_orig_pulse
#define pa_cvolume_max pa_cvolume_max_dylibloader_orig_pulse
#define pa_cvolume_max_mask pa_cvolume_max_mask_dylibloader_orig_pulse
#define pa_cvolume_min pa_cvolume_min_dylibloader_orig_pulse
#define pa_cvolume_min_mask pa_cvolume_min_mask_dylibloader_orig_pulse
#define pa_cvolume_valid pa_cvolume_valid_dylibloader_orig_pulse
#define pa_cvolume_channels_equal_to pa_cvolume_channels_equal_to_dylibloader_orig_pulse
#define pa_sw_volume_multiply pa_sw_volume_multiply_dylibloader_orig_pulse
#define pa_sw_cvolume_multiply pa_sw_cvolume_multiply_dylibloader_orig_pulse
#define pa_sw_cvolume_multiply_scalar pa_sw_cvolume_multiply_scalar_dylibloader_orig_pulse
#define pa_sw_volume_divide pa_sw_volume_divide_dylibloader_orig_pulse
#define pa_sw_cvolume_divide pa_sw_cvolume_divide_dylibloader_orig_pulse
#define pa_sw_cvolume_divide_scalar pa_sw_cvolume_divide_scalar_dylibloader_orig_pulse
#define pa_sw_volume_from_dB pa_sw_volume_from_dB_dylibloader_orig_pulse
#define pa_sw_volume_to_dB pa_sw_volume_to_dB_dylibloader_orig_pulse
#define pa_sw_volume_from_linear pa_sw_volume_from_linear_dylibloader_orig_pulse
#define pa_sw_volume_to_linear pa_sw_volume_to_linear_dylibloader_orig_pulse
#define pa_cvolume_remap pa_cvolume_remap_dylibloader_orig_pulse
#define pa_cvolume_compatible pa_cvolume_compatible_dylibloader_orig_pulse
#define pa_cvolume_compatible_with_channel_map pa_cvolume_compatible_with_channel_map_dylibloader_orig_pulse
#define pa_cvolume_get_balance pa_cvolume_get_balance_dylibloader_orig_pulse
#define pa_cvolume_set_balance pa_cvolume_set_balance_dylibloader_orig_pulse
#define pa_cvolume_get_fade pa_cvolume_get_fade_dylibloader_orig_pulse
#define pa_cvolume_set_fade pa_cvolume_set_fade_dylibloader_orig_pulse
#define pa_cvolume_get_lfe_balance pa_cvolume_get_lfe_balance_dylibloader_orig_pulse
#define pa_cvolume_set_lfe_balance pa_cvolume_set_lfe_balance_dylibloader_orig_pulse
#define pa_cvolume_scale pa_cvolume_scale_dylibloader_orig_pulse
#define pa_cvolume_scale_mask pa_cvolume_scale_mask_dylibloader_orig_pulse
#define pa_cvolume_set_position pa_cvolume_set_position_dylibloader_orig_pulse
#define pa_cvolume_get_position pa_cvolume_get_position_dylibloader_orig_pulse
#define pa_cvolume_merge pa_cvolume_merge_dylibloader_orig_pulse
#define pa_cvolume_inc_clamp pa_cvolume_inc_clamp_dylibloader_orig_pulse
#define pa_cvolume_inc pa_cvolume_inc_dylibloader_orig_pulse
#define pa_cvolume_dec pa_cvolume_dec_dylibloader_orig_pulse
#define pa_stream_new pa_stream_new_dylibloader_orig_pulse
#define pa_stream_new_with_proplist pa_stream_new_with_proplist_dylibloader_orig_pulse
#define pa_stream_new_extended pa_stream_new_extended_dylibloader_orig_pulse
#define pa_stream_unref pa_stream_unref_dylibloader_orig_pulse
#define pa_stream_ref pa_stream_ref_dylibloader_orig_pulse
#define pa_stream_get_state pa_stream_get_state_dylibloader_orig_pulse
#define pa_stream_get_context pa_stream_get_context_dylibloader_orig_pulse
#define pa_stream_get_index pa_stream_get_index_dylibloader_orig_pulse
#define pa_stream_get_device_index pa_stream_get_device_index_dylibloader_orig_pulse
#define pa_stream_get_device_name pa_stream_get_device_name_dylibloader_orig_pulse
#define pa_stream_is_suspended pa_stream_is_suspended_dylibloader_orig_pulse
#define pa_stream_is_corked pa_stream_is_corked_dylibloader_orig_pulse
#define pa_stream_connect_playback pa_stream_connect_playback_dylibloader_orig_pulse
#define pa_stream_connect_record pa_stream_connect_record_dylibloader_orig_pulse
#define pa_stream_disconnect pa_stream_disconnect_dylibloader_orig_pulse
#define pa_stream_begin_write pa_stream_begin_write_dylibloader_orig_pulse
#define pa_stream_cancel_write pa_stream_cancel_write_dylibloader_orig_pulse
#define pa_stream_write pa_stream_write_dylibloader_orig_pulse
#define pa_stream_write_ext_free pa_stream_write_ext_free_dylibloader_orig_pulse
#define pa_stream_peek pa_stream_peek_dylibloader_orig_pulse
#define pa_stream_drop pa_stream_drop_dylibloader_orig_pulse
#define pa_stream_writable_size pa_stream_writable_size_dylibloader_orig_pulse
#define pa_stream_readable_size pa_stream_readable_size_dylibloader_orig_pulse
#define pa_stream_drain pa_stream_drain_dylibloader_orig_pulse
#define pa_stream_update_timing_info pa_stream_update_timing_info_dylibloader_orig_pulse
#define pa_stream_set_state_callback pa_stream_set_state_callback_dylibloader_orig_pulse
#define pa_stream_set_write_callback pa_stream_set_write_callback_dylibloader_orig_pulse
#define pa_stream_set_read_callback pa_stream_set_read_callback_dylibloader_orig_pulse
#define pa_stream_set_overflow_callback pa_stream_set_overflow_callback_dylibloader_orig_pulse
#define pa_stream_get_underflow_index pa_stream_get_underflow_index_dylibloader_orig_pulse
#define pa_stream_set_underflow_callback pa_stream_set_underflow_callback_dylibloader_orig_pulse
#define pa_stream_set_started_callback pa_stream_set_started_callback_dylibloader_orig_pulse
#define pa_stream_set_latency_update_callback pa_stream_set_latency_update_callback_dylibloader_orig_pulse
#define pa_stream_set_moved_callback pa_stream_set_moved_callback_dylibloader_orig_pulse
#define pa_stream_set_suspended_callback pa_stream_set_suspended_callback_dylibloader_orig_pulse
#define pa_stream_set_event_callback pa_stream_set_event_callback_dylibloader_orig_pulse
#define pa_stream_set_buffer_attr_callback pa_stream_set_buffer_attr_callback_dylibloader_orig_pulse
#define pa_stream_cork pa_stream_cork_dylibloader_orig_pulse
#define pa_stream_flush pa_stream_flush_dylibloader_orig_pulse
#define pa_stream_prebuf pa_stream_prebuf_dylibloader_orig_pulse
#define pa_stream_trigger pa_stream_trigger_dylibloader_orig_pulse
#define pa_stream_set_name pa_stream_set_name_dylibloader_orig_pulse
#define pa_stream_get_time pa_stream_get_time_dylibloader_orig_pulse
#define pa_stream_get_latency pa_stream_get_latency_dylibloader_orig_pulse
#define pa_stream_get_timing_info pa_stream_get_timing_info_dylibloader_orig_pulse
#define pa_stream_get_sample_spec pa_stream_get_sample_spec_dylibloader_orig_pulse
#define pa_stream_get_channel_map pa_stream_get_channel_map_dylibloader_orig_pulse
#define pa_stream_get_format_info pa_stream_get_format_info_dylibloader_orig_pulse
#define pa_stream_get_buffer_attr pa_stream_get_buffer_attr_dylibloader_orig_pulse
#define pa_stream_set_buffer_attr pa_stream_set_buffer_attr_dylibloader_orig_pulse
#define pa_stream_update_sample_rate pa_stream_update_sample_rate_dylibloader_orig_pulse
#define pa_stream_proplist_update pa_stream_proplist_update_dylibloader_orig_pulse
#define pa_stream_proplist_remove pa_stream_proplist_remove_dylibloader_orig_pulse
#define pa_stream_set_monitor_stream pa_stream_set_monitor_stream_dylibloader_orig_pulse
#define pa_stream_get_monitor_stream pa_stream_get_monitor_stream_dylibloader_orig_pulse
#define pa_context_get_sink_info_by_name pa_context_get_sink_info_by_name_dylibloader_orig_pulse
#define pa_context_get_sink_info_by_index pa_context_get_sink_info_by_index_dylibloader_orig_pulse
#define pa_context_get_sink_info_list pa_context_get_sink_info_list_dylibloader_orig_pulse
#define pa_context_set_sink_volume_by_index pa_context_set_sink_volume_by_index_dylibloader_orig_pulse
#define pa_context_set_sink_volume_by_name pa_context_set_sink_volume_by_name_dylibloader_orig_pulse
#define pa_context_set_sink_mute_by_index pa_context_set_sink_mute_by_index_dylibloader_orig_pulse
#define pa_context_set_sink_mute_by_name pa_context_set_sink_mute_by_name_dylibloader_orig_pulse
#define pa_context_suspend_sink_by_name pa_context_suspend_sink_by_name_dylibloader_orig_pulse
#define pa_context_suspend_sink_by_index pa_context_suspend_sink_by_index_dylibloader_orig_pulse
#define pa_context_set_sink_port_by_index pa_context_set_sink_port_by_index_dylibloader_orig_pulse
#define pa_context_set_sink_port_by_name pa_context_set_sink_port_by_name_dylibloader_orig_pulse
#define pa_context_get_source_info_by_name pa_context_get_source_info_by_name_dylibloader_orig_pulse
#define pa_context_get_source_info_by_index pa_context_get_source_info_by_index_dylibloader_orig_pulse
#define pa_context_get_source_info_list pa_context_get_source_info_list_dylibloader_orig_pulse
#define pa_context_set_source_volume_by_index pa_context_set_source_volume_by_index_dylibloader_orig_pulse
#define pa_context_set_source_volume_by_name pa_context_set_source_volume_by_name_dylibloader_orig_pulse
#define pa_context_set_source_mute_by_index pa_context_set_source_mute_by_index_dylibloader_orig_pulse
#define pa_context_set_source_mute_by_name pa_context_set_source_mute_by_name_dylibloader_orig_pulse
#define pa_context_suspend_source_by_name pa_context_suspend_source_by_name_dylibloader_orig_pulse
#define pa_context_suspend_source_by_index pa_context_suspend_source_by_index_dylibloader_orig_pulse
#define pa_context_set_source_port_by_index pa_context_set_source_port_by_index_dylibloader_orig_pulse
#define pa_context_set_source_port_by_name pa_context_set_source_port_by_name_dylibloader_orig_pulse
#define pa_context_get_server_info pa_context_get_server_info_dylibloader_orig_pulse
#define pa_context_get_module_info pa_context_get_module_info_dylibloader_orig_pulse
#define pa_context_get_module_info_list pa_context_get_module_info_list_dylibloader_orig_pulse
#define pa_context_load_module pa_context_load_module_dylibloader_orig_pulse
#define pa_context_unload_module pa_context_unload_module_dylibloader_orig_pulse
#define pa_context_get_client_info pa_context_get_client_info_dylibloader_orig_pulse
#define pa_context_get_client_info_list pa_context_get_client_info_list_dylibloader_orig_pulse
#define pa_context_kill_client pa_context_kill_client_dylibloader_orig_pulse
#define pa_context_get_card_info_by_index pa_context_get_card_info_by_index_dylibloader_orig_pulse
#define pa_context_get_card_info_by_name pa_context_get_card_info_by_name_dylibloader_orig_pulse
#define pa_context_get_card_info_list pa_context_get_card_info_list_dylibloader_orig_pulse
#define pa_context_set_card_profile_by_index pa_context_set_card_profile_by_index_dylibloader_orig_pulse
#define pa_context_set_card_profile_by_name pa_context_set_card_profile_by_name_dylibloader_orig_pulse
#define pa_context_set_port_latency_offset pa_context_set_port_latency_offset_dylibloader_orig_pulse
#define pa_context_get_sink_input_info pa_context_get_sink_input_info_dylibloader_orig_pulse
#define pa_context_get_sink_input_info_list pa_context_get_sink_input_info_list_dylibloader_orig_pulse
#define pa_context_move_sink_input_by_name pa_context_move_sink_input_by_name_dylibloader_orig_pulse
#define pa_context_move_sink_input_by_index pa_context_move_sink_input_by_index_dylibloader_orig_pulse
#define pa_context_set_sink_input_volume pa_context_set_sink_input_volume_dylibloader_orig_pulse
#define pa_context_set_sink_input_mute pa_context_set_sink_input_mute_dylibloader_orig_pulse
#define pa_context_kill_sink_input pa_context_kill_sink_input_dylibloader_orig_pulse
#define pa_context_get_source_output_info pa_context_get_source_output_info_dylibloader_orig_pulse
#define pa_context_get_source_output_info_list pa_context_get_source_output_info_list_dylibloader_orig_pulse
#define pa_context_move_source_output_by_name pa_context_move_source_output_by_name_dylibloader_orig_pulse
#define pa_context_move_source_output_by_index pa_context_move_source_output_by_index_dylibloader_orig_pulse
#define pa_context_set_source_output_volume pa_context_set_source_output_volume_dylibloader_orig_pulse
#define pa_context_set_source_output_mute pa_context_set_source_output_mute_dylibloader_orig_pulse
#define pa_context_kill_source_output pa_context_kill_source_output_dylibloader_orig_pulse
#define pa_context_stat pa_context_stat_dylibloader_orig_pulse
#define pa_context_get_sample_info_by_name pa_context_get_sample_info_by_name_dylibloader_orig_pulse
#define pa_context_get_sample_info_by_index pa_context_get_sample_info_by_index_dylibloader_orig_pulse
#define pa_context_get_sample_info_list pa_context_get_sample_info_list_dylibloader_orig_pulse
#define pa_context_get_autoload_info_by_name pa_context_get_autoload_info_by_name_dylibloader_orig_pulse
#define pa_context_get_autoload_info_by_index pa_context_get_autoload_info_by_index_dylibloader_orig_pulse
#define pa_context_get_autoload_info_list pa_context_get_autoload_info_list_dylibloader_orig_pulse
#define pa_context_add_autoload pa_context_add_autoload_dylibloader_orig_pulse
#define pa_context_remove_autoload_by_name pa_context_remove_autoload_by_name_dylibloader_orig_pulse
#define pa_context_remove_autoload_by_index pa_context_remove_autoload_by_index_dylibloader_orig_pulse
#define pa_context_subscribe pa_context_subscribe_dylibloader_orig_pulse
#define pa_context_set_subscribe_callback pa_context_set_subscribe_callback_dylibloader_orig_pulse
#define pa_stream_connect_upload pa_stream_connect_upload_dylibloader_orig_pulse
#define pa_stream_finish_upload pa_stream_finish_upload_dylibloader_orig_pulse
#define pa_context_remove_sample pa_context_remove_sample_dylibloader_orig_pulse
#define pa_context_play_sample pa_context_play_sample_dylibloader_orig_pulse
#define pa_context_play_sample_with_proplist pa_context_play_sample_with_proplist_dylibloader_orig_pulse
#define pa_strerror pa_strerror_dylibloader_orig_pulse
#define pa_xmalloc pa_xmalloc_dylibloader_orig_pulse
#define pa_xmalloc0 pa_xmalloc0_dylibloader_orig_pulse
#define pa_xrealloc pa_xrealloc_dylibloader_orig_pulse
#define pa_xfree pa_xfree_dylibloader_orig_pulse
#define pa_xstrdup pa_xstrdup_dylibloader_orig_pulse
#define pa_xstrndup pa_xstrndup_dylibloader_orig_pulse
#define pa_xmemdup pa_xmemdup_dylibloader_orig_pulse
#define pa_utf8_valid pa_utf8_valid_dylibloader_orig_pulse
#define pa_ascii_valid pa_ascii_valid_dylibloader_orig_pulse
#define pa_utf8_filter pa_utf8_filter_dylibloader_orig_pulse
#define pa_ascii_filter pa_ascii_filter_dylibloader_orig_pulse
#define pa_utf8_to_locale pa_utf8_to_locale_dylibloader_orig_pulse
#define pa_locale_to_utf8 pa_locale_to_utf8_dylibloader_orig_pulse
#define pa_threaded_mainloop_new pa_threaded_mainloop_new_dylibloader_orig_pulse
#define pa_threaded_mainloop_free pa_threaded_mainloop_free_dylibloader_orig_pulse
#define pa_threaded_mainloop_start pa_threaded_mainloop_start_dylibloader_orig_pulse
#define pa_threaded_mainloop_stop pa_threaded_mainloop_stop_dylibloader_orig_pulse
#define pa_threaded_mainloop_lock pa_threaded_mainloop_lock_dylibloader_orig_pulse
#define pa_threaded_mainloop_unlock pa_threaded_mainloop_unlock_dylibloader_orig_pulse
#define pa_threaded_mainloop_wait pa_threaded_mainloop_wait_dylibloader_orig_pulse
#define pa_threaded_mainloop_signal pa_threaded_mainloop_signal_dylibloader_orig_pulse
#define pa_threaded_mainloop_accept pa_threaded_mainloop_accept_dylibloader_orig_pulse
#define pa_threaded_mainloop_get_retval pa_threaded_mainloop_get_retval_dylibloader_orig_pulse
#define pa_threaded_mainloop_get_api pa_threaded_mainloop_get_api_dylibloader_orig_pulse
#define pa_threaded_mainloop_in_thread pa_threaded_mainloop_in_thread_dylibloader_orig_pulse
#define pa_threaded_mainloop_set_name pa_threaded_mainloop_set_name_dylibloader_orig_pulse
#define pa_mainloop_new pa_mainloop_new_dylibloader_orig_pulse
#define pa_mainloop_free pa_mainloop_free_dylibloader_orig_pulse
#define pa_mainloop_prepare pa_mainloop_prepare_dylibloader_orig_pulse
#define pa_mainloop_poll pa_mainloop_poll_dylibloader_orig_pulse
#define pa_mainloop_dispatch pa_mainloop_dispatch_dylibloader_orig_pulse
#define pa_mainloop_get_retval pa_mainloop_get_retval_dylibloader_orig_pulse
#define pa_mainloop_iterate pa_mainloop_iterate_dylibloader_orig_pulse
#define pa_mainloop_run pa_mainloop_run_dylibloader_orig_pulse
#define pa_mainloop_get_api pa_mainloop_get_api_dylibloader_orig_pulse
#define pa_mainloop_quit pa_mainloop_quit_dylibloader_orig_pulse
#define pa_mainloop_wakeup pa_mainloop_wakeup_dylibloader_orig_pulse
#define pa_mainloop_set_poll_func pa_mainloop_set_poll_func_dylibloader_orig_pulse
#define pa_signal_init pa_signal_init_dylibloader_orig_pulse
#define pa_signal_done pa_signal_done_dylibloader_orig_pulse
#define pa_signal_new pa_signal_new_dylibloader_orig_pulse
#define pa_signal_free pa_signal_free_dylibloader_orig_pulse
#define pa_signal_set_destroy pa_signal_set_destroy_dylibloader_orig_pulse
#define pa_get_user_name pa_get_user_name_dylibloader_orig_pulse
#define pa_get_host_name pa_get_host_name_dylibloader_orig_pulse
#define pa_get_fqdn pa_get_fqdn_dylibloader_orig_pulse
#define pa_get_home_dir pa_get_home_dir_dylibloader_orig_pulse
#define pa_get_binary_name pa_get_binary_name_dylibloader_orig_pulse
#define pa_path_get_filename pa_path_get_filename_dylibloader_orig_pulse
#define pa_msleep pa_msleep_dylibloader_orig_pulse
#define pa_gettimeofday pa_gettimeofday_dylibloader_orig_pulse
#define pa_timeval_diff pa_timeval_diff_dylibloader_orig_pulse
#define pa_timeval_cmp pa_timeval_cmp_dylibloader_orig_pulse
#define pa_timeval_age pa_timeval_age_dylibloader_orig_pulse
#define pa_timeval_add pa_timeval_add_dylibloader_orig_pulse
#define pa_timeval_sub pa_timeval_sub_dylibloader_orig_pulse
#define pa_timeval_store pa_timeval_store_dylibloader_orig_pulse
#define pa_timeval_load pa_timeval_load_dylibloader_orig_pulse
#define pa_rtclock_now pa_rtclock_now_dylibloader_orig_pulse
#include <thirdparty/linuxbsd_headers/pulse/pulseaudio.h> // IWYU pragma: export.
#undef pa_get_library_version
#undef pa_bytes_per_second
#undef pa_frame_size
#undef pa_sample_size
#undef pa_sample_size_of_format
#undef pa_bytes_to_usec
#undef pa_usec_to_bytes
#undef pa_sample_spec_init
#undef pa_sample_format_valid
#undef pa_sample_rate_valid
#undef pa_channels_valid
#undef pa_sample_spec_valid
#undef pa_sample_spec_equal
#undef pa_sample_format_to_string
#undef pa_parse_sample_format
#undef pa_sample_spec_snprint
#undef pa_bytes_snprint
#undef pa_sample_format_is_le
#undef pa_sample_format_is_be
#undef pa_direction_valid
#undef pa_direction_to_string
#undef pa_mainloop_api_once
#undef pa_proplist_new
#undef pa_proplist_free
#undef pa_proplist_key_valid
#undef pa_proplist_sets
#undef pa_proplist_setp
#undef pa_proplist_setf
#undef pa_proplist_set
#undef pa_proplist_gets
#undef pa_proplist_get
#undef pa_proplist_update
#undef pa_proplist_unset
#undef pa_proplist_unset_many
#undef pa_proplist_iterate
#undef pa_proplist_to_string
#undef pa_proplist_to_string_sep
#undef pa_proplist_from_string
#undef pa_proplist_contains
#undef pa_proplist_clear
#undef pa_proplist_copy
#undef pa_proplist_size
#undef pa_proplist_isempty
#undef pa_proplist_equal
#undef pa_channel_map_init
#undef pa_channel_map_init_mono
#undef pa_channel_map_init_stereo
#undef pa_channel_map_init_auto
#undef pa_channel_map_init_extend
#undef pa_channel_position_to_string
#undef pa_channel_position_from_string
#undef pa_channel_position_to_pretty_string
#undef pa_channel_map_snprint
#undef pa_channel_map_parse
#undef pa_channel_map_equal
#undef pa_channel_map_valid
#undef pa_channel_map_compatible
#undef pa_channel_map_superset
#undef pa_channel_map_can_balance
#undef pa_channel_map_can_fade
#undef pa_channel_map_can_lfe_balance
#undef pa_channel_map_to_name
#undef pa_channel_map_to_pretty_name
#undef pa_channel_map_has_position
#undef pa_channel_map_mask
#undef pa_encoding_to_string
#undef pa_encoding_from_string
#undef pa_format_info_new
#undef pa_format_info_copy
#undef pa_format_info_free
#undef pa_format_info_valid
#undef pa_format_info_is_pcm
#undef pa_format_info_is_compatible
#undef pa_format_info_snprint
#undef pa_format_info_from_string
#undef pa_format_info_from_sample_spec
#undef pa_format_info_to_sample_spec
#undef pa_format_info_get_prop_type
#undef pa_format_info_get_prop_int
#undef pa_format_info_get_prop_int_range
#undef pa_format_info_get_prop_int_array
#undef pa_format_info_get_prop_string
#undef pa_format_info_get_prop_string_array
#undef pa_format_info_free_string_array
#undef pa_format_info_set_prop_int
#undef pa_format_info_set_prop_int_array
#undef pa_format_info_set_prop_int_range
#undef pa_format_info_set_prop_string
#undef pa_format_info_set_prop_string_array
#undef pa_format_info_set_sample_format
#undef pa_format_info_set_rate
#undef pa_format_info_set_channels
#undef pa_format_info_set_channel_map
#undef pa_operation_ref
#undef pa_operation_unref
#undef pa_operation_cancel
#undef pa_operation_get_state
#undef pa_operation_set_state_callback
#undef pa_context_new
#undef pa_context_new_with_proplist
#undef pa_context_unref
#undef pa_context_ref
#undef pa_context_set_state_callback
#undef pa_context_set_event_callback
#undef pa_context_errno
#undef pa_context_is_pending
#undef pa_context_get_state
#undef pa_context_connect
#undef pa_context_disconnect
#undef pa_context_drain
#undef pa_context_exit_daemon
#undef pa_context_set_default_sink
#undef pa_context_set_default_source
#undef pa_context_is_local
#undef pa_context_set_name
#undef pa_context_get_server
#undef pa_context_get_protocol_version
#undef pa_context_get_server_protocol_version
#undef pa_context_proplist_update
#undef pa_context_proplist_remove
#undef pa_context_get_index
#undef pa_context_rttime_new
#undef pa_context_rttime_restart
#undef pa_context_get_tile_size
#undef pa_context_load_cookie_from_file
#undef pa_cvolume_equal
#undef pa_cvolume_init
#undef pa_cvolume_set
#undef pa_cvolume_snprint
#undef pa_sw_cvolume_snprint_dB
#undef pa_cvolume_snprint_verbose
#undef pa_volume_snprint
#undef pa_sw_volume_snprint_dB
#undef pa_volume_snprint_verbose
#undef pa_cvolume_avg
#undef pa_cvolume_avg_mask
#undef pa_cvolume_max
#undef pa_cvolume_max_mask
#undef pa_cvolume_min
#undef pa_cvolume_min_mask
#undef pa_cvolume_valid
#undef pa_cvolume_channels_equal_to
#undef pa_sw_volume_multiply
#undef pa_sw_cvolume_multiply
#undef pa_sw_cvolume_multiply_scalar
#undef pa_sw_volume_divide
#undef pa_sw_cvolume_divide
#undef pa_sw_cvolume_divide_scalar
#undef pa_sw_volume_from_dB
#undef pa_sw_volume_to_dB
#undef pa_sw_volume_from_linear
#undef pa_sw_volume_to_linear
#undef pa_cvolume_remap
#undef pa_cvolume_compatible
#undef pa_cvolume_compatible_with_channel_map
#undef pa_cvolume_get_balance
#undef pa_cvolume_set_balance
#undef pa_cvolume_get_fade
#undef pa_cvolume_set_fade
#undef pa_cvolume_get_lfe_balance
#undef pa_cvolume_set_lfe_balance
#undef pa_cvolume_scale
#undef pa_cvolume_scale_mask
#undef pa_cvolume_set_position
#undef pa_cvolume_get_position
#undef pa_cvolume_merge
#undef pa_cvolume_inc_clamp
#undef pa_cvolume_inc
#undef pa_cvolume_dec
#undef pa_stream_new
#undef pa_stream_new_with_proplist
#undef pa_stream_new_extended
#undef pa_stream_unref
#undef pa_stream_ref
#undef pa_stream_get_state
#undef pa_stream_get_context
#undef pa_stream_get_index
#undef pa_stream_get_device_index
#undef pa_stream_get_device_name
#undef pa_stream_is_suspended
#undef pa_stream_is_corked
#undef pa_stream_connect_playback
#undef pa_stream_connect_record
#undef pa_stream_disconnect
#undef pa_stream_begin_write
#undef pa_stream_cancel_write
#undef pa_stream_write
#undef pa_stream_write_ext_free
#undef pa_stream_peek
#undef pa_stream_drop
#undef pa_stream_writable_size
#undef pa_stream_readable_size
#undef pa_stream_drain
#undef pa_stream_update_timing_info
#undef pa_stream_set_state_callback
#undef pa_stream_set_write_callback
#undef pa_stream_set_read_callback
#undef pa_stream_set_overflow_callback
#undef pa_stream_get_underflow_index
#undef pa_stream_set_underflow_callback
#undef pa_stream_set_started_callback
#undef pa_stream_set_latency_update_callback
#undef pa_stream_set_moved_callback
#undef pa_stream_set_suspended_callback
#undef pa_stream_set_event_callback
#undef pa_stream_set_buffer_attr_callback
#undef pa_stream_cork
#undef pa_stream_flush
#undef pa_stream_prebuf
#undef pa_stream_trigger
#undef pa_stream_set_name
#undef pa_stream_get_time
#undef pa_stream_get_latency
#undef pa_stream_get_timing_info
#undef pa_stream_get_sample_spec
#undef pa_stream_get_channel_map
#undef pa_stream_get_format_info
#undef pa_stream_get_buffer_attr
#undef pa_stream_set_buffer_attr
#undef pa_stream_update_sample_rate
#undef pa_stream_proplist_update
#undef pa_stream_proplist_remove
#undef pa_stream_set_monitor_stream
#undef pa_stream_get_monitor_stream
#undef pa_context_get_sink_info_by_name
#undef pa_context_get_sink_info_by_index
#undef pa_context_get_sink_info_list
#undef pa_context_set_sink_volume_by_index
#undef pa_context_set_sink_volume_by_name
#undef pa_context_set_sink_mute_by_index
#undef pa_context_set_sink_mute_by_name
#undef pa_context_suspend_sink_by_name
#undef pa_context_suspend_sink_by_index
#undef pa_context_set_sink_port_by_index
#undef pa_context_set_sink_port_by_name
#undef pa_context_get_source_info_by_name
#undef pa_context_get_source_info_by_index
#undef pa_context_get_source_info_list
#undef pa_context_set_source_volume_by_index
#undef pa_context_set_source_volume_by_name
#undef pa_context_set_source_mute_by_index
#undef pa_context_set_source_mute_by_name
#undef pa_context_suspend_source_by_name
#undef pa_context_suspend_source_by_index
#undef pa_context_set_source_port_by_index
#undef pa_context_set_source_port_by_name
#undef pa_context_get_server_info
#undef pa_context_get_module_info
#undef pa_context_get_module_info_list
#undef pa_context_load_module
#undef pa_context_unload_module
#undef pa_context_get_client_info
#undef pa_context_get_client_info_list
#undef pa_context_kill_client
#undef pa_context_get_card_info_by_index
#undef pa_context_get_card_info_by_name
#undef pa_context_get_card_info_list
#undef pa_context_set_card_profile_by_index
#undef pa_context_set_card_profile_by_name
#undef pa_context_set_port_latency_offset
#undef pa_context_get_sink_input_info
#undef pa_context_get_sink_input_info_list
#undef pa_context_move_sink_input_by_name
#undef pa_context_move_sink_input_by_index
#undef pa_context_set_sink_input_volume
#undef pa_context_set_sink_input_mute
#undef pa_context_kill_sink_input
#undef pa_context_get_source_output_info
#undef pa_context_get_source_output_info_list
#undef pa_context_move_source_output_by_name
#undef pa_context_move_source_output_by_index
#undef pa_context_set_source_output_volume
#undef pa_context_set_source_output_mute
#undef pa_context_kill_source_output
#undef pa_context_stat
#undef pa_context_get_sample_info_by_name
#undef pa_context_get_sample_info_by_index
#undef pa_context_get_sample_info_list
#undef pa_context_get_autoload_info_by_name
#undef pa_context_get_autoload_info_by_index
#undef pa_context_get_autoload_info_list
#undef pa_context_add_autoload
#undef pa_context_remove_autoload_by_name
#undef pa_context_remove_autoload_by_index
#undef pa_context_subscribe
#undef pa_context_set_subscribe_callback
#undef pa_stream_connect_upload
#undef pa_stream_finish_upload
#undef pa_context_remove_sample
#undef pa_context_play_sample
#undef pa_context_play_sample_with_proplist
#undef pa_strerror
#undef pa_xmalloc
#undef pa_xmalloc0
#undef pa_xrealloc
#undef pa_xfree
#undef pa_xstrdup
#undef pa_xstrndup
#undef pa_xmemdup
#undef pa_utf8_valid
#undef pa_ascii_valid
#undef pa_utf8_filter
#undef pa_ascii_filter
#undef pa_utf8_to_locale
#undef pa_locale_to_utf8
#undef pa_threaded_mainloop_new
#undef pa_threaded_mainloop_free
#undef pa_threaded_mainloop_start
#undef pa_threaded_mainloop_stop
#undef pa_threaded_mainloop_lock
#undef pa_threaded_mainloop_unlock
#undef pa_threaded_mainloop_wait
#undef pa_threaded_mainloop_signal
#undef pa_threaded_mainloop_accept
#undef pa_threaded_mainloop_get_retval
#undef pa_threaded_mainloop_get_api
#undef pa_threaded_mainloop_in_thread
#undef pa_threaded_mainloop_set_name
#undef pa_mainloop_new
#undef pa_mainloop_free
#undef pa_mainloop_prepare
#undef pa_mainloop_poll
#undef pa_mainloop_dispatch
#undef pa_mainloop_get_retval
#undef pa_mainloop_iterate
#undef pa_mainloop_run
#undef pa_mainloop_get_api
#undef pa_mainloop_quit
#undef pa_mainloop_wakeup
#undef pa_mainloop_set_poll_func
#undef pa_signal_init
#undef pa_signal_done
#undef pa_signal_new
#undef pa_signal_free
#undef pa_signal_set_destroy
#undef pa_get_user_name
#undef pa_get_host_name
#undef pa_get_fqdn
#undef pa_get_home_dir
#undef pa_get_binary_name
#undef pa_path_get_filename
#undef pa_msleep
#undef pa_gettimeofday
#undef pa_timeval_diff
#undef pa_timeval_cmp
#undef pa_timeval_age
#undef pa_timeval_add
#undef pa_timeval_sub
#undef pa_timeval_store
#undef pa_timeval_load
#undef pa_rtclock_now
#ifdef __cplusplus
extern "C" {
#endif
#define pa_get_library_version pa_get_library_version_dylibloader_wrapper_pulse
#define pa_bytes_per_second pa_bytes_per_second_dylibloader_wrapper_pulse
#define pa_frame_size pa_frame_size_dylibloader_wrapper_pulse
#define pa_sample_size pa_sample_size_dylibloader_wrapper_pulse
#define pa_sample_size_of_format pa_sample_size_of_format_dylibloader_wrapper_pulse
#define pa_bytes_to_usec pa_bytes_to_usec_dylibloader_wrapper_pulse
#define pa_usec_to_bytes pa_usec_to_bytes_dylibloader_wrapper_pulse
#define pa_sample_spec_init pa_sample_spec_init_dylibloader_wrapper_pulse
#define pa_sample_format_valid pa_sample_format_valid_dylibloader_wrapper_pulse
#define pa_sample_rate_valid pa_sample_rate_valid_dylibloader_wrapper_pulse
#define pa_channels_valid pa_channels_valid_dylibloader_wrapper_pulse
#define pa_sample_spec_valid pa_sample_spec_valid_dylibloader_wrapper_pulse
#define pa_sample_spec_equal pa_sample_spec_equal_dylibloader_wrapper_pulse
#define pa_sample_format_to_string pa_sample_format_to_string_dylibloader_wrapper_pulse
#define pa_parse_sample_format pa_parse_sample_format_dylibloader_wrapper_pulse
#define pa_sample_spec_snprint pa_sample_spec_snprint_dylibloader_wrapper_pulse
#define pa_bytes_snprint pa_bytes_snprint_dylibloader_wrapper_pulse
#define pa_sample_format_is_le pa_sample_format_is_le_dylibloader_wrapper_pulse
#define pa_sample_format_is_be pa_sample_format_is_be_dylibloader_wrapper_pulse
#define pa_direction_valid pa_direction_valid_dylibloader_wrapper_pulse
#define pa_direction_to_string pa_direction_to_string_dylibloader_wrapper_pulse
#define pa_mainloop_api_once pa_mainloop_api_once_dylibloader_wrapper_pulse
#define pa_proplist_new pa_proplist_new_dylibloader_wrapper_pulse
#define pa_proplist_free pa_proplist_free_dylibloader_wrapper_pulse
#define pa_proplist_key_valid pa_proplist_key_valid_dylibloader_wrapper_pulse
#define pa_proplist_sets pa_proplist_sets_dylibloader_wrapper_pulse
#define pa_proplist_setp pa_proplist_setp_dylibloader_wrapper_pulse
#define pa_proplist_setf pa_proplist_setf_dylibloader_wrapper_pulse
#define pa_proplist_set pa_proplist_set_dylibloader_wrapper_pulse
#define pa_proplist_gets pa_proplist_gets_dylibloader_wrapper_pulse
#define pa_proplist_get pa_proplist_get_dylibloader_wrapper_pulse
#define pa_proplist_update pa_proplist_update_dylibloader_wrapper_pulse
#define pa_proplist_unset pa_proplist_unset_dylibloader_wrapper_pulse
#define pa_proplist_unset_many pa_proplist_unset_many_dylibloader_wrapper_pulse
#define pa_proplist_iterate pa_proplist_iterate_dylibloader_wrapper_pulse
#define pa_proplist_to_string pa_proplist_to_string_dylibloader_wrapper_pulse
#define pa_proplist_to_string_sep pa_proplist_to_string_sep_dylibloader_wrapper_pulse
#define pa_proplist_from_string pa_proplist_from_string_dylibloader_wrapper_pulse
#define pa_proplist_contains pa_proplist_contains_dylibloader_wrapper_pulse
#define pa_proplist_clear pa_proplist_clear_dylibloader_wrapper_pulse
#define pa_proplist_copy pa_proplist_copy_dylibloader_wrapper_pulse
#define pa_proplist_size pa_proplist_size_dylibloader_wrapper_pulse
#define pa_proplist_isempty pa_proplist_isempty_dylibloader_wrapper_pulse
#define pa_proplist_equal pa_proplist_equal_dylibloader_wrapper_pulse
#define pa_channel_map_init pa_channel_map_init_dylibloader_wrapper_pulse
#define pa_channel_map_init_mono pa_channel_map_init_mono_dylibloader_wrapper_pulse
#define pa_channel_map_init_stereo pa_channel_map_init_stereo_dylibloader_wrapper_pulse
#define pa_channel_map_init_auto pa_channel_map_init_auto_dylibloader_wrapper_pulse
#define pa_channel_map_init_extend pa_channel_map_init_extend_dylibloader_wrapper_pulse
#define pa_channel_position_to_string pa_channel_position_to_string_dylibloader_wrapper_pulse
#define pa_channel_position_from_string pa_channel_position_from_string_dylibloader_wrapper_pulse
#define pa_channel_position_to_pretty_string pa_channel_position_to_pretty_string_dylibloader_wrapper_pulse
#define pa_channel_map_snprint pa_channel_map_snprint_dylibloader_wrapper_pulse
#define pa_channel_map_parse pa_channel_map_parse_dylibloader_wrapper_pulse
#define pa_channel_map_equal pa_channel_map_equal_dylibloader_wrapper_pulse
#define pa_channel_map_valid pa_channel_map_valid_dylibloader_wrapper_pulse
#define pa_channel_map_compatible pa_channel_map_compatible_dylibloader_wrapper_pulse
#define pa_channel_map_superset pa_channel_map_superset_dylibloader_wrapper_pulse
#define pa_channel_map_can_balance pa_channel_map_can_balance_dylibloader_wrapper_pulse
#define pa_channel_map_can_fade pa_channel_map_can_fade_dylibloader_wrapper_pulse
#define pa_channel_map_can_lfe_balance pa_channel_map_can_lfe_balance_dylibloader_wrapper_pulse
#define pa_channel_map_to_name pa_channel_map_to_name_dylibloader_wrapper_pulse
#define pa_channel_map_to_pretty_name pa_channel_map_to_pretty_name_dylibloader_wrapper_pulse
#define pa_channel_map_has_position pa_channel_map_has_position_dylibloader_wrapper_pulse
#define pa_channel_map_mask pa_channel_map_mask_dylibloader_wrapper_pulse
#define pa_encoding_to_string pa_encoding_to_string_dylibloader_wrapper_pulse
#define pa_encoding_from_string pa_encoding_from_string_dylibloader_wrapper_pulse
#define pa_format_info_new pa_format_info_new_dylibloader_wrapper_pulse
#define pa_format_info_copy pa_format_info_copy_dylibloader_wrapper_pulse
#define pa_format_info_free pa_format_info_free_dylibloader_wrapper_pulse
#define pa_format_info_valid pa_format_info_valid_dylibloader_wrapper_pulse
#define pa_format_info_is_pcm pa_format_info_is_pcm_dylibloader_wrapper_pulse
#define pa_format_info_is_compatible pa_format_info_is_compatible_dylibloader_wrapper_pulse
#define pa_format_info_snprint pa_format_info_snprint_dylibloader_wrapper_pulse
#define pa_format_info_from_string pa_format_info_from_string_dylibloader_wrapper_pulse
#define pa_format_info_from_sample_spec pa_format_info_from_sample_spec_dylibloader_wrapper_pulse
#define pa_format_info_to_sample_spec pa_format_info_to_sample_spec_dylibloader_wrapper_pulse
#define pa_format_info_get_prop_type pa_format_info_get_prop_type_dylibloader_wrapper_pulse
#define pa_format_info_get_prop_int pa_format_info_get_prop_int_dylibloader_wrapper_pulse
#define pa_format_info_get_prop_int_range pa_format_info_get_prop_int_range_dylibloader_wrapper_pulse
#define pa_format_info_get_prop_int_array pa_format_info_get_prop_int_array_dylibloader_wrapper_pulse
#define pa_format_info_get_prop_string pa_format_info_get_prop_string_dylibloader_wrapper_pulse
#define pa_format_info_get_prop_string_array pa_format_info_get_prop_string_array_dylibloader_wrapper_pulse
#define pa_format_info_free_string_array pa_format_info_free_string_array_dylibloader_wrapper_pulse
#define pa_format_info_set_prop_int pa_format_info_set_prop_int_dylibloader_wrapper_pulse
#define pa_format_info_set_prop_int_array pa_format_info_set_prop_int_array_dylibloader_wrapper_pulse
#define pa_format_info_set_prop_int_range pa_format_info_set_prop_int_range_dylibloader_wrapper_pulse
#define pa_format_info_set_prop_string pa_format_info_set_prop_string_dylibloader_wrapper_pulse
#define pa_format_info_set_prop_string_array pa_format_info_set_prop_string_array_dylibloader_wrapper_pulse
#define pa_format_info_set_sample_format pa_format_info_set_sample_format_dylibloader_wrapper_pulse
#define pa_format_info_set_rate pa_format_info_set_rate_dylibloader_wrapper_pulse
#define pa_format_info_set_channels pa_format_info_set_channels_dylibloader_wrapper_pulse
#define pa_format_info_set_channel_map pa_format_info_set_channel_map_dylibloader_wrapper_pulse
#define pa_operation_ref pa_operation_ref_dylibloader_wrapper_pulse
#define pa_operation_unref pa_operation_unref_dylibloader_wrapper_pulse
#define pa_operation_cancel pa_operation_cancel_dylibloader_wrapper_pulse
#define pa_operation_get_state pa_operation_get_state_dylibloader_wrapper_pulse
#define pa_operation_set_state_callback pa_operation_set_state_callback_dylibloader_wrapper_pulse
#define pa_context_new pa_context_new_dylibloader_wrapper_pulse
#define pa_context_new_with_proplist pa_context_new_with_proplist_dylibloader_wrapper_pulse
#define pa_context_unref pa_context_unref_dylibloader_wrapper_pulse
#define pa_context_ref pa_context_ref_dylibloader_wrapper_pulse
#define pa_context_set_state_callback pa_context_set_state_callback_dylibloader_wrapper_pulse
#define pa_context_set_event_callback pa_context_set_event_callback_dylibloader_wrapper_pulse
#define pa_context_errno pa_context_errno_dylibloader_wrapper_pulse
#define pa_context_is_pending pa_context_is_pending_dylibloader_wrapper_pulse
#define pa_context_get_state pa_context_get_state_dylibloader_wrapper_pulse
#define pa_context_connect pa_context_connect_dylibloader_wrapper_pulse
#define pa_context_disconnect pa_context_disconnect_dylibloader_wrapper_pulse
#define pa_context_drain pa_context_drain_dylibloader_wrapper_pulse
#define pa_context_exit_daemon pa_context_exit_daemon_dylibloader_wrapper_pulse
#define pa_context_set_default_sink pa_context_set_default_sink_dylibloader_wrapper_pulse
#define pa_context_set_default_source pa_context_set_default_source_dylibloader_wrapper_pulse
#define pa_context_is_local pa_context_is_local_dylibloader_wrapper_pulse
#define pa_context_set_name pa_context_set_name_dylibloader_wrapper_pulse
#define pa_context_get_server pa_context_get_server_dylibloader_wrapper_pulse
#define pa_context_get_protocol_version pa_context_get_protocol_version_dylibloader_wrapper_pulse
#define pa_context_get_server_protocol_version pa_context_get_server_protocol_version_dylibloader_wrapper_pulse
#define pa_context_proplist_update pa_context_proplist_update_dylibloader_wrapper_pulse
#define pa_context_proplist_remove pa_context_proplist_remove_dylibloader_wrapper_pulse
#define pa_context_get_index pa_context_get_index_dylibloader_wrapper_pulse
#define pa_context_rttime_new pa_context_rttime_new_dylibloader_wrapper_pulse
#define pa_context_rttime_restart pa_context_rttime_restart_dylibloader_wrapper_pulse
#define pa_context_get_tile_size pa_context_get_tile_size_dylibloader_wrapper_pulse
#define pa_context_load_cookie_from_file pa_context_load_cookie_from_file_dylibloader_wrapper_pulse
#define pa_cvolume_equal pa_cvolume_equal_dylibloader_wrapper_pulse
#define pa_cvolume_init pa_cvolume_init_dylibloader_wrapper_pulse
#define pa_cvolume_set pa_cvolume_set_dylibloader_wrapper_pulse
#define pa_cvolume_snprint pa_cvolume_snprint_dylibloader_wrapper_pulse
#define pa_sw_cvolume_snprint_dB pa_sw_cvolume_snprint_dB_dylibloader_wrapper_pulse
#define pa_cvolume_snprint_verbose pa_cvolume_snprint_verbose_dylibloader_wrapper_pulse
#define pa_volume_snprint pa_volume_snprint_dylibloader_wrapper_pulse
#define pa_sw_volume_snprint_dB pa_sw_volume_snprint_dB_dylibloader_wrapper_pulse
#define pa_volume_snprint_verbose pa_volume_snprint_verbose_dylibloader_wrapper_pulse
#define pa_cvolume_avg pa_cvolume_avg_dylibloader_wrapper_pulse
#define pa_cvolume_avg_mask pa_cvolume_avg_mask_dylibloader_wrapper_pulse
#define pa_cvolume_max pa_cvolume_max_dylibloader_wrapper_pulse
#define pa_cvolume_max_mask pa_cvolume_max_mask_dylibloader_wrapper_pulse
#define pa_cvolume_min pa_cvolume_min_dylibloader_wrapper_pulse
#define pa_cvolume_min_mask pa_cvolume_min_mask_dylibloader_wrapper_pulse
#define pa_cvolume_valid pa_cvolume_valid_dylibloader_wrapper_pulse
#define pa_cvolume_channels_equal_to pa_cvolume_channels_equal_to_dylibloader_wrapper_pulse
#define pa_sw_volume_multiply pa_sw_volume_multiply_dylibloader_wrapper_pulse
#define pa_sw_cvolume_multiply pa_sw_cvolume_multiply_dylibloader_wrapper_pulse
#define pa_sw_cvolume_multiply_scalar pa_sw_cvolume_multiply_scalar_dylibloader_wrapper_pulse
#define pa_sw_volume_divide pa_sw_volume_divide_dylibloader_wrapper_pulse
#define pa_sw_cvolume_divide pa_sw_cvolume_divide_dylibloader_wrapper_pulse
#define pa_sw_cvolume_divide_scalar pa_sw_cvolume_divide_scalar_dylibloader_wrapper_pulse
#define pa_sw_volume_from_dB pa_sw_volume_from_dB_dylibloader_wrapper_pulse
#define pa_sw_volume_to_dB pa_sw_volume_to_dB_dylibloader_wrapper_pulse
#define pa_sw_volume_from_linear pa_sw_volume_from_linear_dylibloader_wrapper_pulse
#define pa_sw_volume_to_linear pa_sw_volume_to_linear_dylibloader_wrapper_pulse
#define pa_cvolume_remap pa_cvolume_remap_dylibloader_wrapper_pulse
#define pa_cvolume_compatible pa_cvolume_compatible_dylibloader_wrapper_pulse
#define pa_cvolume_compatible_with_channel_map pa_cvolume_compatible_with_channel_map_dylibloader_wrapper_pulse
#define pa_cvolume_get_balance pa_cvolume_get_balance_dylibloader_wrapper_pulse
#define pa_cvolume_set_balance pa_cvolume_set_balance_dylibloader_wrapper_pulse
#define pa_cvolume_get_fade pa_cvolume_get_fade_dylibloader_wrapper_pulse
#define pa_cvolume_set_fade pa_cvolume_set_fade_dylibloader_wrapper_pulse
#define pa_cvolume_get_lfe_balance pa_cvolume_get_lfe_balance_dylibloader_wrapper_pulse
#define pa_cvolume_set_lfe_balance pa_cvolume_set_lfe_balance_dylibloader_wrapper_pulse
#define pa_cvolume_scale pa_cvolume_scale_dylibloader_wrapper_pulse
#define pa_cvolume_scale_mask pa_cvolume_scale_mask_dylibloader_wrapper_pulse
#define pa_cvolume_set_position pa_cvolume_set_position_dylibloader_wrapper_pulse
#define pa_cvolume_get_position pa_cvolume_get_position_dylibloader_wrapper_pulse
#define pa_cvolume_merge pa_cvolume_merge_dylibloader_wrapper_pulse
#define pa_cvolume_inc_clamp pa_cvolume_inc_clamp_dylibloader_wrapper_pulse
#define pa_cvolume_inc pa_cvolume_inc_dylibloader_wrapper_pulse
#define pa_cvolume_dec pa_cvolume_dec_dylibloader_wrapper_pulse
#define pa_stream_new pa_stream_new_dylibloader_wrapper_pulse
#define pa_stream_new_with_proplist pa_stream_new_with_proplist_dylibloader_wrapper_pulse
#define pa_stream_new_extended pa_stream_new_extended_dylibloader_wrapper_pulse
#define pa_stream_unref pa_stream_unref_dylibloader_wrapper_pulse
#define pa_stream_ref pa_stream_ref_dylibloader_wrapper_pulse
#define pa_stream_get_state pa_stream_get_state_dylibloader_wrapper_pulse
#define pa_stream_get_context pa_stream_get_context_dylibloader_wrapper_pulse
#define pa_stream_get_index pa_stream_get_index_dylibloader_wrapper_pulse
#define pa_stream_get_device_index pa_stream_get_device_index_dylibloader_wrapper_pulse
#define pa_stream_get_device_name pa_stream_get_device_name_dylibloader_wrapper_pulse
#define pa_stream_is_suspended pa_stream_is_suspended_dylibloader_wrapper_pulse
#define pa_stream_is_corked pa_stream_is_corked_dylibloader_wrapper_pulse
#define pa_stream_connect_playback pa_stream_connect_playback_dylibloader_wrapper_pulse
#define pa_stream_connect_record pa_stream_connect_record_dylibloader_wrapper_pulse
#define pa_stream_disconnect pa_stream_disconnect_dylibloader_wrapper_pulse
#define pa_stream_begin_write pa_stream_begin_write_dylibloader_wrapper_pulse
#define pa_stream_cancel_write pa_stream_cancel_write_dylibloader_wrapper_pulse
#define pa_stream_write pa_stream_write_dylibloader_wrapper_pulse
#define pa_stream_write_ext_free pa_stream_write_ext_free_dylibloader_wrapper_pulse
#define pa_stream_peek pa_stream_peek_dylibloader_wrapper_pulse
#define pa_stream_drop pa_stream_drop_dylibloader_wrapper_pulse
#define pa_stream_writable_size pa_stream_writable_size_dylibloader_wrapper_pulse
#define pa_stream_readable_size pa_stream_readable_size_dylibloader_wrapper_pulse
#define pa_stream_drain pa_stream_drain_dylibloader_wrapper_pulse
#define pa_stream_update_timing_info pa_stream_update_timing_info_dylibloader_wrapper_pulse
#define pa_stream_set_state_callback pa_stream_set_state_callback_dylibloader_wrapper_pulse
#define pa_stream_set_write_callback pa_stream_set_write_callback_dylibloader_wrapper_pulse
#define pa_stream_set_read_callback pa_stream_set_read_callback_dylibloader_wrapper_pulse
#define pa_stream_set_overflow_callback pa_stream_set_overflow_callback_dylibloader_wrapper_pulse
#define pa_stream_get_underflow_index pa_stream_get_underflow_index_dylibloader_wrapper_pulse
#define pa_stream_set_underflow_callback pa_stream_set_underflow_callback_dylibloader_wrapper_pulse
#define pa_stream_set_started_callback pa_stream_set_started_callback_dylibloader_wrapper_pulse
#define pa_stream_set_latency_update_callback pa_stream_set_latency_update_callback_dylibloader_wrapper_pulse
#define pa_stream_set_moved_callback pa_stream_set_moved_callback_dylibloader_wrapper_pulse
#define pa_stream_set_suspended_callback pa_stream_set_suspended_callback_dylibloader_wrapper_pulse
#define pa_stream_set_event_callback pa_stream_set_event_callback_dylibloader_wrapper_pulse
#define pa_stream_set_buffer_attr_callback pa_stream_set_buffer_attr_callback_dylibloader_wrapper_pulse
#define pa_stream_cork pa_stream_cork_dylibloader_wrapper_pulse
#define pa_stream_flush pa_stream_flush_dylibloader_wrapper_pulse
#define pa_stream_prebuf pa_stream_prebuf_dylibloader_wrapper_pulse
#define pa_stream_trigger pa_stream_trigger_dylibloader_wrapper_pulse
#define pa_stream_set_name pa_stream_set_name_dylibloader_wrapper_pulse
#define pa_stream_get_time pa_stream_get_time_dylibloader_wrapper_pulse
#define pa_stream_get_latency pa_stream_get_latency_dylibloader_wrapper_pulse
#define pa_stream_get_timing_info pa_stream_get_timing_info_dylibloader_wrapper_pulse
#define pa_stream_get_sample_spec pa_stream_get_sample_spec_dylibloader_wrapper_pulse
#define pa_stream_get_channel_map pa_stream_get_channel_map_dylibloader_wrapper_pulse
#define pa_stream_get_format_info pa_stream_get_format_info_dylibloader_wrapper_pulse
#define pa_stream_get_buffer_attr pa_stream_get_buffer_attr_dylibloader_wrapper_pulse
#define pa_stream_set_buffer_attr pa_stream_set_buffer_attr_dylibloader_wrapper_pulse
#define pa_stream_update_sample_rate pa_stream_update_sample_rate_dylibloader_wrapper_pulse
#define pa_stream_proplist_update pa_stream_proplist_update_dylibloader_wrapper_pulse
#define pa_stream_proplist_remove pa_stream_proplist_remove_dylibloader_wrapper_pulse
#define pa_stream_set_monitor_stream pa_stream_set_monitor_stream_dylibloader_wrapper_pulse
#define pa_stream_get_monitor_stream pa_stream_get_monitor_stream_dylibloader_wrapper_pulse
#define pa_context_get_sink_info_by_name pa_context_get_sink_info_by_name_dylibloader_wrapper_pulse
#define pa_context_get_sink_info_by_index pa_context_get_sink_info_by_index_dylibloader_wrapper_pulse
#define pa_context_get_sink_info_list pa_context_get_sink_info_list_dylibloader_wrapper_pulse
#define pa_context_set_sink_volume_by_index pa_context_set_sink_volume_by_index_dylibloader_wrapper_pulse
#define pa_context_set_sink_volume_by_name pa_context_set_sink_volume_by_name_dylibloader_wrapper_pulse
#define pa_context_set_sink_mute_by_index pa_context_set_sink_mute_by_index_dylibloader_wrapper_pulse
#define pa_context_set_sink_mute_by_name pa_context_set_sink_mute_by_name_dylibloader_wrapper_pulse
#define pa_context_suspend_sink_by_name pa_context_suspend_sink_by_name_dylibloader_wrapper_pulse
#define pa_context_suspend_sink_by_index pa_context_suspend_sink_by_index_dylibloader_wrapper_pulse
#define pa_context_set_sink_port_by_index pa_context_set_sink_port_by_index_dylibloader_wrapper_pulse
#define pa_context_set_sink_port_by_name pa_context_set_sink_port_by_name_dylibloader_wrapper_pulse
#define pa_context_get_source_info_by_name pa_context_get_source_info_by_name_dylibloader_wrapper_pulse
#define pa_context_get_source_info_by_index pa_context_get_source_info_by_index_dylibloader_wrapper_pulse
#define pa_context_get_source_info_list pa_context_get_source_info_list_dylibloader_wrapper_pulse
#define pa_context_set_source_volume_by_index pa_context_set_source_volume_by_index_dylibloader_wrapper_pulse
#define pa_context_set_source_volume_by_name pa_context_set_source_volume_by_name_dylibloader_wrapper_pulse
#define pa_context_set_source_mute_by_index pa_context_set_source_mute_by_index_dylibloader_wrapper_pulse
#define pa_context_set_source_mute_by_name pa_context_set_source_mute_by_name_dylibloader_wrapper_pulse
#define pa_context_suspend_source_by_name pa_context_suspend_source_by_name_dylibloader_wrapper_pulse
#define pa_context_suspend_source_by_index pa_context_suspend_source_by_index_dylibloader_wrapper_pulse
#define pa_context_set_source_port_by_index pa_context_set_source_port_by_index_dylibloader_wrapper_pulse
#define pa_context_set_source_port_by_name pa_context_set_source_port_by_name_dylibloader_wrapper_pulse
#define pa_context_get_server_info pa_context_get_server_info_dylibloader_wrapper_pulse
#define pa_context_get_module_info pa_context_get_module_info_dylibloader_wrapper_pulse
#define pa_context_get_module_info_list pa_context_get_module_info_list_dylibloader_wrapper_pulse
#define pa_context_load_module pa_context_load_module_dylibloader_wrapper_pulse
#define pa_context_unload_module pa_context_unload_module_dylibloader_wrapper_pulse
#define pa_context_get_client_info pa_context_get_client_info_dylibloader_wrapper_pulse
#define pa_context_get_client_info_list pa_context_get_client_info_list_dylibloader_wrapper_pulse
#define pa_context_kill_client pa_context_kill_client_dylibloader_wrapper_pulse
#define pa_context_get_card_info_by_index pa_context_get_card_info_by_index_dylibloader_wrapper_pulse
#define pa_context_get_card_info_by_name pa_context_get_card_info_by_name_dylibloader_wrapper_pulse
#define pa_context_get_card_info_list pa_context_get_card_info_list_dylibloader_wrapper_pulse
#define pa_context_set_card_profile_by_index pa_context_set_card_profile_by_index_dylibloader_wrapper_pulse
#define pa_context_set_card_profile_by_name pa_context_set_card_profile_by_name_dylibloader_wrapper_pulse
#define pa_context_set_port_latency_offset pa_context_set_port_latency_offset_dylibloader_wrapper_pulse
#define pa_context_get_sink_input_info pa_context_get_sink_input_info_dylibloader_wrapper_pulse
#define pa_context_get_sink_input_info_list pa_context_get_sink_input_info_list_dylibloader_wrapper_pulse
#define pa_context_move_sink_input_by_name pa_context_move_sink_input_by_name_dylibloader_wrapper_pulse
#define pa_context_move_sink_input_by_index pa_context_move_sink_input_by_index_dylibloader_wrapper_pulse
#define pa_context_set_sink_input_volume pa_context_set_sink_input_volume_dylibloader_wrapper_pulse
#define pa_context_set_sink_input_mute pa_context_set_sink_input_mute_dylibloader_wrapper_pulse
#define pa_context_kill_sink_input pa_context_kill_sink_input_dylibloader_wrapper_pulse
#define pa_context_get_source_output_info pa_context_get_source_output_info_dylibloader_wrapper_pulse
#define pa_context_get_source_output_info_list pa_context_get_source_output_info_list_dylibloader_wrapper_pulse
#define pa_context_move_source_output_by_name pa_context_move_source_output_by_name_dylibloader_wrapper_pulse
#define pa_context_move_source_output_by_index pa_context_move_source_output_by_index_dylibloader_wrapper_pulse
#define pa_context_set_source_output_volume pa_context_set_source_output_volume_dylibloader_wrapper_pulse
#define pa_context_set_source_output_mute pa_context_set_source_output_mute_dylibloader_wrapper_pulse
#define pa_context_kill_source_output pa_context_kill_source_output_dylibloader_wrapper_pulse
#define pa_context_stat pa_context_stat_dylibloader_wrapper_pulse
#define pa_context_get_sample_info_by_name pa_context_get_sample_info_by_name_dylibloader_wrapper_pulse
#define pa_context_get_sample_info_by_index pa_context_get_sample_info_by_index_dylibloader_wrapper_pulse
#define pa_context_get_sample_info_list pa_context_get_sample_info_list_dylibloader_wrapper_pulse
#define pa_context_get_autoload_info_by_name pa_context_get_autoload_info_by_name_dylibloader_wrapper_pulse
#define pa_context_get_autoload_info_by_index pa_context_get_autoload_info_by_index_dylibloader_wrapper_pulse
#define pa_context_get_autoload_info_list pa_context_get_autoload_info_list_dylibloader_wrapper_pulse
#define pa_context_add_autoload pa_context_add_autoload_dylibloader_wrapper_pulse
#define pa_context_remove_autoload_by_name pa_context_remove_autoload_by_name_dylibloader_wrapper_pulse
#define pa_context_remove_autoload_by_index pa_context_remove_autoload_by_index_dylibloader_wrapper_pulse
#define pa_context_subscribe pa_context_subscribe_dylibloader_wrapper_pulse
#define pa_context_set_subscribe_callback pa_context_set_subscribe_callback_dylibloader_wrapper_pulse
#define pa_stream_connect_upload pa_stream_connect_upload_dylibloader_wrapper_pulse
#define pa_stream_finish_upload pa_stream_finish_upload_dylibloader_wrapper_pulse
#define pa_context_remove_sample pa_context_remove_sample_dylibloader_wrapper_pulse
#define pa_context_play_sample pa_context_play_sample_dylibloader_wrapper_pulse
#define pa_context_play_sample_with_proplist pa_context_play_sample_with_proplist_dylibloader_wrapper_pulse
#define pa_strerror pa_strerror_dylibloader_wrapper_pulse
#define pa_xmalloc pa_xmalloc_dylibloader_wrapper_pulse
#define pa_xmalloc0 pa_xmalloc0_dylibloader_wrapper_pulse
#define pa_xrealloc pa_xrealloc_dylibloader_wrapper_pulse
#define pa_xfree pa_xfree_dylibloader_wrapper_pulse
#define pa_xstrdup pa_xstrdup_dylibloader_wrapper_pulse
#define pa_xstrndup pa_xstrndup_dylibloader_wrapper_pulse
#define pa_xmemdup pa_xmemdup_dylibloader_wrapper_pulse
#define pa_utf8_valid pa_utf8_valid_dylibloader_wrapper_pulse
#define pa_ascii_valid pa_ascii_valid_dylibloader_wrapper_pulse
#define pa_utf8_filter pa_utf8_filter_dylibloader_wrapper_pulse
#define pa_ascii_filter pa_ascii_filter_dylibloader_wrapper_pulse
#define pa_utf8_to_locale pa_utf8_to_locale_dylibloader_wrapper_pulse
#define pa_locale_to_utf8 pa_locale_to_utf8_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_new pa_threaded_mainloop_new_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_free pa_threaded_mainloop_free_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_start pa_threaded_mainloop_start_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_stop pa_threaded_mainloop_stop_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_lock pa_threaded_mainloop_lock_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_unlock pa_threaded_mainloop_unlock_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_wait pa_threaded_mainloop_wait_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_signal pa_threaded_mainloop_signal_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_accept pa_threaded_mainloop_accept_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_get_retval pa_threaded_mainloop_get_retval_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_get_api pa_threaded_mainloop_get_api_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_in_thread pa_threaded_mainloop_in_thread_dylibloader_wrapper_pulse
#define pa_threaded_mainloop_set_name pa_threaded_mainloop_set_name_dylibloader_wrapper_pulse
#define pa_mainloop_new pa_mainloop_new_dylibloader_wrapper_pulse
#define pa_mainloop_free pa_mainloop_free_dylibloader_wrapper_pulse
#define pa_mainloop_prepare pa_mainloop_prepare_dylibloader_wrapper_pulse
#define pa_mainloop_poll pa_mainloop_poll_dylibloader_wrapper_pulse
#define pa_mainloop_dispatch pa_mainloop_dispatch_dylibloader_wrapper_pulse
#define pa_mainloop_get_retval pa_mainloop_get_retval_dylibloader_wrapper_pulse
#define pa_mainloop_iterate pa_mainloop_iterate_dylibloader_wrapper_pulse
#define pa_mainloop_run pa_mainloop_run_dylibloader_wrapper_pulse
#define pa_mainloop_get_api pa_mainloop_get_api_dylibloader_wrapper_pulse
#define pa_mainloop_quit pa_mainloop_quit_dylibloader_wrapper_pulse
#define pa_mainloop_wakeup pa_mainloop_wakeup_dylibloader_wrapper_pulse
#define pa_mainloop_set_poll_func pa_mainloop_set_poll_func_dylibloader_wrapper_pulse
#define pa_signal_init pa_signal_init_dylibloader_wrapper_pulse
#define pa_signal_done pa_signal_done_dylibloader_wrapper_pulse
#define pa_signal_new pa_signal_new_dylibloader_wrapper_pulse
#define pa_signal_free pa_signal_free_dylibloader_wrapper_pulse
#define pa_signal_set_destroy pa_signal_set_destroy_dylibloader_wrapper_pulse
#define pa_get_user_name pa_get_user_name_dylibloader_wrapper_pulse
#define pa_get_host_name pa_get_host_name_dylibloader_wrapper_pulse
#define pa_get_fqdn pa_get_fqdn_dylibloader_wrapper_pulse
#define pa_get_home_dir pa_get_home_dir_dylibloader_wrapper_pulse
#define pa_get_binary_name pa_get_binary_name_dylibloader_wrapper_pulse
#define pa_path_get_filename pa_path_get_filename_dylibloader_wrapper_pulse
#define pa_msleep pa_msleep_dylibloader_wrapper_pulse
#define pa_gettimeofday pa_gettimeofday_dylibloader_wrapper_pulse
#define pa_timeval_diff pa_timeval_diff_dylibloader_wrapper_pulse
#define pa_timeval_cmp pa_timeval_cmp_dylibloader_wrapper_pulse
#define pa_timeval_age pa_timeval_age_dylibloader_wrapper_pulse
#define pa_timeval_add pa_timeval_add_dylibloader_wrapper_pulse
#define pa_timeval_sub pa_timeval_sub_dylibloader_wrapper_pulse
#define pa_timeval_store pa_timeval_store_dylibloader_wrapper_pulse
#define pa_timeval_load pa_timeval_load_dylibloader_wrapper_pulse
#define pa_rtclock_now pa_rtclock_now_dylibloader_wrapper_pulse
extern const char* (*pa_get_library_version_dylibloader_wrapper_pulse)( void);
extern size_t (*pa_bytes_per_second_dylibloader_wrapper_pulse)(const pa_sample_spec*);
extern size_t (*pa_frame_size_dylibloader_wrapper_pulse)(const pa_sample_spec*);
extern size_t (*pa_sample_size_dylibloader_wrapper_pulse)(const pa_sample_spec*);
extern size_t (*pa_sample_size_of_format_dylibloader_wrapper_pulse)( pa_sample_format_t);
extern pa_usec_t (*pa_bytes_to_usec_dylibloader_wrapper_pulse)( uint64_t,const pa_sample_spec*);
extern size_t (*pa_usec_to_bytes_dylibloader_wrapper_pulse)( pa_usec_t,const pa_sample_spec*);
extern pa_sample_spec* (*pa_sample_spec_init_dylibloader_wrapper_pulse)( pa_sample_spec*);
extern int (*pa_sample_format_valid_dylibloader_wrapper_pulse)( unsigned);
extern int (*pa_sample_rate_valid_dylibloader_wrapper_pulse)( uint32_t);
extern int (*pa_channels_valid_dylibloader_wrapper_pulse)( uint8_t);
extern int (*pa_sample_spec_valid_dylibloader_wrapper_pulse)(const pa_sample_spec*);
extern int (*pa_sample_spec_equal_dylibloader_wrapper_pulse)(const pa_sample_spec*,const pa_sample_spec*);
extern const char* (*pa_sample_format_to_string_dylibloader_wrapper_pulse)( pa_sample_format_t);
extern pa_sample_format_t (*pa_parse_sample_format_dylibloader_wrapper_pulse)(const char*);
extern char* (*pa_sample_spec_snprint_dylibloader_wrapper_pulse)( char*, size_t,const pa_sample_spec*);
extern char* (*pa_bytes_snprint_dylibloader_wrapper_pulse)( char*, size_t, unsigned);
extern int (*pa_sample_format_is_le_dylibloader_wrapper_pulse)( pa_sample_format_t);
extern int (*pa_sample_format_is_be_dylibloader_wrapper_pulse)( pa_sample_format_t);
extern int (*pa_direction_valid_dylibloader_wrapper_pulse)( pa_direction_t);
extern const char* (*pa_direction_to_string_dylibloader_wrapper_pulse)( pa_direction_t);
extern void (*pa_mainloop_api_once_dylibloader_wrapper_pulse)( pa_mainloop_api*, void*, void*);
extern pa_proplist* (*pa_proplist_new_dylibloader_wrapper_pulse)( void);
extern void (*pa_proplist_free_dylibloader_wrapper_pulse)( pa_proplist*);
extern int (*pa_proplist_key_valid_dylibloader_wrapper_pulse)(const char*);
extern int (*pa_proplist_sets_dylibloader_wrapper_pulse)( pa_proplist*,const char*,const char*);
extern int (*pa_proplist_setp_dylibloader_wrapper_pulse)( pa_proplist*,const char*);
extern int (*pa_proplist_setf_dylibloader_wrapper_pulse)( pa_proplist*,const char*,const char*,...);
extern int (*pa_proplist_set_dylibloader_wrapper_pulse)( pa_proplist*,const char*,const void*, size_t);
extern const char* (*pa_proplist_gets_dylibloader_wrapper_pulse)( pa_proplist*,const char*);
extern int (*pa_proplist_get_dylibloader_wrapper_pulse)( pa_proplist*,const char*,const void**, size_t*);
extern void (*pa_proplist_update_dylibloader_wrapper_pulse)( pa_proplist*, pa_update_mode_t,const pa_proplist*);
extern int (*pa_proplist_unset_dylibloader_wrapper_pulse)( pa_proplist*,const char*);
extern int (*pa_proplist_unset_many_dylibloader_wrapper_pulse)( pa_proplist*,const char* []);
extern const char* (*pa_proplist_iterate_dylibloader_wrapper_pulse)( pa_proplist*, void**);
extern char* (*pa_proplist_to_string_dylibloader_wrapper_pulse)( pa_proplist*);
extern char* (*pa_proplist_to_string_sep_dylibloader_wrapper_pulse)( pa_proplist*,const char*);
extern pa_proplist* (*pa_proplist_from_string_dylibloader_wrapper_pulse)(const char*);
extern int (*pa_proplist_contains_dylibloader_wrapper_pulse)( pa_proplist*,const char*);
extern void (*pa_proplist_clear_dylibloader_wrapper_pulse)( pa_proplist*);
extern pa_proplist* (*pa_proplist_copy_dylibloader_wrapper_pulse)(const pa_proplist*);
extern unsigned (*pa_proplist_size_dylibloader_wrapper_pulse)( pa_proplist*);
extern int (*pa_proplist_isempty_dylibloader_wrapper_pulse)( pa_proplist*);
extern int (*pa_proplist_equal_dylibloader_wrapper_pulse)( pa_proplist*, pa_proplist*);
extern pa_channel_map* (*pa_channel_map_init_dylibloader_wrapper_pulse)( pa_channel_map*);
extern pa_channel_map* (*pa_channel_map_init_mono_dylibloader_wrapper_pulse)( pa_channel_map*);
extern pa_channel_map* (*pa_channel_map_init_stereo_dylibloader_wrapper_pulse)( pa_channel_map*);
extern pa_channel_map* (*pa_channel_map_init_auto_dylibloader_wrapper_pulse)( pa_channel_map*, unsigned, pa_channel_map_def_t);
extern pa_channel_map* (*pa_channel_map_init_extend_dylibloader_wrapper_pulse)( pa_channel_map*, unsigned, pa_channel_map_def_t);
extern const char* (*pa_channel_position_to_string_dylibloader_wrapper_pulse)( pa_channel_position_t);
extern pa_channel_position_t (*pa_channel_position_from_string_dylibloader_wrapper_pulse)(const char*);
extern const char* (*pa_channel_position_to_pretty_string_dylibloader_wrapper_pulse)( pa_channel_position_t);
extern char* (*pa_channel_map_snprint_dylibloader_wrapper_pulse)( char*, size_t,const pa_channel_map*);
extern pa_channel_map* (*pa_channel_map_parse_dylibloader_wrapper_pulse)( pa_channel_map*,const char*);
extern int (*pa_channel_map_equal_dylibloader_wrapper_pulse)(const pa_channel_map*,const pa_channel_map*);
extern int (*pa_channel_map_valid_dylibloader_wrapper_pulse)(const pa_channel_map*);
extern int (*pa_channel_map_compatible_dylibloader_wrapper_pulse)(const pa_channel_map*,const pa_sample_spec*);
extern int (*pa_channel_map_superset_dylibloader_wrapper_pulse)(const pa_channel_map*,const pa_channel_map*);
extern int (*pa_channel_map_can_balance_dylibloader_wrapper_pulse)(const pa_channel_map*);
extern int (*pa_channel_map_can_fade_dylibloader_wrapper_pulse)(const pa_channel_map*);
extern int (*pa_channel_map_can_lfe_balance_dylibloader_wrapper_pulse)(const pa_channel_map*);
extern const char* (*pa_channel_map_to_name_dylibloader_wrapper_pulse)(const pa_channel_map*);
extern const char* (*pa_channel_map_to_pretty_name_dylibloader_wrapper_pulse)(const pa_channel_map*);
extern int (*pa_channel_map_has_position_dylibloader_wrapper_pulse)(const pa_channel_map*, pa_channel_position_t);
extern pa_channel_position_mask_t (*pa_channel_map_mask_dylibloader_wrapper_pulse)(const pa_channel_map*);
extern const char* (*pa_encoding_to_string_dylibloader_wrapper_pulse)( pa_encoding_t);
extern pa_encoding_t (*pa_encoding_from_string_dylibloader_wrapper_pulse)(const char*);
extern pa_format_info* (*pa_format_info_new_dylibloader_wrapper_pulse)( void);
extern pa_format_info* (*pa_format_info_copy_dylibloader_wrapper_pulse)(const pa_format_info*);
extern void (*pa_format_info_free_dylibloader_wrapper_pulse)( pa_format_info*);
extern int (*pa_format_info_valid_dylibloader_wrapper_pulse)(const pa_format_info*);
extern int (*pa_format_info_is_pcm_dylibloader_wrapper_pulse)(const pa_format_info*);
extern int (*pa_format_info_is_compatible_dylibloader_wrapper_pulse)(const pa_format_info*,const pa_format_info*);
extern char* (*pa_format_info_snprint_dylibloader_wrapper_pulse)( char*, size_t,const pa_format_info*);
extern pa_format_info* (*pa_format_info_from_string_dylibloader_wrapper_pulse)(const char*);
extern pa_format_info* (*pa_format_info_from_sample_spec_dylibloader_wrapper_pulse)(const pa_sample_spec*,const pa_channel_map*);
extern int (*pa_format_info_to_sample_spec_dylibloader_wrapper_pulse)(const pa_format_info*, pa_sample_spec*, pa_channel_map*);
extern pa_prop_type_t (*pa_format_info_get_prop_type_dylibloader_wrapper_pulse)(const pa_format_info*,const char*);
extern int (*pa_format_info_get_prop_int_dylibloader_wrapper_pulse)(const pa_format_info*,const char*, int*);
extern int (*pa_format_info_get_prop_int_range_dylibloader_wrapper_pulse)(const pa_format_info*,const char*, int*, int*);
extern int (*pa_format_info_get_prop_int_array_dylibloader_wrapper_pulse)(const pa_format_info*,const char*, int**, int*);
extern int (*pa_format_info_get_prop_string_dylibloader_wrapper_pulse)(const pa_format_info*,const char*, char**);
extern int (*pa_format_info_get_prop_string_array_dylibloader_wrapper_pulse)(const pa_format_info*,const char*, char***, int*);
extern void (*pa_format_info_free_string_array_dylibloader_wrapper_pulse)( char**, int);
extern void (*pa_format_info_set_prop_int_dylibloader_wrapper_pulse)( pa_format_info*,const char*, int);
extern void (*pa_format_info_set_prop_int_array_dylibloader_wrapper_pulse)( pa_format_info*,const char*,const int*, int);
extern void (*pa_format_info_set_prop_int_range_dylibloader_wrapper_pulse)( pa_format_info*,const char*, int, int);
extern void (*pa_format_info_set_prop_string_dylibloader_wrapper_pulse)( pa_format_info*,const char*,const char*);
extern void (*pa_format_info_set_prop_string_array_dylibloader_wrapper_pulse)( pa_format_info*,const char*,const char**, int);
extern void (*pa_format_info_set_sample_format_dylibloader_wrapper_pulse)( pa_format_info*, pa_sample_format_t);
extern void (*pa_format_info_set_rate_dylibloader_wrapper_pulse)( pa_format_info*, int);
extern void (*pa_format_info_set_channels_dylibloader_wrapper_pulse)( pa_format_info*, int);
extern void (*pa_format_info_set_channel_map_dylibloader_wrapper_pulse)( pa_format_info*,const pa_channel_map*);
extern pa_operation* (*pa_operation_ref_dylibloader_wrapper_pulse)( pa_operation*);
extern void (*pa_operation_unref_dylibloader_wrapper_pulse)( pa_operation*);
extern void (*pa_operation_cancel_dylibloader_wrapper_pulse)( pa_operation*);
extern pa_operation_state_t (*pa_operation_get_state_dylibloader_wrapper_pulse)( pa_operation*);
extern void (*pa_operation_set_state_callback_dylibloader_wrapper_pulse)( pa_operation*, pa_operation_notify_cb_t, void*);
extern pa_context* (*pa_context_new_dylibloader_wrapper_pulse)( pa_mainloop_api*,const char*);
extern pa_context* (*pa_context_new_with_proplist_dylibloader_wrapper_pulse)( pa_mainloop_api*,const char*, pa_proplist*);
extern void (*pa_context_unref_dylibloader_wrapper_pulse)( pa_context*);
extern pa_context* (*pa_context_ref_dylibloader_wrapper_pulse)( pa_context*);
extern void (*pa_context_set_state_callback_dylibloader_wrapper_pulse)( pa_context*, pa_context_notify_cb_t, void*);
extern void (*pa_context_set_event_callback_dylibloader_wrapper_pulse)( pa_context*, pa_context_event_cb_t, void*);
extern int (*pa_context_errno_dylibloader_wrapper_pulse)( pa_context*);
extern int (*pa_context_is_pending_dylibloader_wrapper_pulse)( pa_context*);
extern pa_context_state_t (*pa_context_get_state_dylibloader_wrapper_pulse)( pa_context*);
extern int (*pa_context_connect_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_context_flags_t,const pa_spawn_api*);
extern void (*pa_context_disconnect_dylibloader_wrapper_pulse)( pa_context*);
extern pa_operation* (*pa_context_drain_dylibloader_wrapper_pulse)( pa_context*, pa_context_notify_cb_t, void*);
extern pa_operation* (*pa_context_exit_daemon_dylibloader_wrapper_pulse)( pa_context*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_default_sink_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_default_source_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_context_success_cb_t, void*);
extern int (*pa_context_is_local_dylibloader_wrapper_pulse)( pa_context*);
extern pa_operation* (*pa_context_set_name_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_context_success_cb_t, void*);
extern const char* (*pa_context_get_server_dylibloader_wrapper_pulse)( pa_context*);
extern uint32_t (*pa_context_get_protocol_version_dylibloader_wrapper_pulse)( pa_context*);
extern uint32_t (*pa_context_get_server_protocol_version_dylibloader_wrapper_pulse)( pa_context*);
extern pa_operation* (*pa_context_proplist_update_dylibloader_wrapper_pulse)( pa_context*, pa_update_mode_t, pa_proplist*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_proplist_remove_dylibloader_wrapper_pulse)( pa_context*,const char* [], pa_context_success_cb_t, void*);
extern uint32_t (*pa_context_get_index_dylibloader_wrapper_pulse)( pa_context*);
extern pa_time_event* (*pa_context_rttime_new_dylibloader_wrapper_pulse)( pa_context*, pa_usec_t, pa_time_event_cb_t, void*);
extern void (*pa_context_rttime_restart_dylibloader_wrapper_pulse)( pa_context*, pa_time_event*, pa_usec_t);
extern size_t (*pa_context_get_tile_size_dylibloader_wrapper_pulse)( pa_context*,const pa_sample_spec*);
extern int (*pa_context_load_cookie_from_file_dylibloader_wrapper_pulse)( pa_context*,const char*);
extern int (*pa_cvolume_equal_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_cvolume*);
extern pa_cvolume* (*pa_cvolume_init_dylibloader_wrapper_pulse)( pa_cvolume*);
extern pa_cvolume* (*pa_cvolume_set_dylibloader_wrapper_pulse)( pa_cvolume*, unsigned, pa_volume_t);
extern char* (*pa_cvolume_snprint_dylibloader_wrapper_pulse)( char*, size_t,const pa_cvolume*);
extern char* (*pa_sw_cvolume_snprint_dB_dylibloader_wrapper_pulse)( char*, size_t,const pa_cvolume*);
extern char* (*pa_cvolume_snprint_verbose_dylibloader_wrapper_pulse)( char*, size_t,const pa_cvolume*,const pa_channel_map*, int);
extern char* (*pa_volume_snprint_dylibloader_wrapper_pulse)( char*, size_t, pa_volume_t);
extern char* (*pa_sw_volume_snprint_dB_dylibloader_wrapper_pulse)( char*, size_t, pa_volume_t);
extern char* (*pa_volume_snprint_verbose_dylibloader_wrapper_pulse)( char*, size_t, pa_volume_t, int);
extern pa_volume_t (*pa_cvolume_avg_dylibloader_wrapper_pulse)(const pa_cvolume*);
extern pa_volume_t (*pa_cvolume_avg_mask_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_channel_map*, pa_channel_position_mask_t);
extern pa_volume_t (*pa_cvolume_max_dylibloader_wrapper_pulse)(const pa_cvolume*);
extern pa_volume_t (*pa_cvolume_max_mask_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_channel_map*, pa_channel_position_mask_t);
extern pa_volume_t (*pa_cvolume_min_dylibloader_wrapper_pulse)(const pa_cvolume*);
extern pa_volume_t (*pa_cvolume_min_mask_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_channel_map*, pa_channel_position_mask_t);
extern int (*pa_cvolume_valid_dylibloader_wrapper_pulse)(const pa_cvolume*);
extern int (*pa_cvolume_channels_equal_to_dylibloader_wrapper_pulse)(const pa_cvolume*, pa_volume_t);
extern pa_volume_t (*pa_sw_volume_multiply_dylibloader_wrapper_pulse)( pa_volume_t, pa_volume_t);
extern pa_cvolume* (*pa_sw_cvolume_multiply_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_cvolume*,const pa_cvolume*);
extern pa_cvolume* (*pa_sw_cvolume_multiply_scalar_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_cvolume*, pa_volume_t);
extern pa_volume_t (*pa_sw_volume_divide_dylibloader_wrapper_pulse)( pa_volume_t, pa_volume_t);
extern pa_cvolume* (*pa_sw_cvolume_divide_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_cvolume*,const pa_cvolume*);
extern pa_cvolume* (*pa_sw_cvolume_divide_scalar_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_cvolume*, pa_volume_t);
extern pa_volume_t (*pa_sw_volume_from_dB_dylibloader_wrapper_pulse)( double);
extern double (*pa_sw_volume_to_dB_dylibloader_wrapper_pulse)( pa_volume_t);
extern pa_volume_t (*pa_sw_volume_from_linear_dylibloader_wrapper_pulse)( double);
extern double (*pa_sw_volume_to_linear_dylibloader_wrapper_pulse)( pa_volume_t);
extern pa_cvolume* (*pa_cvolume_remap_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_channel_map*,const pa_channel_map*);
extern int (*pa_cvolume_compatible_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_sample_spec*);
extern int (*pa_cvolume_compatible_with_channel_map_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_channel_map*);
extern float (*pa_cvolume_get_balance_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_channel_map*);
extern pa_cvolume* (*pa_cvolume_set_balance_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_channel_map*, float);
extern float (*pa_cvolume_get_fade_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_channel_map*);
extern pa_cvolume* (*pa_cvolume_set_fade_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_channel_map*, float);
extern float (*pa_cvolume_get_lfe_balance_dylibloader_wrapper_pulse)(const pa_cvolume*,const pa_channel_map*);
extern pa_cvolume* (*pa_cvolume_set_lfe_balance_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_channel_map*, float);
extern pa_cvolume* (*pa_cvolume_scale_dylibloader_wrapper_pulse)( pa_cvolume*, pa_volume_t);
extern pa_cvolume* (*pa_cvolume_scale_mask_dylibloader_wrapper_pulse)( pa_cvolume*, pa_volume_t, pa_channel_map*, pa_channel_position_mask_t);
extern pa_cvolume* (*pa_cvolume_set_position_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_channel_map*, pa_channel_position_t, pa_volume_t);
extern pa_volume_t (*pa_cvolume_get_position_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_channel_map*, pa_channel_position_t);
extern pa_cvolume* (*pa_cvolume_merge_dylibloader_wrapper_pulse)( pa_cvolume*,const pa_cvolume*,const pa_cvolume*);
extern pa_cvolume* (*pa_cvolume_inc_clamp_dylibloader_wrapper_pulse)( pa_cvolume*, pa_volume_t, pa_volume_t);
extern pa_cvolume* (*pa_cvolume_inc_dylibloader_wrapper_pulse)( pa_cvolume*, pa_volume_t);
extern pa_cvolume* (*pa_cvolume_dec_dylibloader_wrapper_pulse)( pa_cvolume*, pa_volume_t);
extern pa_stream* (*pa_stream_new_dylibloader_wrapper_pulse)( pa_context*,const char*,const pa_sample_spec*,const pa_channel_map*);
extern pa_stream* (*pa_stream_new_with_proplist_dylibloader_wrapper_pulse)( pa_context*,const char*,const pa_sample_spec*,const pa_channel_map*, pa_proplist*);
extern pa_stream* (*pa_stream_new_extended_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_format_info**, unsigned int, pa_proplist*);
extern void (*pa_stream_unref_dylibloader_wrapper_pulse)( pa_stream*);
extern pa_stream* (*pa_stream_ref_dylibloader_wrapper_pulse)( pa_stream*);
extern pa_stream_state_t (*pa_stream_get_state_dylibloader_wrapper_pulse)( pa_stream*);
extern pa_context* (*pa_stream_get_context_dylibloader_wrapper_pulse)( pa_stream*);
extern uint32_t (*pa_stream_get_index_dylibloader_wrapper_pulse)( pa_stream*);
extern uint32_t (*pa_stream_get_device_index_dylibloader_wrapper_pulse)( pa_stream*);
extern const char* (*pa_stream_get_device_name_dylibloader_wrapper_pulse)( pa_stream*);
extern int (*pa_stream_is_suspended_dylibloader_wrapper_pulse)( pa_stream*);
extern int (*pa_stream_is_corked_dylibloader_wrapper_pulse)( pa_stream*);
extern int (*pa_stream_connect_playback_dylibloader_wrapper_pulse)( pa_stream*,const char*,const pa_buffer_attr*, pa_stream_flags_t,const pa_cvolume*, pa_stream*);
extern int (*pa_stream_connect_record_dylibloader_wrapper_pulse)( pa_stream*,const char*,const pa_buffer_attr*, pa_stream_flags_t);
extern int (*pa_stream_disconnect_dylibloader_wrapper_pulse)( pa_stream*);
extern int (*pa_stream_begin_write_dylibloader_wrapper_pulse)( pa_stream*, void**, size_t*);
extern int (*pa_stream_cancel_write_dylibloader_wrapper_pulse)( pa_stream*);
extern int (*pa_stream_write_dylibloader_wrapper_pulse)( pa_stream*,const void*, size_t, pa_free_cb_t, int64_t, pa_seek_mode_t);
extern int (*pa_stream_write_ext_free_dylibloader_wrapper_pulse)( pa_stream*,const void*, size_t, pa_free_cb_t, void*, int64_t, pa_seek_mode_t);
extern int (*pa_stream_peek_dylibloader_wrapper_pulse)( pa_stream*,const void**, size_t*);
extern int (*pa_stream_drop_dylibloader_wrapper_pulse)( pa_stream*);
extern size_t (*pa_stream_writable_size_dylibloader_wrapper_pulse)( pa_stream*);
extern size_t (*pa_stream_readable_size_dylibloader_wrapper_pulse)( pa_stream*);
extern pa_operation* (*pa_stream_drain_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_success_cb_t, void*);
extern pa_operation* (*pa_stream_update_timing_info_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_success_cb_t, void*);
extern void (*pa_stream_set_state_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_notify_cb_t, void*);
extern void (*pa_stream_set_write_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_request_cb_t, void*);
extern void (*pa_stream_set_read_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_request_cb_t, void*);
extern void (*pa_stream_set_overflow_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_notify_cb_t, void*);
extern int64_t (*pa_stream_get_underflow_index_dylibloader_wrapper_pulse)( pa_stream*);
extern void (*pa_stream_set_underflow_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_notify_cb_t, void*);
extern void (*pa_stream_set_started_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_notify_cb_t, void*);
extern void (*pa_stream_set_latency_update_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_notify_cb_t, void*);
extern void (*pa_stream_set_moved_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_notify_cb_t, void*);
extern void (*pa_stream_set_suspended_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_notify_cb_t, void*);
extern void (*pa_stream_set_event_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_event_cb_t, void*);
extern void (*pa_stream_set_buffer_attr_callback_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_notify_cb_t, void*);
extern pa_operation* (*pa_stream_cork_dylibloader_wrapper_pulse)( pa_stream*, int, pa_stream_success_cb_t, void*);
extern pa_operation* (*pa_stream_flush_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_success_cb_t, void*);
extern pa_operation* (*pa_stream_prebuf_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_success_cb_t, void*);
extern pa_operation* (*pa_stream_trigger_dylibloader_wrapper_pulse)( pa_stream*, pa_stream_success_cb_t, void*);
extern pa_operation* (*pa_stream_set_name_dylibloader_wrapper_pulse)( pa_stream*,const char*, pa_stream_success_cb_t, void*);
extern int (*pa_stream_get_time_dylibloader_wrapper_pulse)( pa_stream*, pa_usec_t*);
extern int (*pa_stream_get_latency_dylibloader_wrapper_pulse)( pa_stream*, pa_usec_t*, int*);
extern const pa_timing_info* (*pa_stream_get_timing_info_dylibloader_wrapper_pulse)( pa_stream*);
extern const pa_sample_spec* (*pa_stream_get_sample_spec_dylibloader_wrapper_pulse)( pa_stream*);
extern const pa_channel_map* (*pa_stream_get_channel_map_dylibloader_wrapper_pulse)( pa_stream*);
extern const pa_format_info* (*pa_stream_get_format_info_dylibloader_wrapper_pulse)( pa_stream*);
extern const pa_buffer_attr* (*pa_stream_get_buffer_attr_dylibloader_wrapper_pulse)( pa_stream*);
extern pa_operation* (*pa_stream_set_buffer_attr_dylibloader_wrapper_pulse)( pa_stream*,const pa_buffer_attr*, pa_stream_success_cb_t, void*);
extern pa_operation* (*pa_stream_update_sample_rate_dylibloader_wrapper_pulse)( pa_stream*, uint32_t, pa_stream_success_cb_t, void*);
extern pa_operation* (*pa_stream_proplist_update_dylibloader_wrapper_pulse)( pa_stream*, pa_update_mode_t, pa_proplist*, pa_stream_success_cb_t, void*);
extern pa_operation* (*pa_stream_proplist_remove_dylibloader_wrapper_pulse)( pa_stream*,const char* [], pa_stream_success_cb_t, void*);
extern int (*pa_stream_set_monitor_stream_dylibloader_wrapper_pulse)( pa_stream*, uint32_t);
extern uint32_t (*pa_stream_get_monitor_stream_dylibloader_wrapper_pulse)( pa_stream*);
extern pa_operation* (*pa_context_get_sink_info_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_sink_info_cb_t, void*);
extern pa_operation* (*pa_context_get_sink_info_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_sink_info_cb_t, void*);
extern pa_operation* (*pa_context_get_sink_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_sink_info_cb_t, void*);
extern pa_operation* (*pa_context_set_sink_volume_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const pa_cvolume*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_sink_volume_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*,const pa_cvolume*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_sink_mute_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_sink_mute_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_suspend_sink_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_suspend_sink_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_sink_port_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_sink_port_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_get_source_info_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_source_info_cb_t, void*);
extern pa_operation* (*pa_context_get_source_info_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_source_info_cb_t, void*);
extern pa_operation* (*pa_context_get_source_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_source_info_cb_t, void*);
extern pa_operation* (*pa_context_set_source_volume_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const pa_cvolume*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_source_volume_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*,const pa_cvolume*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_source_mute_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_source_mute_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_suspend_source_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_suspend_source_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_source_port_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_source_port_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_get_server_info_dylibloader_wrapper_pulse)( pa_context*, pa_server_info_cb_t, void*);
extern pa_operation* (*pa_context_get_module_info_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_module_info_cb_t, void*);
extern pa_operation* (*pa_context_get_module_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_module_info_cb_t, void*);
extern pa_operation* (*pa_context_load_module_dylibloader_wrapper_pulse)( pa_context*,const char*,const char*, pa_context_index_cb_t, void*);
extern pa_operation* (*pa_context_unload_module_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_get_client_info_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_client_info_cb_t, void*);
extern pa_operation* (*pa_context_get_client_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_client_info_cb_t, void*);
extern pa_operation* (*pa_context_kill_client_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_get_card_info_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_card_info_cb_t, void*);
extern pa_operation* (*pa_context_get_card_info_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_card_info_cb_t, void*);
extern pa_operation* (*pa_context_get_card_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_card_info_cb_t, void*);
extern pa_operation* (*pa_context_set_card_profile_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_card_profile_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_port_latency_offset_dylibloader_wrapper_pulse)( pa_context*,const char*,const char*, int64_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_get_sink_input_info_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_sink_input_info_cb_t, void*);
extern pa_operation* (*pa_context_get_sink_input_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_sink_input_info_cb_t, void*);
extern pa_operation* (*pa_context_move_sink_input_by_name_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_move_sink_input_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, uint32_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_sink_input_volume_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const pa_cvolume*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_sink_input_mute_dylibloader_wrapper_pulse)( pa_context*, uint32_t, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_kill_sink_input_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_get_source_output_info_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_source_output_info_cb_t, void*);
extern pa_operation* (*pa_context_get_source_output_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_source_output_info_cb_t, void*);
extern pa_operation* (*pa_context_move_source_output_by_name_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_move_source_output_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, uint32_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_source_output_volume_dylibloader_wrapper_pulse)( pa_context*, uint32_t,const pa_cvolume*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_set_source_output_mute_dylibloader_wrapper_pulse)( pa_context*, uint32_t, int, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_kill_source_output_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_stat_dylibloader_wrapper_pulse)( pa_context*, pa_stat_info_cb_t, void*);
extern pa_operation* (*pa_context_get_sample_info_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_sample_info_cb_t, void*);
extern pa_operation* (*pa_context_get_sample_info_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_sample_info_cb_t, void*);
extern pa_operation* (*pa_context_get_sample_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_sample_info_cb_t, void*);
extern pa_operation* (*pa_context_get_autoload_info_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_autoload_type_t, pa_autoload_info_cb_t, void*);
extern pa_operation* (*pa_context_get_autoload_info_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_autoload_info_cb_t, void*);
extern pa_operation* (*pa_context_get_autoload_info_list_dylibloader_wrapper_pulse)( pa_context*, pa_autoload_info_cb_t, void*);
extern pa_operation* (*pa_context_add_autoload_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_autoload_type_t,const char*,const char*, pa_context_index_cb_t, void*);
extern pa_operation* (*pa_context_remove_autoload_by_name_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_autoload_type_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_remove_autoload_by_index_dylibloader_wrapper_pulse)( pa_context*, uint32_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_subscribe_dylibloader_wrapper_pulse)( pa_context*, pa_subscription_mask_t, pa_context_success_cb_t, void*);
extern void (*pa_context_set_subscribe_callback_dylibloader_wrapper_pulse)( pa_context*, pa_context_subscribe_cb_t, void*);
extern int (*pa_stream_connect_upload_dylibloader_wrapper_pulse)( pa_stream*, size_t);
extern int (*pa_stream_finish_upload_dylibloader_wrapper_pulse)( pa_stream*);
extern pa_operation* (*pa_context_remove_sample_dylibloader_wrapper_pulse)( pa_context*,const char*, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_play_sample_dylibloader_wrapper_pulse)( pa_context*,const char*,const char*, pa_volume_t, pa_context_success_cb_t, void*);
extern pa_operation* (*pa_context_play_sample_with_proplist_dylibloader_wrapper_pulse)( pa_context*,const char*,const char*, pa_volume_t, pa_proplist*, pa_context_play_sample_cb_t, void*);
extern const char* (*pa_strerror_dylibloader_wrapper_pulse)( int);
extern void* (*pa_xmalloc_dylibloader_wrapper_pulse)( size_t);
extern void* (*pa_xmalloc0_dylibloader_wrapper_pulse)( size_t);
extern void* (*pa_xrealloc_dylibloader_wrapper_pulse)( void*, size_t);
extern void (*pa_xfree_dylibloader_wrapper_pulse)( void*);
extern char* (*pa_xstrdup_dylibloader_wrapper_pulse)(const char*);
extern char* (*pa_xstrndup_dylibloader_wrapper_pulse)(const char*, size_t);
extern void* (*pa_xmemdup_dylibloader_wrapper_pulse)(const void*, size_t);
extern char* (*pa_utf8_valid_dylibloader_wrapper_pulse)(const char*);
extern char* (*pa_ascii_valid_dylibloader_wrapper_pulse)(const char*);
extern char* (*pa_utf8_filter_dylibloader_wrapper_pulse)(const char*);
extern char* (*pa_ascii_filter_dylibloader_wrapper_pulse)(const char*);
extern char* (*pa_utf8_to_locale_dylibloader_wrapper_pulse)(const char*);
extern char* (*pa_locale_to_utf8_dylibloader_wrapper_pulse)(const char*);
extern pa_threaded_mainloop* (*pa_threaded_mainloop_new_dylibloader_wrapper_pulse)( void);
extern void (*pa_threaded_mainloop_free_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern int (*pa_threaded_mainloop_start_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern void (*pa_threaded_mainloop_stop_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern void (*pa_threaded_mainloop_lock_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern void (*pa_threaded_mainloop_unlock_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern void (*pa_threaded_mainloop_wait_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern void (*pa_threaded_mainloop_signal_dylibloader_wrapper_pulse)( pa_threaded_mainloop*, int);
extern void (*pa_threaded_mainloop_accept_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern int (*pa_threaded_mainloop_get_retval_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern pa_mainloop_api* (*pa_threaded_mainloop_get_api_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern int (*pa_threaded_mainloop_in_thread_dylibloader_wrapper_pulse)( pa_threaded_mainloop*);
extern void (*pa_threaded_mainloop_set_name_dylibloader_wrapper_pulse)( pa_threaded_mainloop*,const char*);
extern pa_mainloop* (*pa_mainloop_new_dylibloader_wrapper_pulse)( void);
extern void (*pa_mainloop_free_dylibloader_wrapper_pulse)( pa_mainloop*);
extern int (*pa_mainloop_prepare_dylibloader_wrapper_pulse)( pa_mainloop*, int);
extern int (*pa_mainloop_poll_dylibloader_wrapper_pulse)( pa_mainloop*);
extern int (*pa_mainloop_dispatch_dylibloader_wrapper_pulse)( pa_mainloop*);
extern int (*pa_mainloop_get_retval_dylibloader_wrapper_pulse)( pa_mainloop*);
extern int (*pa_mainloop_iterate_dylibloader_wrapper_pulse)( pa_mainloop*, int, int*);
extern int (*pa_mainloop_run_dylibloader_wrapper_pulse)( pa_mainloop*, int*);
extern pa_mainloop_api* (*pa_mainloop_get_api_dylibloader_wrapper_pulse)( pa_mainloop*);
extern void (*pa_mainloop_quit_dylibloader_wrapper_pulse)( pa_mainloop*, int);
extern void (*pa_mainloop_wakeup_dylibloader_wrapper_pulse)( pa_mainloop*);
extern void (*pa_mainloop_set_poll_func_dylibloader_wrapper_pulse)( pa_mainloop*, pa_poll_func, void*);
extern int (*pa_signal_init_dylibloader_wrapper_pulse)( pa_mainloop_api*);
extern void (*pa_signal_done_dylibloader_wrapper_pulse)( void);
extern pa_signal_event* (*pa_signal_new_dylibloader_wrapper_pulse)( int, pa_signal_cb_t, void*);
extern void (*pa_signal_free_dylibloader_wrapper_pulse)( pa_signal_event*);
extern void (*pa_signal_set_destroy_dylibloader_wrapper_pulse)( pa_signal_event*, pa_signal_destroy_cb_t);
extern char* (*pa_get_user_name_dylibloader_wrapper_pulse)( char*, size_t);
extern char* (*pa_get_host_name_dylibloader_wrapper_pulse)( char*, size_t);
extern char* (*pa_get_fqdn_dylibloader_wrapper_pulse)( char*, size_t);
extern char* (*pa_get_home_dir_dylibloader_wrapper_pulse)( char*, size_t);
extern char* (*pa_get_binary_name_dylibloader_wrapper_pulse)( char*, size_t);
extern char* (*pa_path_get_filename_dylibloader_wrapper_pulse)(const char*);
extern int (*pa_msleep_dylibloader_wrapper_pulse)( unsigned long);
extern struct timeval* (*pa_gettimeofday_dylibloader_wrapper_pulse)(struct timeval*);
extern pa_usec_t (*pa_timeval_diff_dylibloader_wrapper_pulse)(struct timeval*,struct timeval*);
extern int (*pa_timeval_cmp_dylibloader_wrapper_pulse)(struct timeval*,struct timeval*);
extern pa_usec_t (*pa_timeval_age_dylibloader_wrapper_pulse)(struct timeval*);
extern struct timeval* (*pa_timeval_add_dylibloader_wrapper_pulse)(struct timeval*, pa_usec_t);
extern struct timeval* (*pa_timeval_sub_dylibloader_wrapper_pulse)(struct timeval*, pa_usec_t);
extern struct timeval* (*pa_timeval_store_dylibloader_wrapper_pulse)(struct timeval*, pa_usec_t);
extern pa_usec_t (*pa_timeval_load_dylibloader_wrapper_pulse)(struct timeval*);
extern pa_usec_t (*pa_rtclock_now_dylibloader_wrapper_pulse)( void);
int initialize_pulse(int verbose);
#ifdef __cplusplus
}
#endif
#endif
#else
#include <pulse/pulseaudio.h>
#endif

class AudioDriverPulseAudio : public AudioDriver {
	Thread thread;
	Mutex mutex;

	pa_mainloop *pa_ml = nullptr;
	pa_context *pa_ctx = nullptr;
	pa_stream *pa_str = nullptr;
	pa_stream *pa_rec_str = nullptr;
	pa_channel_map pa_map = {};
	pa_channel_map pa_rec_map = {};

	String output_device_name = "Default";
	String new_output_device = "Default";
	String default_output_device;

	String input_device_name;
	String new_input_device;
	String default_input_device;

	Vector<int32_t> samples_in;
	Vector<int16_t> samples_out;

	unsigned int mix_rate = 0;
	unsigned int buffer_frames = 0;
	unsigned int pa_buffer_size = 0;
	int channels = 0;
	int pa_ready = 0;
	int pa_status = 0;
	PackedStringArray pa_devices;
	PackedStringArray pa_rec_devices;

	SafeFlag active;
	SafeFlag exit_thread;

	float latency = 0;

	static void pa_state_cb(pa_context *c, void *userdata);
	static void pa_sink_info_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
	static void pa_source_info_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);
	static void pa_server_info_cb(pa_context *c, const pa_server_info *i, void *userdata);
	static void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
	static void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);

	Error init_output_device();
	void finish_output_device();

	Error init_input_device();
	void finish_input_device();

	Error detect_channels(bool capture = false);

	static void thread_func(void *p_udata);

public:
	virtual const char *get_name() const override {
		return "PulseAudio";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;
	virtual float get_latency() override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

	virtual PackedStringArray get_output_device_list() override;
	virtual String get_output_device() override;
	virtual void set_output_device(const String &p_name) override;

	virtual Error input_start() override;
	virtual Error input_stop() override;

	virtual PackedStringArray get_input_device_list() override;
	virtual String get_input_device() override;
	virtual void set_input_device(const String &p_name) override;

	AudioDriverPulseAudio();
	~AudioDriverPulseAudio() {}
};

#endif // PULSEAUDIO_ENABLED

// --- End drivers/pulseaudio/audio_driver_pulseaudio.h ---


class JoypadSDL;

#ifdef FONTCONFIG_ENABLED
typedef struct _FcConfig FcConfig;
typedef struct _FcObjectSet FcObjectSet;
#endif

class OS_LinuxBSD final
{
public:
	OS_LinuxBSD() = delete;
	~OS_LinuxBSD() = delete;
	OS_LinuxBSD(const OS_LinuxBSD&) = delete;
	OS_LinuxBSD& operator=(const OS_LinuxBSD&) = delete;

private:
#ifdef FONTCONFIG_ENABLED
	static inline bool font_config_initialized = false;
	static inline FcConfig* config = nullptr;
	static inline FcObjectSet* object_set = nullptr;

	static int _weight_to_fc(int p_weight);
	static int _stretch_to_fc(int p_stretch);
#endif

#ifdef SDL_ENABLED
	static inline JoypadSDL* joypad_sdl = nullptr;
#endif

#ifdef ALSA_ENABLED
	static inline AudioDriverALSA driver_alsa;
#endif

#ifdef PULSEAUDIO_ENABLED
	static inline AudioDriverPulseAudio driver_pulseaudio;
#endif

	static inline CrashHandler crash_handler;

	static String get_systemd_os_release_info_value(const String& key);

	static Vector<String> lspci_device_filter(Vector<String> vendor_device_id_mapping,
		String class_suffix, String check_column, String whitelist);
	static Vector<String> lspci_get_device_value(
		Vector<String> vendor_device_id_mapping, String check_column, String blacklist);

	static inline String system_dir_desktop_cache;

public:
	static void initialize();
	static void initialize_core();
	static void finalize();
	static void finalize_core();

	static void initialize_joypads();

	static void delete_main_loop();

	static String get_identifier();
	static String get_name();
	static String get_distribution_name();
	static String get_version();

	static Vector<String> get_video_adapter_driver_info();

	static uint64_t get_embedded_pck_offset();

	static Vector<String> get_system_fonts();
	static String get_system_font_path(const String& p_font_name, int p_weight = 400,
		int p_stretch = 100, bool p_italic = false);
	static Vector<String> get_system_font_path_for_text(const String& p_font_name,
		const String& p_text, const String& p_locale = String(), const String& p_script = String(),
		int p_weight = 400, int p_stretch = 100, bool p_italic = false);

	static String get_config_path();
	static String get_data_path();
	static String get_cache_path();

	static String get_system_dir(OS::SystemDir p_dir, bool p_shared_storage = true);

	static Error shell_open(const String& p_uri);

	static String get_unique_id();
	static String get_processor_name();

	static bool is_sandboxed();

	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	static bool _check_internal_feature_support(const String& p_feature);

	static void run();

	static void disable_crash_handler();
	static bool is_disable_crash_handler();

	static Error move_to_trash(const String& p_path);

	static String get_system_ca_certificates();

#ifdef TOOLS_ENABLED
	static bool _test_create_rendering_device_and_gl(const String& p_display_driver);
	static bool _test_create_rendering_device(const String& p_display_driver);
#endif
};

// --- Folded from platform/linuxbsd/freedesktop_at_spi_monitor.h ---
#ifdef DBUS_ENABLED

#include "core/os/mutex.h"
#include "core/os/memory.h"

class FreeDesktopAtSPIMonitor {
private:
	Thread thread;

	SafeFlag exit_thread;
	SafeFlag ac_enabled;
	SafeFlag sr_enabled;
	SafeFlag supported;

	static void monitor_thread_func(void *p_userdata);

public:
	FreeDesktopAtSPIMonitor();
	~FreeDesktopAtSPIMonitor();

	bool is_supported() { return supported.is_set(); }
	bool is_active() { return sr_enabled.is_set() && ac_enabled.is_set(); }
};

#endif // DBUS_ENABLED

// --- End platform/linuxbsd/freedesktop_at_spi_monitor.h ---

// --- Folded from platform/linuxbsd/freedesktop_portal_desktop.h ---
#ifdef DBUS_ENABLED

#include "core/os/mutex.h"
#include "core/os/memory.h"
#include "servers/display/display_server.h"
#include "core/io/ip.h"

struct DBusMessage;
struct DBusConnection;
struct DBusMessageIter;

class FreeDesktopPortalDesktop
{
private:
	bool unsupported = false;

	enum ReadVariantType
	{
		VAR_TYPE_UINT32, // u
		VAR_TYPE_BOOL,	 // b
		VAR_TYPE_COLOR,	 // (ddd)
	};

	static bool try_parse_variant(
		DBusMessage* p_reply_message, ReadVariantType p_type, void* r_value);
	// Read a setting from org.freekdesktop.portal.Settings
	bool read_setting(
		const char* p_namespace, const char* p_key, ReadVariantType p_type, void* r_value);

	static void append_dbus_string(DBusMessageIter* p_iter, const String& p_string);
	static void append_dbus_dict_string(DBusMessageIter* p_iter, const String& p_key,
		const String& p_value, bool p_as_byte_array = false);
	static void append_dbus_dict_bool(DBusMessageIter* p_iter, const String& p_key, bool p_value);

	static Error make_request_token(String& r_token);
	bool send_request(DBusMessage* p_message, const String& r_token, String& r_response_path,
		String& r_response_filter);

	struct ColorPickerData
	{
		String filter;
		String path;
	};

	Mutex color_picker_mutex;
	Vector<ColorPickerData> color_pickers;

	struct FileDialogData
	{
		Vector<String> filter_names;
		HashMap<String, String> option_ids;
		DisplayServerEnums::WindowID prev_focus = DisplayServerEnums::INVALID_WINDOW_ID;
		String filter;
		String path;
		bool opt_in_cb = false;
	};

	Mutex file_dialog_mutex;
	Vector<FileDialogData> file_dialogs;
	Thread monitor_thread;
	SafeFlag monitor_thread_abort;
	DBusConnection* monitor_connection = nullptr;

	String theme_path;
	void _system_theme_changed_callback();
	bool _is_interface_supported(const char* p_iface, uint32_t p_minimum_version);

	Mutex inhibit_mutex;
	String inhibit_path;
	String inhibit_filter;

	static void _thread_monitor(void* p_ud);

public:
	FreeDesktopPortalDesktop();
	~FreeDesktopPortalDesktop();

	bool is_supported() { return !unsupported; }

	bool is_file_chooser_supported();
	bool is_settings_supported();
	bool is_screenshot_supported();
	bool is_inhibit_supported();

	void process_callbacks();

	// org.freedesktop.portal.Settings methods.

	// Retrieve the system's preferred color scheme.
	// 0: No preference or unknown.
	// 1: Prefer dark appearance.
	// 2: Prefer light appearance.
	uint32_t get_appearance_color_scheme();
	Color get_appearance_accent_color();

	// Retrieve high-contrast setting.
	// -1: Unknown.
	// 0: Disabled.
	// 1: Enabled.
	uint32_t get_high_contrast();

	// org.freedesktop.portal.Inhibit methods.
	bool inhibit(const String& p_xid);
	void uninhibit();
};

#endif // DBUS_ENABLED



// --- End platform/linuxbsd/freedesktop_portal_desktop.h ---

// --- Folded from platform/linuxbsd/freedesktop_screensaver.h ---
#ifdef DBUS_ENABLED

#include <cstdint>

class FreeDesktopScreenSaver {
private:
	uint32_t cookie = 0;
	bool unsupported = false;

public:
	FreeDesktopScreenSaver();
	void inhibit();
	void uninhibit();
};

#endif // DBUS_ENABLED

// --- End platform/linuxbsd/freedesktop_screensaver.h ---

// --- Folded from platform/linuxbsd/tts_linux.h ---
#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/io/ip.h"
#include "core/templates/list.h"

#ifdef SOWRAP_ENABLED
// --- Inlined/Folded from platform/linuxbsd/speechd-so_wrap.h ---
#ifndef DYLIBLOAD_WRAPPER_SPEECHD
#define DYLIBLOAD_WRAPPER_SPEECHD
// This file is generated. Do not edit!
// see https://github.com/hpvb/dynload-wrapper for details
// generated by generate-wrapper.py 0.3 on 2023-01-12 10:07:46
// flags: generate-wrapper.py --include ./thirdparty/linuxbsd_headers/speechd/libspeechd.h --sys-include "thirdparty/linuxbsd_headers/speechd/libspeechd.h" --soname libspeechd.so.2 --init-name speechd --omit-prefix spd_get_client_list --output-header ./platform/linuxbsd/speechd-so_wrap.h --output-implementation ./platform/linuxbsd/speechd-so_wrap.c
//
#include <stdint.h>

#define SPDConnectionAddress__free SPDConnectionAddress__free_dylibloader_orig_speechd
#define spd_get_default_address spd_get_default_address_dylibloader_orig_speechd
#define spd_open spd_open_dylibloader_orig_speechd
#define spd_open2 spd_open2_dylibloader_orig_speechd
#define spd_close spd_close_dylibloader_orig_speechd
#define spd_say spd_say_dylibloader_orig_speechd
#define spd_sayf spd_sayf_dylibloader_orig_speechd
#define spd_stop spd_stop_dylibloader_orig_speechd
#define spd_stop_all spd_stop_all_dylibloader_orig_speechd
#define spd_stop_uid spd_stop_uid_dylibloader_orig_speechd
#define spd_cancel spd_cancel_dylibloader_orig_speechd
#define spd_cancel_all spd_cancel_all_dylibloader_orig_speechd
#define spd_cancel_uid spd_cancel_uid_dylibloader_orig_speechd
#define spd_pause spd_pause_dylibloader_orig_speechd
#define spd_pause_all spd_pause_all_dylibloader_orig_speechd
#define spd_pause_uid spd_pause_uid_dylibloader_orig_speechd
#define spd_resume spd_resume_dylibloader_orig_speechd
#define spd_resume_all spd_resume_all_dylibloader_orig_speechd
#define spd_resume_uid spd_resume_uid_dylibloader_orig_speechd
#define spd_key spd_key_dylibloader_orig_speechd
#define spd_char spd_char_dylibloader_orig_speechd
#define spd_wchar spd_wchar_dylibloader_orig_speechd
#define spd_sound_icon spd_sound_icon_dylibloader_orig_speechd
#define spd_set_voice_type spd_set_voice_type_dylibloader_orig_speechd
#define spd_set_voice_type_all spd_set_voice_type_all_dylibloader_orig_speechd
#define spd_set_voice_type_uid spd_set_voice_type_uid_dylibloader_orig_speechd
#define spd_get_voice_type spd_get_voice_type_dylibloader_orig_speechd
#define spd_set_synthesis_voice spd_set_synthesis_voice_dylibloader_orig_speechd
#define spd_set_synthesis_voice_all spd_set_synthesis_voice_all_dylibloader_orig_speechd
#define spd_set_synthesis_voice_uid spd_set_synthesis_voice_uid_dylibloader_orig_speechd
#define spd_set_data_mode spd_set_data_mode_dylibloader_orig_speechd
#define spd_set_notification_on spd_set_notification_on_dylibloader_orig_speechd
#define spd_set_notification_off spd_set_notification_off_dylibloader_orig_speechd
#define spd_set_notification spd_set_notification_dylibloader_orig_speechd
#define spd_set_voice_rate spd_set_voice_rate_dylibloader_orig_speechd
#define spd_set_voice_rate_all spd_set_voice_rate_all_dylibloader_orig_speechd
#define spd_set_voice_rate_uid spd_set_voice_rate_uid_dylibloader_orig_speechd
#define spd_get_voice_rate spd_get_voice_rate_dylibloader_orig_speechd
#define spd_set_voice_pitch spd_set_voice_pitch_dylibloader_orig_speechd
#define spd_set_voice_pitch_all spd_set_voice_pitch_all_dylibloader_orig_speechd
#define spd_set_voice_pitch_uid spd_set_voice_pitch_uid_dylibloader_orig_speechd
#define spd_get_voice_pitch spd_get_voice_pitch_dylibloader_orig_speechd
#define spd_set_volume spd_set_volume_dylibloader_orig_speechd
#define spd_set_volume_all spd_set_volume_all_dylibloader_orig_speechd
#define spd_set_volume_uid spd_set_volume_uid_dylibloader_orig_speechd
#define spd_get_volume spd_get_volume_dylibloader_orig_speechd
#define spd_set_punctuation spd_set_punctuation_dylibloader_orig_speechd
#define spd_set_punctuation_all spd_set_punctuation_all_dylibloader_orig_speechd
#define spd_set_punctuation_uid spd_set_punctuation_uid_dylibloader_orig_speechd
#define spd_set_capital_letters spd_set_capital_letters_dylibloader_orig_speechd
#define spd_set_capital_letters_all spd_set_capital_letters_all_dylibloader_orig_speechd
#define spd_set_capital_letters_uid spd_set_capital_letters_uid_dylibloader_orig_speechd
#define spd_set_spelling spd_set_spelling_dylibloader_orig_speechd
#define spd_set_spelling_all spd_set_spelling_all_dylibloader_orig_speechd
#define spd_set_spelling_uid spd_set_spelling_uid_dylibloader_orig_speechd
#define spd_set_language spd_set_language_dylibloader_orig_speechd
#define spd_set_language_all spd_set_language_all_dylibloader_orig_speechd
#define spd_set_language_uid spd_set_language_uid_dylibloader_orig_speechd
#define spd_get_language spd_get_language_dylibloader_orig_speechd
#define spd_set_output_module spd_set_output_module_dylibloader_orig_speechd
#define spd_set_output_module_all spd_set_output_module_all_dylibloader_orig_speechd
#define spd_set_output_module_uid spd_set_output_module_uid_dylibloader_orig_speechd
#define spd_get_message_list_fd spd_get_message_list_fd_dylibloader_orig_speechd
#define spd_list_modules spd_list_modules_dylibloader_orig_speechd
#define free_spd_modules free_spd_modules_dylibloader_orig_speechd
#define spd_get_output_module spd_get_output_module_dylibloader_orig_speechd
#define spd_list_voices spd_list_voices_dylibloader_orig_speechd
#define spd_list_synthesis_voices spd_list_synthesis_voices_dylibloader_orig_speechd
#define free_spd_voices free_spd_voices_dylibloader_orig_speechd
#define spd_execute_command_with_list_reply spd_execute_command_with_list_reply_dylibloader_orig_speechd
#define spd_execute_command spd_execute_command_dylibloader_orig_speechd
#define spd_execute_command_with_reply spd_execute_command_with_reply_dylibloader_orig_speechd
#define spd_execute_command_wo_mutex spd_execute_command_wo_mutex_dylibloader_orig_speechd
#define spd_send_data spd_send_data_dylibloader_orig_speechd
#define spd_send_data_wo_mutex spd_send_data_wo_mutex_dylibloader_orig_speechd
#include <thirdparty/linuxbsd_headers/speechd/libspeechd.h> // IWYU pragma: export.
#undef SPDConnectionAddress__free
#undef spd_get_default_address
#undef spd_open
#undef spd_open2
#undef spd_close
#undef spd_say
#undef spd_sayf
#undef spd_stop
#undef spd_stop_all
#undef spd_stop_uid
#undef spd_cancel
#undef spd_cancel_all
#undef spd_cancel_uid
#undef spd_pause
#undef spd_pause_all
#undef spd_pause_uid
#undef spd_resume
#undef spd_resume_all
#undef spd_resume_uid
#undef spd_key
#undef spd_char
#undef spd_wchar
#undef spd_sound_icon
#undef spd_set_voice_type
#undef spd_set_voice_type_all
#undef spd_set_voice_type_uid
#undef spd_get_voice_type
#undef spd_set_synthesis_voice
#undef spd_set_synthesis_voice_all
#undef spd_set_synthesis_voice_uid
#undef spd_set_data_mode
#undef spd_set_notification_on
#undef spd_set_notification_off
#undef spd_set_notification
#undef spd_set_voice_rate
#undef spd_set_voice_rate_all
#undef spd_set_voice_rate_uid
#undef spd_get_voice_rate
#undef spd_set_voice_pitch
#undef spd_set_voice_pitch_all
#undef spd_set_voice_pitch_uid
#undef spd_get_voice_pitch
#undef spd_set_volume
#undef spd_set_volume_all
#undef spd_set_volume_uid
#undef spd_get_volume
#undef spd_set_punctuation
#undef spd_set_punctuation_all
#undef spd_set_punctuation_uid
#undef spd_set_capital_letters
#undef spd_set_capital_letters_all
#undef spd_set_capital_letters_uid
#undef spd_set_spelling
#undef spd_set_spelling_all
#undef spd_set_spelling_uid
#undef spd_set_language
#undef spd_set_language_all
#undef spd_set_language_uid
#undef spd_get_language
#undef spd_set_output_module
#undef spd_set_output_module_all
#undef spd_set_output_module_uid
#undef spd_get_message_list_fd
#undef spd_list_modules
#undef free_spd_modules
#undef spd_get_output_module
#undef spd_list_voices
#undef spd_list_synthesis_voices
#undef free_spd_voices
#undef spd_execute_command_with_list_reply
#undef spd_execute_command
#undef spd_execute_command_with_reply
#undef spd_execute_command_wo_mutex
#undef spd_send_data
#undef spd_send_data_wo_mutex
#ifdef __cplusplus
extern "C" {
#endif
#define SPDConnectionAddress__free SPDConnectionAddress__free_dylibloader_wrapper_speechd
#define spd_get_default_address spd_get_default_address_dylibloader_wrapper_speechd
#define spd_open spd_open_dylibloader_wrapper_speechd
#define spd_open2 spd_open2_dylibloader_wrapper_speechd
#define spd_close spd_close_dylibloader_wrapper_speechd
#define spd_say spd_say_dylibloader_wrapper_speechd
#define spd_sayf spd_sayf_dylibloader_wrapper_speechd
#define spd_stop spd_stop_dylibloader_wrapper_speechd
#define spd_stop_all spd_stop_all_dylibloader_wrapper_speechd
#define spd_stop_uid spd_stop_uid_dylibloader_wrapper_speechd
#define spd_cancel spd_cancel_dylibloader_wrapper_speechd
#define spd_cancel_all spd_cancel_all_dylibloader_wrapper_speechd
#define spd_cancel_uid spd_cancel_uid_dylibloader_wrapper_speechd
#define spd_pause spd_pause_dylibloader_wrapper_speechd
#define spd_pause_all spd_pause_all_dylibloader_wrapper_speechd
#define spd_pause_uid spd_pause_uid_dylibloader_wrapper_speechd
#define spd_resume spd_resume_dylibloader_wrapper_speechd
#define spd_resume_all spd_resume_all_dylibloader_wrapper_speechd
#define spd_resume_uid spd_resume_uid_dylibloader_wrapper_speechd
#define spd_key spd_key_dylibloader_wrapper_speechd
#define spd_char spd_char_dylibloader_wrapper_speechd
#define spd_wchar spd_wchar_dylibloader_wrapper_speechd
#define spd_sound_icon spd_sound_icon_dylibloader_wrapper_speechd
#define spd_set_voice_type spd_set_voice_type_dylibloader_wrapper_speechd
#define spd_set_voice_type_all spd_set_voice_type_all_dylibloader_wrapper_speechd
#define spd_set_voice_type_uid spd_set_voice_type_uid_dylibloader_wrapper_speechd
#define spd_get_voice_type spd_get_voice_type_dylibloader_wrapper_speechd
#define spd_set_synthesis_voice spd_set_synthesis_voice_dylibloader_wrapper_speechd
#define spd_set_synthesis_voice_all spd_set_synthesis_voice_all_dylibloader_wrapper_speechd
#define spd_set_synthesis_voice_uid spd_set_synthesis_voice_uid_dylibloader_wrapper_speechd
#define spd_set_data_mode spd_set_data_mode_dylibloader_wrapper_speechd
#define spd_set_notification_on spd_set_notification_on_dylibloader_wrapper_speechd
#define spd_set_notification_off spd_set_notification_off_dylibloader_wrapper_speechd
#define spd_set_notification spd_set_notification_dylibloader_wrapper_speechd
#define spd_set_voice_rate spd_set_voice_rate_dylibloader_wrapper_speechd
#define spd_set_voice_rate_all spd_set_voice_rate_all_dylibloader_wrapper_speechd
#define spd_set_voice_rate_uid spd_set_voice_rate_uid_dylibloader_wrapper_speechd
#define spd_get_voice_rate spd_get_voice_rate_dylibloader_wrapper_speechd
#define spd_set_voice_pitch spd_set_voice_pitch_dylibloader_wrapper_speechd
#define spd_set_voice_pitch_all spd_set_voice_pitch_all_dylibloader_wrapper_speechd
#define spd_set_voice_pitch_uid spd_set_voice_pitch_uid_dylibloader_wrapper_speechd
#define spd_get_voice_pitch spd_get_voice_pitch_dylibloader_wrapper_speechd
#define spd_set_volume spd_set_volume_dylibloader_wrapper_speechd
#define spd_set_volume_all spd_set_volume_all_dylibloader_wrapper_speechd
#define spd_set_volume_uid spd_set_volume_uid_dylibloader_wrapper_speechd
#define spd_get_volume spd_get_volume_dylibloader_wrapper_speechd
#define spd_set_punctuation spd_set_punctuation_dylibloader_wrapper_speechd
#define spd_set_punctuation_all spd_set_punctuation_all_dylibloader_wrapper_speechd
#define spd_set_punctuation_uid spd_set_punctuation_uid_dylibloader_wrapper_speechd
#define spd_set_capital_letters spd_set_capital_letters_dylibloader_wrapper_speechd
#define spd_set_capital_letters_all spd_set_capital_letters_all_dylibloader_wrapper_speechd
#define spd_set_capital_letters_uid spd_set_capital_letters_uid_dylibloader_wrapper_speechd
#define spd_set_spelling spd_set_spelling_dylibloader_wrapper_speechd
#define spd_set_spelling_all spd_set_spelling_all_dylibloader_wrapper_speechd
#define spd_set_spelling_uid spd_set_spelling_uid_dylibloader_wrapper_speechd
#define spd_set_language spd_set_language_dylibloader_wrapper_speechd
#define spd_set_language_all spd_set_language_all_dylibloader_wrapper_speechd
#define spd_set_language_uid spd_set_language_uid_dylibloader_wrapper_speechd
#define spd_get_language spd_get_language_dylibloader_wrapper_speechd
#define spd_set_output_module spd_set_output_module_dylibloader_wrapper_speechd
#define spd_set_output_module_all spd_set_output_module_all_dylibloader_wrapper_speechd
#define spd_set_output_module_uid spd_set_output_module_uid_dylibloader_wrapper_speechd
#define spd_get_message_list_fd spd_get_message_list_fd_dylibloader_wrapper_speechd
#define spd_list_modules spd_list_modules_dylibloader_wrapper_speechd
#define free_spd_modules free_spd_modules_dylibloader_wrapper_speechd
#define spd_get_output_module spd_get_output_module_dylibloader_wrapper_speechd
#define spd_list_voices spd_list_voices_dylibloader_wrapper_speechd
#define spd_list_synthesis_voices spd_list_synthesis_voices_dylibloader_wrapper_speechd
#define free_spd_voices free_spd_voices_dylibloader_wrapper_speechd
#define spd_execute_command_with_list_reply spd_execute_command_with_list_reply_dylibloader_wrapper_speechd
#define spd_execute_command spd_execute_command_dylibloader_wrapper_speechd
#define spd_execute_command_with_reply spd_execute_command_with_reply_dylibloader_wrapper_speechd
#define spd_execute_command_wo_mutex spd_execute_command_wo_mutex_dylibloader_wrapper_speechd
#define spd_send_data spd_send_data_dylibloader_wrapper_speechd
#define spd_send_data_wo_mutex spd_send_data_wo_mutex_dylibloader_wrapper_speechd
extern void (*SPDConnectionAddress__free_dylibloader_wrapper_speechd)( SPDConnectionAddress*);
extern SPDConnectionAddress* (*spd_get_default_address_dylibloader_wrapper_speechd)( char**);
extern SPDConnection* (*spd_open_dylibloader_wrapper_speechd)(const char*,const char*,const char*, SPDConnectionMode);
extern SPDConnection* (*spd_open2_dylibloader_wrapper_speechd)(const char*,const char*,const char*, SPDConnectionMode, SPDConnectionAddress*, int, char**);
extern void (*spd_close_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_say_dylibloader_wrapper_speechd)( SPDConnection*, SPDPriority,const char*);
extern int (*spd_sayf_dylibloader_wrapper_speechd)( SPDConnection*, SPDPriority,const char*,...);
extern int (*spd_stop_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_stop_all_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_stop_uid_dylibloader_wrapper_speechd)( SPDConnection*, int);
extern int (*spd_cancel_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_cancel_all_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_cancel_uid_dylibloader_wrapper_speechd)( SPDConnection*, int);
extern int (*spd_pause_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_pause_all_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_pause_uid_dylibloader_wrapper_speechd)( SPDConnection*, int);
extern int (*spd_resume_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_resume_all_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_resume_uid_dylibloader_wrapper_speechd)( SPDConnection*, int);
extern int (*spd_key_dylibloader_wrapper_speechd)( SPDConnection*, SPDPriority,const char*);
extern int (*spd_char_dylibloader_wrapper_speechd)( SPDConnection*, SPDPriority,const char*);
extern int (*spd_wchar_dylibloader_wrapper_speechd)( SPDConnection*, SPDPriority, wchar_t);
extern int (*spd_sound_icon_dylibloader_wrapper_speechd)( SPDConnection*, SPDPriority,const char*);
extern int (*spd_set_voice_type_dylibloader_wrapper_speechd)( SPDConnection*, SPDVoiceType);
extern int (*spd_set_voice_type_all_dylibloader_wrapper_speechd)( SPDConnection*, SPDVoiceType);
extern int (*spd_set_voice_type_uid_dylibloader_wrapper_speechd)( SPDConnection*, SPDVoiceType, unsigned int);
extern SPDVoiceType (*spd_get_voice_type_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_set_synthesis_voice_dylibloader_wrapper_speechd)( SPDConnection*,const char*);
extern int (*spd_set_synthesis_voice_all_dylibloader_wrapper_speechd)( SPDConnection*,const char*);
extern int (*spd_set_synthesis_voice_uid_dylibloader_wrapper_speechd)( SPDConnection*,const char*, unsigned int);
extern int (*spd_set_data_mode_dylibloader_wrapper_speechd)( SPDConnection*, SPDDataMode);
extern int (*spd_set_notification_on_dylibloader_wrapper_speechd)( SPDConnection*, SPDNotification);
extern int (*spd_set_notification_off_dylibloader_wrapper_speechd)( SPDConnection*, SPDNotification);
extern int (*spd_set_notification_dylibloader_wrapper_speechd)( SPDConnection*, SPDNotification,const char*);
extern int (*spd_set_voice_rate_dylibloader_wrapper_speechd)( SPDConnection*, signed int);
extern int (*spd_set_voice_rate_all_dylibloader_wrapper_speechd)( SPDConnection*, signed int);
extern int (*spd_set_voice_rate_uid_dylibloader_wrapper_speechd)( SPDConnection*, signed int, unsigned int);
extern int (*spd_get_voice_rate_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_set_voice_pitch_dylibloader_wrapper_speechd)( SPDConnection*, signed int);
extern int (*spd_set_voice_pitch_all_dylibloader_wrapper_speechd)( SPDConnection*, signed int);
extern int (*spd_set_voice_pitch_uid_dylibloader_wrapper_speechd)( SPDConnection*, signed int, unsigned int);
extern int (*spd_get_voice_pitch_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_set_volume_dylibloader_wrapper_speechd)( SPDConnection*, signed int);
extern int (*spd_set_volume_all_dylibloader_wrapper_speechd)( SPDConnection*, signed int);
extern int (*spd_set_volume_uid_dylibloader_wrapper_speechd)( SPDConnection*, signed int, unsigned int);
extern int (*spd_get_volume_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_set_punctuation_dylibloader_wrapper_speechd)( SPDConnection*, SPDPunctuation);
extern int (*spd_set_punctuation_all_dylibloader_wrapper_speechd)( SPDConnection*, SPDPunctuation);
extern int (*spd_set_punctuation_uid_dylibloader_wrapper_speechd)( SPDConnection*, SPDPunctuation, unsigned int);
extern int (*spd_set_capital_letters_dylibloader_wrapper_speechd)( SPDConnection*, SPDCapitalLetters);
extern int (*spd_set_capital_letters_all_dylibloader_wrapper_speechd)( SPDConnection*, SPDCapitalLetters);
extern int (*spd_set_capital_letters_uid_dylibloader_wrapper_speechd)( SPDConnection*, SPDCapitalLetters, unsigned int);
extern int (*spd_set_spelling_dylibloader_wrapper_speechd)( SPDConnection*, SPDSpelling);
extern int (*spd_set_spelling_all_dylibloader_wrapper_speechd)( SPDConnection*, SPDSpelling);
extern int (*spd_set_spelling_uid_dylibloader_wrapper_speechd)( SPDConnection*, SPDSpelling, unsigned int);
extern int (*spd_set_language_dylibloader_wrapper_speechd)( SPDConnection*,const char*);
extern int (*spd_set_language_all_dylibloader_wrapper_speechd)( SPDConnection*,const char*);
extern int (*spd_set_language_uid_dylibloader_wrapper_speechd)( SPDConnection*,const char*, unsigned int);
extern char* (*spd_get_language_dylibloader_wrapper_speechd)( SPDConnection*);
extern int (*spd_set_output_module_dylibloader_wrapper_speechd)( SPDConnection*,const char*);
extern int (*spd_set_output_module_all_dylibloader_wrapper_speechd)( SPDConnection*,const char*);
extern int (*spd_set_output_module_uid_dylibloader_wrapper_speechd)( SPDConnection*,const char*, unsigned int);
extern int (*spd_get_message_list_fd_dylibloader_wrapper_speechd)( SPDConnection*, int, int*, char**);
extern char** (*spd_list_modules_dylibloader_wrapper_speechd)( SPDConnection*);
extern void (*free_spd_modules_dylibloader_wrapper_speechd)( char**);
extern char* (*spd_get_output_module_dylibloader_wrapper_speechd)( SPDConnection*);
extern char** (*spd_list_voices_dylibloader_wrapper_speechd)( SPDConnection*);
extern SPDVoice** (*spd_list_synthesis_voices_dylibloader_wrapper_speechd)( SPDConnection*);
extern void (*free_spd_voices_dylibloader_wrapper_speechd)( SPDVoice**);
extern char** (*spd_execute_command_with_list_reply_dylibloader_wrapper_speechd)( SPDConnection*, char*);
extern int (*spd_execute_command_dylibloader_wrapper_speechd)( SPDConnection*, char*);
extern int (*spd_execute_command_with_reply_dylibloader_wrapper_speechd)( SPDConnection*, char*, char**);
extern int (*spd_execute_command_wo_mutex_dylibloader_wrapper_speechd)( SPDConnection*, char*);
extern char* (*spd_send_data_dylibloader_wrapper_speechd)( SPDConnection*,const char*, int);
extern char* (*spd_send_data_wo_mutex_dylibloader_wrapper_speechd)( SPDConnection*,const char*, int);
int initialize_speechd(int verbose);
#ifdef __cplusplus
}
#endif
#endif
#else
#include <libspeechd.h>
#endif

struct TTSUtterance;

class TTS_Linux final
{
	struct VoiceInfo
	{
		String language;
		String variant;
	};

	struct Data
	{
		Mutex mutex;
		List<TTSUtterance> queue;
		SPDConnection* synth = nullptr;
		bool speaking = false;
		bool paused = false;
		int last_msg_id = -1;
		HashMap<int, int64_t> ids;
		bool voices_loaded = false;
		HashMap<String, VoiceInfo> voices;
		Thread init_thread;
	};

	static inline Data* data = nullptr;

	static void speech_init_thread_func(void* p_userdata);
	static void speech_event_callback(
		size_t p_msg_id, size_t p_client_id, SPDNotificationType p_type);
	static void speech_event_index_mark(
		size_t p_msg_id, size_t p_client_id, SPDNotificationType p_type, char* p_index_mark);

	static void _load_voices();
	static void _speech_event(int p_msg_id, int p_type);
	static void _speech_index_mark(int p_msg_id, int p_type, const String& p_index_mark);

public:
	TTS_Linux() = delete;

	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static bool is_speaking();
	static bool is_paused();

	static void speak(const String& p_text, const String& p_voice, int p_volume = 50,
		float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0,
		bool p_interrupt = false);
	static void pause();
	static void resume();
	static void stop();
};

// --- End platform/linuxbsd/tts_linux.h ---

// --- Folded from platform/linuxbsd/dbus-so_wrap.h ---
#ifndef DYLIBLOAD_WRAPPER_DBUS
#define DYLIBLOAD_WRAPPER_DBUS
// This file is generated. Do not edit!
// see https://github.com/hpvb/dynload-wrapper for details
// generated by generate-wrapper.py 0.3 on 2023-01-12 10:26:35
// flags: generate-wrapper.py --include ./thirdparty/linuxbsd_headers/dbus/dbus.h --sys-include "thirdparty/linuxbsd_headers/dbus/dbus.h" --soname libdbus-1.so.3 --init-name dbus --output-header ./platform/linuxbsd/dbus-so_wrap.h --output-implementation ./platform/linuxbsd/dbus-so_wrap.c
//
#include <stdint.h>

#define dbus_error_init dbus_error_init_dylibloader_orig_dbus
#define dbus_error_free dbus_error_free_dylibloader_orig_dbus
#define dbus_set_error dbus_set_error_dylibloader_orig_dbus
#define dbus_set_error_const dbus_set_error_const_dylibloader_orig_dbus
#define dbus_move_error dbus_move_error_dylibloader_orig_dbus
#define dbus_error_has_name dbus_error_has_name_dylibloader_orig_dbus
#define dbus_error_is_set dbus_error_is_set_dylibloader_orig_dbus
#define dbus_parse_address dbus_parse_address_dylibloader_orig_dbus
#define dbus_address_entry_get_value dbus_address_entry_get_value_dylibloader_orig_dbus
#define dbus_address_entry_get_method dbus_address_entry_get_method_dylibloader_orig_dbus
#define dbus_address_entries_free dbus_address_entries_free_dylibloader_orig_dbus
#define dbus_address_escape_value dbus_address_escape_value_dylibloader_orig_dbus
#define dbus_address_unescape_value dbus_address_unescape_value_dylibloader_orig_dbus
#define dbus_malloc dbus_malloc_dylibloader_orig_dbus
#define dbus_malloc0 dbus_malloc0_dylibloader_orig_dbus
#define dbus_realloc dbus_realloc_dylibloader_orig_dbus
#define dbus_free dbus_free_dylibloader_orig_dbus
#define dbus_free_string_array dbus_free_string_array_dylibloader_orig_dbus
#define dbus_shutdown dbus_shutdown_dylibloader_orig_dbus
#define dbus_message_new dbus_message_new_dylibloader_orig_dbus
#define dbus_message_new_method_call dbus_message_new_method_call_dylibloader_orig_dbus
#define dbus_message_new_method_return dbus_message_new_method_return_dylibloader_orig_dbus
#define dbus_message_new_signal dbus_message_new_signal_dylibloader_orig_dbus
#define dbus_message_new_error dbus_message_new_error_dylibloader_orig_dbus
#define dbus_message_new_error_printf dbus_message_new_error_printf_dylibloader_orig_dbus
#define dbus_message_copy dbus_message_copy_dylibloader_orig_dbus
#define dbus_message_ref dbus_message_ref_dylibloader_orig_dbus
#define dbus_message_unref dbus_message_unref_dylibloader_orig_dbus
#define dbus_message_get_type dbus_message_get_type_dylibloader_orig_dbus
#define dbus_message_set_path dbus_message_set_path_dylibloader_orig_dbus
#define dbus_message_get_path dbus_message_get_path_dylibloader_orig_dbus
#define dbus_message_has_path dbus_message_has_path_dylibloader_orig_dbus
#define dbus_message_set_interface dbus_message_set_interface_dylibloader_orig_dbus
#define dbus_message_get_interface dbus_message_get_interface_dylibloader_orig_dbus
#define dbus_message_has_interface dbus_message_has_interface_dylibloader_orig_dbus
#define dbus_message_set_member dbus_message_set_member_dylibloader_orig_dbus
#define dbus_message_get_member dbus_message_get_member_dylibloader_orig_dbus
#define dbus_message_has_member dbus_message_has_member_dylibloader_orig_dbus
#define dbus_message_set_error_name dbus_message_set_error_name_dylibloader_orig_dbus
#define dbus_message_get_error_name dbus_message_get_error_name_dylibloader_orig_dbus
#define dbus_message_set_destination dbus_message_set_destination_dylibloader_orig_dbus
#define dbus_message_get_destination dbus_message_get_destination_dylibloader_orig_dbus
#define dbus_message_set_sender dbus_message_set_sender_dylibloader_orig_dbus
#define dbus_message_get_sender dbus_message_get_sender_dylibloader_orig_dbus
#define dbus_message_get_signature dbus_message_get_signature_dylibloader_orig_dbus
#define dbus_message_set_no_reply dbus_message_set_no_reply_dylibloader_orig_dbus
#define dbus_message_get_no_reply dbus_message_get_no_reply_dylibloader_orig_dbus
#define dbus_message_is_method_call dbus_message_is_method_call_dylibloader_orig_dbus
#define dbus_message_is_signal dbus_message_is_signal_dylibloader_orig_dbus
#define dbus_message_is_error dbus_message_is_error_dylibloader_orig_dbus
#define dbus_message_has_destination dbus_message_has_destination_dylibloader_orig_dbus
#define dbus_message_has_sender dbus_message_has_sender_dylibloader_orig_dbus
#define dbus_message_has_signature dbus_message_has_signature_dylibloader_orig_dbus
#define dbus_message_get_serial dbus_message_get_serial_dylibloader_orig_dbus
#define dbus_message_set_serial dbus_message_set_serial_dylibloader_orig_dbus
#define dbus_message_set_reply_serial dbus_message_set_reply_serial_dylibloader_orig_dbus
#define dbus_message_get_reply_serial dbus_message_get_reply_serial_dylibloader_orig_dbus
#define dbus_message_set_auto_start dbus_message_set_auto_start_dylibloader_orig_dbus
#define dbus_message_get_auto_start dbus_message_get_auto_start_dylibloader_orig_dbus
#define dbus_message_get_path_decomposed dbus_message_get_path_decomposed_dylibloader_orig_dbus
#define dbus_message_append_args dbus_message_append_args_dylibloader_orig_dbus
#define dbus_message_append_args_valist dbus_message_append_args_valist_dylibloader_orig_dbus
#define dbus_message_get_args dbus_message_get_args_dylibloader_orig_dbus
#define dbus_message_get_args_valist dbus_message_get_args_valist_dylibloader_orig_dbus
#define dbus_message_contains_unix_fds dbus_message_contains_unix_fds_dylibloader_orig_dbus
#define dbus_message_iter_init_closed dbus_message_iter_init_closed_dylibloader_orig_dbus
#define dbus_message_iter_init dbus_message_iter_init_dylibloader_orig_dbus
#define dbus_message_iter_has_next dbus_message_iter_has_next_dylibloader_orig_dbus
#define dbus_message_iter_next dbus_message_iter_next_dylibloader_orig_dbus
#define dbus_message_iter_get_signature dbus_message_iter_get_signature_dylibloader_orig_dbus
#define dbus_message_iter_get_arg_type dbus_message_iter_get_arg_type_dylibloader_orig_dbus
#define dbus_message_iter_get_element_type dbus_message_iter_get_element_type_dylibloader_orig_dbus
#define dbus_message_iter_recurse dbus_message_iter_recurse_dylibloader_orig_dbus
#define dbus_message_iter_get_basic dbus_message_iter_get_basic_dylibloader_orig_dbus
#define dbus_message_iter_get_element_count dbus_message_iter_get_element_count_dylibloader_orig_dbus
#define dbus_message_iter_get_array_len dbus_message_iter_get_array_len_dylibloader_orig_dbus
#define dbus_message_iter_get_fixed_array dbus_message_iter_get_fixed_array_dylibloader_orig_dbus
#define dbus_message_iter_init_append dbus_message_iter_init_append_dylibloader_orig_dbus
#define dbus_message_iter_append_basic dbus_message_iter_append_basic_dylibloader_orig_dbus
#define dbus_message_iter_append_fixed_array dbus_message_iter_append_fixed_array_dylibloader_orig_dbus
#define dbus_message_iter_open_container dbus_message_iter_open_container_dylibloader_orig_dbus
#define dbus_message_iter_close_container dbus_message_iter_close_container_dylibloader_orig_dbus
#define dbus_message_iter_abandon_container dbus_message_iter_abandon_container_dylibloader_orig_dbus
#define dbus_message_iter_abandon_container_if_open dbus_message_iter_abandon_container_if_open_dylibloader_orig_dbus
#define dbus_message_lock dbus_message_lock_dylibloader_orig_dbus
#define dbus_set_error_from_message dbus_set_error_from_message_dylibloader_orig_dbus
#define dbus_message_allocate_data_slot dbus_message_allocate_data_slot_dylibloader_orig_dbus
#define dbus_message_free_data_slot dbus_message_free_data_slot_dylibloader_orig_dbus
#define dbus_message_set_data dbus_message_set_data_dylibloader_orig_dbus
#define dbus_message_get_data dbus_message_get_data_dylibloader_orig_dbus
#define dbus_message_type_from_string dbus_message_type_from_string_dylibloader_orig_dbus
#define dbus_message_type_to_string dbus_message_type_to_string_dylibloader_orig_dbus
#define dbus_message_marshal dbus_message_marshal_dylibloader_orig_dbus
#define dbus_message_demarshal dbus_message_demarshal_dylibloader_orig_dbus
#define dbus_message_demarshal_bytes_needed dbus_message_demarshal_bytes_needed_dylibloader_orig_dbus
#define dbus_message_set_allow_interactive_authorization dbus_message_set_allow_interactive_authorization_dylibloader_orig_dbus
#define dbus_message_get_allow_interactive_authorization dbus_message_get_allow_interactive_authorization_dylibloader_orig_dbus
#define dbus_connection_open dbus_connection_open_dylibloader_orig_dbus
#define dbus_connection_open_private dbus_connection_open_private_dylibloader_orig_dbus
#define dbus_connection_ref dbus_connection_ref_dylibloader_orig_dbus
#define dbus_connection_unref dbus_connection_unref_dylibloader_orig_dbus
#define dbus_connection_close dbus_connection_close_dylibloader_orig_dbus
#define dbus_connection_get_is_connected dbus_connection_get_is_connected_dylibloader_orig_dbus
#define dbus_connection_get_is_authenticated dbus_connection_get_is_authenticated_dylibloader_orig_dbus
#define dbus_connection_get_is_anonymous dbus_connection_get_is_anonymous_dylibloader_orig_dbus
#define dbus_connection_get_server_id dbus_connection_get_server_id_dylibloader_orig_dbus
#define dbus_connection_can_send_type dbus_connection_can_send_type_dylibloader_orig_dbus
#define dbus_connection_set_exit_on_disconnect dbus_connection_set_exit_on_disconnect_dylibloader_orig_dbus
#define dbus_connection_flush dbus_connection_flush_dylibloader_orig_dbus
#define dbus_connection_read_write_dispatch dbus_connection_read_write_dispatch_dylibloader_orig_dbus
#define dbus_connection_read_write dbus_connection_read_write_dylibloader_orig_dbus
#define dbus_connection_borrow_message dbus_connection_borrow_message_dylibloader_orig_dbus
#define dbus_connection_return_message dbus_connection_return_message_dylibloader_orig_dbus
#define dbus_connection_steal_borrowed_message dbus_connection_steal_borrowed_message_dylibloader_orig_dbus
#define dbus_connection_pop_message dbus_connection_pop_message_dylibloader_orig_dbus
#define dbus_connection_get_dispatch_status dbus_connection_get_dispatch_status_dylibloader_orig_dbus
#define dbus_connection_dispatch dbus_connection_dispatch_dylibloader_orig_dbus
#define dbus_connection_has_messages_to_send dbus_connection_has_messages_to_send_dylibloader_orig_dbus
#define dbus_connection_send dbus_connection_send_dylibloader_orig_dbus
#define dbus_connection_send_with_reply dbus_connection_send_with_reply_dylibloader_orig_dbus
#define dbus_connection_send_with_reply_and_block dbus_connection_send_with_reply_and_block_dylibloader_orig_dbus
#define dbus_connection_set_watch_functions dbus_connection_set_watch_functions_dylibloader_orig_dbus
#define dbus_connection_set_timeout_functions dbus_connection_set_timeout_functions_dylibloader_orig_dbus
#define dbus_connection_set_wakeup_main_function dbus_connection_set_wakeup_main_function_dylibloader_orig_dbus
#define dbus_connection_set_dispatch_status_function dbus_connection_set_dispatch_status_function_dylibloader_orig_dbus
#define dbus_connection_get_unix_user dbus_connection_get_unix_user_dylibloader_orig_dbus
#define dbus_connection_get_unix_process_id dbus_connection_get_unix_process_id_dylibloader_orig_dbus
#define dbus_connection_get_adt_audit_session_data dbus_connection_get_adt_audit_session_data_dylibloader_orig_dbus
#define dbus_connection_set_unix_user_function dbus_connection_set_unix_user_function_dylibloader_orig_dbus
#define dbus_connection_get_windows_user dbus_connection_get_windows_user_dylibloader_orig_dbus
#define dbus_connection_set_windows_user_function dbus_connection_set_windows_user_function_dylibloader_orig_dbus
#define dbus_connection_set_allow_anonymous dbus_connection_set_allow_anonymous_dylibloader_orig_dbus
#define dbus_connection_set_route_peer_messages dbus_connection_set_route_peer_messages_dylibloader_orig_dbus
#define dbus_connection_add_filter dbus_connection_add_filter_dylibloader_orig_dbus
#define dbus_connection_remove_filter dbus_connection_remove_filter_dylibloader_orig_dbus
#define dbus_connection_allocate_data_slot dbus_connection_allocate_data_slot_dylibloader_orig_dbus
#define dbus_connection_free_data_slot dbus_connection_free_data_slot_dylibloader_orig_dbus
#define dbus_connection_set_data dbus_connection_set_data_dylibloader_orig_dbus
#define dbus_connection_get_data dbus_connection_get_data_dylibloader_orig_dbus
#define dbus_connection_set_change_sigpipe dbus_connection_set_change_sigpipe_dylibloader_orig_dbus
#define dbus_connection_set_max_message_size dbus_connection_set_max_message_size_dylibloader_orig_dbus
#define dbus_connection_get_max_message_size dbus_connection_get_max_message_size_dylibloader_orig_dbus
#define dbus_connection_set_max_received_size dbus_connection_set_max_received_size_dylibloader_orig_dbus
#define dbus_connection_get_max_received_size dbus_connection_get_max_received_size_dylibloader_orig_dbus
#define dbus_connection_set_max_message_unix_fds dbus_connection_set_max_message_unix_fds_dylibloader_orig_dbus
#define dbus_connection_get_max_message_unix_fds dbus_connection_get_max_message_unix_fds_dylibloader_orig_dbus
#define dbus_connection_set_max_received_unix_fds dbus_connection_set_max_received_unix_fds_dylibloader_orig_dbus
#define dbus_connection_get_max_received_unix_fds dbus_connection_get_max_received_unix_fds_dylibloader_orig_dbus
#define dbus_connection_get_outgoing_size dbus_connection_get_outgoing_size_dylibloader_orig_dbus
#define dbus_connection_get_outgoing_unix_fds dbus_connection_get_outgoing_unix_fds_dylibloader_orig_dbus
#define dbus_connection_preallocate_send dbus_connection_preallocate_send_dylibloader_orig_dbus
#define dbus_connection_free_preallocated_send dbus_connection_free_preallocated_send_dylibloader_orig_dbus
#define dbus_connection_send_preallocated dbus_connection_send_preallocated_dylibloader_orig_dbus
#define dbus_connection_try_register_object_path dbus_connection_try_register_object_path_dylibloader_orig_dbus
#define dbus_connection_register_object_path dbus_connection_register_object_path_dylibloader_orig_dbus
#define dbus_connection_try_register_fallback dbus_connection_try_register_fallback_dylibloader_orig_dbus
#define dbus_connection_register_fallback dbus_connection_register_fallback_dylibloader_orig_dbus
#define dbus_connection_unregister_object_path dbus_connection_unregister_object_path_dylibloader_orig_dbus
#define dbus_connection_get_object_path_data dbus_connection_get_object_path_data_dylibloader_orig_dbus
#define dbus_connection_list_registered dbus_connection_list_registered_dylibloader_orig_dbus
#define dbus_connection_get_unix_fd dbus_connection_get_unix_fd_dylibloader_orig_dbus
#define dbus_connection_get_socket dbus_connection_get_socket_dylibloader_orig_dbus
#define dbus_watch_get_fd dbus_watch_get_fd_dylibloader_orig_dbus
#define dbus_watch_get_unix_fd dbus_watch_get_unix_fd_dylibloader_orig_dbus
#define dbus_watch_get_socket dbus_watch_get_socket_dylibloader_orig_dbus
#define dbus_watch_get_flags dbus_watch_get_flags_dylibloader_orig_dbus
#define dbus_watch_get_data dbus_watch_get_data_dylibloader_orig_dbus
#define dbus_watch_set_data dbus_watch_set_data_dylibloader_orig_dbus
#define dbus_watch_handle dbus_watch_handle_dylibloader_orig_dbus
#define dbus_watch_get_enabled dbus_watch_get_enabled_dylibloader_orig_dbus
#define dbus_timeout_get_interval dbus_timeout_get_interval_dylibloader_orig_dbus
#define dbus_timeout_get_data dbus_timeout_get_data_dylibloader_orig_dbus
#define dbus_timeout_set_data dbus_timeout_set_data_dylibloader_orig_dbus
#define dbus_timeout_handle dbus_timeout_handle_dylibloader_orig_dbus
#define dbus_timeout_get_enabled dbus_timeout_get_enabled_dylibloader_orig_dbus
#define dbus_bus_get dbus_bus_get_dylibloader_orig_dbus
#define dbus_bus_get_private dbus_bus_get_private_dylibloader_orig_dbus
#define dbus_bus_register dbus_bus_register_dylibloader_orig_dbus
#define dbus_bus_set_unique_name dbus_bus_set_unique_name_dylibloader_orig_dbus
#define dbus_bus_get_unique_name dbus_bus_get_unique_name_dylibloader_orig_dbus
#define dbus_bus_get_unix_user dbus_bus_get_unix_user_dylibloader_orig_dbus
#define dbus_bus_get_id dbus_bus_get_id_dylibloader_orig_dbus
#define dbus_bus_request_name dbus_bus_request_name_dylibloader_orig_dbus
#define dbus_bus_release_name dbus_bus_release_name_dylibloader_orig_dbus
#define dbus_bus_name_has_owner dbus_bus_name_has_owner_dylibloader_orig_dbus
#define dbus_bus_start_service_by_name dbus_bus_start_service_by_name_dylibloader_orig_dbus
#define dbus_bus_add_match dbus_bus_add_match_dylibloader_orig_dbus
#define dbus_bus_remove_match dbus_bus_remove_match_dylibloader_orig_dbus
#define dbus_get_local_machine_id dbus_get_local_machine_id_dylibloader_orig_dbus
#define dbus_get_version dbus_get_version_dylibloader_orig_dbus
#define dbus_setenv dbus_setenv_dylibloader_orig_dbus
#define dbus_try_get_local_machine_id dbus_try_get_local_machine_id_dylibloader_orig_dbus
#define dbus_pending_call_ref dbus_pending_call_ref_dylibloader_orig_dbus
#define dbus_pending_call_unref dbus_pending_call_unref_dylibloader_orig_dbus
#define dbus_pending_call_set_notify dbus_pending_call_set_notify_dylibloader_orig_dbus
#define dbus_pending_call_cancel dbus_pending_call_cancel_dylibloader_orig_dbus
#define dbus_pending_call_get_completed dbus_pending_call_get_completed_dylibloader_orig_dbus
#define dbus_pending_call_steal_reply dbus_pending_call_steal_reply_dylibloader_orig_dbus
#define dbus_pending_call_block dbus_pending_call_block_dylibloader_orig_dbus
#define dbus_pending_call_allocate_data_slot dbus_pending_call_allocate_data_slot_dylibloader_orig_dbus
#define dbus_pending_call_free_data_slot dbus_pending_call_free_data_slot_dylibloader_orig_dbus
#define dbus_pending_call_set_data dbus_pending_call_set_data_dylibloader_orig_dbus
#define dbus_pending_call_get_data dbus_pending_call_get_data_dylibloader_orig_dbus
#define dbus_server_listen dbus_server_listen_dylibloader_orig_dbus
#define dbus_server_ref dbus_server_ref_dylibloader_orig_dbus
#define dbus_server_unref dbus_server_unref_dylibloader_orig_dbus
#define dbus_server_disconnect dbus_server_disconnect_dylibloader_orig_dbus
#define dbus_server_get_is_connected dbus_server_get_is_connected_dylibloader_orig_dbus
#define dbus_server_get_address dbus_server_get_address_dylibloader_orig_dbus
#define dbus_server_get_id dbus_server_get_id_dylibloader_orig_dbus
#define dbus_server_set_new_connection_function dbus_server_set_new_connection_function_dylibloader_orig_dbus
#define dbus_server_set_watch_functions dbus_server_set_watch_functions_dylibloader_orig_dbus
#define dbus_server_set_timeout_functions dbus_server_set_timeout_functions_dylibloader_orig_dbus
#define dbus_server_set_auth_mechanisms dbus_server_set_auth_mechanisms_dylibloader_orig_dbus
#define dbus_server_allocate_data_slot dbus_server_allocate_data_slot_dylibloader_orig_dbus
#define dbus_server_free_data_slot dbus_server_free_data_slot_dylibloader_orig_dbus
#define dbus_server_set_data dbus_server_set_data_dylibloader_orig_dbus
#define dbus_server_get_data dbus_server_get_data_dylibloader_orig_dbus
#define dbus_signature_iter_init dbus_signature_iter_init_dylibloader_orig_dbus
#define dbus_signature_iter_get_current_type dbus_signature_iter_get_current_type_dylibloader_orig_dbus
#define dbus_signature_iter_get_signature dbus_signature_iter_get_signature_dylibloader_orig_dbus
#define dbus_signature_iter_get_element_type dbus_signature_iter_get_element_type_dylibloader_orig_dbus
#define dbus_signature_iter_next dbus_signature_iter_next_dylibloader_orig_dbus
#define dbus_signature_iter_recurse dbus_signature_iter_recurse_dylibloader_orig_dbus
#define dbus_signature_validate dbus_signature_validate_dylibloader_orig_dbus
#define dbus_signature_validate_single dbus_signature_validate_single_dylibloader_orig_dbus
#define dbus_type_is_valid dbus_type_is_valid_dylibloader_orig_dbus
#define dbus_type_is_basic dbus_type_is_basic_dylibloader_orig_dbus
#define dbus_type_is_container dbus_type_is_container_dylibloader_orig_dbus
#define dbus_type_is_fixed dbus_type_is_fixed_dylibloader_orig_dbus
#define dbus_validate_path dbus_validate_path_dylibloader_orig_dbus
#define dbus_validate_interface dbus_validate_interface_dylibloader_orig_dbus
#define dbus_validate_member dbus_validate_member_dylibloader_orig_dbus
#define dbus_validate_error_name dbus_validate_error_name_dylibloader_orig_dbus
#define dbus_validate_bus_name dbus_validate_bus_name_dylibloader_orig_dbus
#define dbus_validate_utf8 dbus_validate_utf8_dylibloader_orig_dbus
#define dbus_threads_init dbus_threads_init_dylibloader_orig_dbus
#define dbus_threads_init_default dbus_threads_init_default_dylibloader_orig_dbus
#include <thirdparty/linuxbsd_headers/dbus/dbus.h> // IWYU pragma: export.
#undef dbus_error_init
#undef dbus_error_free
#undef dbus_set_error
#undef dbus_set_error_const
#undef dbus_move_error
#undef dbus_error_has_name
#undef dbus_error_is_set
#undef dbus_parse_address
#undef dbus_address_entry_get_value
#undef dbus_address_entry_get_method
#undef dbus_address_entries_free
#undef dbus_address_escape_value
#undef dbus_address_unescape_value
#undef dbus_malloc
#undef dbus_malloc0
#undef dbus_realloc
#undef dbus_free
#undef dbus_free_string_array
#undef dbus_shutdown
#undef dbus_message_new
#undef dbus_message_new_method_call
#undef dbus_message_new_method_return
#undef dbus_message_new_signal
#undef dbus_message_new_error
#undef dbus_message_new_error_printf
#undef dbus_message_copy
#undef dbus_message_ref
#undef dbus_message_unref
#undef dbus_message_get_type
#undef dbus_message_set_path
#undef dbus_message_get_path
#undef dbus_message_has_path
#undef dbus_message_set_interface
#undef dbus_message_get_interface
#undef dbus_message_has_interface
#undef dbus_message_set_member
#undef dbus_message_get_member
#undef dbus_message_has_member
#undef dbus_message_set_error_name
#undef dbus_message_get_error_name
#undef dbus_message_set_destination
#undef dbus_message_get_destination
#undef dbus_message_set_sender
#undef dbus_message_get_sender
#undef dbus_message_get_signature
#undef dbus_message_set_no_reply
#undef dbus_message_get_no_reply
#undef dbus_message_is_method_call
#undef dbus_message_is_signal
#undef dbus_message_is_error
#undef dbus_message_has_destination
#undef dbus_message_has_sender
#undef dbus_message_has_signature
#undef dbus_message_get_serial
#undef dbus_message_set_serial
#undef dbus_message_set_reply_serial
#undef dbus_message_get_reply_serial
#undef dbus_message_set_auto_start
#undef dbus_message_get_auto_start
#undef dbus_message_get_path_decomposed
#undef dbus_message_append_args
#undef dbus_message_append_args_valist
#undef dbus_message_get_args
#undef dbus_message_get_args_valist
#undef dbus_message_contains_unix_fds
#undef dbus_message_iter_init_closed
#undef dbus_message_iter_init
#undef dbus_message_iter_has_next
#undef dbus_message_iter_next
#undef dbus_message_iter_get_signature
#undef dbus_message_iter_get_arg_type
#undef dbus_message_iter_get_element_type
#undef dbus_message_iter_recurse
#undef dbus_message_iter_get_basic
#undef dbus_message_iter_get_element_count
#undef dbus_message_iter_get_array_len
#undef dbus_message_iter_get_fixed_array
#undef dbus_message_iter_init_append
#undef dbus_message_iter_append_basic
#undef dbus_message_iter_append_fixed_array
#undef dbus_message_iter_open_container
#undef dbus_message_iter_close_container
#undef dbus_message_iter_abandon_container
#undef dbus_message_iter_abandon_container_if_open
#undef dbus_message_lock
#undef dbus_set_error_from_message
#undef dbus_message_allocate_data_slot
#undef dbus_message_free_data_slot
#undef dbus_message_set_data
#undef dbus_message_get_data
#undef dbus_message_type_from_string
#undef dbus_message_type_to_string
#undef dbus_message_marshal
#undef dbus_message_demarshal
#undef dbus_message_demarshal_bytes_needed
#undef dbus_message_set_allow_interactive_authorization
#undef dbus_message_get_allow_interactive_authorization
#undef dbus_connection_open
#undef dbus_connection_open_private
#undef dbus_connection_ref
#undef dbus_connection_unref
#undef dbus_connection_close
#undef dbus_connection_get_is_connected
#undef dbus_connection_get_is_authenticated
#undef dbus_connection_get_is_anonymous
#undef dbus_connection_get_server_id
#undef dbus_connection_can_send_type
#undef dbus_connection_set_exit_on_disconnect
#undef dbus_connection_flush
#undef dbus_connection_read_write_dispatch
#undef dbus_connection_read_write
#undef dbus_connection_borrow_message
#undef dbus_connection_return_message
#undef dbus_connection_steal_borrowed_message
#undef dbus_connection_pop_message
#undef dbus_connection_get_dispatch_status
#undef dbus_connection_dispatch
#undef dbus_connection_has_messages_to_send
#undef dbus_connection_send
#undef dbus_connection_send_with_reply
#undef dbus_connection_send_with_reply_and_block
#undef dbus_connection_set_watch_functions
#undef dbus_connection_set_timeout_functions
#undef dbus_connection_set_wakeup_main_function
#undef dbus_connection_set_dispatch_status_function
#undef dbus_connection_get_unix_user
#undef dbus_connection_get_unix_process_id
#undef dbus_connection_get_adt_audit_session_data
#undef dbus_connection_set_unix_user_function
#undef dbus_connection_get_windows_user
#undef dbus_connection_set_windows_user_function
#undef dbus_connection_set_allow_anonymous
#undef dbus_connection_set_route_peer_messages
#undef dbus_connection_add_filter
#undef dbus_connection_remove_filter
#undef dbus_connection_allocate_data_slot
#undef dbus_connection_free_data_slot
#undef dbus_connection_set_data
#undef dbus_connection_get_data
#undef dbus_connection_set_change_sigpipe
#undef dbus_connection_set_max_message_size
#undef dbus_connection_get_max_message_size
#undef dbus_connection_set_max_received_size
#undef dbus_connection_get_max_received_size
#undef dbus_connection_set_max_message_unix_fds
#undef dbus_connection_get_max_message_unix_fds
#undef dbus_connection_set_max_received_unix_fds
#undef dbus_connection_get_max_received_unix_fds
#undef dbus_connection_get_outgoing_size
#undef dbus_connection_get_outgoing_unix_fds
#undef dbus_connection_preallocate_send
#undef dbus_connection_free_preallocated_send
#undef dbus_connection_send_preallocated
#undef dbus_connection_try_register_object_path
#undef dbus_connection_register_object_path
#undef dbus_connection_try_register_fallback
#undef dbus_connection_register_fallback
#undef dbus_connection_unregister_object_path
#undef dbus_connection_get_object_path_data
#undef dbus_connection_list_registered
#undef dbus_connection_get_unix_fd
#undef dbus_connection_get_socket
#undef dbus_watch_get_fd
#undef dbus_watch_get_unix_fd
#undef dbus_watch_get_socket
#undef dbus_watch_get_flags
#undef dbus_watch_get_data
#undef dbus_watch_set_data
#undef dbus_watch_handle
#undef dbus_watch_get_enabled
#undef dbus_timeout_get_interval
#undef dbus_timeout_get_data
#undef dbus_timeout_set_data
#undef dbus_timeout_handle
#undef dbus_timeout_get_enabled
#undef dbus_bus_get
#undef dbus_bus_get_private
#undef dbus_bus_register
#undef dbus_bus_set_unique_name
#undef dbus_bus_get_unique_name
#undef dbus_bus_get_unix_user
#undef dbus_bus_get_id
#undef dbus_bus_request_name
#undef dbus_bus_release_name
#undef dbus_bus_name_has_owner
#undef dbus_bus_start_service_by_name
#undef dbus_bus_add_match
#undef dbus_bus_remove_match
#undef dbus_get_local_machine_id
#undef dbus_get_version
#undef dbus_setenv
#undef dbus_try_get_local_machine_id
#undef dbus_pending_call_ref
#undef dbus_pending_call_unref
#undef dbus_pending_call_set_notify
#undef dbus_pending_call_cancel
#undef dbus_pending_call_get_completed
#undef dbus_pending_call_steal_reply
#undef dbus_pending_call_block
#undef dbus_pending_call_allocate_data_slot
#undef dbus_pending_call_free_data_slot
#undef dbus_pending_call_set_data
#undef dbus_pending_call_get_data
#undef dbus_server_listen
#undef dbus_server_ref
#undef dbus_server_unref
#undef dbus_server_disconnect
#undef dbus_server_get_is_connected
#undef dbus_server_get_address
#undef dbus_server_get_id
#undef dbus_server_set_new_connection_function
#undef dbus_server_set_watch_functions
#undef dbus_server_set_timeout_functions
#undef dbus_server_set_auth_mechanisms
#undef dbus_server_allocate_data_slot
#undef dbus_server_free_data_slot
#undef dbus_server_set_data
#undef dbus_server_get_data
#undef dbus_signature_iter_init
#undef dbus_signature_iter_get_current_type
#undef dbus_signature_iter_get_signature
#undef dbus_signature_iter_get_element_type
#undef dbus_signature_iter_next
#undef dbus_signature_iter_recurse
#undef dbus_signature_validate
#undef dbus_signature_validate_single
#undef dbus_type_is_valid
#undef dbus_type_is_basic
#undef dbus_type_is_container
#undef dbus_type_is_fixed
#undef dbus_validate_path
#undef dbus_validate_interface
#undef dbus_validate_member
#undef dbus_validate_error_name
#undef dbus_validate_bus_name
#undef dbus_validate_utf8
#undef dbus_threads_init
#undef dbus_threads_init_default
#ifdef __cplusplus
extern "C" {
#endif
#define dbus_error_init dbus_error_init_dylibloader_wrapper_dbus
#define dbus_error_free dbus_error_free_dylibloader_wrapper_dbus
#define dbus_set_error dbus_set_error_dylibloader_wrapper_dbus
#define dbus_set_error_const dbus_set_error_const_dylibloader_wrapper_dbus
#define dbus_move_error dbus_move_error_dylibloader_wrapper_dbus
#define dbus_error_has_name dbus_error_has_name_dylibloader_wrapper_dbus
#define dbus_error_is_set dbus_error_is_set_dylibloader_wrapper_dbus
#define dbus_parse_address dbus_parse_address_dylibloader_wrapper_dbus
#define dbus_address_entry_get_value dbus_address_entry_get_value_dylibloader_wrapper_dbus
#define dbus_address_entry_get_method dbus_address_entry_get_method_dylibloader_wrapper_dbus
#define dbus_address_entries_free dbus_address_entries_free_dylibloader_wrapper_dbus
#define dbus_address_escape_value dbus_address_escape_value_dylibloader_wrapper_dbus
#define dbus_address_unescape_value dbus_address_unescape_value_dylibloader_wrapper_dbus
#define dbus_malloc dbus_malloc_dylibloader_wrapper_dbus
#define dbus_malloc0 dbus_malloc0_dylibloader_wrapper_dbus
#define dbus_realloc dbus_realloc_dylibloader_wrapper_dbus
#define dbus_free dbus_free_dylibloader_wrapper_dbus
#define dbus_free_string_array dbus_free_string_array_dylibloader_wrapper_dbus
#define dbus_shutdown dbus_shutdown_dylibloader_wrapper_dbus
#define dbus_message_new dbus_message_new_dylibloader_wrapper_dbus
#define dbus_message_new_method_call dbus_message_new_method_call_dylibloader_wrapper_dbus
#define dbus_message_new_method_return dbus_message_new_method_return_dylibloader_wrapper_dbus
#define dbus_message_new_signal dbus_message_new_signal_dylibloader_wrapper_dbus
#define dbus_message_new_error dbus_message_new_error_dylibloader_wrapper_dbus
#define dbus_message_new_error_printf dbus_message_new_error_printf_dylibloader_wrapper_dbus
#define dbus_message_copy dbus_message_copy_dylibloader_wrapper_dbus
#define dbus_message_ref dbus_message_ref_dylibloader_wrapper_dbus
#define dbus_message_unref dbus_message_unref_dylibloader_wrapper_dbus
#define dbus_message_get_type dbus_message_get_type_dylibloader_wrapper_dbus
#define dbus_message_set_path dbus_message_set_path_dylibloader_wrapper_dbus
#define dbus_message_get_path dbus_message_get_path_dylibloader_wrapper_dbus
#define dbus_message_has_path dbus_message_has_path_dylibloader_wrapper_dbus
#define dbus_message_set_interface dbus_message_set_interface_dylibloader_wrapper_dbus
#define dbus_message_get_interface dbus_message_get_interface_dylibloader_wrapper_dbus
#define dbus_message_has_interface dbus_message_has_interface_dylibloader_wrapper_dbus
#define dbus_message_set_member dbus_message_set_member_dylibloader_wrapper_dbus
#define dbus_message_get_member dbus_message_get_member_dylibloader_wrapper_dbus
#define dbus_message_has_member dbus_message_has_member_dylibloader_wrapper_dbus
#define dbus_message_set_error_name dbus_message_set_error_name_dylibloader_wrapper_dbus
#define dbus_message_get_error_name dbus_message_get_error_name_dylibloader_wrapper_dbus
#define dbus_message_set_destination dbus_message_set_destination_dylibloader_wrapper_dbus
#define dbus_message_get_destination dbus_message_get_destination_dylibloader_wrapper_dbus
#define dbus_message_set_sender dbus_message_set_sender_dylibloader_wrapper_dbus
#define dbus_message_get_sender dbus_message_get_sender_dylibloader_wrapper_dbus
#define dbus_message_get_signature dbus_message_get_signature_dylibloader_wrapper_dbus
#define dbus_message_set_no_reply dbus_message_set_no_reply_dylibloader_wrapper_dbus
#define dbus_message_get_no_reply dbus_message_get_no_reply_dylibloader_wrapper_dbus
#define dbus_message_is_method_call dbus_message_is_method_call_dylibloader_wrapper_dbus
#define dbus_message_is_signal dbus_message_is_signal_dylibloader_wrapper_dbus
#define dbus_message_is_error dbus_message_is_error_dylibloader_wrapper_dbus
#define dbus_message_has_destination dbus_message_has_destination_dylibloader_wrapper_dbus
#define dbus_message_has_sender dbus_message_has_sender_dylibloader_wrapper_dbus
#define dbus_message_has_signature dbus_message_has_signature_dylibloader_wrapper_dbus
#define dbus_message_get_serial dbus_message_get_serial_dylibloader_wrapper_dbus
#define dbus_message_set_serial dbus_message_set_serial_dylibloader_wrapper_dbus
#define dbus_message_set_reply_serial dbus_message_set_reply_serial_dylibloader_wrapper_dbus
#define dbus_message_get_reply_serial dbus_message_get_reply_serial_dylibloader_wrapper_dbus
#define dbus_message_set_auto_start dbus_message_set_auto_start_dylibloader_wrapper_dbus
#define dbus_message_get_auto_start dbus_message_get_auto_start_dylibloader_wrapper_dbus
#define dbus_message_get_path_decomposed dbus_message_get_path_decomposed_dylibloader_wrapper_dbus
#define dbus_message_append_args dbus_message_append_args_dylibloader_wrapper_dbus
#define dbus_message_append_args_valist dbus_message_append_args_valist_dylibloader_wrapper_dbus
#define dbus_message_get_args dbus_message_get_args_dylibloader_wrapper_dbus
#define dbus_message_get_args_valist dbus_message_get_args_valist_dylibloader_wrapper_dbus
#define dbus_message_contains_unix_fds dbus_message_contains_unix_fds_dylibloader_wrapper_dbus
#define dbus_message_iter_init_closed dbus_message_iter_init_closed_dylibloader_wrapper_dbus
#define dbus_message_iter_init dbus_message_iter_init_dylibloader_wrapper_dbus
#define dbus_message_iter_has_next dbus_message_iter_has_next_dylibloader_wrapper_dbus
#define dbus_message_iter_next dbus_message_iter_next_dylibloader_wrapper_dbus
#define dbus_message_iter_get_signature dbus_message_iter_get_signature_dylibloader_wrapper_dbus
#define dbus_message_iter_get_arg_type dbus_message_iter_get_arg_type_dylibloader_wrapper_dbus
#define dbus_message_iter_get_element_type dbus_message_iter_get_element_type_dylibloader_wrapper_dbus
#define dbus_message_iter_recurse dbus_message_iter_recurse_dylibloader_wrapper_dbus
#define dbus_message_iter_get_basic dbus_message_iter_get_basic_dylibloader_wrapper_dbus
#define dbus_message_iter_get_element_count dbus_message_iter_get_element_count_dylibloader_wrapper_dbus
#define dbus_message_iter_get_array_len dbus_message_iter_get_array_len_dylibloader_wrapper_dbus
#define dbus_message_iter_get_fixed_array dbus_message_iter_get_fixed_array_dylibloader_wrapper_dbus
#define dbus_message_iter_init_append dbus_message_iter_init_append_dylibloader_wrapper_dbus
#define dbus_message_iter_append_basic dbus_message_iter_append_basic_dylibloader_wrapper_dbus
#define dbus_message_iter_append_fixed_array dbus_message_iter_append_fixed_array_dylibloader_wrapper_dbus
#define dbus_message_iter_open_container dbus_message_iter_open_container_dylibloader_wrapper_dbus
#define dbus_message_iter_close_container dbus_message_iter_close_container_dylibloader_wrapper_dbus
#define dbus_message_iter_abandon_container dbus_message_iter_abandon_container_dylibloader_wrapper_dbus
#define dbus_message_iter_abandon_container_if_open dbus_message_iter_abandon_container_if_open_dylibloader_wrapper_dbus
#define dbus_message_lock dbus_message_lock_dylibloader_wrapper_dbus
#define dbus_set_error_from_message dbus_set_error_from_message_dylibloader_wrapper_dbus
#define dbus_message_allocate_data_slot dbus_message_allocate_data_slot_dylibloader_wrapper_dbus
#define dbus_message_free_data_slot dbus_message_free_data_slot_dylibloader_wrapper_dbus
#define dbus_message_set_data dbus_message_set_data_dylibloader_wrapper_dbus
#define dbus_message_get_data dbus_message_get_data_dylibloader_wrapper_dbus
#define dbus_message_type_from_string dbus_message_type_from_string_dylibloader_wrapper_dbus
#define dbus_message_type_to_string dbus_message_type_to_string_dylibloader_wrapper_dbus
#define dbus_message_marshal dbus_message_marshal_dylibloader_wrapper_dbus
#define dbus_message_demarshal dbus_message_demarshal_dylibloader_wrapper_dbus
#define dbus_message_demarshal_bytes_needed dbus_message_demarshal_bytes_needed_dylibloader_wrapper_dbus
#define dbus_message_set_allow_interactive_authorization dbus_message_set_allow_interactive_authorization_dylibloader_wrapper_dbus
#define dbus_message_get_allow_interactive_authorization dbus_message_get_allow_interactive_authorization_dylibloader_wrapper_dbus
#define dbus_connection_open dbus_connection_open_dylibloader_wrapper_dbus
#define dbus_connection_open_private dbus_connection_open_private_dylibloader_wrapper_dbus
#define dbus_connection_ref dbus_connection_ref_dylibloader_wrapper_dbus
#define dbus_connection_unref dbus_connection_unref_dylibloader_wrapper_dbus
#define dbus_connection_close dbus_connection_close_dylibloader_wrapper_dbus
#define dbus_connection_get_is_connected dbus_connection_get_is_connected_dylibloader_wrapper_dbus
#define dbus_connection_get_is_authenticated dbus_connection_get_is_authenticated_dylibloader_wrapper_dbus
#define dbus_connection_get_is_anonymous dbus_connection_get_is_anonymous_dylibloader_wrapper_dbus
#define dbus_connection_get_server_id dbus_connection_get_server_id_dylibloader_wrapper_dbus
#define dbus_connection_can_send_type dbus_connection_can_send_type_dylibloader_wrapper_dbus
#define dbus_connection_set_exit_on_disconnect dbus_connection_set_exit_on_disconnect_dylibloader_wrapper_dbus
#define dbus_connection_flush dbus_connection_flush_dylibloader_wrapper_dbus
#define dbus_connection_read_write_dispatch dbus_connection_read_write_dispatch_dylibloader_wrapper_dbus
#define dbus_connection_read_write dbus_connection_read_write_dylibloader_wrapper_dbus
#define dbus_connection_borrow_message dbus_connection_borrow_message_dylibloader_wrapper_dbus
#define dbus_connection_return_message dbus_connection_return_message_dylibloader_wrapper_dbus
#define dbus_connection_steal_borrowed_message dbus_connection_steal_borrowed_message_dylibloader_wrapper_dbus
#define dbus_connection_pop_message dbus_connection_pop_message_dylibloader_wrapper_dbus
#define dbus_connection_get_dispatch_status dbus_connection_get_dispatch_status_dylibloader_wrapper_dbus
#define dbus_connection_dispatch dbus_connection_dispatch_dylibloader_wrapper_dbus
#define dbus_connection_has_messages_to_send dbus_connection_has_messages_to_send_dylibloader_wrapper_dbus
#define dbus_connection_send dbus_connection_send_dylibloader_wrapper_dbus
#define dbus_connection_send_with_reply dbus_connection_send_with_reply_dylibloader_wrapper_dbus
#define dbus_connection_send_with_reply_and_block dbus_connection_send_with_reply_and_block_dylibloader_wrapper_dbus
#define dbus_connection_set_watch_functions dbus_connection_set_watch_functions_dylibloader_wrapper_dbus
#define dbus_connection_set_timeout_functions dbus_connection_set_timeout_functions_dylibloader_wrapper_dbus
#define dbus_connection_set_wakeup_main_function dbus_connection_set_wakeup_main_function_dylibloader_wrapper_dbus
#define dbus_connection_set_dispatch_status_function dbus_connection_set_dispatch_status_function_dylibloader_wrapper_dbus
#define dbus_connection_get_unix_user dbus_connection_get_unix_user_dylibloader_wrapper_dbus
#define dbus_connection_get_unix_process_id dbus_connection_get_unix_process_id_dylibloader_wrapper_dbus
#define dbus_connection_get_adt_audit_session_data dbus_connection_get_adt_audit_session_data_dylibloader_wrapper_dbus
#define dbus_connection_set_unix_user_function dbus_connection_set_unix_user_function_dylibloader_wrapper_dbus
#define dbus_connection_get_windows_user dbus_connection_get_windows_user_dylibloader_wrapper_dbus
#define dbus_connection_set_windows_user_function dbus_connection_set_windows_user_function_dylibloader_wrapper_dbus
#define dbus_connection_set_allow_anonymous dbus_connection_set_allow_anonymous_dylibloader_wrapper_dbus
#define dbus_connection_set_route_peer_messages dbus_connection_set_route_peer_messages_dylibloader_wrapper_dbus
#define dbus_connection_add_filter dbus_connection_add_filter_dylibloader_wrapper_dbus
#define dbus_connection_remove_filter dbus_connection_remove_filter_dylibloader_wrapper_dbus
#define dbus_connection_allocate_data_slot dbus_connection_allocate_data_slot_dylibloader_wrapper_dbus
#define dbus_connection_free_data_slot dbus_connection_free_data_slot_dylibloader_wrapper_dbus
#define dbus_connection_set_data dbus_connection_set_data_dylibloader_wrapper_dbus
#define dbus_connection_get_data dbus_connection_get_data_dylibloader_wrapper_dbus
#define dbus_connection_set_change_sigpipe dbus_connection_set_change_sigpipe_dylibloader_wrapper_dbus
#define dbus_connection_set_max_message_size dbus_connection_set_max_message_size_dylibloader_wrapper_dbus
#define dbus_connection_get_max_message_size dbus_connection_get_max_message_size_dylibloader_wrapper_dbus
#define dbus_connection_set_max_received_size dbus_connection_set_max_received_size_dylibloader_wrapper_dbus
#define dbus_connection_get_max_received_size dbus_connection_get_max_received_size_dylibloader_wrapper_dbus
#define dbus_connection_set_max_message_unix_fds dbus_connection_set_max_message_unix_fds_dylibloader_wrapper_dbus
#define dbus_connection_get_max_message_unix_fds dbus_connection_get_max_message_unix_fds_dylibloader_wrapper_dbus
#define dbus_connection_set_max_received_unix_fds dbus_connection_set_max_received_unix_fds_dylibloader_wrapper_dbus
#define dbus_connection_get_max_received_unix_fds dbus_connection_get_max_received_unix_fds_dylibloader_wrapper_dbus
#define dbus_connection_get_outgoing_size dbus_connection_get_outgoing_size_dylibloader_wrapper_dbus
#define dbus_connection_get_outgoing_unix_fds dbus_connection_get_outgoing_unix_fds_dylibloader_wrapper_dbus
#define dbus_connection_preallocate_send dbus_connection_preallocate_send_dylibloader_wrapper_dbus
#define dbus_connection_free_preallocated_send dbus_connection_free_preallocated_send_dylibloader_wrapper_dbus
#define dbus_connection_send_preallocated dbus_connection_send_preallocated_dylibloader_wrapper_dbus
#define dbus_connection_try_register_object_path dbus_connection_try_register_object_path_dylibloader_wrapper_dbus
#define dbus_connection_register_object_path dbus_connection_register_object_path_dylibloader_wrapper_dbus
#define dbus_connection_try_register_fallback dbus_connection_try_register_fallback_dylibloader_wrapper_dbus
#define dbus_connection_register_fallback dbus_connection_register_fallback_dylibloader_wrapper_dbus
#define dbus_connection_unregister_object_path dbus_connection_unregister_object_path_dylibloader_wrapper_dbus
#define dbus_connection_get_object_path_data dbus_connection_get_object_path_data_dylibloader_wrapper_dbus
#define dbus_connection_list_registered dbus_connection_list_registered_dylibloader_wrapper_dbus
#define dbus_connection_get_unix_fd dbus_connection_get_unix_fd_dylibloader_wrapper_dbus
#define dbus_connection_get_socket dbus_connection_get_socket_dylibloader_wrapper_dbus
#define dbus_watch_get_fd dbus_watch_get_fd_dylibloader_wrapper_dbus
#define dbus_watch_get_unix_fd dbus_watch_get_unix_fd_dylibloader_wrapper_dbus
#define dbus_watch_get_socket dbus_watch_get_socket_dylibloader_wrapper_dbus
#define dbus_watch_get_flags dbus_watch_get_flags_dylibloader_wrapper_dbus
#define dbus_watch_get_data dbus_watch_get_data_dylibloader_wrapper_dbus
#define dbus_watch_set_data dbus_watch_set_data_dylibloader_wrapper_dbus
#define dbus_watch_handle dbus_watch_handle_dylibloader_wrapper_dbus
#define dbus_watch_get_enabled dbus_watch_get_enabled_dylibloader_wrapper_dbus
#define dbus_timeout_get_interval dbus_timeout_get_interval_dylibloader_wrapper_dbus
#define dbus_timeout_get_data dbus_timeout_get_data_dylibloader_wrapper_dbus
#define dbus_timeout_set_data dbus_timeout_set_data_dylibloader_wrapper_dbus
#define dbus_timeout_handle dbus_timeout_handle_dylibloader_wrapper_dbus
#define dbus_timeout_get_enabled dbus_timeout_get_enabled_dylibloader_wrapper_dbus
#define dbus_bus_get dbus_bus_get_dylibloader_wrapper_dbus
#define dbus_bus_get_private dbus_bus_get_private_dylibloader_wrapper_dbus
#define dbus_bus_register dbus_bus_register_dylibloader_wrapper_dbus
#define dbus_bus_set_unique_name dbus_bus_set_unique_name_dylibloader_wrapper_dbus
#define dbus_bus_get_unique_name dbus_bus_get_unique_name_dylibloader_wrapper_dbus
#define dbus_bus_get_unix_user dbus_bus_get_unix_user_dylibloader_wrapper_dbus
#define dbus_bus_get_id dbus_bus_get_id_dylibloader_wrapper_dbus
#define dbus_bus_request_name dbus_bus_request_name_dylibloader_wrapper_dbus
#define dbus_bus_release_name dbus_bus_release_name_dylibloader_wrapper_dbus
#define dbus_bus_name_has_owner dbus_bus_name_has_owner_dylibloader_wrapper_dbus
#define dbus_bus_start_service_by_name dbus_bus_start_service_by_name_dylibloader_wrapper_dbus
#define dbus_bus_add_match dbus_bus_add_match_dylibloader_wrapper_dbus
#define dbus_bus_remove_match dbus_bus_remove_match_dylibloader_wrapper_dbus
#define dbus_get_local_machine_id dbus_get_local_machine_id_dylibloader_wrapper_dbus
#define dbus_get_version dbus_get_version_dylibloader_wrapper_dbus
#define dbus_setenv dbus_setenv_dylibloader_wrapper_dbus
#define dbus_try_get_local_machine_id dbus_try_get_local_machine_id_dylibloader_wrapper_dbus
#define dbus_pending_call_ref dbus_pending_call_ref_dylibloader_wrapper_dbus
#define dbus_pending_call_unref dbus_pending_call_unref_dylibloader_wrapper_dbus
#define dbus_pending_call_set_notify dbus_pending_call_set_notify_dylibloader_wrapper_dbus
#define dbus_pending_call_cancel dbus_pending_call_cancel_dylibloader_wrapper_dbus
#define dbus_pending_call_get_completed dbus_pending_call_get_completed_dylibloader_wrapper_dbus
#define dbus_pending_call_steal_reply dbus_pending_call_steal_reply_dylibloader_wrapper_dbus
#define dbus_pending_call_block dbus_pending_call_block_dylibloader_wrapper_dbus
#define dbus_pending_call_allocate_data_slot dbus_pending_call_allocate_data_slot_dylibloader_wrapper_dbus
#define dbus_pending_call_free_data_slot dbus_pending_call_free_data_slot_dylibloader_wrapper_dbus
#define dbus_pending_call_set_data dbus_pending_call_set_data_dylibloader_wrapper_dbus
#define dbus_pending_call_get_data dbus_pending_call_get_data_dylibloader_wrapper_dbus
#define dbus_server_listen dbus_server_listen_dylibloader_wrapper_dbus
#define dbus_server_ref dbus_server_ref_dylibloader_wrapper_dbus
#define dbus_server_unref dbus_server_unref_dylibloader_wrapper_dbus
#define dbus_server_disconnect dbus_server_disconnect_dylibloader_wrapper_dbus
#define dbus_server_get_is_connected dbus_server_get_is_connected_dylibloader_wrapper_dbus
#define dbus_server_get_address dbus_server_get_address_dylibloader_wrapper_dbus
#define dbus_server_get_id dbus_server_get_id_dylibloader_wrapper_dbus
#define dbus_server_set_new_connection_function dbus_server_set_new_connection_function_dylibloader_wrapper_dbus
#define dbus_server_set_watch_functions dbus_server_set_watch_functions_dylibloader_wrapper_dbus
#define dbus_server_set_timeout_functions dbus_server_set_timeout_functions_dylibloader_wrapper_dbus
#define dbus_server_set_auth_mechanisms dbus_server_set_auth_mechanisms_dylibloader_wrapper_dbus
#define dbus_server_allocate_data_slot dbus_server_allocate_data_slot_dylibloader_wrapper_dbus
#define dbus_server_free_data_slot dbus_server_free_data_slot_dylibloader_wrapper_dbus
#define dbus_server_set_data dbus_server_set_data_dylibloader_wrapper_dbus
#define dbus_server_get_data dbus_server_get_data_dylibloader_wrapper_dbus
#define dbus_signature_iter_init dbus_signature_iter_init_dylibloader_wrapper_dbus
#define dbus_signature_iter_get_current_type dbus_signature_iter_get_current_type_dylibloader_wrapper_dbus
#define dbus_signature_iter_get_signature dbus_signature_iter_get_signature_dylibloader_wrapper_dbus
#define dbus_signature_iter_get_element_type dbus_signature_iter_get_element_type_dylibloader_wrapper_dbus
#define dbus_signature_iter_next dbus_signature_iter_next_dylibloader_wrapper_dbus
#define dbus_signature_iter_recurse dbus_signature_iter_recurse_dylibloader_wrapper_dbus
#define dbus_signature_validate dbus_signature_validate_dylibloader_wrapper_dbus
#define dbus_signature_validate_single dbus_signature_validate_single_dylibloader_wrapper_dbus
#define dbus_type_is_valid dbus_type_is_valid_dylibloader_wrapper_dbus
#define dbus_type_is_basic dbus_type_is_basic_dylibloader_wrapper_dbus
#define dbus_type_is_container dbus_type_is_container_dylibloader_wrapper_dbus
#define dbus_type_is_fixed dbus_type_is_fixed_dylibloader_wrapper_dbus
#define dbus_validate_path dbus_validate_path_dylibloader_wrapper_dbus
#define dbus_validate_interface dbus_validate_interface_dylibloader_wrapper_dbus
#define dbus_validate_member dbus_validate_member_dylibloader_wrapper_dbus
#define dbus_validate_error_name dbus_validate_error_name_dylibloader_wrapper_dbus
#define dbus_validate_bus_name dbus_validate_bus_name_dylibloader_wrapper_dbus
#define dbus_validate_utf8 dbus_validate_utf8_dylibloader_wrapper_dbus
#define dbus_threads_init dbus_threads_init_dylibloader_wrapper_dbus
#define dbus_threads_init_default dbus_threads_init_default_dylibloader_wrapper_dbus
extern void (*dbus_error_init_dylibloader_wrapper_dbus)( DBusError*);
extern void (*dbus_error_free_dylibloader_wrapper_dbus)( DBusError*);
extern void (*dbus_set_error_dylibloader_wrapper_dbus)( DBusError*,const char*,const char*,...);
extern void (*dbus_set_error_const_dylibloader_wrapper_dbus)( DBusError*,const char*,const char*);
extern void (*dbus_move_error_dylibloader_wrapper_dbus)( DBusError*, DBusError*);
extern dbus_bool_t (*dbus_error_has_name_dylibloader_wrapper_dbus)(const DBusError*,const char*);
extern dbus_bool_t (*dbus_error_is_set_dylibloader_wrapper_dbus)(const DBusError*);
extern dbus_bool_t (*dbus_parse_address_dylibloader_wrapper_dbus)(const char*, DBusAddressEntry***, int*, DBusError*);
extern const char* (*dbus_address_entry_get_value_dylibloader_wrapper_dbus)( DBusAddressEntry*,const char*);
extern const char* (*dbus_address_entry_get_method_dylibloader_wrapper_dbus)( DBusAddressEntry*);
extern void (*dbus_address_entries_free_dylibloader_wrapper_dbus)( DBusAddressEntry**);
extern char* (*dbus_address_escape_value_dylibloader_wrapper_dbus)(const char*);
extern char* (*dbus_address_unescape_value_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern void* (*dbus_malloc_dylibloader_wrapper_dbus)( size_t);
extern void* (*dbus_malloc0_dylibloader_wrapper_dbus)( size_t);
extern void* (*dbus_realloc_dylibloader_wrapper_dbus)( void*, size_t);
extern void (*dbus_free_dylibloader_wrapper_dbus)( void*);
extern void (*dbus_free_string_array_dylibloader_wrapper_dbus)( char**);
extern void (*dbus_shutdown_dylibloader_wrapper_dbus)( void);
extern DBusMessage* (*dbus_message_new_dylibloader_wrapper_dbus)( int);
extern DBusMessage* (*dbus_message_new_method_call_dylibloader_wrapper_dbus)(const char*,const char*,const char*,const char*);
extern DBusMessage* (*dbus_message_new_method_return_dylibloader_wrapper_dbus)( DBusMessage*);
extern DBusMessage* (*dbus_message_new_signal_dylibloader_wrapper_dbus)(const char*,const char*,const char*);
extern DBusMessage* (*dbus_message_new_error_dylibloader_wrapper_dbus)( DBusMessage*,const char*,const char*);
extern DBusMessage* (*dbus_message_new_error_printf_dylibloader_wrapper_dbus)( DBusMessage*,const char*,const char*,...);
extern DBusMessage* (*dbus_message_copy_dylibloader_wrapper_dbus)(const DBusMessage*);
extern DBusMessage* (*dbus_message_ref_dylibloader_wrapper_dbus)( DBusMessage*);
extern void (*dbus_message_unref_dylibloader_wrapper_dbus)( DBusMessage*);
extern int (*dbus_message_get_type_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_message_set_path_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern const char* (*dbus_message_get_path_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_message_has_path_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern dbus_bool_t (*dbus_message_set_interface_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern const char* (*dbus_message_get_interface_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_message_has_interface_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern dbus_bool_t (*dbus_message_set_member_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern const char* (*dbus_message_get_member_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_message_has_member_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern dbus_bool_t (*dbus_message_set_error_name_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern const char* (*dbus_message_get_error_name_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_message_set_destination_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern const char* (*dbus_message_get_destination_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_message_set_sender_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern const char* (*dbus_message_get_sender_dylibloader_wrapper_dbus)( DBusMessage*);
extern const char* (*dbus_message_get_signature_dylibloader_wrapper_dbus)( DBusMessage*);
extern void (*dbus_message_set_no_reply_dylibloader_wrapper_dbus)( DBusMessage*, dbus_bool_t);
extern dbus_bool_t (*dbus_message_get_no_reply_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_message_is_method_call_dylibloader_wrapper_dbus)( DBusMessage*,const char*,const char*);
extern dbus_bool_t (*dbus_message_is_signal_dylibloader_wrapper_dbus)( DBusMessage*,const char*,const char*);
extern dbus_bool_t (*dbus_message_is_error_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern dbus_bool_t (*dbus_message_has_destination_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern dbus_bool_t (*dbus_message_has_sender_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern dbus_bool_t (*dbus_message_has_signature_dylibloader_wrapper_dbus)( DBusMessage*,const char*);
extern dbus_uint32_t (*dbus_message_get_serial_dylibloader_wrapper_dbus)( DBusMessage*);
extern void (*dbus_message_set_serial_dylibloader_wrapper_dbus)( DBusMessage*, dbus_uint32_t);
extern dbus_bool_t (*dbus_message_set_reply_serial_dylibloader_wrapper_dbus)( DBusMessage*, dbus_uint32_t);
extern dbus_uint32_t (*dbus_message_get_reply_serial_dylibloader_wrapper_dbus)( DBusMessage*);
extern void (*dbus_message_set_auto_start_dylibloader_wrapper_dbus)( DBusMessage*, dbus_bool_t);
extern dbus_bool_t (*dbus_message_get_auto_start_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_message_get_path_decomposed_dylibloader_wrapper_dbus)( DBusMessage*, char***);
extern dbus_bool_t (*dbus_message_append_args_dylibloader_wrapper_dbus)( DBusMessage*, int,...);
extern dbus_bool_t (*dbus_message_append_args_valist_dylibloader_wrapper_dbus)( DBusMessage*, int, va_list);
extern dbus_bool_t (*dbus_message_get_args_dylibloader_wrapper_dbus)( DBusMessage*, DBusError*, int,...);
extern dbus_bool_t (*dbus_message_get_args_valist_dylibloader_wrapper_dbus)( DBusMessage*, DBusError*, int, va_list);
extern dbus_bool_t (*dbus_message_contains_unix_fds_dylibloader_wrapper_dbus)( DBusMessage*);
extern void (*dbus_message_iter_init_closed_dylibloader_wrapper_dbus)( DBusMessageIter*);
extern dbus_bool_t (*dbus_message_iter_init_dylibloader_wrapper_dbus)( DBusMessage*, DBusMessageIter*);
extern dbus_bool_t (*dbus_message_iter_has_next_dylibloader_wrapper_dbus)( DBusMessageIter*);
extern dbus_bool_t (*dbus_message_iter_next_dylibloader_wrapper_dbus)( DBusMessageIter*);
extern char* (*dbus_message_iter_get_signature_dylibloader_wrapper_dbus)( DBusMessageIter*);
extern int (*dbus_message_iter_get_arg_type_dylibloader_wrapper_dbus)( DBusMessageIter*);
extern int (*dbus_message_iter_get_element_type_dylibloader_wrapper_dbus)( DBusMessageIter*);
extern void (*dbus_message_iter_recurse_dylibloader_wrapper_dbus)( DBusMessageIter*, DBusMessageIter*);
extern void (*dbus_message_iter_get_basic_dylibloader_wrapper_dbus)( DBusMessageIter*, void*);
extern int (*dbus_message_iter_get_element_count_dylibloader_wrapper_dbus)( DBusMessageIter*);
extern int (*dbus_message_iter_get_array_len_dylibloader_wrapper_dbus)( DBusMessageIter*);
extern void (*dbus_message_iter_get_fixed_array_dylibloader_wrapper_dbus)( DBusMessageIter*, void*, int*);
extern void (*dbus_message_iter_init_append_dylibloader_wrapper_dbus)( DBusMessage*, DBusMessageIter*);
extern dbus_bool_t (*dbus_message_iter_append_basic_dylibloader_wrapper_dbus)( DBusMessageIter*, int,const void*);
extern dbus_bool_t (*dbus_message_iter_append_fixed_array_dylibloader_wrapper_dbus)( DBusMessageIter*, int,const void*, int);
extern dbus_bool_t (*dbus_message_iter_open_container_dylibloader_wrapper_dbus)( DBusMessageIter*, int,const char*, DBusMessageIter*);
extern dbus_bool_t (*dbus_message_iter_close_container_dylibloader_wrapper_dbus)( DBusMessageIter*, DBusMessageIter*);
extern void (*dbus_message_iter_abandon_container_dylibloader_wrapper_dbus)( DBusMessageIter*, DBusMessageIter*);
extern void (*dbus_message_iter_abandon_container_if_open_dylibloader_wrapper_dbus)( DBusMessageIter*, DBusMessageIter*);
extern void (*dbus_message_lock_dylibloader_wrapper_dbus)( DBusMessage*);
extern dbus_bool_t (*dbus_set_error_from_message_dylibloader_wrapper_dbus)( DBusError*, DBusMessage*);
extern dbus_bool_t (*dbus_message_allocate_data_slot_dylibloader_wrapper_dbus)( dbus_int32_t*);
extern void (*dbus_message_free_data_slot_dylibloader_wrapper_dbus)( dbus_int32_t*);
extern dbus_bool_t (*dbus_message_set_data_dylibloader_wrapper_dbus)( DBusMessage*, dbus_int32_t, void*, DBusFreeFunction);
extern void* (*dbus_message_get_data_dylibloader_wrapper_dbus)( DBusMessage*, dbus_int32_t);
extern int (*dbus_message_type_from_string_dylibloader_wrapper_dbus)(const char*);
extern const char* (*dbus_message_type_to_string_dylibloader_wrapper_dbus)( int);
extern dbus_bool_t (*dbus_message_marshal_dylibloader_wrapper_dbus)( DBusMessage*, char**, int*);
extern DBusMessage* (*dbus_message_demarshal_dylibloader_wrapper_dbus)(const char*, int, DBusError*);
extern int (*dbus_message_demarshal_bytes_needed_dylibloader_wrapper_dbus)(const char*, int);
extern void (*dbus_message_set_allow_interactive_authorization_dylibloader_wrapper_dbus)( DBusMessage*, dbus_bool_t);
extern dbus_bool_t (*dbus_message_get_allow_interactive_authorization_dylibloader_wrapper_dbus)( DBusMessage*);
extern DBusConnection* (*dbus_connection_open_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern DBusConnection* (*dbus_connection_open_private_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern DBusConnection* (*dbus_connection_ref_dylibloader_wrapper_dbus)( DBusConnection*);
extern void (*dbus_connection_unref_dylibloader_wrapper_dbus)( DBusConnection*);
extern void (*dbus_connection_close_dylibloader_wrapper_dbus)( DBusConnection*);
extern dbus_bool_t (*dbus_connection_get_is_connected_dylibloader_wrapper_dbus)( DBusConnection*);
extern dbus_bool_t (*dbus_connection_get_is_authenticated_dylibloader_wrapper_dbus)( DBusConnection*);
extern dbus_bool_t (*dbus_connection_get_is_anonymous_dylibloader_wrapper_dbus)( DBusConnection*);
extern char* (*dbus_connection_get_server_id_dylibloader_wrapper_dbus)( DBusConnection*);
extern dbus_bool_t (*dbus_connection_can_send_type_dylibloader_wrapper_dbus)( DBusConnection*, int);
extern void (*dbus_connection_set_exit_on_disconnect_dylibloader_wrapper_dbus)( DBusConnection*, dbus_bool_t);
extern void (*dbus_connection_flush_dylibloader_wrapper_dbus)( DBusConnection*);
extern dbus_bool_t (*dbus_connection_read_write_dispatch_dylibloader_wrapper_dbus)( DBusConnection*, int);
extern dbus_bool_t (*dbus_connection_read_write_dylibloader_wrapper_dbus)( DBusConnection*, int);
extern DBusMessage* (*dbus_connection_borrow_message_dylibloader_wrapper_dbus)( DBusConnection*);
extern void (*dbus_connection_return_message_dylibloader_wrapper_dbus)( DBusConnection*, DBusMessage*);
extern void (*dbus_connection_steal_borrowed_message_dylibloader_wrapper_dbus)( DBusConnection*, DBusMessage*);
extern DBusMessage* (*dbus_connection_pop_message_dylibloader_wrapper_dbus)( DBusConnection*);
extern DBusDispatchStatus (*dbus_connection_get_dispatch_status_dylibloader_wrapper_dbus)( DBusConnection*);
extern DBusDispatchStatus (*dbus_connection_dispatch_dylibloader_wrapper_dbus)( DBusConnection*);
extern dbus_bool_t (*dbus_connection_has_messages_to_send_dylibloader_wrapper_dbus)( DBusConnection*);
extern dbus_bool_t (*dbus_connection_send_dylibloader_wrapper_dbus)( DBusConnection*, DBusMessage*, dbus_uint32_t*);
extern dbus_bool_t (*dbus_connection_send_with_reply_dylibloader_wrapper_dbus)( DBusConnection*, DBusMessage*, DBusPendingCall**, int);
extern DBusMessage* (*dbus_connection_send_with_reply_and_block_dylibloader_wrapper_dbus)( DBusConnection*, DBusMessage*, int, DBusError*);
extern dbus_bool_t (*dbus_connection_set_watch_functions_dylibloader_wrapper_dbus)( DBusConnection*, DBusAddWatchFunction, DBusRemoveWatchFunction, DBusWatchToggledFunction, void*, DBusFreeFunction);
extern dbus_bool_t (*dbus_connection_set_timeout_functions_dylibloader_wrapper_dbus)( DBusConnection*, DBusAddTimeoutFunction, DBusRemoveTimeoutFunction, DBusTimeoutToggledFunction, void*, DBusFreeFunction);
extern void (*dbus_connection_set_wakeup_main_function_dylibloader_wrapper_dbus)( DBusConnection*, DBusWakeupMainFunction, void*, DBusFreeFunction);
extern void (*dbus_connection_set_dispatch_status_function_dylibloader_wrapper_dbus)( DBusConnection*, DBusDispatchStatusFunction, void*, DBusFreeFunction);
extern dbus_bool_t (*dbus_connection_get_unix_user_dylibloader_wrapper_dbus)( DBusConnection*, unsigned long*);
extern dbus_bool_t (*dbus_connection_get_unix_process_id_dylibloader_wrapper_dbus)( DBusConnection*, unsigned long*);
extern dbus_bool_t (*dbus_connection_get_adt_audit_session_data_dylibloader_wrapper_dbus)( DBusConnection*, void**, dbus_int32_t*);
extern void (*dbus_connection_set_unix_user_function_dylibloader_wrapper_dbus)( DBusConnection*, DBusAllowUnixUserFunction, void*, DBusFreeFunction);
extern dbus_bool_t (*dbus_connection_get_windows_user_dylibloader_wrapper_dbus)( DBusConnection*, char**);
extern void (*dbus_connection_set_windows_user_function_dylibloader_wrapper_dbus)( DBusConnection*, DBusAllowWindowsUserFunction, void*, DBusFreeFunction);
extern void (*dbus_connection_set_allow_anonymous_dylibloader_wrapper_dbus)( DBusConnection*, dbus_bool_t);
extern void (*dbus_connection_set_route_peer_messages_dylibloader_wrapper_dbus)( DBusConnection*, dbus_bool_t);
extern dbus_bool_t (*dbus_connection_add_filter_dylibloader_wrapper_dbus)( DBusConnection*, DBusHandleMessageFunction, void*, DBusFreeFunction);
extern void (*dbus_connection_remove_filter_dylibloader_wrapper_dbus)( DBusConnection*, DBusHandleMessageFunction, void*);
extern dbus_bool_t (*dbus_connection_allocate_data_slot_dylibloader_wrapper_dbus)( dbus_int32_t*);
extern void (*dbus_connection_free_data_slot_dylibloader_wrapper_dbus)( dbus_int32_t*);
extern dbus_bool_t (*dbus_connection_set_data_dylibloader_wrapper_dbus)( DBusConnection*, dbus_int32_t, void*, DBusFreeFunction);
extern void* (*dbus_connection_get_data_dylibloader_wrapper_dbus)( DBusConnection*, dbus_int32_t);
extern void (*dbus_connection_set_change_sigpipe_dylibloader_wrapper_dbus)( dbus_bool_t);
extern void (*dbus_connection_set_max_message_size_dylibloader_wrapper_dbus)( DBusConnection*, long);
extern long (*dbus_connection_get_max_message_size_dylibloader_wrapper_dbus)( DBusConnection*);
extern void (*dbus_connection_set_max_received_size_dylibloader_wrapper_dbus)( DBusConnection*, long);
extern long (*dbus_connection_get_max_received_size_dylibloader_wrapper_dbus)( DBusConnection*);
extern void (*dbus_connection_set_max_message_unix_fds_dylibloader_wrapper_dbus)( DBusConnection*, long);
extern long (*dbus_connection_get_max_message_unix_fds_dylibloader_wrapper_dbus)( DBusConnection*);
extern void (*dbus_connection_set_max_received_unix_fds_dylibloader_wrapper_dbus)( DBusConnection*, long);
extern long (*dbus_connection_get_max_received_unix_fds_dylibloader_wrapper_dbus)( DBusConnection*);
extern long (*dbus_connection_get_outgoing_size_dylibloader_wrapper_dbus)( DBusConnection*);
extern long (*dbus_connection_get_outgoing_unix_fds_dylibloader_wrapper_dbus)( DBusConnection*);
extern DBusPreallocatedSend* (*dbus_connection_preallocate_send_dylibloader_wrapper_dbus)( DBusConnection*);
extern void (*dbus_connection_free_preallocated_send_dylibloader_wrapper_dbus)( DBusConnection*, DBusPreallocatedSend*);
extern void (*dbus_connection_send_preallocated_dylibloader_wrapper_dbus)( DBusConnection*, DBusPreallocatedSend*, DBusMessage*, dbus_uint32_t*);
extern dbus_bool_t (*dbus_connection_try_register_object_path_dylibloader_wrapper_dbus)( DBusConnection*,const char*,const DBusObjectPathVTable*, void*, DBusError*);
extern dbus_bool_t (*dbus_connection_register_object_path_dylibloader_wrapper_dbus)( DBusConnection*,const char*,const DBusObjectPathVTable*, void*);
extern dbus_bool_t (*dbus_connection_try_register_fallback_dylibloader_wrapper_dbus)( DBusConnection*,const char*,const DBusObjectPathVTable*, void*, DBusError*);
extern dbus_bool_t (*dbus_connection_register_fallback_dylibloader_wrapper_dbus)( DBusConnection*,const char*,const DBusObjectPathVTable*, void*);
extern dbus_bool_t (*dbus_connection_unregister_object_path_dylibloader_wrapper_dbus)( DBusConnection*,const char*);
extern dbus_bool_t (*dbus_connection_get_object_path_data_dylibloader_wrapper_dbus)( DBusConnection*,const char*, void**);
extern dbus_bool_t (*dbus_connection_list_registered_dylibloader_wrapper_dbus)( DBusConnection*,const char*, char***);
extern dbus_bool_t (*dbus_connection_get_unix_fd_dylibloader_wrapper_dbus)( DBusConnection*, int*);
extern dbus_bool_t (*dbus_connection_get_socket_dylibloader_wrapper_dbus)( DBusConnection*, int*);
extern int (*dbus_watch_get_fd_dylibloader_wrapper_dbus)( DBusWatch*);
extern int (*dbus_watch_get_unix_fd_dylibloader_wrapper_dbus)( DBusWatch*);
extern int (*dbus_watch_get_socket_dylibloader_wrapper_dbus)( DBusWatch*);
extern unsigned int (*dbus_watch_get_flags_dylibloader_wrapper_dbus)( DBusWatch*);
extern void* (*dbus_watch_get_data_dylibloader_wrapper_dbus)( DBusWatch*);
extern void (*dbus_watch_set_data_dylibloader_wrapper_dbus)( DBusWatch*, void*, DBusFreeFunction);
extern dbus_bool_t (*dbus_watch_handle_dylibloader_wrapper_dbus)( DBusWatch*, unsigned int);
extern dbus_bool_t (*dbus_watch_get_enabled_dylibloader_wrapper_dbus)( DBusWatch*);
extern int (*dbus_timeout_get_interval_dylibloader_wrapper_dbus)( DBusTimeout*);
extern void* (*dbus_timeout_get_data_dylibloader_wrapper_dbus)( DBusTimeout*);
extern void (*dbus_timeout_set_data_dylibloader_wrapper_dbus)( DBusTimeout*, void*, DBusFreeFunction);
extern dbus_bool_t (*dbus_timeout_handle_dylibloader_wrapper_dbus)( DBusTimeout*);
extern dbus_bool_t (*dbus_timeout_get_enabled_dylibloader_wrapper_dbus)( DBusTimeout*);
extern DBusConnection* (*dbus_bus_get_dylibloader_wrapper_dbus)( DBusBusType, DBusError*);
extern DBusConnection* (*dbus_bus_get_private_dylibloader_wrapper_dbus)( DBusBusType, DBusError*);
extern dbus_bool_t (*dbus_bus_register_dylibloader_wrapper_dbus)( DBusConnection*, DBusError*);
extern dbus_bool_t (*dbus_bus_set_unique_name_dylibloader_wrapper_dbus)( DBusConnection*,const char*);
extern const char* (*dbus_bus_get_unique_name_dylibloader_wrapper_dbus)( DBusConnection*);
extern unsigned long (*dbus_bus_get_unix_user_dylibloader_wrapper_dbus)( DBusConnection*,const char*, DBusError*);
extern char* (*dbus_bus_get_id_dylibloader_wrapper_dbus)( DBusConnection*, DBusError*);
extern int (*dbus_bus_request_name_dylibloader_wrapper_dbus)( DBusConnection*,const char*, unsigned int, DBusError*);
extern int (*dbus_bus_release_name_dylibloader_wrapper_dbus)( DBusConnection*,const char*, DBusError*);
extern dbus_bool_t (*dbus_bus_name_has_owner_dylibloader_wrapper_dbus)( DBusConnection*,const char*, DBusError*);
extern dbus_bool_t (*dbus_bus_start_service_by_name_dylibloader_wrapper_dbus)( DBusConnection*,const char*, dbus_uint32_t, dbus_uint32_t*, DBusError*);
extern void (*dbus_bus_add_match_dylibloader_wrapper_dbus)( DBusConnection*,const char*, DBusError*);
extern void (*dbus_bus_remove_match_dylibloader_wrapper_dbus)( DBusConnection*,const char*, DBusError*);
extern char* (*dbus_get_local_machine_id_dylibloader_wrapper_dbus)( void);
extern void (*dbus_get_version_dylibloader_wrapper_dbus)( int*, int*, int*);
extern dbus_bool_t (*dbus_setenv_dylibloader_wrapper_dbus)(const char*,const char*);
extern char* (*dbus_try_get_local_machine_id_dylibloader_wrapper_dbus)( DBusError*);
extern DBusPendingCall* (*dbus_pending_call_ref_dylibloader_wrapper_dbus)( DBusPendingCall*);
extern void (*dbus_pending_call_unref_dylibloader_wrapper_dbus)( DBusPendingCall*);
extern dbus_bool_t (*dbus_pending_call_set_notify_dylibloader_wrapper_dbus)( DBusPendingCall*, DBusPendingCallNotifyFunction, void*, DBusFreeFunction);
extern void (*dbus_pending_call_cancel_dylibloader_wrapper_dbus)( DBusPendingCall*);
extern dbus_bool_t (*dbus_pending_call_get_completed_dylibloader_wrapper_dbus)( DBusPendingCall*);
extern DBusMessage* (*dbus_pending_call_steal_reply_dylibloader_wrapper_dbus)( DBusPendingCall*);
extern void (*dbus_pending_call_block_dylibloader_wrapper_dbus)( DBusPendingCall*);
extern dbus_bool_t (*dbus_pending_call_allocate_data_slot_dylibloader_wrapper_dbus)( dbus_int32_t*);
extern void (*dbus_pending_call_free_data_slot_dylibloader_wrapper_dbus)( dbus_int32_t*);
extern dbus_bool_t (*dbus_pending_call_set_data_dylibloader_wrapper_dbus)( DBusPendingCall*, dbus_int32_t, void*, DBusFreeFunction);
extern void* (*dbus_pending_call_get_data_dylibloader_wrapper_dbus)( DBusPendingCall*, dbus_int32_t);
extern DBusServer* (*dbus_server_listen_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern DBusServer* (*dbus_server_ref_dylibloader_wrapper_dbus)( DBusServer*);
extern void (*dbus_server_unref_dylibloader_wrapper_dbus)( DBusServer*);
extern void (*dbus_server_disconnect_dylibloader_wrapper_dbus)( DBusServer*);
extern dbus_bool_t (*dbus_server_get_is_connected_dylibloader_wrapper_dbus)( DBusServer*);
extern char* (*dbus_server_get_address_dylibloader_wrapper_dbus)( DBusServer*);
extern char* (*dbus_server_get_id_dylibloader_wrapper_dbus)( DBusServer*);
extern void (*dbus_server_set_new_connection_function_dylibloader_wrapper_dbus)( DBusServer*, DBusNewConnectionFunction, void*, DBusFreeFunction);
extern dbus_bool_t (*dbus_server_set_watch_functions_dylibloader_wrapper_dbus)( DBusServer*, DBusAddWatchFunction, DBusRemoveWatchFunction, DBusWatchToggledFunction, void*, DBusFreeFunction);
extern dbus_bool_t (*dbus_server_set_timeout_functions_dylibloader_wrapper_dbus)( DBusServer*, DBusAddTimeoutFunction, DBusRemoveTimeoutFunction, DBusTimeoutToggledFunction, void*, DBusFreeFunction);
extern dbus_bool_t (*dbus_server_set_auth_mechanisms_dylibloader_wrapper_dbus)( DBusServer*,const char**);
extern dbus_bool_t (*dbus_server_allocate_data_slot_dylibloader_wrapper_dbus)( dbus_int32_t*);
extern void (*dbus_server_free_data_slot_dylibloader_wrapper_dbus)( dbus_int32_t*);
extern dbus_bool_t (*dbus_server_set_data_dylibloader_wrapper_dbus)( DBusServer*, int, void*, DBusFreeFunction);
extern void* (*dbus_server_get_data_dylibloader_wrapper_dbus)( DBusServer*, int);
extern void (*dbus_signature_iter_init_dylibloader_wrapper_dbus)( DBusSignatureIter*,const char*);
extern int (*dbus_signature_iter_get_current_type_dylibloader_wrapper_dbus)(const DBusSignatureIter*);
extern char* (*dbus_signature_iter_get_signature_dylibloader_wrapper_dbus)(const DBusSignatureIter*);
extern int (*dbus_signature_iter_get_element_type_dylibloader_wrapper_dbus)(const DBusSignatureIter*);
extern dbus_bool_t (*dbus_signature_iter_next_dylibloader_wrapper_dbus)( DBusSignatureIter*);
extern void (*dbus_signature_iter_recurse_dylibloader_wrapper_dbus)(const DBusSignatureIter*, DBusSignatureIter*);
extern dbus_bool_t (*dbus_signature_validate_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern dbus_bool_t (*dbus_signature_validate_single_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern dbus_bool_t (*dbus_type_is_valid_dylibloader_wrapper_dbus)( int);
extern dbus_bool_t (*dbus_type_is_basic_dylibloader_wrapper_dbus)( int);
extern dbus_bool_t (*dbus_type_is_container_dylibloader_wrapper_dbus)( int);
extern dbus_bool_t (*dbus_type_is_fixed_dylibloader_wrapper_dbus)( int);
extern dbus_bool_t (*dbus_validate_path_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern dbus_bool_t (*dbus_validate_interface_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern dbus_bool_t (*dbus_validate_member_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern dbus_bool_t (*dbus_validate_error_name_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern dbus_bool_t (*dbus_validate_bus_name_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern dbus_bool_t (*dbus_validate_utf8_dylibloader_wrapper_dbus)(const char*, DBusError*);
extern dbus_bool_t (*dbus_threads_init_dylibloader_wrapper_dbus)(const DBusThreadFunctions*);
extern dbus_bool_t (*dbus_threads_init_default_dylibloader_wrapper_dbus)( void);
int initialize_dbus(int verbose);
#ifdef __cplusplus
}
#endif
#endif
#endif // defined(LINUXBSD_ENABLED)
// --- End platform/linuxbsd/os_linuxbsd.h ---

// --- Folded from platform/android/os_android.h ---
#if defined(ANDROID_ENABLED)
#ifdef ANDROID_ENABLED
// --- Folded from platform/android/audio_driver_opensl.h ---
#include "core/os/mutex.h"
#include "servers/display/display_server.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

class AudioDriverOpenSL : public AudioDriver {
	bool active = false;
	Mutex mutex;

	enum {
		BUFFER_COUNT = 2
	};

	bool pause = false;

	uint32_t buffer_size = 0;
	int16_t *buffers[BUFFER_COUNT] = {};
	int32_t *mixdown_buffer = nullptr;
	int last_free = 0;

	Vector<int16_t> rec_buffer;

	SLPlayItf playItf = nullptr;
	SLRecordItf recordItf = nullptr;
	SLObjectItf sl = nullptr;
	SLEngineItf EngineItf = nullptr;
	SLObjectItf OutputMix = nullptr;
	SLObjectItf player = nullptr;
	SLObjectItf recorder = nullptr;
	SLAndroidSimpleBufferQueueItf bufferQueueItf = nullptr;
	SLAndroidSimpleBufferQueueItf recordBufferQueueItf = nullptr;
	SLDataSource audioSource;
	SLDataFormat_PCM pcm;
	SLDataSink audioSink;
	SLDataLocator_OutputMix locator_outputmix;

	static AudioDriverOpenSL *s_ad;

	void _buffer_callback(
			SLAndroidSimpleBufferQueueItf queueItf);

	static void _buffer_callbacks(
			SLAndroidSimpleBufferQueueItf queueItf,
			void *pContext);

	void _record_buffer_callback(
			SLAndroidSimpleBufferQueueItf queueItf);

	static void _record_buffer_callbacks(
			SLAndroidSimpleBufferQueueItf queueItf,
			void *pContext);

	Error init_input_device();

public:
	virtual const char *get_name() const override {
		return "Android";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

	virtual Error input_start() override;
	virtual Error input_stop() override;

	void set_pause(bool p_pause);

	AudioDriverOpenSL();
};

// --- End platform/android/audio_driver_opensl.h ---

#include "core/os/os.h"


class GodotJavaWrapper;
class GodotIOJavaWrapper;

struct ANativeWindow;

class OS_Android final
{
public:
	OS_Android() = delete;
	~OS_Android() = delete;
	OS_Android(const OS_Android&) = delete;
	OS_Android& operator=(const OS_Android&) = delete;

	struct FontInfo
	{
		String font_name;
		HashSet<String> lang;
		HashSet<String> script;
		int weight = 400;
		int stretch = 100;
		bool italic = false;
		int priority = 0;
		String filename;
	};

	struct Data
	{
		Size2i display_size;
		bool use_apk_expansion = false;

#if defined(GLES3_ENABLED)
		const char* gl_extensions = nullptr;
#endif

#if defined(VULKAN_ENABLED)
		ANativeWindow* native_window = nullptr;
#endif

		String data_dir_cache;
		String cache_dir_cache;
		String temp_dir_cache;
		String remote_fs_dir;

		AudioDriverOpenSL audio_driver_android;

		HashMap<String, String> font_aliases;
		List<FontInfo> fonts;
		HashSet<String> font_names;
		bool font_config_loaded = false;

		GodotJavaWrapper* godot_java = nullptr;
		GodotIOJavaWrapper* godot_io_java = nullptr;
	};

	static inline Data* data = nullptr;

	static const char* ANDROID_EXEC_PATH;
	static const int DEFAULT_WINDOW_WIDTH = 800;
	static const int DEFAULT_WINDOW_HEIGHT = 600;

private:
	static void _load_system_font_config();
	static String get_system_property(const char* key);
	static String get_dynamic_libraries_path();
	static bool copy_dynamic_library(const String& p_library_path, const String& p_target_dir,
		String* r_copy_path = nullptr);

#ifdef TOOLS_ENABLED
	static void _on_main_screen_changed(const String& p_screen_name);
	static void _on_distraction_free_mode_changed(bool p_enable);
#endif

public:
	static void setup(GodotJavaWrapper* p_godot_java, GodotIOJavaWrapper* p_godot_io_java,
		bool p_use_apk_expansion);
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

#ifdef TOOLS_ENABLED
	static Error sign_apk(const String& p_input_path, const String& p_output_path,
		const String& p_keystore_path, const String& p_keystore_user,
		const String& p_keystore_password);
	static Error verify_apk(const String& p_apk_path);
#endif

	static void initialize_core();
	static void initialize();
	static void initialize_joypads();
	static void delete_main_loop();
	static void finalize();
	static void finalize_core();

	static GodotJavaWrapper* get_godot_java();
	static GodotIOJavaWrapper* get_godot_io_java();

	static bool request_permission(const String& p_name);
	static bool request_permissions();
	static Vector<String> get_granted_permissions();

	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	static Error open_dynamic_library(
		const String& p_path, void*& p_library_handle, GDExtensionData* p_data = nullptr);

	static String get_name();
	static String get_distribution_name();
	static String get_version();
	static String get_version_alias();

	static void main_loop_begin();
	static bool main_loop_iterate(bool* r_should_swap_buffers = nullptr);
	static void main_loop_end();
	static void main_loop_focusout();
	static void main_loop_focusin();

	static void set_display_size(const Size2i& p_size);
	static Size2i get_display_size();

	static void set_opengl_extensions(const char* p_gl_extensions);

	static void set_native_window(ANativeWindow* p_native_window);
	static ANativeWindow* get_native_window();

	static Error shell_open(const String& p_uri);

	static Vector<String> get_system_fonts();
	static String get_system_font_path(
		const String& p_font_name, int p_weight = 400, int p_stretch = 100, bool p_italic = false);
	static Vector<String> get_system_font_path_for_text(const String& p_font_name,
		const String& p_text, const String& p_locale = String(), const String& p_script = String(),
		int p_weight = 400, int p_stretch = 100, bool p_italic = false);
	static String get_executable_path();
	static String get_user_data_dir(const String& p_user_dir);
	static String get_data_path();
	static String get_cache_path();
	static String get_temp_path();
	static String get_resource_dir();
	static String get_locale();
	static String get_model_name();
	static String get_processor_name();

	static String get_unique_id();

	static String get_system_dir(OS::SystemDir p_dir, bool p_shared_storage = true);

	static Error move_to_trash(const String& p_path);

	static void vibrate_handheld(int p_duration_ms, float p_amplitude = -1.0);

	static String get_config_path();

	static Error execute(const String& p_path, const List<String>& p_arguments,
		String* r_pipe = nullptr, int* r_exitcode = nullptr, bool read_stderr = false,
		Mutex* p_pipe_mutex = nullptr, bool p_open_console = false);
	static Error create_process(const String& p_path, const List<String>& p_arguments,
		OS::ProcessID* r_child_id = nullptr, bool p_open_console = false);
	static Error create_instance(
		const List<String>& p_arguments, OS::ProcessID* r_child_id = nullptr);
	static Error kill(const OS::ProcessID& p_pid);
	static String get_system_ca_certificates();

	static Error setup_remote_filesystem(const String& p_server_host, int p_port,
		const String& p_password, String& r_project_path);

	static void benchmark_begin_measure(const String& p_context, const String& p_what);
	static void benchmark_end_measure(const String& p_context, const String& p_what);
	static void benchmark_dump();

	static void load_platform_gdextensions();

	static bool _check_internal_feature_support(const String& p_feature);
};

// --- Folded from platform/android/dir_access_jandroid.h ---


#include <jni.h>

/// Android implementation of the DirAccess interface used to provide access to
/// ACCESS_FILESYSTEM and ACCESS_RESOURCES directory resources.
/// The implementation use jni in order to comply with Android filesystem
/// access restriction.
class DirAccessJAndroid : public DirAccessUnix {
	static jobject dir_access_handler;
	static jclass cls;

	static jmethodID _dir_open;
	static jmethodID _dir_next;
	static jmethodID _dir_close;
	static jmethodID _dir_is_dir;
	static jmethodID _dir_exists;
	static jmethodID _file_exists;
	static jmethodID _get_drive_count;
	static jmethodID _get_drive;
	static jmethodID _make_dir;
	static jmethodID _get_space_left;
	static jmethodID _rename;
	static jmethodID _remove;
	static jmethodID _current_is_hidden;

public:
	virtual Error list_dir_begin() override; ///< This starts dir listing
	virtual String get_next() override;
	virtual bool current_is_dir() const override;
	virtual bool current_is_hidden() const override;
	virtual void list_dir_end() override; ///<

	virtual int get_drive_count() override;
	virtual String get_drive(int p_drive) override;
	virtual String get_current_dir(bool p_include_drive = true) const override; ///< return current dir location

	virtual Error change_dir(String p_dir) override; ///< can be relative or absolute, return false on success

	virtual bool file_exists(String p_file) override;
	virtual bool dir_exists(String p_dir) override;

	virtual Error make_dir(String p_dir) override;
	virtual Error make_dir_recursive(const String &p_dir) override;

	virtual Error rename(String p_from, String p_to) override;
	virtual Error remove(String p_name) override;

	virtual bool is_link(String p_file) override { return false; }
	virtual String read_link(String p_file) override { return p_file; }
	virtual Error create_link(String p_source, String p_target) override { return ERR_UNAVAILABLE; }

	virtual uint64_t get_space_left() override;

	static void setup(jobject p_dir_access_handler);
	static void terminate();

	DirAccessJAndroid();
	~DirAccessJAndroid();

protected:
	String _get_root_string() const override;

private:
	int id = 0;

	int dir_open(String p_path);
	void dir_close(int p_id);
	String get_absolute_path(String p_path);
};

// --- End platform/android/dir_access_jandroid.h ---

// --- Folded from platform/android/file_access_android.h ---
#include "core/io/file_access.h"

#include <android/asset_manager.h>
#include <android/log.h>
#include <jni.h>

class FileAccessAndroid : public FileAccess {
	static AAssetManager *asset_manager;
	static jobject j_asset_manager;

	mutable AAsset *asset = nullptr;
	mutable uint64_t len = 0;
	mutable uint64_t pos = 0;
	mutable bool eof = false;
	String absolute_path;
	String path_src;

	void _close();

public:
	virtual Error open_internal(const String &p_path, int p_mode_flags) override; // open a file
	virtual bool is_open() const override; // true when file is open

	/// returns the path for the current open file
	virtual String get_path() const override;
	/// returns the absolute path for the current open file
	virtual String get_path_absolute() const override;

	virtual void seek(uint64_t p_position) override; // seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; // seek from the end of file
	virtual uint64_t get_position() const override; // get position in the file
	virtual uint64_t get_length() const override; // get size of the file

	virtual bool eof_reached() const override; // reading passed EOF

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }
	virtual uint64_t get_buffer(uint8_t *p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; // get last error

	virtual void flush() override;
	virtual bool store_buffer(const uint8_t *p_src, uint64_t p_length) override;

	virtual bool file_exists(const String &p_path) override; // return true if a file exists

	virtual uint64_t _get_modified_time(const String &p_file) override { return 0; }
	virtual uint64_t _get_access_time(const String &p_file) override { return 0; }
	virtual int64_t _get_size(const String &p_file) override;
	virtual uint32_t _get_unix_permissions(const String &p_file) override { return 0; }
	virtual Error _set_unix_permissions(const String &p_file, uint32_t p_permissions) override { return ERR_UNAVAILABLE; }

	virtual bool _get_hidden_attribute(const String &p_file) override { return false; }
	virtual Error _set_hidden_attribute(const String &p_file, bool p_hidden) override { return ERR_UNAVAILABLE; }
	virtual bool _get_read_only_attribute(const String &p_file) override { return false; }
	virtual Error _set_read_only_attribute(const String &p_file, bool p_ro) override { return ERR_UNAVAILABLE; }

	virtual void close() override;

	static void setup(jobject p_asset_manager);

	static void terminate();

	~FileAccessAndroid();
};

// --- End platform/android/file_access_android.h ---

// --- Folded from platform/android/file_access_filesystem_jandroid.h ---
#include "core/io/file_access.h"

#include <jni.h>

class FileAccessFilesystemJAndroid : public FileAccess {
	static jobject file_access_handler;
	static jclass cls;

	static jmethodID _file_open;
	static jmethodID _file_get_size;
	static jmethodID _file_seek;
	static jmethodID _file_seek_end;
	static jmethodID _file_tell;
	static jmethodID _file_eof;
	static jmethodID _file_set_eof;
	static jmethodID _file_read;
	static jmethodID _file_write;
	static jmethodID _file_flush;
	static jmethodID _file_close;
	static jmethodID _file_exists;
	static jmethodID _file_last_modified;
	static jmethodID _file_last_accessed;
	static jmethodID _file_resize;
	static jmethodID _file_size;

	int id;
	String absolute_path;
	String path_src;

	void _close(); ///< close a file
	void _set_eof(bool eof);

public:
	virtual Error open_internal(const String &p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	/// returns the path for the current open file
	virtual String get_path() const override;
	/// returns the absolute path for the current open file
	virtual String get_path_absolute() const override;

	virtual void seek(uint64_t p_position) override; ///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override; ///< get position in the file
	virtual uint64_t get_length() const override; ///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual Error resize(int64_t p_length) override;
	virtual String get_line() const override; ///< get a line
	virtual uint64_t get_buffer(uint8_t *p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual void flush() override;
	virtual bool store_buffer(const uint8_t *p_src, uint64_t p_length) override;

	virtual bool file_exists(const String &p_path) override; ///< return true if a file exists

	static void setup(jobject p_file_access_handler);
	static void terminate();

	virtual uint64_t _get_modified_time(const String &p_file) override;
	virtual uint64_t _get_access_time(const String &p_file) override;
	virtual int64_t _get_size(const String &p_file) override;
	virtual uint32_t _get_unix_permissions(const String &p_file) override { return 0; }
	virtual Error _set_unix_permissions(const String &p_file, uint32_t p_permissions) override { return ERR_UNAVAILABLE; }

	virtual bool _get_hidden_attribute(const String &p_file) override { return false; }
	virtual Error _set_hidden_attribute(const String &p_file, bool p_hidden) override { return ERR_UNAVAILABLE; }
	virtual bool _get_read_only_attribute(const String &p_file) override { return false; }
	virtual Error _set_read_only_attribute(const String &p_file, bool p_ro) override { return ERR_UNAVAILABLE; }

	virtual void close() override;

	FileAccessFilesystemJAndroid();
	~FileAccessFilesystemJAndroid();
};

// --- End platform/android/file_access_filesystem_jandroid.h ---

// --- Folded from platform/android/net_socket_android.h ---


#include <jni.h>

/**
 * Specialized NetSocket implementation for Android.
 *
 * Some devices requires Android-specific code to acquire a MulticastLock
 * before sockets are allowed to receive broadcast and multicast packets.
 * This implementation calls into Java code and automatically acquire/release
 * the lock when broadcasting is enabled/disabled on a socket, or that socket
 * joins/leaves a multicast group.
 */
class NetSocketAndroid : public NetSocketUnix {

private:
	static jobject net_utils;
	static jclass cls;
	static jmethodID _multicast_lock_acquire;
	static jmethodID _multicast_lock_release;

	bool wants_broadcast = false;
	int multicast_groups = 0;

	static void multicast_lock_acquire();
	static void multicast_lock_release();

protected:
	static Ref<NetSocket> _create_func();

public:
	static void make_default();
	static void setup(jobject p_net_utils);
	static void terminate();

	virtual void close() override;

	virtual Error set_broadcasting_enabled(bool p_enabled) override;
	virtual Error join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) override;
	virtual Error leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) override;

	NetSocketAndroid() {}
	~NetSocketAndroid();
};

// --- End platform/android/net_socket_android.h ---

// --- Folded from platform/android/android_keys_utils.h ---
#include "core/os/os.h"

#include <android/input.h>

#define AKEYCODE_MAX 0xFFFF

struct AndroidGodotCodePair {
	unsigned int android_code = 0;
	Key godot_code = Key::NONE;
};

static AndroidGodotCodePair android_godot_code_pairs[] = {
	{ AKEYCODE_UNKNOWN, Key::UNKNOWN }, // (0) Unknown key code.
	{ AKEYCODE_BACK, Key::BACK }, // (4) Back key.
	{ AKEYCODE_0, Key::KEY_0 }, // (7) '0' key.
	{ AKEYCODE_1, Key::KEY_1 }, // (8) '1' key.
	{ AKEYCODE_2, Key::KEY_2 }, // (9) '2' key.
	{ AKEYCODE_3, Key::KEY_3 }, // (10) '3' key.
	{ AKEYCODE_4, Key::KEY_4 }, // (11) '4' key.
	{ AKEYCODE_5, Key::KEY_5 }, // (12) '5' key.
	{ AKEYCODE_6, Key::KEY_6 }, // (13) '6' key.
	{ AKEYCODE_7, Key::KEY_7 }, // (14) '7' key.
	{ AKEYCODE_8, Key::KEY_8 }, // (15) '8' key.
	{ AKEYCODE_9, Key::KEY_9 }, // (16) '9' key.
	{ AKEYCODE_STAR, Key::ASTERISK }, // (17) '*' key.
	{ AKEYCODE_POUND, Key::NUMBERSIGN }, // (18) '#' key.
	{ AKEYCODE_DPAD_UP, Key::UP }, // (19) Directional Pad Up key.
	{ AKEYCODE_DPAD_DOWN, Key::DOWN }, // (20) Directional Pad Down key.
	{ AKEYCODE_DPAD_LEFT, Key::LEFT }, // (21) Directional Pad Left key.
	{ AKEYCODE_DPAD_RIGHT, Key::RIGHT }, // (22) Directional Pad Right key.
	{ AKEYCODE_DPAD_CENTER, Key::ENTER }, // (23) Directional Pad Center key.
	{ AKEYCODE_VOLUME_UP, Key::VOLUMEUP }, // (24) Volume Up key.
	{ AKEYCODE_VOLUME_DOWN, Key::VOLUMEDOWN }, // (25) Volume Down key.
	{ AKEYCODE_POWER, Key::STANDBY }, // (26) Power key.
	{ AKEYCODE_CLEAR, Key::CLEAR }, // (28) Clear key.
	{ AKEYCODE_A, Key::A }, // (29) 'A' key.
	{ AKEYCODE_B, Key::B }, // (30) 'B' key.
	{ AKEYCODE_C, Key::C }, // (31) 'C' key.
	{ AKEYCODE_D, Key::D }, // (32) 'D' key.
	{ AKEYCODE_E, Key::E }, // (33) 'E' key.
	{ AKEYCODE_F, Key::F }, // (34) 'F' key.
	{ AKEYCODE_G, Key::G }, // (35) 'G' key.
	{ AKEYCODE_H, Key::H }, // (36) 'H' key.
	{ AKEYCODE_I, Key::I }, // (37) 'I' key.
	{ AKEYCODE_J, Key::J }, // (38) 'J' key.
	{ AKEYCODE_K, Key::K }, // (39) 'K' key.
	{ AKEYCODE_L, Key::L }, // (40) 'L' key.
	{ AKEYCODE_M, Key::M }, // (41) 'M' key.
	{ AKEYCODE_N, Key::N }, // (42) 'N' key.
	{ AKEYCODE_O, Key::O }, // (43) 'O' key.
	{ AKEYCODE_P, Key::P }, // (44) 'P' key.
	{ AKEYCODE_Q, Key::Q }, // (45) 'Q' key.
	{ AKEYCODE_R, Key::R }, // (46) 'R' key.
	{ AKEYCODE_S, Key::S }, // (47) 'S' key.
	{ AKEYCODE_T, Key::T }, // (48) 'T' key.
	{ AKEYCODE_U, Key::U }, // (49) 'U' key.
	{ AKEYCODE_V, Key::V }, // (50) 'V' key.
	{ AKEYCODE_W, Key::W }, // (51) 'W' key.
	{ AKEYCODE_X, Key::X }, // (52) 'X' key.
	{ AKEYCODE_Y, Key::Y }, // (53) 'Y' key.
	{ AKEYCODE_Z, Key::Z }, // (54) 'Z' key.
	{ AKEYCODE_COMMA, Key::COMMA }, // (55) ',’ key.
	{ AKEYCODE_PERIOD, Key::PERIOD }, // (56) '.' key.
	{ AKEYCODE_ALT_LEFT, Key::ALT }, // (57) Left Alt modifier key.
	{ AKEYCODE_ALT_RIGHT, Key::ALT }, // (58) Right Alt modifier key.
	{ AKEYCODE_SHIFT_LEFT, Key::SHIFT }, // (59) Left Shift modifier key.
	{ AKEYCODE_SHIFT_RIGHT, Key::SHIFT }, // (60) Right Shift modifier key.
	{ AKEYCODE_TAB, Key::TAB }, // (61) Tab key.
	{ AKEYCODE_SPACE, Key::SPACE }, // (62) Space key.
	{ AKEYCODE_ENVELOPE, Key::LAUNCHMAIL }, // (65) Envelope special function key.
	{ AKEYCODE_ENTER, Key::ENTER }, // (66) Enter key.
	{ AKEYCODE_DEL, Key::BACKSPACE }, // (67) Backspace key.
	{ AKEYCODE_GRAVE, Key::QUOTELEFT }, // (68) '`' (backtick) key.
	{ AKEYCODE_MINUS, Key::MINUS }, // (69) '-'.
	{ AKEYCODE_EQUALS, Key::EQUAL }, // (70) '=' key.
	{ AKEYCODE_LEFT_BRACKET, Key::BRACKETLEFT }, // (71) '[' key.
	{ AKEYCODE_RIGHT_BRACKET, Key::BRACKETRIGHT }, // (72) ']' key.
	{ AKEYCODE_BACKSLASH, Key::BACKSLASH }, // (73) '\' key.
	{ AKEYCODE_SEMICOLON, Key::SEMICOLON }, // (74) ';' key.
	{ AKEYCODE_APOSTROPHE, Key::APOSTROPHE }, // (75) ''' (apostrophe) key.
	{ AKEYCODE_SLASH, Key::SLASH }, // (76) '/' key.
	{ AKEYCODE_AT, Key::AT }, // (77) '@' key.
	{ AKEYCODE_PLUS, Key::PLUS }, // (81) '+' key.
	{ AKEYCODE_MENU, Key::MENU }, // (82) Menu key.
	{ AKEYCODE_SEARCH, Key::SEARCH }, // (84) Search key.
	{ AKEYCODE_MEDIA_STOP, Key::MEDIASTOP }, // (86) Stop media key.
	{ AKEYCODE_MEDIA_NEXT, Key::MEDIANEXT }, // (87) Play Next media key.
	{ AKEYCODE_MEDIA_PREVIOUS, Key::MEDIAPREVIOUS }, // (88) Play Previous media key.
	{ AKEYCODE_PAGE_UP, Key::PAGEUP }, // (92) Page Up key.
	{ AKEYCODE_PAGE_DOWN, Key::PAGEDOWN }, // (93) Page Down key.
	{ AKEYCODE_ESCAPE, Key::ESCAPE }, // (111) Escape key.
	{ AKEYCODE_FORWARD_DEL, Key::KEY_DELETE }, // (112) Forward Delete key.
	{ AKEYCODE_CTRL_LEFT, Key::CTRL }, // (113) Left Control modifier key.
	{ AKEYCODE_CTRL_RIGHT, Key::CTRL }, // (114) Right Control modifier key.
	{ AKEYCODE_CAPS_LOCK, Key::CAPSLOCK }, // (115) Caps Lock key.
	{ AKEYCODE_SCROLL_LOCK, Key::SCROLLLOCK }, // (116) Scroll Lock key.
	{ AKEYCODE_META_LEFT, Key::META }, // (117) Left Meta modifier key.
	{ AKEYCODE_META_RIGHT, Key::META }, // (118) Right Meta modifier key.
	{ AKEYCODE_SYSRQ, Key::PRINT }, // (120) System Request / Print Screen key.
	{ AKEYCODE_BREAK, Key::PAUSE }, // (121) Break / Pause key.
	{ AKEYCODE_MOVE_HOME, Key::HOME }, // (122) Home Movement key.
	{ AKEYCODE_MOVE_END, Key::END }, // (123) End Movement key.
	{ AKEYCODE_INSERT, Key::INSERT }, // (124) Insert key.
	{ AKEYCODE_FORWARD, Key::FORWARD }, // (125) Forward key.
	{ AKEYCODE_MEDIA_PLAY, Key::MEDIAPLAY }, // (126) Play media key.
	{ AKEYCODE_MEDIA_RECORD, Key::MEDIARECORD }, // (130) Record media key.
	{ AKEYCODE_F1, Key::F1 }, // (131) F1 key.
	{ AKEYCODE_F2, Key::F2 }, // (132) F2 key.
	{ AKEYCODE_F3, Key::F3 }, // (133) F3 key.
	{ AKEYCODE_F4, Key::F4 }, // (134) F4 key.
	{ AKEYCODE_F5, Key::F5 }, // (135) F5 key.
	{ AKEYCODE_F6, Key::F6 }, // (136) F6 key.
	{ AKEYCODE_F7, Key::F7 }, // (137) F7 key.
	{ AKEYCODE_F8, Key::F8 }, // (138) F8 key.
	{ AKEYCODE_F9, Key::F9 }, // (139) F9 key.
	{ AKEYCODE_F10, Key::F10 }, // (140) F10 key.
	{ AKEYCODE_F11, Key::F11 }, // (141) F11 key.
	{ AKEYCODE_F12, Key::F12 }, // (142) F12 key.
	{ AKEYCODE_NUM_LOCK, Key::NUMLOCK }, // (143) Num Lock key.
	{ AKEYCODE_NUMPAD_0, Key::KP_0 }, // (144) Numeric keypad '0' key.
	{ AKEYCODE_NUMPAD_1, Key::KP_1 }, // (145) Numeric keypad '1' key.
	{ AKEYCODE_NUMPAD_2, Key::KP_2 }, // (146) Numeric keypad '2' key.
	{ AKEYCODE_NUMPAD_3, Key::KP_3 }, // (147) Numeric keypad '3' key.
	{ AKEYCODE_NUMPAD_4, Key::KP_4 }, // (148) Numeric keypad '4' key.
	{ AKEYCODE_NUMPAD_5, Key::KP_5 }, // (149) Numeric keypad '5' key.
	{ AKEYCODE_NUMPAD_6, Key::KP_6 }, // (150) Numeric keypad '6' key.
	{ AKEYCODE_NUMPAD_7, Key::KP_7 }, // (151) Numeric keypad '7' key.
	{ AKEYCODE_NUMPAD_8, Key::KP_8 }, // (152) Numeric keypad '8' key.
	{ AKEYCODE_NUMPAD_9, Key::KP_9 }, // (153) Numeric keypad '9' key.
	{ AKEYCODE_NUMPAD_DIVIDE, Key::KP_DIVIDE }, // (154) Numeric keypad '/' key (for division).
	{ AKEYCODE_NUMPAD_MULTIPLY, Key::KP_MULTIPLY }, // (155) Numeric keypad '*' key (for multiplication).
	{ AKEYCODE_NUMPAD_SUBTRACT, Key::KP_SUBTRACT }, // (156) Numeric keypad '-' key (for subtraction).
	{ AKEYCODE_NUMPAD_ADD, Key::KP_ADD }, // (157) Numeric keypad '+' key (for addition).
	{ AKEYCODE_NUMPAD_DOT, Key::KP_PERIOD }, // (158) Numeric keypad '.' key (for decimals or digit grouping).
	{ AKEYCODE_NUMPAD_ENTER, Key::KP_ENTER }, // (160) Numeric keypad Enter key.
	{ AKEYCODE_VOLUME_MUTE, Key::VOLUMEMUTE }, // (164) Volume Mute key.
	{ AKEYCODE_EISU, Key::JIS_EISU }, // (212) JIS EISU key.
	{ AKEYCODE_YEN, Key::YEN }, // (216) Japanese Yen key.
	{ AKEYCODE_KANA, Key::JIS_KANA }, // (218) JIS KANA key.
	{ AKEYCODE_HELP, Key::HELP }, // (259) Help key.
	{ AKEYCODE_REFRESH, Key::REFRESH }, // (285) Refresh key.
	{ AKEYCODE_MAX, Key::UNKNOWN }
};

Key godot_code_from_android_code(unsigned int p_code);

// Key location determination.
struct AndroidGodotLocationPair {
	unsigned int android_code = 0;
	KeyLocation godot_code = KeyLocation::UNSPECIFIED;
};

static AndroidGodotLocationPair android_godot_location_pairs[] = {
	{ AKEYCODE_ALT_LEFT, KeyLocation::LEFT },
	{ AKEYCODE_ALT_RIGHT, KeyLocation::RIGHT },
	{ AKEYCODE_SHIFT_LEFT, KeyLocation::LEFT },
	{ AKEYCODE_SHIFT_RIGHT, KeyLocation::RIGHT },
	{ AKEYCODE_CTRL_LEFT, KeyLocation::LEFT },
	{ AKEYCODE_CTRL_RIGHT, KeyLocation::RIGHT },
	{ AKEYCODE_META_LEFT, KeyLocation::LEFT },
	{ AKEYCODE_META_RIGHT, KeyLocation::RIGHT },
	{ AKEYCODE_MAX, KeyLocation::UNSPECIFIED }
};

KeyLocation godot_location_from_android_code(unsigned int p_code);

// --- End platform/android/android_keys_utils.h ---

// --- Folded from platform/android/android_input_handler.h ---
#include "core/os/os.h"

// This class encapsulates all the handling of input events that come from the Android UI thread.
// Remarks:
// - It's not thread-safe by itself, so its functions must only be called on a single thread, which is the Android UI thread.
// - Its functions must only call thread-safe methods.
class AndroidInputHandler {
public:
	struct TouchPos {
		int id = 0;
		Point2 pos;
		float pressure = 0;
		Vector2 tilt;
		bool double_tap = false;
	};

	struct MouseEventInfo {
		bool valid = false;
		Point2 pos;
	};

	enum {
		JOY_EVENT_BUTTON = 0,
		JOY_EVENT_AXIS = 1,
		JOY_EVENT_HAT = 2
	};

	struct JoypadEvent {
		int device = 0;
		int type = 0;
		int index = 0; // Can be either JoyAxis or JoyButton.
		bool pressed = false;
		float value = 0;
		uint32_t hat = HatMask::CENTER;
	};

private:
	bool alt_mem = false;
	bool shift_mem = false;
	bool control_mem = false;
	bool meta_mem = false;

	uint32_t buttons_state = MouseButtonMask::NONE;

	Vector<TouchPos> touch;
	MouseEventInfo mouse_event_info;
	Point2 hover_prev_pos; // needed to calculate the relative position on hover events

	void _set_key_modifier_state(Ref<InputEventWithModifiers> ev, Key p_keycode);

	static MouseButton _button_index_from_mask(uint32_t button_mask);
	static uint32_t _android_button_mask_to_godot_button_mask(int android_button_mask);

	void _wheel_button_click(uint32_t &ev, MouseButton wheel_button, float factor);

	void _parse_mouse_event_info(uint32_t event_buttons_mask, bool p_pressed, bool p_canceled, bool p_double_click, bool p_source_mouse_relative);

	void _release_mouse_event_info(bool p_source_mouse_relative = false);

	void _cancel_mouse_event_info(bool p_source_mouse_relative = false);

	void _parse_all_touch(bool p_pressed, bool p_canceled = false);

	void _release_all_touch();

	void _cancel_all_touch();

public:
	void process_mouse_event(int p_event_action, int p_event_android_buttons_mask, Point2 p_event_pos, Vector2 p_delta, bool p_double_click, bool p_source_mouse_relative, float p_pressure, Vector2 p_tilt);
	void process_touch_event(int p_event, int p_pointer, const Vector<TouchPos> &p_points);
	void process_magnify(Point2 p_pos, float p_factor);
	void process_pan(Point2 p_pos, Vector2 p_delta);
	void process_joy_event(JoypadEvent p_event);
	void process_key_event(int p_physical_keycode, int p_unicode, int p_key_label, bool p_pressed, bool p_echo);
};

// --- End platform/android/android_input_handler.h ---

// --- Folded from platform/android/tts_android.h ---
#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/io/ip.h"
#include "core/os/memory.h"

#include <jni.h>

class TTS_Android {
	static inline int INIT_STATE_UNKNOWN = 0;
	static inline int INIT_STATE_SUCCESS = 1;
	static inline int INIT_STATE_FAIL = -1;

	static bool initialized;
	static jobject tts;
	static jclass cls;

	static jmethodID _init;
	static jmethodID _is_speaking;
	static jmethodID _is_paused;
	static jmethodID _get_state;
	static jmethodID _get_voices;
	static jmethodID _speak;
	static jmethodID _pause_speaking;
	static jmethodID _resume_speaking;
	static jmethodID _stop_speaking;

	static Thread init_thread;
	static SafeFlag quit_request;
	static SafeFlag init_done;

	static void _thread_function(void *self);

	static HashMap<int64_t, Char16String> ids;

	static void initialize_tts(bool p_wait = true);

public:
	static void setup(jobject p_tts);
	static void terminate();
	static void _java_utterance_callback(int p_event, int64_t p_id, int p_pos);

	static bool is_speaking();
	static bool is_paused();
	static Array get_voices();
	static void speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt);
	static void pause();
	static void resume();
	static void stop();
};

// --- End platform/android/tts_android.h ---

// --- Folded from platform/android/rendering_context_driver_vulkan_android.h ---
#ifdef VULKAN_ENABLED

#include "drivers/vulkan/rendering_context_driver_vulkan.h"

struct ANativeWindow;

class RenderingContextDriverVulkanAndroid : public RenderingContextDriverVulkan {
private:
	virtual const char *_get_platform_surface_extension() const override final;

protected:
	SurfaceID surface_create(const void *p_platform_data) override final;
	bool _use_validation_layers() const override final;

public:
	struct WindowPlatformData {
		ANativeWindow *window;
	};

	RenderingContextDriverVulkanAndroid() = default;
	~RenderingContextDriverVulkanAndroid() override = default;
};

#endif // VULKAN_ENABLED

// --- End platform/android/rendering_context_driver_vulkan_android.h ---

// --- Folded from platform/android/plugin/godot_plugin_jni.h ---
#include <android/log.h>
#include <jni.h>

void unregister_plugins_singletons();

extern "C" {
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterSingleton(JNIEnv *env, jclass clazz, jstring name, jobject obj);
JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterMethod(JNIEnv *env, jclass clazz, jstring sname, jstring name, jstring ret, jobjectArray args);
JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterSignal(JNIEnv *env, jclass clazz, jstring j_plugin_name, jstring j_signal_name, jobjectArray j_signal_param_types);
JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeEmitSignal(JNIEnv *env, jclass clazz, jstring j_plugin_name, jstring j_signal_name, jobjectArray j_signal_params);
}

// --- End platform/android/plugin/godot_plugin_jni.h ---
#endif // ANDROID_ENABLED

// --- Folded from platform/android/api/jni_singleton.h ---
#include "core/io/ip.h"
#include "servers/display/display_server.h"

class JNISingleton
{
	Ref<JavaObject> wrapped_object;

public:
	JNISingleton() {}
	~JNISingleton() { wrapped_object.unref(); }
};



// --- End platform/android/api/jni_singleton.h ---

#ifdef ANDROID_ENABLED
// --- Folded from platform/android/display_server_android.h ---
#include "servers/display/display_server.h"

class InputEvent;

#if defined(RD_ENABLED)
class RenderingContextDriver;
class RenderingDevice;
#endif

class DisplayServerAndroid : public DisplayServerDriver {

	String rendering_driver;

	// https://developer.android.com/reference/android/view/PointerIcon
	// mapping between Godot's cursor shape to Android's'
	int android_cursors[DisplayServerEnums::CURSOR_MAX] = {
		1000, //DisplayServerEnums::CURSOR_ARROW
		1008, //DisplayServerEnums::CURSOR_IBEAM
		1002, //DisplayServerEnums::CURSOR_POINTIN
		1007, //DisplayServerEnums::CURSOR_CROSS
		1004, //DisplayServerEnums::CURSOR_WAIT
		1004, //DisplayServerEnums::CURSOR_BUSY
		1021, //DisplayServerEnums::CURSOR_DRAG
		1021, //DisplayServerEnums::CURSOR_CAN_DRO
		1000, //DisplayServerEnums::CURSOR_FORBIDD (no corresponding icon in Android's icon so fallback to default)
		1015, //DisplayServerEnums::CURSOR_VSIZE
		1014, //DisplayServerEnums::CURSOR_HSIZE
		1017, //DisplayServerEnums::CURSOR_BDIAGSI
		1016, //DisplayServerEnums::CURSOR_FDIAGSI
		1020, //DisplayServerEnums::CURSOR_MOVE
		1015, //DisplayServerEnums::CURSOR_VSPLIT
		1014, //DisplayServerEnums::CURSOR_HSPLIT
		1003, //DisplayServerEnums::CURSOR_HELP
	};
	const int CURSOR_TYPE_NULL = 0;
	DisplayServerEnums::MouseMode mouse_mode = DisplayServerEnums::MouseMode::MOUSE_MODE_VISIBLE;
	DisplayServerEnums::MouseMode mouse_mode_base = DisplayServerEnums::MouseMode::MOUSE_MODE_VISIBLE;
	DisplayServerEnums::MouseMode mouse_mode_override = DisplayServerEnums::MouseMode::MOUSE_MODE_VISIBLE;
	bool mouse_mode_override_enabled = false;
	void _mouse_update_mode();

	bool keep_screen_on;
	bool swap_buffers_flag;

	DisplayServerEnums::CursorShape cursor_shape = DisplayServerEnums::CursorShape::CURSOR_ARROW;

#if defined(RD_ENABLED)
	RenderingDevice *rendering_device = nullptr;
#endif

	ObjectID window_attached_instance_id;

	Callable window_event_callback;
	Callable input_event_callback;
	Callable input_text_callback;
	Callable rect_changed_callback;

	Callable system_theme_changed;
	Callable hardware_keyboard_connection_changed;

	Callable dialog_callback;
	Callable input_dialog_callback;

	Callable file_picker_callback;

	template <typename... Args>
	void _window_callback(const Callable &p_callable, bool p_deferred, const Args &...p_rest_args) const;

	static void _dispatch_input_events(const Ref<InputEvent> &p_event);

public:
	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;
	virtual String get_name() const override;

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
	void emit_system_theme_changed();

	virtual void clipboard_set(const String &p_text) override;
	virtual String clipboard_get() const override;
	virtual bool clipboard_has() const override;

	virtual Error dialog_show(String p_title, String p_description, Vector<String> p_buttons, const Callable &p_callback) override;
	void emit_dialog_callback(int p_button_index);

	virtual Error dialog_input_text(String p_title, String p_description, String p_partial, const Callable &p_callback) override;
	void emit_input_dialog_callback(String p_text);

	virtual Error file_dialog_show(const String &p_title, const String &p_current_directory, const String &p_filename, bool p_show_hidden, const DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) override;
	void emit_file_picker_callback(bool p_ok, const Vector<String> &p_selected_paths);

	virtual Color get_accent_color() const override;
	virtual Color get_base_color() const override;

	virtual TypedArray<Rect2> get_display_cutouts(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i get_display_safe_area(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

	virtual void screen_set_keep_on(bool p_enable) override;
	virtual bool screen_is_kept_on() const override;

	virtual void screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) override;
	virtual DisplayServerEnums::ScreenOrientation screen_get_orientation(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	int get_display_rotation() const;

	virtual int get_screen_count() const override;
	virtual int get_primary_screen() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual bool is_touchscreen_available() const override;

	virtual void virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect = Rect2(), DisplayServerEnums::VirtualKeyboardType p_type = DisplayServerEnums::KEYBOARD_TYPE_DEFAULT, int p_max_length = -1, int p_cursor_start = -1, int p_cursor_end = -1) override;
	virtual void virtual_keyboard_hide() override;
	virtual int virtual_keyboard_get_height() const override;
	virtual bool has_hardware_keyboard() const override;
	virtual void set_hardware_keyboard_connection_change_callback(const Callable &p_callable) override;
	void emit_hardware_keyboard_connection_changed(bool p_connected);

	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	void send_window_event(DisplayServerEnums::WindowEvent p_event, bool p_deferred = false) const;
	void send_input_event(const Ref<InputEvent> &p_event) const;
	void send_input_text(const String &p_text) const;

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

	virtual bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool can_any_window_draw() const override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

	virtual void window_set_color(const Color &p_color) override;

	virtual void process_events() override;

	void process_accelerometer(const Vector3 &p_accelerometer);
	void process_gravity(const Vector3 &p_gravity);
	void process_magnetometer(const Vector3 &p_magnetometer);
	void process_gyroscope(const Vector3 &p_gyroscope);

	void _cursor_set_shape_helper(DisplayServerEnums::CursorShape p_shape, bool force = false);
	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override;
	virtual DisplayServerEnums::CursorShape cursor_get_shape() const override;
	virtual void cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW, const Vector2 &p_hotspot = Vector2()) override;

	virtual void mouse_set_mode(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode() const override;
	virtual void mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode_override() const override;
	virtual void mouse_set_mode_override_enabled(bool p_override_enabled) override;
	virtual bool mouse_is_mode_override_enabled() const override;

	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();
	static void register_android_driver();

#ifdef VULKAN_ENABLED
	static bool check_vulkan_global_context(bool p_vulkan_requirements_met);
	static void free_vulkan_global_context();
#endif

	void reset_window();
	void notify_surface_changed(int p_width, int p_height);
	void notify_application_paused();

	virtual Point2i mouse_get_position() const override;
	virtual uint32_t mouse_get_button_state() const override;

	void reset_swap_buffers_flag();
	bool should_swap_buffers() const;
	virtual void swap_buffers() override;

	virtual void set_native_icon(const String &p_filename) override;
	virtual void set_icon(const Ref<Image> &p_icon) override;

	virtual bool is_window_transparency_available() const override;

	virtual bool is_in_pip_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void pip_mode_enter(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void pip_mode_set_aspect_ratio(int p_numerator, int p_denominator, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void pip_mode_set_auto_enter_on_background(bool p_auto_enter_on_background, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	DisplayServerAndroid(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerAndroid();
};
#endif // ANDROID_ENABLED

#ifdef ANDROID_ENABLED
// --- Folded from platform/android/java_godot_io_wrapper.h ---
#include "core/io/ip.h"

#include <android/log.h>
#include <jni.h>

// Class that makes functions in java/src/org/godotengine/godot/GodotIO.java callable from C++
class GodotIOJavaWrapper {
private:
	jobject godot_io_instance;
	jclass cls;

	jmethodID _open_URI = nullptr;
	jmethodID _get_cache_dir = nullptr;
	jmethodID _get_data_dir = nullptr;
	jmethodID _get_temp_dir = nullptr;
	jmethodID _get_display_cutouts = nullptr;
	jmethodID _get_display_rotation = nullptr;
	jmethodID _get_display_safe_area = nullptr;
	jmethodID _get_locale = nullptr;
	jmethodID _get_model = nullptr;
	jmethodID _get_screen_DPI = nullptr;
	jmethodID _get_scaled_density = nullptr;
	jmethodID _get_screen_refresh_rate = nullptr;
	jmethodID _get_unique_id = nullptr;
	jmethodID _show_keyboard = nullptr;
	jmethodID _hide_keyboard = nullptr;
	jmethodID _has_hardware_keyboard = nullptr;
	jmethodID _set_screen_orientation = nullptr;
	jmethodID _get_screen_orientation = nullptr;
	jmethodID _get_system_dir = nullptr;

public:
	GodotIOJavaWrapper(JNIEnv *p_env, jobject p_godot_io_instance);
	~GodotIOJavaWrapper();

	jobject get_instance();

	Error open_uri(const String &p_uri);
	String get_cache_dir();
	String get_temp_dir();
	String get_user_data_dir(const String &p_user_dir);
	String get_locale();
	String get_model();
	int get_screen_dpi();
	float get_scaled_density();
	float get_screen_refresh_rate(float fallback);
	TypedArray<Rect2> get_display_cutouts();
	Rect2i get_display_safe_area();
	String get_unique_id();
	bool has_vk();
	bool has_hardware_keyboard();
	void show_vk(const String &p_existing, int p_type, int p_max_input_length, int p_cursor_start, int p_cursor_end);
	void hide_vk();
	int get_vk_height();
	void set_vk_height(int p_height);
	void set_screen_orientation(int p_orient);
	int get_screen_orientation();
	String get_system_dir(int p_dir, bool p_shared_storage);
	int get_display_rotation();
};
#endif // ANDROID_ENABLED

#ifdef ANDROID_ENABLED
// --- Folded from platform/android/java_godot_wrapper.h ---
// --- Folded from platform/android/java_godot_view_wrapper.h ---
#include "core/math/vector2.h"

#include <android/log.h>
#include <jni.h>

// Class that makes functions in java/src/org/godotengine/godot/GodotRenderView.java callable from C++
class GodotJavaViewWrapper {
private:
	jclass _cls;

	jobject _godot_view;

	jmethodID _can_capture_pointer = nullptr;
	jmethodID _request_pointer_capture = nullptr;
	jmethodID _release_pointer_capture = nullptr;

	jmethodID _configure_pointer_icon = nullptr;
	jmethodID _set_pointer_icon = nullptr;

public:
	GodotJavaViewWrapper(jobject godot_view);

	bool can_update_pointer_icon() const;
	bool can_capture_pointer() const;

	void request_pointer_capture();
	void release_pointer_capture();

	void configure_pointer_icon(int pointer_type, const String &image_path, const Vector2 &p_hotspot);
	void set_pointer_icon(int pointer_type);

	~GodotJavaViewWrapper();
};

// --- End platform/android/java_godot_view_wrapper.h ---

#include "core/math/vector2.h"
#include "core/templates/list.h"
#include "core/string/ustring.h"

#include <jni.h>

// Class that makes functions in java/src/org/godotengine/godot/Godot.kt callable from C++
class GodotJavaWrapper {
private:
	jobject godot_native_bridge;
	jclass godot_native_bridge_class;

	GodotJavaViewWrapper *godot_view = nullptr;

	jmethodID _restart = nullptr;
	jmethodID _finish = nullptr;
	jmethodID _set_keep_screen_on = nullptr;
	jmethodID _alert = nullptr;
	jmethodID _is_dark_mode_supported = nullptr;
	jmethodID _is_dark_mode = nullptr;
	jmethodID _get_accent_color = nullptr;
	jmethodID _get_base_color = nullptr;
	jmethodID _get_clipboard = nullptr;
	jmethodID _set_clipboard = nullptr;
	jmethodID _has_clipboard = nullptr;
	jmethodID _show_dialog = nullptr;
	jmethodID _show_input_dialog = nullptr;
	jmethodID _show_file_picker = nullptr;
	jmethodID _request_permission = nullptr;
	jmethodID _request_permissions = nullptr;
	jmethodID _get_granted_permissions = nullptr;
	jmethodID _get_gdextension_list_config_file = nullptr;
	jmethodID _get_ca_certificates = nullptr;
	jmethodID _init_input_devices = nullptr;
	jmethodID _vibrate = nullptr;
	jmethodID _get_input_fallback_mapping = nullptr;
	jmethodID _on_godot_setup_completed = nullptr;
	jmethodID _on_godot_main_loop_started = nullptr;
	jmethodID _on_godot_terminating = nullptr;
	jmethodID _create_new_godot_instance = nullptr;
	jmethodID _get_render_view = nullptr;
	jmethodID _begin_benchmark_measure = nullptr;
	jmethodID _end_benchmark_measure = nullptr;
	jmethodID _dump_benchmark = nullptr;
	jmethodID _check_internal_feature_support = nullptr;
	jmethodID _sign_apk = nullptr;
	jmethodID _verify_apk = nullptr;
	jmethodID _enable_immersive_mode = nullptr;
	jmethodID _is_in_immersive_mode = nullptr;
	jmethodID _set_window_color = nullptr;
	jmethodID _on_editor_workspace_selected = nullptr;
	jmethodID _on_distraction_free_mode_changed = nullptr;
	jmethodID _get_activity = nullptr;
	jmethodID _build_env_connect = nullptr;
	jmethodID _build_env_disconnect = nullptr;
	jmethodID _build_env_execute = nullptr;
	jmethodID _build_env_cancel = nullptr;
	jmethodID _build_env_clean_project = nullptr;
	jmethodID _is_pip_mode_supported = nullptr;
	jmethodID _is_in_pip_mode = nullptr;
	jmethodID _enter_pip_mode = nullptr;
	jmethodID _set_pip_mode_aspect_ratio = nullptr;
	jmethodID _set_auto_enter_pip_mode_on_background = nullptr;

public:
	GodotJavaWrapper(JNIEnv *p_env, jobject p_godot_native_bridge);
	~GodotJavaWrapper();

	jobject get_activity();

	GodotJavaViewWrapper *get_godot_view();

	void on_godot_setup_completed(JNIEnv *p_env = nullptr);
	void on_godot_main_loop_started(JNIEnv *p_env = nullptr);
	void on_godot_terminating(JNIEnv *p_env = nullptr);
	void restart(JNIEnv *p_env = nullptr);
	bool force_quit(JNIEnv *p_env = nullptr, int p_instance_id = 0);
	void set_keep_screen_on(bool p_enabled);
	void alert(const String &p_message, const String &p_title);
	bool is_dark_mode_supported();
	bool is_dark_mode();
	Color get_accent_color();
	Color get_base_color();
	bool has_get_clipboard();
	String get_clipboard();
	bool has_set_clipboard();
	void set_clipboard(const String &p_text);
	bool has_has_clipboard();
	bool has_clipboard();
	Error show_dialog(const String &p_title, const String &p_description, const Vector<String> &p_buttons);
	Error show_input_dialog(const String &p_title, const String &p_message, const String &p_existing_text);
	Error show_file_picker(const String &p_current_directory, const String &p_filename, int p_mode, const Vector<String> &p_filters);
	bool request_permission(const String &p_name);
	bool request_permissions();
	Vector<String> get_granted_permissions() const;
	String get_ca_certificates() const;
	void init_input_devices();
	void vibrate(int p_duration_ms, float p_amplitude = -1.0);
	String get_input_fallback_mapping();
	int create_new_godot_instance(const List<String> &args);
	void begin_benchmark_measure(const String &p_context, const String &p_label);
	void end_benchmark_measure(const String &p_context, const String &p_label);
	void dump_benchmark(const String &benchmark_file);

	// Return the list of gdextensions config file.
	Vector<String> get_gdextension_list_config_file() const;

	// Return true if the given feature is supported.
	bool check_internal_feature_support(const String &p_feature) const;

	// Sign and verify apks
	Error sign_apk(const String &p_input_path, const String &p_output_path, const String &p_keystore_path, const String &p_keystore_user, const String &p_keystore_password);
	Error verify_apk(const String &p_apk_path);

	void enable_immersive_mode(bool p_enabled);
	bool is_in_immersive_mode();

	void set_window_color(const Color &p_color);

	void on_editor_workspace_selected(const String &p_workspace);
	void on_distraction_free_mode_changed(bool p_enabled);

	bool build_env_connect(const Callable &p_callback);
	void build_env_disconnect();
	int build_env_execute(const String &p_build_tool, const List<String> &p_arguments, const String &p_project_path, const String &p_gradle_build_directory, const Callable &p_output_callback, const Callable &p_result_callback);
	void build_env_cancel(int p_job_id);
	void build_env_clean_project(const String &p_project_path, const String &p_gradle_build_directory, const Callable &p_callback);

	bool is_pip_mode_supported();
	bool is_in_pip_mode();
	void enter_pip_mode();
	void set_pip_mode_aspect_ratio(int p_numerator, int p_denominator);
	void set_auto_enter_pip_mode_on_background(bool p_auto_enter_on_background);
};
#endif // ANDROID_ENABLED

#ifdef ANDROID_ENABLED
// --- Folded from platform/android/thread_jandroid.h ---
#include <jni.h>

void init_thread_jandroid(JavaVM *p_jvm, JNIEnv *p_env);

void setup_android_thread();
JNIEnv *get_jni_env();
#endif // ANDROID_ENABLED

#ifdef ANDROID_ENABLED
// --- Folded from platform/android/jni_utils.h ---
#include "core/string/ustring.h"

#include <jni.h>

jobject _variant_to_jobject(JNIEnv *env, Variant::Type p_type, const Variant *p_arg, int p_depth = 0);

String _get_class_name(JNIEnv *env, jclass cls, bool *array);

Variant _jobject_to_variant(JNIEnv *env, jobject obj, int p_depth = 0);

Variant::Type get_jni_type(const String &p_type);

/**
 * Convert a Godot Callable to a org.godotengine.godot.variant.Callable java object.
 * @param p_env JNI environment instance
 * @param p_callable Callable parameter to convert. If null or invalid type, a null jobject is returned.
 * @return org.godotengine.godot.variant.Callable jobject or null
 */
jobject callable_to_jcallable(JNIEnv *p_env, const Variant &p_callable);

/**
 * Convert a org.godotengine.godot.variant.Callable java object to a Godot Callable variant.
 * @param p_env JNI environment instance
 * @param p_jcallable_obj org.godotengine.godot.variant.Callable java object to convert.
 * @return Callable variant
 */
Callable jcallable_to_callable(JNIEnv *p_env, jobject p_jcallable_obj);

/**
 * Converts a java.lang.CharSequence object to a Godot String.
 * @param p_env  JNI environment instance
 * @param p_charsequence java.lang.CharSequence object to convert
 * @return Godot String instance.
 */
String charsequence_to_string(JNIEnv *p_env, jobject p_charsequence);

/**
 * Converts JNI jstring to Godot String.
 * @param source Source JNI string. If null an empty string is returned.
 * @param env JNI environment instance. If null obtained by get_jni_env().
 * @return Godot string instance.
 */
static inline String jstring_to_string(jstring source, JNIEnv *env = nullptr) {
	String result;
	if (source) {
		if (!env) {
			env = get_jni_env();
		}
		const char *const source_utf8 = env->GetStringUTFChars(source, nullptr);
		if (source_utf8) {
			result.append_utf8(source_utf8);
			env->ReleaseStringUTFChars(source, source_utf8);
		}
	}
	return result;
}

/**
 * Set up thread-safe Android ClassLoader (used by jni_find_class() below).
 */
void setup_android_class_loader();
void cleanup_android_class_loader();

/**
 * Thread-safe JNI class finder using Android ClassLoader.
 * Works on any thread, unlike standard FindClass which may fail on non-main threads.
 * @param p_env JNI environment instance.
 * @param p_class_name Class name to find.
 * @return jclass reference or null if not found.
 */
jclass jni_find_class(JNIEnv *p_env, const char *p_class_name);
#endif // ANDROID_ENABLED
#endif // defined(ANDROID_ENABLED)
// --- End platform/android/os_android.h ---

// --- Folded from platform/windows/os_windows.h ---
#if defined(_WIN32)
#ifdef _WIN32
#include "core/os/os.h"
// --- Folded from platform/windows/crash_handler_windows.h ---
#include <windows.h>

// Crash handler exception only enabled with MSVC
#if defined(DEBUG_ENABLED)
#define CRASH_HANDLER_EXCEPTION 1

#ifdef _MSC_VER
extern DWORD CrashHandlerException(EXCEPTION_POINTERS* ep);
#endif

#endif

class CrashHandler
{
	bool disabled;

public:
	void initialize();

	void disable();

	bool is_disabled() const { return disabled; }

	CrashHandler();
	~CrashHandler();
};




// --- End platform/windows/crash_handler_windows.h ---
// --- Folded from drivers/wasapi/audio_driver_wasapi.h ---
#ifdef WASAPI_ENABLED

#include "core/os/mutex.h"
#include "core/os/memory.h"
#include "servers/display/display_server.h"

#include <windows.h>

#include <audioclient.h>
#include <mmdeviceapi.h>

class AudioDriverWASAPI : public AudioDriver {
	class AudioDeviceWASAPI {
	public:
		IAudioClient *audio_client = nullptr;
		IAudioRenderClient *render_client = nullptr; // Output
		IAudioCaptureClient *capture_client = nullptr; // Input
		SafeFlag active;

		WORD format_tag = 0;
		WORD bits_per_sample = 0;
		unsigned int channels = 0;
		unsigned int frame_size = 0;

		String device_name = "Default"; // Output OR Input
		String new_device = "Default"; // Output OR Input

		AudioDeviceWASAPI() {}
	};

	AudioDeviceWASAPI audio_input;
	AudioDeviceWASAPI audio_output;

	Mutex mutex;
	Thread thread;

	Vector<int32_t> samples_in;

	unsigned int channels = 0;
	int mix_rate = 0;
	int buffer_frames = 0;
	int target_latency_ms = 0;
	float real_latency = 0.0;
	bool using_audio_client_3 = false;

	SafeFlag exit_thread;

	static _FORCE_INLINE_ void write_sample(WORD format_tag, int bits_per_sample, BYTE *buffer, int i, int32_t sample);
	static _FORCE_INLINE_ int32_t read_sample(WORD format_tag, int bits_per_sample, BYTE *buffer, int i);
	static void thread_func(void *p_udata);

	Error init_output_device(bool p_reinit = false);
	Error init_input_device(bool p_reinit = false);

	Error finish_output_device();
	Error finish_input_device();

	Error audio_device_init(AudioDeviceWASAPI *p_device, bool p_input, bool p_reinit, bool p_no_audio_client_3 = false);
	Error audio_device_finish(AudioDeviceWASAPI *p_device);
	PackedStringArray audio_device_get_list(bool p_input);

public:
	virtual const char *get_name() const override {
		return "WASAPI";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;
	virtual float get_latency() override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

	virtual PackedStringArray get_output_device_list() override;
	virtual String get_output_device() override;
	virtual void set_output_device(const String &p_name) override;

	virtual Error input_start() override;
	virtual Error input_stop() override;

	virtual PackedStringArray get_input_device_list() override;
	virtual String get_input_device() override;
	virtual void set_input_device(const String &p_name) override;

	AudioDriverWASAPI();
};

#endif // WASAPI_ENABLED

// --- End drivers/wasapi/audio_driver_wasapi.h ---
// --- Folded from drivers/winmidi/midi_driver_winmidi.h ---
#ifdef WINMIDI_ENABLED

#include "core/os/os.h"
#include "core/string/ustring.h"

#include <windows.h>

#include <mmsystem.h>

class MIDIDriverWinMidi final
{
public:
	struct Data
	{
		Vector<HMIDIIN> connected_sources;
	};

	static inline Data* data = nullptr;

private:
	static void CALLBACK read(
		HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

public:
	static void initialize();
	static void finalize();
	static Error open();
	static void close();

	MIDIDriverWinMidi() = delete;
	MIDIDriverWinMidi(const MIDIDriverWinMidi&) = delete;
	MIDIDriverWinMidi& operator=(const MIDIDriverWinMidi&) = delete;
	~MIDIDriverWinMidi() = delete;
};

#endif // WINMIDI_ENABLED

// --- End drivers/winmidi/midi_driver_winmidi.h ---

#ifdef XAUDIO2_ENABLED
// --- Folded from drivers/xaudio2/audio_driver_xaudio2.h ---
#include "core/os/mutex.h"
#include "core/os/memory.h"
#include "servers/display/display_server.h"

#include <windows.h>

#include <wrl/client.h>
#include <xaudio2.h>

class AudioDriverXAudio2 : public AudioDriver {
	enum {
		AUDIO_BUFFERS = 2
	};

	struct XAudio2DriverVoiceCallback : public IXAudio2VoiceCallback {
		HANDLE buffer_end_event;

		XAudio2DriverVoiceCallback() :
				buffer_end_event(CreateEvent(nullptr, FALSE, FALSE, nullptr)) {}

		virtual ~XAudio2DriverVoiceCallback() {
			CloseHandle(buffer_end_event);
		}

		void STDMETHODCALLTYPE OnBufferEnd(void *pBufferContext) {
			SetEvent(buffer_end_event);
		}

		//Unused methods are stubs
		void STDMETHODCALLTYPE OnStreamEnd() {}
		void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() {}
		void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
		void STDMETHODCALLTYPE OnBufferStart(void *pBufferContext) {}
		void STDMETHODCALLTYPE OnLoopEnd(void *pBufferContext) {}
		void STDMETHODCALLTYPE OnVoiceError(void *pBufferContext, HRESULT Error) {}
	};

	Thread thread;
	Mutex mutex;

	int32_t *samples_in = nullptr;
	int16_t *samples_out[AUDIO_BUFFERS];

	static void thread_func(void *p_udata);
	int buffer_size = 0;

	unsigned int mix_rate = 0;
	SpeakerMode speaker_mode = SpeakerMode::SPEAKER_MODE_STEREO;

	int channels = 0;

	SafeFlag active;
	SafeFlag exit_thread;
	bool pcm_open = false;

	WAVEFORMATEX wave_format = {};
	Microsoft::WRL::ComPtr<IXAudio2> xaudio;
	int current_buffer = 0;
	IXAudio2MasteringVoice *mastering_voice = nullptr;
	XAUDIO2_BUFFER xaudio_buffer[AUDIO_BUFFERS];
	IXAudio2SourceVoice *source_voice = nullptr;
	XAudio2DriverVoiceCallback voice_callback;

public:
	virtual const char *get_name() const override {
		return "XAudio2";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;
	virtual float get_latency() override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

	AudioDriverXAudio2();
	~AudioDriverXAudio2() {}
};

// --- End drivers/xaudio2/audio_driver_xaudio2.h ---
#endif

#include <dwrite.h>
#include <dwrite_2.h>
#include <io.h>
#include <shellapi.h>
#include <windows.h>
#include <windowsx.h>

#ifdef DEBUG_ENABLED
// Forward error messages to OutputDebugString
#define WINDOWS_DEBUG_OUTPUT_ENABLED
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x)                                                                            \
	if (x != nullptr) {                                                                            \
		x->Release();                                                                              \
		x = nullptr;                                                                               \
	}
#endif

template <typename T> class ComAutoreleaseRef
{
public:
	T* reference = nullptr;

	_FORCE_INLINE_ T* operator->() { return reference; }

	_FORCE_INLINE_ const T* operator->() const { return reference; }

	_FORCE_INLINE_ T* operator*() { return reference; }

	_FORCE_INLINE_ const T* operator*() const { return reference; }

	_FORCE_INLINE_ bool is_valid() const { return reference != nullptr; }

	_FORCE_INLINE_ bool is_null() const { return reference == nullptr; }

	ComAutoreleaseRef() {}

	ComAutoreleaseRef(T* p_ref) { reference = p_ref; }

	~ComAutoreleaseRef()
	{
		if (reference != nullptr) {
			reference->Release();
			reference = nullptr;
		}
	}
};

class OS_Windows final
{
public:
	OS_Windows() = delete;
	~OS_Windows() = delete;
	OS_Windows(const OS_Windows&) = delete;
	OS_Windows& operator=(const OS_Windows&) = delete;

	struct ProcessInfo
	{
		STARTUPINFOEX si;
		PROCESS_INFORMATION pi;
		mutable bool is_running = true;
		mutable int exit_code = -1;
	};

private:
	static inline uint64_t target_ticks = 0;
	static inline uint64_t ticks_start = 0;
	static inline uint64_t ticks_per_second = 0;
	static inline uint64_t delay_resolution = 1000;

	static inline HINSTANCE hInstance = nullptr;

#ifdef WASAPI_ENABLED
	static inline AudioDriverWASAPI driver_wasapi;
#endif
#ifdef XAUDIO2_ENABLED
	static inline AudioDriverXAudio2 driver_xaudio2;
#endif

	static inline CrashHandler crash_handler;

#ifdef WINDOWS_DEBUG_OUTPUT_ENABLED
	static inline ErrorHandlerList error_handlers;
#endif

	static inline HWND main_window = nullptr;

	static inline IDWriteFactory* dwrite_factory = nullptr;
	static inline IDWriteFactory2* dwrite_factory2 = nullptr;
	static inline IDWriteFontCollection* font_collection = nullptr;
	static inline IDWriteFontFallback* system_font_fallback = nullptr;

	static inline bool dwrite_init = false;
	static inline bool dwrite2_init = false;

	static inline HashMap<void*, String> temp_libraries;

	static void _remove_temp_library(void* p_library_handle);
	static String _get_default_fontname(const String& p_font_name);
	static DWRITE_FONT_WEIGHT _weight_to_dw(int p_weight);
	static DWRITE_FONT_STRETCH _stretch_to_dw(int p_stretch);

	static bool is_using_con_wrapper();

	static inline HashMap<String, int> encodings;
	static void _init_encodings();

	static Vector<String> _get_video_adapter_driver_info_reg(const String& p_name);
	static Vector<String> _get_video_adapter_driver_info_wmi(const String& p_name);

	static String _quote_command_line_argument(const String& p_text);

	static inline HashMap<ProcessID, ProcessInfo>* process_map = nullptr;
	static inline Mutex process_map_mutex;

public:
	static void initialize();
	static void delete_main_loop();
	static void finalize();
	static void finalize_core();

	static String get_stdin_string(int64_t p_buffer_size = 1024);
	static PackedByteArray get_stdin_buffer(int64_t p_buffer_size = 1024);
	static OS::StdHandleType get_stdin_type();
	static OS::StdHandleType get_stdout_type();
	static OS::StdHandleType get_stderr_type();

	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	static Error get_entropy(uint8_t* r_buffer, int p_bytes);

	static Error open_dynamic_library(
		const String& p_path, void*& p_library_handle, OS::GDExtensionData* p_data = nullptr);
	static Error close_dynamic_library(void* p_library_handle);
	static Error get_dynamic_library_symbol_handle(void* p_library_handle, const String& p_name,
		void*& p_symbol_handle, bool p_optional = false);

	static String get_name();
	static String get_distribution_name();
	static String get_version();
	static String get_version_alias();

	static Vector<String> get_video_adapter_driver_info();
	static bool get_user_prefers_integrated_gpu();

	static void initialize_joypads() {}

	static OS::DateTime get_datetime(bool p_utc);
	static OS::TimeZoneInfo get_time_zone_info();
	static double get_unix_time();

	static Error set_cwd(const String& p_cwd);
	static String get_cwd();

	static void add_frame_delay(bool p_can_draw, bool p_wake_for_events);
	static void delay_usec(uint32_t p_usec);
	static uint64_t get_ticks_usec();

	static Dictionary get_memory_info();

	static Error execute(const String& p_path, const List<String>& p_arguments,
		String* r_pipe = nullptr, int* r_exitcode = nullptr, bool read_stderr = false,
		Mutex* p_pipe_mutex = nullptr, bool p_open_console = false);
	static Dictionary execute_with_pipe(
		const String& p_path, const List<String>& p_arguments, bool p_blocking = true);
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

	static Vector<String> get_system_fonts();
	static String get_system_font_path(
		const String& p_font_name, int p_weight = 400, int p_stretch = 100, bool p_italic = false);
	static Vector<String> get_system_font_path_for_text(const String& p_font_name,
		const String& p_text, const String& p_locale = String(), const String& p_script = String(),
		int p_weight = 400, int p_stretch = 100, bool p_italic = false);

	static String get_executable_path();

	static String get_locale();
	static Vector<String> get_preferred_locales();

	static String get_processor_name();
	static String get_model_name();

	static uint64_t get_embedded_pck_offset();

	static String get_config_path();
	static String get_data_path();
	static String get_cache_path();
	static String get_temp_path();
	static String get_godot_dir_name();

	static String get_system_dir(OS::SystemDir p_dir, bool p_shared_storage = true);
	static String get_user_data_dir(const String& p_user_dir);

	static String expand_path(const String& p_path);

	static String get_unique_id();

	static Error shell_open(const String& p_uri);
	static Error shell_show_in_file_manager(String p_path, bool p_open_folder);

	static void run();

	static bool _check_internal_feature_support(const String& p_feature);

	static String multibyte_to_string(const String& p_encoding, const PackedByteArray& p_array);
	static PackedByteArray string_to_multibyte(const String& p_encoding, const String& p_string);

	static void disable_crash_handler();
	static bool is_disable_crash_handler();
	static void initialize_debugging();

	static Error move_to_trash(const String& p_path);

	static String get_system_ca_certificates();

	static void set_main_window(HWND p_main_window) { main_window = p_main_window; }

	static String get_platform_string(OS::PlatformString p_platform_string)
	{
		switch (p_platform_string) {
		case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_OPEN:
			return ETR("Open in File Explorer");
		case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_SHOW:
			return ETR("Show in File Explorer");
		default:
			return OS::get_platform_string(p_platform_string);
		}
	}

#ifdef TOOLS_ENABLED
	static bool _test_create_rendering_device_and_gl(const String& p_display_driver);
	static bool _test_create_rendering_device(const String& p_display_driver);
#endif

	static void set_hinstance(HINSTANCE p_hInstance) { hInstance = p_hInstance; }

	static HINSTANCE get_hinstance() { return hInstance; }
};

#endif // _WIN32

// --- Folded from drivers/windows/file_access_windows.h ---
#ifdef WINDOWS_ENABLED

#include "core/io/file_access.h"

#include <cstdio>

class FileAccessWindows : public FileAccess {
	FILE *f = nullptr;
	int flags = 0;
	void check_errors(bool p_write = false) const;
	mutable int prev_op = 0;
	mutable Error last_error = OK;
	String path;
	String path_src;
	String save_path;

	void _close();

	static HashSet<String> invalid_files;

public:
	static bool is_path_invalid(const String &p_path);

	virtual String fix_path(const String &p_path) const override;
	virtual Error open_internal(const String &p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override; ///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override; ///< get position in the file
	virtual uint64_t get_length() const override; ///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(uint8_t *p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override;
	virtual void flush() override;
	virtual bool store_buffer(const uint8_t *p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String &p_name) override; ///< return true if a file exists

	uint64_t _get_modified_time(const String &p_file) override;
	uint64_t _get_access_time(const String &p_file) override;
	int64_t _get_size(const String &p_file) override;
	virtual uint32_t _get_unix_permissions(const String &p_file) override;
	virtual Error _set_unix_permissions(const String &p_file, uint32_t p_permissions) override;

	virtual bool _get_hidden_attribute(const String &p_file) override;
	virtual Error _set_hidden_attribute(const String &p_file, bool p_hidden) override;
	virtual bool _get_read_only_attribute(const String &p_file) override;
	virtual Error _set_read_only_attribute(const String &p_file, bool p_ro) override;

	virtual PackedByteArray _get_extended_attribute(const String &p_file, const String &p_attribute_name) override;
	virtual Error _set_extended_attribute(const String &p_file, const String &p_attribute_name, const PackedByteArray &p_data) override;
	virtual Error _remove_extended_attribute(const String &p_file, const String &p_attribute_name) override;
	virtual PackedStringArray _get_extended_attributes_list(const String &p_file) override;

	virtual void close() override;

	static void initialize();
	static void finalize();

	FileAccessWindows() {}
	virtual ~FileAccessWindows();
};

#endif // WINDOWS_ENABLED

// --- Folded from drivers/windows/dir_access_windows.h ---
#ifdef WINDOWS_ENABLED

#include "core/io/net_socket.h"

struct DirAccessWindowsPrivate;

class DirAccessWindows : public DirAccess {
	enum {
		MAX_DRIVES = 26
	};

	DirAccessWindowsPrivate *p = nullptr;
	/* Windows stuff */

	struct DriveInfo {
		String path;
		String label;
	};

	LocalVector<DriveInfo> drives;
	void _update_drives();

	String current_dir;

	bool _cisdir = false;
	bool _cishidden = false;

protected:
	virtual String fix_path(const String &p_path) const override;

public:
	virtual Error list_dir_begin() override; ///< This starts dir listing
	virtual String get_next() override;
	virtual bool current_is_dir() const override;
	virtual bool current_is_hidden() const override;
	virtual void list_dir_end() override; ///<

	virtual int get_drive_count() override;
	virtual String get_drive(int p_drive) override;
	virtual String get_drive_label(int p_drive) override;

	virtual Error change_dir(String p_dir) override; ///< can be relative or absolute, return false on success
	virtual String get_current_dir(bool p_include_drive = true) const override; ///< return current dir location

	virtual bool file_exists(String p_file) override;
	virtual bool dir_exists(String p_dir) override;

	virtual Error make_dir(String p_dir) override;

	virtual Error rename(String p_path, String p_new_path) override;
	virtual Error remove(String p_path) override;

	virtual bool is_link(String p_file) override;
	virtual String read_link(String p_file) override;
	virtual Error create_link(String p_source, String p_target) override;

	uint64_t get_space_left() override;

	virtual String get_filesystem_type() const override;
	virtual bool is_case_sensitive(const String &p_path) const override;
	virtual bool is_equivalent(const String &p_path_a, const String &p_path_b) const override;

	DirAccessWindows();
	~DirAccessWindows();
};

#endif // WINDOWS_ENABLED

// --- End drivers/windows/dir_access_windows.h ---

// --- Folded from drivers/windows/file_access_windows_pipe.h ---
#ifdef WINDOWS_ENABLED

#include "core/io/file_access.h"

#include <windows.h>

class FileAccessWindowsPipe : public FileAccess {
	HANDLE fd[2] = { nullptr, nullptr };

	mutable Error last_error = OK;

	String path;
	String path_src;

	void _close();

public:
	Error open_existing(HANDLE p_rfd, HANDLE p_wfd, bool p_blocking);

	virtual Error open_internal(const String &p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override {}
	virtual void seek_end(int64_t p_position = 0) override {}
	virtual uint64_t get_position() const override { return 0; }
	virtual uint64_t get_length() const override;

	virtual bool eof_reached() const override { return false; }

	virtual uint64_t get_buffer(uint8_t *p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }
	virtual void flush() override {}
	virtual bool store_buffer(const uint8_t *p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String &p_name) override { return false; }

	uint64_t _get_modified_time(const String &p_file) override { return 0; }
	virtual uint64_t _get_access_time(const String &p_file) override { return 0; }
	virtual int64_t _get_size(const String &p_file) override { return -1; }
	virtual uint32_t _get_unix_permissions(const String &p_file) override { return 0; }
	virtual Error _set_unix_permissions(const String &p_file, uint32_t p_permissions) override { return ERR_UNAVAILABLE; }

	virtual bool _get_hidden_attribute(const String &p_file) override { return false; }
	virtual Error _set_hidden_attribute(const String &p_file, bool p_hidden) override { return ERR_UNAVAILABLE; }
	virtual bool _get_read_only_attribute(const String &p_file) override { return false; }
	virtual Error _set_read_only_attribute(const String &p_file, bool p_ro) override { return ERR_UNAVAILABLE; }

	virtual void close() override;

	FileAccessWindowsPipe() {}
	virtual ~FileAccessWindowsPipe();
};

#endif // WINDOWS_ENABLED

// --- End drivers/windows/file_access_windows_pipe.h ---

// --- Folded from drivers/windows/ip_windows.h ---
#if defined(WINDOWS_ENABLED)

#include "core/io/ip.h"

class IPWindows final
{
public:
	static void _resolve_hostname(
		List<IPAddress>& r_addresses, const String& p_hostname, IP::Type p_type = IP::TYPE_ANY);
	static void get_local_interfaces(HashMap<String, IP::Interface_Info>* r_interfaces);

	static void make_default();

	IPWindows() = delete;
	IPWindows(const IPWindows&) = delete;
	IPWindows& operator=(const IPWindows&) = delete;
	~IPWindows() = delete;
};

#endif // WINDOWS_ENABLED

// --- End drivers/windows/ip_windows.h ---

// --- Folded from drivers/windows/net_socket_winsock.h ---
#ifdef WINDOWS_ENABLED

#include "core/io/net_socket.h"

#include <winsock2.h>
#include <ws2tcpip.h>

class NetSocketWinSock : public NetSocket {

private:
	SOCKET _sock = INVALID_SOCKET;
	IP::Type _ip_type = IP::TYPE_NONE;
	bool _is_stream = false;

	enum NetError {
		ERR_NET_WOULD_BLOCK,
		ERR_NET_IS_CONNECTED,
		ERR_NET_IN_PROGRESS,
		ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE,
		ERR_NET_UNAUTHORIZED,
		ERR_NET_BUFFER_TOO_SMALL,
		ERR_NET_OTHER,
	};

	NetError _get_socket_error() const;
	void _set_socket(SOCKET p_sock, IP::Type p_ip_type, bool p_is_stream);
	_FORCE_INLINE_ Error _change_multicast_group(IPAddress p_ip, String p_if_name, bool p_add);

protected:
	static Ref<NetSocket> _create_func();

	bool _can_use_ip(const IPAddress &p_ip, const bool p_for_bind) const;

public:
	static void make_default();
	static void cleanup();
	static void _set_ip_port(struct sockaddr_storage *p_addr, IPAddress *r_ip, uint16_t *r_port);
	static size_t _set_addr_storage(struct sockaddr_storage *p_addr, const IPAddress &p_ip, uint16_t p_port, IP::Type p_ip_type);

	virtual Error open(Family p_family, Type p_sock_type, IP::Type &ip_type) override;
	virtual void close() override;
	virtual Error bind(Address p_addr) override;
	virtual Error listen(int p_max_pending) override;
	virtual Error connect_to_host(Address p_addr) override;
	virtual Error poll(PollType p_type, int timeout) const override;
	virtual Error recv(uint8_t *p_buffer, int p_len, int &r_read) override;
	virtual Error recvfrom(uint8_t *p_buffer, int p_len, int &r_read, IPAddress &r_ip, uint16_t &r_port, bool p_peek = false) override;
	virtual Error send(const uint8_t *p_buffer, int p_len, int &r_sent) override;
	virtual Error sendto(const uint8_t *p_buffer, int p_len, int &r_sent, IPAddress p_ip, uint16_t p_port) override;
	virtual Ref<NetSocket> accept(Address &r_addr) override;

	virtual bool is_open() const override;
	virtual int get_available_bytes() const override;
	virtual Error get_socket_address(Address *r_addr) const override;

	virtual Error set_broadcasting_enabled(bool p_enabled) override;
	virtual void set_blocking_enabled(bool p_enabled) override;
	virtual void set_ipv6_only_enabled(bool p_enabled) override;
	virtual void set_tcp_no_delay_enabled(bool p_enabled) override;
	virtual void set_reuse_address_enabled(bool p_enabled) override;
	virtual Error join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) override;
	virtual Error leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) override;

	NetSocketWinSock();
	~NetSocketWinSock() override;
};

#endif // WINDOWS_ENABLED

// --- End drivers/windows/net_socket_winsock.h ---

// --- Folded from drivers/windows/thread_windows.h ---
#ifdef WINDOWS_ENABLED

void init_thread_win();

#endif // WINDOWS_ENABLED

// --- End drivers/windows/thread_windows.h ---

// --- End drivers/windows/file_access_windows.h ---

// --- Folded from platform/windows/windows_terminal_logger.h ---
#ifdef WINDOWS_ENABLED

#include "core/os/os.h"

class WindowsTerminalLogger : public StdLogger {
public:
	virtual void logv(const char *p_format, va_list p_list, bool p_err) override;
	virtual void log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, bool p_editor_notify = false, ErrorType p_type = ERR_ERROR, const Vector<Ref<ScriptBacktrace>> &p_script_backtraces = {}) override;
	virtual ~WindowsTerminalLogger();
};

#endif // WINDOWS_ENABLED

// --- End platform/windows/windows_terminal_logger.h ---

// --- Folded from platform/windows/windows_utils.h ---
#ifdef WINDOWS_ENABLED

#include "core/string/ustring.h"
#include "core/io/ip.h"

class WindowsUtils {
	static HashMap<String, Vector<String>> temp_pdbs;

public:
	static Error copy_and_rename_pdb(const String &p_dll_path);
	static void remove_temp_pdbs(const String &p_dll_path);
};

#endif // WINDOWS_ENABLED

// --- End platform/windows/windows_utils.h ---

// --- Folded from platform/windows/winrt_utils.h ---
#include "core/typedefs.h"

#include <windows.h>

class WinRTWindowData;

class WinRTUtils {
public:
	static bool try_show_onecore_emoji_picker();

	static bool create_queue();
	static void destroy_queue();

	static Vector<String> get_preferred_locales();

	static WinRTWindowData *create_wd(HWND p_window, const Callable &p_color_cb, int64_t p_window_id);
	static bool window_has_display_info(const WinRTWindowData *p_data);
	static void window_get_advanced_color_info(const WinRTWindowData *p_data, bool &r_hdr_supported, float &r_min_luminance, float &r_max_luminance, float &r_max_average_luminance, float &r_sdr_white_level);
	static void destroy_wd(WinRTWindowData *p_data);
};

// --- End platform/windows/winrt_utils.h ---

// --- Folded from platform/windows/display_server_windows.h ---
#if defined(WINDOWS_ENABLED)
#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/os/os.h"
#include "core/io/ip.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

#ifdef XAUDIO2_ENABLED

#endif

#if defined(RD_ENABLED)
#include "servers/rendering/renderer.h"
#endif

#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
// --- Folded from platform/windows/gl_manager_windows_angle.h ---
#if defined(WINDOWS_ENABLED) && defined(GLES3_ENABLED)

#include "servers/rendering/renderer.h"
#include "servers/display/display_server.h"

#include <windows.h>

class GLManagerANGLE_Windows : public EGLManager {
private:
	virtual const char *_get_platform_extension_name() const override;
	virtual EGLenum _get_platform_extension_enum() const override;
	virtual EGLenum _get_platform_api_enum() const override;
	virtual Vector<EGLAttrib> _get_platform_display_attributes() const override;
	virtual Vector<EGLint> _get_platform_context_attribs() const override;

public:
	void window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height);

	GLManagerANGLE_Windows() {}
	~GLManagerANGLE_Windows() {}
};

#endif // WINDOWS_ENABLED && GLES3_ENABLED

// --- End platform/windows/gl_manager_windows_angle.h ---
#endif // ANGLE_ENABLED
// --- Folded from platform/windows/gl_manager_windows_native.h ---
#if defined(WINDOWS_ENABLED) && defined(GLES3_ENABLED)

#include "core/string/ustring.h"
#include "core/io/ip.h"
#include "servers/display/display_server.h"

#include <windows.h>

typedef bool(APIENTRY *PFNWGLSWAPINTERVALEXTPROC)(int interval);
typedef int(APIENTRY *PFNWGLGETSWAPINTERVALEXTPROC)(void);

class GLManagerNative_Windows {
private:
	// any data specific to the window
	struct GLWindow {
		bool use_vsync = false;

		// windows specific
		HDC hDC;
		HWND hwnd;

		int gldisplay_id = 0;
	};

	struct GLDisplay {
		// windows specific
		HGLRC hRC;
	};

	RBMap<DisplayServerEnums::WindowID, GLWindow> _windows;
	LocalVector<GLDisplay> _displays;

	GLWindow *_current_window = nullptr;

	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;

	GLWindow &get_window(unsigned int id) { return _windows[id]; }
	const GLWindow &get_window(unsigned int id) const { return _windows[id]; }

	const GLDisplay &get_current_display() const { return _displays[_current_window->gldisplay_id]; }
	const GLDisplay &get_display(unsigned int id) { return _displays[id]; }

	bool direct_render;
	int glx_minor, glx_major;

private:
	void _nvapi_setup_profile();
	int _find_or_create_display(GLWindow &win);
	Error _create_context(GLWindow &win, GLDisplay &gl_display);

public:
	Error window_create(DisplayServerEnums::WindowID p_window_id, HWND p_hwnd, HINSTANCE p_hinstance, int p_width, int p_height);
	void window_destroy(DisplayServerEnums::WindowID p_window_id);
	void window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height) {}

	void release_current();
	void swap_buffers();

	void window_make_current(DisplayServerEnums::WindowID p_window_id);

	Error initialize();

	void set_use_vsync(DisplayServerEnums::WindowID p_window_id, bool p_use);
	bool is_using_vsync(DisplayServerEnums::WindowID p_window_id) const;

	HDC get_hdc(DisplayServerEnums::WindowID p_window_id);
	HGLRC get_hglrc(DisplayServerEnums::WindowID p_window_id);

	GLManagerNative_Windows();
	~GLManagerNative_Windows();
};

#endif // WINDOWS_ENABLED && GLES3_ENABLED

// --- End platform/windows/gl_manager_windows_native.h ---
#endif // GLES3_ENABLED

#include <windows.h>

#include <io.h>
#include <shobjidl.h>
#include <windowsx.h>

// WinTab API
#define WT_PACKET 0x7FF0
#define WT_PROXIMITY 0x7FF5
#define WT_INFOCHANGE 0x7FF6
#define WT_CSRCHANGE 0x7FF7

#define WTI_DEFSYSCTX 4
#define WTI_DEVICES 100
#define DVC_NPRESSURE 15
#define DVC_TPRESSURE 16
#define DVC_ORIENTATION 17
#define DVC_ROTATION 18

#define CXO_MESSAGES 0x0004
#define PK_STATUS 0x0002
#define PK_NORMAL_PRESSURE 0x0400
#define PK_TANGENT_PRESSURE 0x0800
#define PK_ORIENTATION 0x1000

#define TPS_INVERT 0x0010 /* 1.1 */

typedef struct tagLOGCONTEXTW {
	WCHAR lcName[40];
	UINT lcOptions;
	UINT lcStatus;
	UINT lcLocks;
	UINT lcMsgBase;
	UINT lcDevice;
	UINT lcPktRate;
	DWORD lcPktData;
	DWORD lcPktMode;
	DWORD lcMoveMask;
	DWORD lcBtnDnMask;
	DWORD lcBtnUpMask;
	LONG lcInOrgX;
	LONG lcInOrgY;
	LONG lcInOrgZ;
	LONG lcInExtX;
	LONG lcInExtY;
	LONG lcInExtZ;
	LONG lcOutOrgX;
	LONG lcOutOrgY;
	LONG lcOutOrgZ;
	LONG lcOutExtX;
	LONG lcOutExtY;
	LONG lcOutExtZ;
	DWORD lcSensX;
	DWORD lcSensY;
	DWORD lcSensZ;
	BOOL lcSysMode;
	int lcSysOrgX;
	int lcSysOrgY;
	int lcSysExtX;
	int lcSysExtY;
	DWORD lcSysSensX;
	DWORD lcSysSensY;
} LOGCONTEXTW;

typedef struct tagAXIS {
	LONG axMin;
	LONG axMax;
	UINT axUnits;
	DWORD axResolution;
} AXIS;

typedef struct tagORIENTATION {
	int orAzimuth;
	int orAltitude;
	int orTwist;
} ORIENTATION;

typedef struct tagPACKET {
	int pkStatus;
	int pkNormalPressure;
	int pkTangentPressure;
	ORIENTATION pkOrientation;
} PACKET;

typedef HANDLE(WINAPI *WTOpenPtr)(HWND p_window, LOGCONTEXTW *p_ctx, BOOL p_enable);
typedef BOOL(WINAPI *WTClosePtr)(HANDLE p_ctx);
typedef UINT(WINAPI *WTInfoPtr)(UINT p_category, UINT p_index, LPVOID p_output);
typedef BOOL(WINAPI *WTPacketPtr)(HANDLE p_ctx, UINT p_param, LPVOID p_packets);
typedef BOOL(WINAPI *WTEnablePtr)(HANDLE p_ctx, BOOL p_enable);

enum PreferredAppMode {
	APPMODE_DEFAULT = 0,
	APPMODE_ALLOWDARK = 1,
	APPMODE_FORCEDARK = 2,
	APPMODE_FORCELIGHT = 3,
	APPMODE_MAX = 4
};

typedef const char *(CDECL *WineGetVersionPtr)();
typedef bool(WINAPI *ShouldAppsUseDarkModePtr)();
typedef DWORD(WINAPI *GetImmersiveColorFromColorSetExPtr)(UINT dwImmersiveColorSet, UINT dwImmersiveColorType, bool bIgnoreHighContrast, UINT dwHighContrastCacheMode);
typedef int(WINAPI *GetImmersiveColorTypeFromNamePtr)(const WCHAR *name);
typedef int(WINAPI *GetImmersiveUserColorSetPreferencePtr)(bool bForceCheckRegistry, bool bSkipCheckOnFail);
typedef HRESULT(WINAPI *RtlGetVersionPtr)(OSVERSIONINFOEXW *lpVersionInformation);
typedef bool(WINAPI *AllowDarkModeForAppPtr)(bool darkMode);
typedef PreferredAppMode(WINAPI *SetPreferredAppModePtr)(PreferredAppMode appMode);
typedef void(WINAPI *RefreshImmersiveColorPolicyStatePtr)();
typedef void(WINAPI *FlushMenuThemesPtr)();

typedef struct {
	BYTE bWidth; // Width, in pixels, of the image
	BYTE bHeight; // Height, in pixels, of the image
	BYTE bColorCount; // Number of colors in image (0 if >=8bpp)
	BYTE bReserved; // Reserved ( must be 0)
	WORD wPlanes; // Color Planes
	WORD wBitCount; // Bits per pixel
	DWORD dwBytesInRes; // How many bytes in this resource?
	DWORD dwImageOffset; // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct {
	WORD idReserved; // Reserved (must be 0)
	WORD idType; // Resource Type (1 for icons)
	WORD idCount; // How many images?
	ICONDIRENTRY idEntries[1]; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;

class DropTargetWindows;
class NativeMenuWindows;
class WinRTWindowData;

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

class JoypadSDL;

class DisplayServerWindows : public DisplayServerDriver {

	friend class DropTargetWindows;

	_THREAD_SAFE_CLASS_

	// UXTheme API
	static bool dark_title_available;
	static bool use_legacy_dark_mode_before_20H1;
	static bool ux_theme_available;
	static ShouldAppsUseDarkModePtr ShouldAppsUseDarkMode;
	static GetImmersiveColorFromColorSetExPtr GetImmersiveColorFromColorSetEx;
	static GetImmersiveColorTypeFromNamePtr GetImmersiveColorTypeFromName;
	static GetImmersiveUserColorSetPreferencePtr GetImmersiveUserColorSetPreference;

	// WinTab API
	static bool wintab_available;
	static WTOpenPtr wintab_WTOpen;
	static WTClosePtr wintab_WTClose;
	static WTInfoPtr wintab_WTInfo;
	static WTPacketPtr wintab_WTPacket;
	static WTEnablePtr wintab_WTEnable;

	void _update_tablet_ctx(const String &p_old_driver, const String &p_new_driver);
	String tablet_driver;
	Vector<String> tablet_drivers;
	bool winink_disabled = false;

	enum DriverID {
		DRIVER_ID_COMPAT_OPENGL3 = 1 << 0,
		DRIVER_ID_COMPAT_ANGLE_D3D11 = 1 << 1,
		DRIVER_ID_RD_VULKAN = 1 << 2,
		DRIVER_ID_RD_D3D12 = 1 << 3,
	};
	static uint32_t tested_drivers;

	enum TimerID {
		TIMER_ID_MOVE_REDRAW = 1,
		TIMER_ID_WINDOW_ACTIVATION = 2,
	};

	OSVERSIONINFOEXW os_ver;

	enum {
		KEY_EVENT_BUFFER_SIZE = 512
	};

	struct KeyEvent {
		DisplayServerEnums::WindowID window_id;
		bool alt, shift, control, meta, altgr;
		UINT uMsg;
		WPARAM wParam;
		LPARAM lParam;
	};

	DisplayServerEnums::WindowID window_mouseover_id = DisplayServerEnums::INVALID_WINDOW_ID;

	KeyEvent key_event_buffer[KEY_EVENT_BUFFER_SIZE];
	int key_event_pos;

	bool old_invalid;
	int old_x, old_y;
	Point2i center;

#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	GLManagerANGLE_Windows *gl_manager_angle = nullptr;
#endif
	GLManagerNative_Windows *gl_manager_native = nullptr;
#endif

#if defined(RD_ENABLED)
	RenderingContextDriver *rendering_context = nullptr;
	RenderingDevice *rendering_device = nullptr;
#endif

	RBMap<int, Vector2> touch_state;

	int pressrc;
	HINSTANCE hInstance; // Holds The Instance Of The Application
	String rendering_driver;
	bool app_focused = false;
	bool keep_screen_on = false;
	bool get_object_received = false;
	HANDLE power_request;

	NativeMenuWindows *native_menu = nullptr;
	ITaskbarList3 *taskbar = nullptr;

	bool has_winrt_queue = false;
	void _winrt_adv_color_info_cb(DisplayServerEnums::WindowID p_id);

	struct WindowData {
		HWND hWnd;
		DisplayServerEnums::WindowID id;
		WinRTWindowData *wrt_wd = nullptr;

		Vector<Vector2> mpath;
		DisplayServerEnums::ProgressState progress_state = DisplayServerEnums::PROGRESS_STATE_NOPROGRESS;
		float progress_value = 0.0;

		bool create_completed = false;
		bool pre_fs_valid = false;
		RECT pre_fs_rect;
		bool maximized = false;
		bool maximized_fs = false;
		bool minimized = false;
		bool fullscreen = false;
		bool multiwindow_fs = false;
		bool borderless = false;
		bool resizable = true;
		bool no_min_btn = false;
		bool no_max_btn = false;
		bool window_focused = false;
		int activate_state = 0;
		bool was_maximized_pre_fs = false;
		bool was_fullscreen_pre_min = false;
		bool first_activation_done = false;
		bool was_maximized = false;
		bool always_on_top = false;
		bool no_focus = false;
		bool exclusive = false;
		bool rendering_context_window_created = false;
		bool gl_native_window_created = false;
#ifdef ANGLE_ENABLED
		bool gl_angle_window_created = false;
#endif
		bool mpass = false;
		bool sharp_corners = false;
		bool hide_from_capture = false;

		// Used to transfer data between events using timer.
		WPARAM saved_wparam;
		LPARAM saved_lparam;

		// Timers.
		uint32_t move_timer_id = 0U;
		uint32_t activate_timer_id = 0U;

		HANDLE wtctx;
		LOGCONTEXTW wtlc;
		int min_pressure;
		int max_pressure;
		bool tilt_supported;
		bool pen_inverted = false;
		bool block_mm = false;

		int last_pressure_update;
		float last_pressure;
		Vector2 last_tilt;
		bool last_pen_inverted = false;

		Size2 min_size;
		Size2 max_size;
		int width = 0, height = 0;
		int width_with_decorations = 0, height_with_decorations = 0;

		Size2 window_rect;
		Point2 last_pos;

		ObjectID instance_id;
		bool icon_set = false;
		Vector<BYTE> icon_buffer_big;
		HICON icon_big = nullptr;
		Vector<BYTE> icon_buffer_small;
		HICON icon_small = nullptr;

		// IME
		HIMC im_himc;
		Vector2 im_position;
		bool ime_active = false;
		bool ime_in_progress = false;
		bool ime_suppress_next_keyup = false;

		bool layered_window = false;

		Callable rect_changed_callback;
		Callable event_callback;
		Callable input_event_callback;
		Callable input_text_callback;
		Callable drop_files_callback;

		// OLE API
		DropTargetWindows *drop_target = nullptr;

		DisplayServerEnums::WindowID transient_parent = DisplayServerEnums::INVALID_WINDOW_ID;
		HashSet<DisplayServerEnums::WindowID> transient_children;

		bool is_popup = false;
		Rect2i parent_safe_rect;

		// HDR
		bool hdr_output_requested = false;
		float hdr_output_reference_luminance = -1.0f;
		float hdr_output_max_luminance = -1.0f;

		bool initialized = false;

		HWND parent_hwnd = nullptr;

		bool no_redirection_bitmap = false;
	};

#ifdef SDL_ENABLED
	JoypadSDL *joypad_sdl = nullptr;
#endif
	HHOOK mouse_monitor = nullptr;
	List<DisplayServerEnums::WindowID> popup_list;
	uint64_t time_since_popup = 0;

	Error _create_window(DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::WindowMode p_mode, uint32_t p_flags, const Rect2i &p_rect, bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent, HWND p_parent_hwnd, bool p_no_redirection_bitmap);
	void _destroy_window(DisplayServerEnums::WindowID p_window_id); // Destroys only what was needed to be created for the main window. Does not destroy transient parent dependencies or GL/rendering context windows.

	void _window_set_native_icon(const String &p_filename, DisplayServerEnums::WindowID p_window);

#ifdef RD_ENABLED
	Error _create_rendering_context_window(DisplayServerEnums::WindowID p_window_id, const String &p_rendering_driver);
	void _destroy_rendering_context_window(DisplayServerEnums::WindowID p_window_id);
#endif

#ifdef GLES3_ENABLED
	Error _create_gl_window(DisplayServerEnums::WindowID p_window_id);
#endif

	DisplayServerEnums::WindowID window_id_counter = DisplayServerEnums::MAIN_WINDOW_ID;
	RBMap<DisplayServerEnums::WindowID, WindowData> windows;

	DisplayServerEnums::WindowID last_focused_window = DisplayServerEnums::INVALID_WINDOW_ID;
	DisplayServerEnums::WindowID last_mouse_button_down_window = DisplayServerEnums::INVALID_WINDOW_ID;
	HCURSOR hCursor;

	WNDPROC user_proc = nullptr;

	struct IndicatorData {
		RID menu_rid;
		Callable callback;
		Vector<BYTE> icon_buffer;
		HICON icon = nullptr;
	};

	DisplayServerEnums::IndicatorID indicator_id_counter = 0;
	HashMap<DisplayServerEnums::IndicatorID, IndicatorData> indicators;

	struct FileDialogData {
		HWND hwnd_owner = nullptr;
		Rect2i wrect;
		String appid;
		String title;
		String current_directory;
		String root;
		String filename;
		bool show_hidden = false;
		DisplayServerEnums::FileDialogMode mode = DisplayServerEnums::FileDialogMode::FILE_DIALOG_MODE_OPEN_ANY;
		Vector<String> filters;
		TypedArray<Dictionary> options;
		DisplayServerEnums::WindowID window_id = DisplayServerEnums::INVALID_WINDOW_ID;
		Callable callback;
		bool options_in_cb = false;
		Thread listener_thread;
		SafeFlag close_requested;
		SafeFlag finished;
	};
	Mutex file_dialog_mutex;
	List<FileDialogData *> file_dialogs;
	HashMap<HWND, FileDialogData *> file_dialog_wnd;
	struct FileDialogCallback {
		Callable callback;
		Variant status;
		Variant files;
		Variant index;
		Variant options;
		bool opt_in_cb = false;
	};
	List<FileDialogCallback> pending_cbs;
	void process_file_dialog_callbacks();

	static void _thread_fd_monitor(void *p_ud);

	HashMap<int64_t, MouseButton> pointer_prev_button;
	HashMap<int64_t, MouseButton> pointer_button;
	HashMap<int64_t, LONG> pointer_down_time;
	HashMap<int64_t, Vector2> pointer_last_pos;

	void _send_window_event(const WindowData &wd, DisplayServerEnums::WindowEvent p_event);
	void _get_window_style(bool p_main_window, bool p_initialized, bool p_fullscreen, bool p_multiwindow_fs, bool p_borderless, bool p_resizable, bool p_no_min_btn, bool p_no_max_btn, bool p_minimized, bool p_maximized, bool p_maximized_fs, bool p_no_activate_focus, bool p_embed_child, bool p_no_redirection_bitmap, DWORD &r_style, DWORD &r_style_ex);

	DisplayServerEnums::MouseMode mouse_mode;
	DisplayServerEnums::MouseMode mouse_mode_base = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	DisplayServerEnums::MouseMode mouse_mode_override = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	bool mouse_mode_override_enabled = false;
	void _mouse_update_mode();
	int restore_mouse_trails = 0;

	bool use_raw_input = false;
	bool drop_events = false;
	bool in_dispatch_input_event = false;

	WNDCLASSEXW wc;
	HBRUSH window_bkg_brush = nullptr;
	uint32_t window_bkg_brush_color = 0;

	HCURSOR cursors[DisplayServerEnums::CURSOR_MAX] = { nullptr };
	DisplayServerEnums::CursorShape cursor_shape = DisplayServerEnums::CursorShape::CURSOR_ARROW;
	RBMap<DisplayServerEnums::CursorShape, Vector<Variant>> cursors_cache;

	Callable system_theme_changed;

	void _drag_event(DisplayServerEnums::WindowID p_window, float p_x, float p_y, int idx);
	void _touch_event(DisplayServerEnums::WindowID p_window, bool p_pressed, float p_x, float p_y, int idx);

	bool _is_always_on_top_recursive(DisplayServerEnums::WindowID p_window) const;

	void _update_window_style(DisplayServerEnums::WindowID p_window, bool p_repaint = true);
	void _update_window_mouse_passthrough(DisplayServerEnums::WindowID p_window);

	void _update_real_mouse_position(DisplayServerEnums::WindowID p_window);

	void _set_mouse_mode_impl(DisplayServerEnums::MouseMode p_mode);
	DisplayServerEnums::WindowID _get_focused_window_or_popup() const;
	void _register_raw_input_devices(DisplayServerEnums::WindowID p_target_window);
	bool _has_moving_window() const;

	void _process_activate_event(DisplayServerEnums::WindowID p_window_id);
	void _process_key_events();

	static void _dispatch_input_events(const Ref<InputEvent> &p_event);
	void _dispatch_input_event(const Ref<InputEvent> &p_event);

	LRESULT _handle_early_window_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	Point2i _get_screens_origin() const;

	Vector2i _get_screen_expand_offset(int p_screen) const;

	enum class WinKeyModifierMask {
		ALT_GR = (1 << 1),
		SHIFT = (1 << 2),
		ALT = (1 << 3),
		META = (1 << 4),
		CTRL = (1 << 5),
	};
	uint32_t _get_mods() const;

	Error _file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, bool p_options_in_cb, DisplayServerEnums::WindowID p_window_id);

	String _get_keyboard_layout_display_name(const String &p_klid) const;
	String _get_klid(HKL p_hkl) const;

	struct EmbeddedProcessData {
		HWND window_handle = nullptr;
		HWND parent_window_handle = nullptr;
		bool is_visible = false;
	};
	HashMap<ProcessID, EmbeddedProcessData *> embedded_processes;

	HWND _find_window_from_process_id(ProcessID p_pid, HWND p_current_hwnd);

	void initialize_tts() const;

	struct ScreenHdrData {
		bool hdr_supported = false;
		float min_luminance = 0.0f;
		float max_luminance = 0.0f;
		float max_average_luminance = 0.0f;
		float sdr_white_level = 0.0f;
	};
	AHashMap<int, ScreenHdrData> hdr_output_cache;

	ScreenHdrData _get_screen_hdr_data(DisplayServerEnums::WindowID p_window, bool p_include_sdr_white_level) const;
	void _update_hdr_output_for_window(DisplayServerEnums::WindowID p_window, const WindowData &p_window_data, ScreenHdrData p_screen_data);
	void _legacy_update_hdr_output_for_tracked_windows(bool p_include_sdr_white_level);

public:
	LRESULT WndProcFileDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT MouseProc(int code, WPARAM wParam, LPARAM lParam);

	void popup_open(DisplayServerEnums::WindowID p_window);
	void popup_close(DisplayServerEnums::WindowID p_window);

	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;
	virtual String get_name() const override;

	virtual bool tts_is_speaking() const override;
	virtual bool tts_is_paused() const override;
	virtual TypedArray<Dictionary> tts_get_voices() const override;

	virtual void tts_speak(const String &p_text, const String &p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false) override;
	virtual void tts_pause() override;
	virtual void tts_resume() override;
	virtual void tts_stop() override;

	virtual bool is_dark_mode_supported() const override;
	virtual bool is_dark_mode() const override;
	virtual Color get_accent_color() const override;
	virtual Color get_base_color() const override;
	virtual void set_system_theme_change_callback(const Callable &p_callable) override;

	virtual Error file_dialog_show(const String &p_title, const String &p_current_directory, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) override;
	virtual Error file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) override;

	virtual void beep() const override;

	virtual void mouse_set_mode(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode() const override;
	virtual void mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode_override() const override;
	virtual void mouse_set_mode_override_enabled(bool p_override_enabled) override;
	virtual bool mouse_is_mode_override_enabled() const override;

	virtual void warp_mouse(const Point2i &p_position) override;
	virtual Point2i mouse_get_position() const override;
	virtual uint32_t mouse_get_button_state() const override;

	virtual void clipboard_set(const String &p_text) override;
	virtual String clipboard_get() const override;
	virtual Ref<Image> clipboard_get_image() const override;
	virtual bool clipboard_has() const override;
	virtual bool clipboard_has_image() const override;

	virtual int get_screen_count() const override;
	virtual int get_primary_screen() const override;
	virtual int get_keyboard_focus_screen() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Color screen_get_pixel(const Point2i &p_position) const override;
	virtual Ref<Image> screen_get_image(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Ref<Image> screen_get_image_rect(const Rect2i &p_rect) const override;

	virtual void screen_set_keep_on(bool p_enable) override; //disable screensaver
	virtual bool screen_is_kept_on() const override;

	virtual Vector<DisplayServerEnums::WindowID> get_window_list() const override;

	virtual DisplayServerEnums::WindowID create_sub_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i &p_rect = Rect2i(), bool p_exclusive = false, DisplayServerEnums::WindowID p_transient_parent = DisplayServerEnums::INVALID_WINDOW_ID) override;
	virtual void show_window(DisplayServerEnums::WindowID p_window) override;
	virtual void delete_sub_window(DisplayServerEnums::WindowID p_window) override;

	virtual DisplayServerEnums::WindowID window_get_active_popup() const override;
	virtual void window_set_popup_safe_rect(DisplayServerEnums::WindowID p_window, const Rect2i &p_rect) override;
	virtual Rect2i window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window) const override;

	virtual int64_t window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i &p_position) const override;

	virtual void window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual ObjectID window_get_attached_instance_id(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void gl_window_make_current(DisplayServerEnums::WindowID p_window_id) override;

	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_title_size(const String &p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
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

	virtual void window_set_icon(const Ref<Image> &p_icon, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

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

	virtual int accessibility_should_increase_contrast() const override;
	virtual int accessibility_should_reduce_animation() const override;
	virtual int accessibility_should_reduce_transparency() const override;
	virtual int accessibility_screen_reader_active() const override;

	virtual Point2i ime_get_selection() const override;
	virtual String ime_get_text() const override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

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

	virtual void window_start_drag(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override;
	virtual DisplayServerEnums::CursorShape cursor_get_shape() const override;
	virtual void cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW, const Vector2 &p_hotspot = Vector2()) override;

	virtual bool get_swap_cancel_ok() override;

	virtual void enable_for_stealing_focus(ProcessID pid) override;
	virtual Error embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid, const Rect2i &p_rect, bool p_visible, bool p_grab_focus) override;
	virtual Error request_close_embedded_process(ProcessID p_pid) override;
	virtual Error remove_embedded_process(ProcessID p_pid) override;
	virtual ProcessID get_focused_process_id() override;

	virtual Error dialog_show(String p_title, String p_description, Vector<String> p_buttons, const Callable &p_callback) override;
	virtual Error dialog_input_text(String p_title, String p_description, String p_partial, const Callable &p_callback) override;

	virtual int keyboard_get_layout_count() const override;
	virtual int keyboard_get_current_layout() const override;
	virtual void keyboard_set_current_layout(int p_index) override;
	virtual String keyboard_get_layout_language(int p_index) const override;
	virtual String keyboard_get_layout_name(int p_index) const override;
	virtual Key keyboard_get_keycode_from_physical(Key p_keycode) const override;
	virtual Key keyboard_get_label_from_physical(Key p_keycode) const override;
	virtual void show_emoji_and_symbol_picker() const override;

	virtual int tablet_get_driver_count() const override;
	virtual String tablet_get_driver_name(int p_driver) const override;
	virtual String tablet_get_current_driver() const override;
	virtual void tablet_set_current_driver(const String &p_driver) override;

	virtual void process_events() override;

	virtual void force_process_and_drop_events() override;

	virtual void release_rendering_thread() override;
	virtual void swap_buffers() override;

	virtual void set_native_icon(const String &p_filename) override;
	virtual void set_icon(const Ref<Image> &p_icon) override;

	virtual DisplayServerEnums::IndicatorID create_status_indicator(const Ref<Texture2D> &p_icon, const String &p_tooltip, const Callable &p_callback) override;
	virtual void status_indicator_set_icon(DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D> &p_icon) override;
	virtual void status_indicator_set_tooltip(DisplayServerEnums::IndicatorID p_id, const String &p_tooltip) override;
	virtual void status_indicator_set_menu(DisplayServerEnums::IndicatorID p_id, const RID &p_rid) override;
	virtual void status_indicator_set_callback(DisplayServerEnums::IndicatorID p_id, const Callable &p_callback) override;
	virtual Rect2 status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const override;
	virtual void delete_status_indicator(DisplayServerEnums::IndicatorID p_id) override;

	virtual void set_context(DisplayServerEnums::Context p_context) override;

	virtual bool is_window_transparency_available() const override;

	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();
	static void register_windows_driver();

	DisplayServerWindows(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerWindows();
};

// --- Folded from platform/windows/drop_target_windows.h ---


#include <shlobj.h>

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wnon-virtual-dtor") // Silence warning due to a COM API weirdness.

// https://learn.microsoft.com/en-us/windows/win32/api/ole2/nf-ole2-dodragdrop#remarks
class DropTargetWindows : public IDropTarget {
	LONG ref_count;
	DisplayServerWindows::WindowData *window_data = nullptr;
	CLIPFORMAT cf_filedescriptor = 0;
	CLIPFORMAT cf_filecontents = 0;
	String tmp_path;

	bool is_valid_filedescriptor();
	HRESULT handle_hdrop_format(Vector<String> *p_files, IDataObject *pDataObj);
	HRESULT handle_filedescriptor_format(Vector<String> *p_files, IDataObject *pDataObj);
	HRESULT save_as_file(const String &p_out_dir, FILEDESCRIPTORW *p_file_desc, IDataObject *pDataObj, int p_file_idx);

public:
	DropTargetWindows(DisplayServerWindows::WindowData *p_window_data);
	virtual ~DropTargetWindows() {}

	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;

	// IDropTarget
	HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
	HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
	HRESULT STDMETHODCALLTYPE DragLeave() override;
	HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
};

VLTR_GCC_WARNING_POP

// --- End platform/windows/drop_target_windows.h ---

// --- Folded from platform/windows/key_mapping_windows.h ---
#include "core/os/os.h"

#include <windows.h>

#include <winuser.h>

class KeyMappingWindows {
	KeyMappingWindows() {}

public:
	static void initialize();

	static Key get_keysym(unsigned int p_code);
	static unsigned int get_scancode(Key p_keycode);
	static Key get_scansym(unsigned int p_code, bool p_extended);
	static bool is_extended_key(unsigned int p_code);
	static KeyLocation get_location(unsigned int p_code, bool p_extended);
};

// --- End platform/windows/key_mapping_windows.h ---

// --- Folded from platform/windows/native_menu_windows.h ---
#include "core/io/net_socket.h"
#include "core/io/ip.h"
#include "servers/display/display_server.h"

#include <windows.h>

class NativeMenuWindows : public NativeMenuDriver {

	enum GlobalMenuCheckType {
		CHECKABLE_TYPE_NONE,
		CHECKABLE_TYPE_CHECK_BOX,
		CHECKABLE_TYPE_RADIO_BUTTON,
	};

	struct MenuItemData {
		Callable callback;
		Variant meta;
		GlobalMenuCheckType checkable_type;
		bool checked = false;
		int max_states = 0;
		int state = 0;
		Ref<Image> img;
		HBITMAP bmp = nullptr;
	};

	struct MenuData {
		HMENU menu = nullptr;

		Callable close_cb;
		bool is_rtl = false;
	};

	mutable RID_PtrOwner<MenuData> menus;
	HashMap<HMENU, RID> menu_lookup;

	HBITMAP _make_bitmap(const Ref<Image> &p_img) const;

public:
	void _menu_activate(HMENU p_menu, int p_index) const;

	virtual bool has_feature(Feature p_feature) const override;

	virtual bool has_system_menu(SystemMenus p_menu_id) const override;
	virtual RID get_system_menu(SystemMenus p_menu_id) const override;

	virtual RID create_menu() override;
	virtual bool has_menu(const RID &p_rid) const override;
	virtual void free_menu(const RID &p_rid) override;

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

	NativeMenuWindows();
	~NativeMenuWindows();
};

// --- End platform/windows/native_menu_windows.h ---

// --- Folded from platform/windows/rendering_context_driver_vulkan_windows.h ---
#ifdef VULKAN_ENABLED

#include "drivers/vulkan/rendering_context_driver_vulkan.h"

#include <windows.h>

class RenderingContextDriverVulkanWindows : public RenderingContextDriverVulkan {
private:
	const char *_get_platform_surface_extension() const override final;

protected:
	SurfaceID surface_create(const void *p_platform_data) override final;

public:
	struct WindowPlatformData {
		HWND window;
		HINSTANCE instance;
	};

	RenderingContextDriverVulkanWindows();
	~RenderingContextDriverVulkanWindows() override;
};

#endif // VULKAN_ENABLED

// --- End platform/windows/rendering_context_driver_vulkan_windows.h ---

// --- Folded from platform/windows/tts_driver.h ---
class TTSDriver
{
public:
	mem_unique_ptr<Object> obj;
	virtual bool is_speaking() const = 0;
	virtual bool is_paused() const = 0;
	virtual Array get_voices() const = 0;

	virtual void speak(const String& p_text, const String& p_voice, int p_volume = 50,
		float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0,
		bool p_interrupt = false) = 0;
	virtual void pause() = 0;
	virtual void resume() = 0;
	virtual void stop() = 0;

	virtual void process_events() = 0;

	virtual bool init() = 0;

	virtual ~TTSDriver() {}
};



// --- End platform/windows/tts_driver.h ---

// --- Folded from platform/windows/tts_driver_onecore.h ---


VLTR_GCC_WARNING_PUSH
VLTR_GCC_WARNING_IGNORE("-Wnon-virtual-dtor")
VLTR_GCC_WARNING_IGNORE("-Wctor-dtor-privacy")
VLTR_GCC_WARNING_IGNORE("-Wshadow")
VLTR_GCC_WARNING_IGNORE("-Wstrict-aliasing")
GODOT_CLANG_WARNING_PUSH
GODOT_CLANG_WARNING_IGNORE("-Wnon-virtual-dtor")

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Media.Playback.h>
#include <winrt/Windows.Media.SpeechSynthesis.h>
#include <winrt/Windows.Storage.Streams.h>

VLTR_GCC_WARNING_POP
GODOT_CLANG_WARNING_POP

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Foundation::Metadata;
using namespace winrt::Windows::Media::Core;
using namespace winrt::Windows::Media::Playback;
using namespace winrt::Windows::Media::SpeechSynthesis;
using namespace winrt::Windows::Storage::Streams;

struct TTSUtterance;

class TTSDriverOneCore : public TTSDriver {
	List<TTSUtterance> queue;

	bool playing = false;
	bool paused = false;
	bool update_requested = false;

	int64_t id = -1;
	Char16String string;
	std::shared_ptr<MediaPlayer> media;
	struct TrackData {
		TimedMetadataTrack track;
		winrt::event_token token{};
	};
	Vector<TrackData> tracks;
	winrt::event_token token_s{};
	winrt::event_token token_f{};
	winrt::event_token token_e{};
	int64_t offset = 0;

	void _dispose_current(bool p_silent = false, bool p_canceled = false);

	void _speech_cancel(int p_msg_id);
	void _speech_end(int p_msg_id);
	void _speech_index_mark(int p_msg_id, int p_index_mark);

public:
	virtual bool is_speaking() const override;
	virtual bool is_paused() const override;
	virtual Array get_voices() const override;

	virtual void speak(const String &p_text, const String &p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false) override;
	virtual void pause() override;
	virtual void resume() override;
	virtual void stop() override;

	virtual void process_events() override;

	virtual bool init() override;

	TTSDriverOneCore();
	~TTSDriverOneCore();
};

// --- End platform/windows/tts_driver_onecore.h ---

// --- Folded from platform/windows/tts_driver_sapi.h ---


#include <windows.h>

#include <objbase.h>
#include <sapi.h>
#include <winnls.h>

#include <cwchar>

struct TTSUtterance;

class TTSDriverSAPI : public TTSDriver {
	List<TTSUtterance> queue;
	ISpVoice *synth = nullptr;
	bool paused = false;
	struct UTData {
		Char16String string;
		int offset;
		int64_t id;
	};
	HashMap<uint32_t, UTData> ids;
	bool update_requested = false;

	static void __stdcall speech_event_callback(WPARAM wParam, LPARAM lParam);

public:
	virtual bool is_speaking() const override;
	virtual bool is_paused() const override;
	virtual Array get_voices() const override;

	virtual void speak(const String &p_text, const String &p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false) override;
	virtual void pause() override;
	virtual void resume() override;
	virtual void stop() override;

	virtual void process_events() override;

	virtual bool init() override;

	TTSDriverSAPI();
	~TTSDriverSAPI();
};

// --- End platform/windows/tts_driver_sapi.h ---

// --- Folded from platform/windows/tts_windows.h ---
#include "core/string/ustring.h"

class TTSDriver;

struct TTSUtterance;

class TTS_Windows final
{
	struct Data
	{
		TTSDriver* driver = nullptr;
	};

	static inline Data* data = nullptr;

public:
	TTS_Windows() = delete;

	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static bool is_speaking();
	static bool is_paused();
	static Array get_voices();

	static void speak(const String& p_text, const String& p_voice, int p_volume = 50,
		float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0,
		bool p_interrupt = false);
	static void pause();
	static void resume();
	static void stop();

	static void process_events();
};

// --- End platform/windows/tts_windows.h ---

// --- Folded from platform/windows/wgl_detect_version.h ---
#if defined(WINDOWS_ENABLED) && defined(GLES3_ENABLED)

class Dictionary;

Dictionary detect_wgl();

#endif // WINDOWS_ENABLED && GLES3_ENABLED

// --- End platform/windows/wgl_detect_version.h ---
#endif // defined(WINDOWS_ENABLED)
// --- End platform/windows/display_server_windows.h ---
#endif // defined(_WIN32)
// --- End platform/windows/os_windows.h ---
