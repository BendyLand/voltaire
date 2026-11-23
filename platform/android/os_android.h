/**************************************************************************/
/*  os_android.h                                                          */
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

#ifdef ANDROID_ENABLED
// --- Folded from platform/android/audio_driver_opensl.h ---
#include "core/os/mutex.h"
#include "servers/audio/audio_server.h"

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

#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "drivers/unix/os_unix.h"

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
#include "drivers/unix/dir_access_unix.h"

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
#include "drivers/unix/os_unix.h"

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
#include "core/os/keyboard.h"

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
#include "core/input/input_event.h"

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
#include "core/os/thread.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/safe_refcount.h"

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
#include "core/templates/rb_map.h"
#include "platform/android/api/java_class_wrapper.h"

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
#include "core/math/rect2.h"

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

#include "core/math/color.h"
#include "core/templates/list.h"
#include "core/templates/vector.h"

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
