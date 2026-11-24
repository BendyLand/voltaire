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

#ifdef WEB_ENABLED
#include <emscripten/html5.h>
// --- Folded from platform/web/audio_driver_web.h ---
#include "core/os/mutex.h"
#include "core/os/thread.h"
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
#include "core/input/input_event.h"
#include "core/os/os.h"
#include "drivers/unix/os_unix.h"

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


#include "core/input/input_event.h"
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
