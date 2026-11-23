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
