/**************************************************************************/
/*  os_windows.h                                                          */
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

#ifdef _WIN32
#include "core/input/input_event.h"
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
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/audio/audio_server.h"

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
#include "core/templates/vector.h"

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
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/audio/audio_server.h"

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

#include "core/io/dir_access.h"

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
#include "core/templates/hash_map.h"

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
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "core/templates/a_hash_map.h"
#include "core/templates/rb_map.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

#ifdef XAUDIO2_ENABLED

#endif

#if defined(RD_ENABLED)
#include "servers/rendering/rendering_device.h"
#endif

#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
// --- Folded from platform/windows/gl_manager_windows_angle.h ---
#if defined(WINDOWS_ENABLED) && defined(GLES3_ENABLED)

#include "drivers/egl/egl_manager.h"
#include "servers/display/display_server_enums.h"

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

#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "servers/display/display_server_enums.h"

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
#include "core/os/keyboard.h"

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
#include "core/io/image.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
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
