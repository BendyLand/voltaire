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

#include "platform/web/os_web.h"
#include "platform/web/os_web.h"
// --- Inlined from platform/web/ip_web.h ---
#include "core/io/ip.h"

class IPWeb final
{
public:
	static void _resolve_hostname(
		List<IPAddress>& r_addresses, const String& p_hostname, IP::Type p_type = IP::TYPE_ANY)
	{
		(void)r_addresses;
		(void)p_hostname;
		(void)p_type;
	}

	static void get_local_interfaces(HashMap<String, IP::Interface_Info>* r_interfaces)
	{
		(void)r_interfaces;
	}

	static void make_default() { IP::bind<IPWeb>(); }

	IPWeb() = delete;
	IPWeb(const IPWeb&) = delete;
	IPWeb& operator=(const IPWeb&) = delete;
	~IPWeb() = delete;
};

// --- End platform/web/ip_web.h ---
// --- Inlined from platform/web/net_socket_web.h ---
#include "core/io/net_socket.h"

#include <sys/socket.h>

class NetSocketWeb : public NetSocket {

protected:
	static Ref<NetSocket> _create_func() {
		return memnew(NetSocketWeb);
	}

public:
	static void make_default() {
		_create = _create_func;
	}

	virtual Error open(Family p_family, Type p_sock_type, IP::Type &ip_type) override { return ERR_UNAVAILABLE; }
	virtual void close() override {}
	virtual Error bind(Address p_addr) override { return ERR_UNAVAILABLE; }
	virtual Error listen(int p_max_pending) override { return ERR_UNAVAILABLE; }
	virtual Error connect_to_host(Address p_addr) override { return ERR_UNAVAILABLE; }
	virtual Error poll(PollType p_type, int timeout) const override { return ERR_UNAVAILABLE; }
	virtual Error recv(uint8_t *p_buffer, int p_len, int &r_read) override { return ERR_UNAVAILABLE; }
	virtual Error recvfrom(uint8_t *p_buffer, int p_len, int &r_read, IPAddress &r_ip, uint16_t &r_port, bool p_peek = false) override { return ERR_UNAVAILABLE; }
	virtual Error send(const uint8_t *p_buffer, int p_len, int &r_sent) override { return ERR_UNAVAILABLE; }
	virtual Error sendto(const uint8_t *p_buffer, int p_len, int &r_sent, IPAddress p_ip, uint16_t p_port) override { return ERR_UNAVAILABLE; }
	virtual Ref<NetSocket> accept(Address &r_addr) override { return Ref<NetSocket>(); }

	virtual bool is_open() const override { return false; }
	virtual int get_available_bytes() const override { return -1; }
	virtual Error get_socket_address(Address *r_addr) const override { return ERR_UNAVAILABLE; }

	virtual Error set_broadcasting_enabled(bool p_enabled) override { return ERR_UNAVAILABLE; }
	virtual void set_blocking_enabled(bool p_enabled) override {}
	virtual void set_ipv6_only_enabled(bool p_enabled) override {}
	virtual void set_tcp_no_delay_enabled(bool p_enabled) override {}
	virtual void set_reuse_address_enabled(bool p_enabled) override {}
	virtual Error join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) override { return ERR_UNAVAILABLE; }
	virtual Error leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) override { return ERR_UNAVAILABLE; }
};

// --- End platform/web/net_socket_web.h ---

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


// --- Merged from platform/web/audio_driver_web.cpp ---
#include "platform/web/os_web.h"
// --- Inlined from platform/web/godot_audio.h ---
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

extern int godot_audio_is_available();
extern int godot_audio_has_worklet();
extern int godot_audio_has_script_processor();
extern int godot_audio_init(int *p_mix_rate, int p_latency, void (*_state_cb)(int), void (*_latency_cb)(float));
extern void godot_audio_resume();

extern int godot_audio_input_start();
extern void godot_audio_input_stop();

// Samples
extern int godot_audio_sample_stream_is_registered(const char *p_stream_object_id);
extern void godot_audio_sample_register_stream(const char *p_stream_object_id, float *p_frames_buf, int p_frames_total, const char *p_loop_mode, int p_loop_begin, int p_loop_end);
extern void godot_audio_sample_unregister_stream(const char *p_stream_object_id);
extern void godot_audio_sample_start(const char *p_playback_object_id, const char *p_stream_object_id, int p_bus_index, float p_offset, float p_pitch_scale, float *p_volume_ptr);
extern void godot_audio_sample_stop(const char *p_playback_object_id);
extern void godot_audio_sample_set_pause(const char *p_playback_object_id, bool p_pause);
extern int godot_audio_sample_is_active(const char *p_playback_object_id);
extern double godot_audio_get_sample_playback_position(const char *p_playback_object_id);
extern void godot_audio_sample_update_pitch_scale(const char *p_playback_object_id, float p_pitch_scale);
extern void godot_audio_sample_set_volumes_linear(const char *p_playback_object_id, int *p_buses_buf, int p_buses_size, float *p_volumes_buf, int p_volumes_size);
extern void godot_audio_sample_set_finished_callback(void (*p_callback)(const char *));

extern void godot_audio_sample_bus_set_count(int p_count);
extern void godot_audio_sample_bus_remove(int p_index);
extern void godot_audio_sample_bus_add(int p_at_pos = -1);
extern void godot_audio_sample_bus_move(int p_bus, int p_to_pos);
extern void godot_audio_sample_bus_set_send(int p_bus, int p_send_index);
extern void godot_audio_sample_bus_set_volume_db(int p_bus, float p_volume_db);
extern void godot_audio_sample_bus_set_solo(int p_bus, bool p_enable);
extern void godot_audio_sample_bus_set_mute(int p_bus, bool p_enable);

// Worklet
typedef int32_t GodotAudioState[4];
extern int godot_audio_worklet_create(int p_channels);
extern void godot_audio_worklet_start(float *p_in_buf, int p_in_size, float *p_out_buf, int p_out_size, GodotAudioState p_state);
extern void godot_audio_worklet_start_no_threads(float *p_out_buf, int p_out_size, void (*p_out_cb)(int p_pos, int p_frames), float *p_in_buf, int p_in_size, void (*p_in_cb)(int p_pos, int p_frames));
extern int godot_audio_worklet_state_add(GodotAudioState p_state, int p_idx, int p_value);
extern int godot_audio_worklet_state_get(GodotAudioState p_state, int p_idx);
extern int godot_audio_worklet_state_wait(int32_t *p_state, int p_idx, int32_t p_expected, int p_timeout);

// Script
extern int godot_audio_script_create(int *p_buffer_size, int p_channels);
extern void godot_audio_script_start(float *p_in_buf, int p_in_size, float *p_out_buf, int p_out_size, void (*p_cb)());

#ifdef __cplusplus
}
#endif

// --- End platform/web/godot_audio.h ---

#include "core/config/engine.h"
#include "core/math/math_funcs_binary.h"
#include "servers/audio/audio_stream.h"

#include <emscripten.h>

AudioDriverWeb::AudioContext AudioDriverWeb::audio_context;

bool AudioDriverWeb::is_available() {
	return godot_audio_is_available() != 0;
}

void AudioDriverWeb::_state_change_callback(int p_state) {
	AudioDriverWeb::audio_context.state = p_state;
}

void AudioDriverWeb::_latency_update_callback(float p_latency) {
	AudioDriverWeb::audio_context.output_latency = p_latency;
}

void AudioDriverWeb::_sample_playback_finished_callback(const char *p_playback_object_id) {
	const ObjectID playback_id = ObjectID(String::to_int(p_playback_object_id));

	Object *playback_object = ObjectDB::get_instance(playback_id);
	if (playback_object == nullptr) {
		return;
	}
	Ref<AudioSamplePlayback> playback = Object::cast_to<AudioSamplePlayback>(playback_object);
	if (playback.is_null()) {
		return;
	}

	AudioServer::stop_sample_playback(playback);
}

void AudioDriverWeb::_audio_driver_process(int p_from, int p_samples) {
	int32_t *stream_buffer = reinterpret_cast<int32_t *>(output_rb);
	const int max_samples = memarr_len(output_rb);

	int write_pos = p_from;
	int to_write = p_samples;
	if (to_write == 0) {
		to_write = max_samples;
	}
	// High part
	if (write_pos + to_write > max_samples) {
		const int samples_high = max_samples - write_pos;
		audio_server_process(samples_high / channel_count, &stream_buffer[write_pos]);
		for (int i = write_pos; i < max_samples; i++) {
			output_rb[i] = float(stream_buffer[i] >> 16) / 32768.f;
		}
		to_write -= samples_high;
		write_pos = 0;
	}
	// Leftover
	audio_server_process(to_write / channel_count, &stream_buffer[write_pos]);
	for (int i = write_pos; i < write_pos + to_write; i++) {
		output_rb[i] = float(stream_buffer[i] >> 16) / 32768.f;
	}
}

void AudioDriverWeb::_audio_driver_capture(int p_from, int p_samples) {
	if (get_input_buffer().is_empty()) {
		return; // Input capture stopped.
	}
	const int max_samples = memarr_len(input_rb);

	int read_pos = p_from;
	int to_read = p_samples;
	if (to_read == 0) {
		to_read = max_samples;
	}
	// High part
	if (read_pos + to_read > max_samples) {
		const int samples_high = max_samples - read_pos;
		for (int i = read_pos; i < max_samples; i++) {
			input_buffer_write(int32_t(input_rb[i] * 32768.f) * (1U << 16));
		}
		to_read -= samples_high;
		read_pos = 0;
	}
	// Leftover
	for (int i = read_pos; i < read_pos + to_read; i++) {
		input_buffer_write(int32_t(input_rb[i] * 32768.f) * (1U << 16));
	}
}

Error AudioDriverWeb::init() {
	int latency = Engine::get_audio_output_latency();
	if (!audio_context.inited) {
		audio_context.mix_rate = _get_configured_mix_rate();
		audio_context.channel_count = godot_audio_init(&audio_context.mix_rate, latency, &_state_change_callback, &_latency_update_callback);
		audio_context.inited = true;
	}
	mix_rate = audio_context.mix_rate;
	channel_count = audio_context.channel_count;
	buffer_length = Math::closest_power_of_2(uint32_t(latency * mix_rate / 1000));
	Error err = create(buffer_length, channel_count);
	if (err != OK) {
		return err;
	}
	if (output_rb) {
		memdelete_arr(output_rb);
	}
	const size_t array_size = buffer_length * (size_t)channel_count;
	output_rb = memnew_arr(float, array_size);
	if (!output_rb) {
		return ERR_OUT_OF_MEMORY;
	}
	if (input_rb) {
		memdelete_arr(input_rb);
	}
	input_rb = memnew_arr(float, array_size);
	if (!input_rb) {
		return ERR_OUT_OF_MEMORY;
	}

	godot_audio_sample_set_finished_callback(&_sample_playback_finished_callback);

	return OK;
}

void AudioDriverWeb::start() {
	start(output_rb, memarr_len(output_rb), input_rb, memarr_len(input_rb));
}

void AudioDriverWeb::resume() {
	if (audio_context.state == 0) { // 'suspended'
		godot_audio_resume();
	}
}

float AudioDriverWeb::get_latency() {
	return audio_context.output_latency + (float(buffer_length) / mix_rate);
}

int AudioDriverWeb::get_mix_rate() const {
	return mix_rate;
}

AudioDriver::SpeakerMode AudioDriverWeb::get_speaker_mode() const {
	return get_speaker_mode_by_total_channels(channel_count);
}

void AudioDriverWeb::finish() {
	finish_driver();
	if (output_rb) {
		memdelete_arr(output_rb);
		output_rb = nullptr;
	}
	if (input_rb) {
		memdelete_arr(input_rb);
		input_rb = nullptr;
	}
}

Error AudioDriverWeb::input_start() {
	lock();
	input_buffer_init(buffer_length);
	unlock();
	if (godot_audio_input_start()) {
		return FAILED;
	}
	return OK;
}

Error AudioDriverWeb::input_stop() {
	godot_audio_input_stop();
	lock();
	input_buffer.clear();
	unlock();
	return OK;
}

bool AudioDriverWeb::is_stream_registered_as_sample(const Ref<AudioStream> &p_stream) const {
	ERR_FAIL_COND_V_MSG(p_stream.is_null(), false, "Parameter p_stream is null.");
	return godot_audio_sample_stream_is_registered(itos(p_stream->get_instance_id()).utf8().get_data()) != 0;
}

void AudioDriverWeb::register_sample(const Ref<AudioSample> &p_sample) {
	ERR_FAIL_COND_MSG(p_sample.is_null(), "Parameter p_sample is null.");
	ERR_FAIL_COND_MSG(p_sample->stream.is_null(), "Parameter p_sample->stream is null.");

	String loop_mode;
	switch (p_sample->loop_mode) {
		case AudioSample::LoopMode::LOOP_DISABLED: {
			loop_mode = "disabled";
		} break;

		case AudioSample::LoopMode::LOOP_FORWARD: {
			loop_mode = "forward";
		} break;

		case AudioSample::LoopMode::LOOP_PINGPONG: {
			loop_mode = "pingpong";
		} break;

		case AudioSample::LoopMode::LOOP_BACKWARD: {
			loop_mode = "backward";
		} break;
	}

	double length = p_sample->stream->get_length();

	Vector<AudioFrame> frames;
	int frames_total = mix_rate * length;
	{
		Ref<AudioStreamPlayback> stream_playback = p_sample->stream->instantiate_playback();
		frames.resize(frames_total);
		AudioFrame *frames_ptr = frames.ptrw();
		stream_playback->start();
		stream_playback->mix(frames_ptr, 1.0f, frames_total);
	}

	PackedFloat32Array data;
	data.resize(frames_total * 2);
	float *data_ptrw = data.ptrw();
	for (int i = 0; i < frames_total; i++) {
		data_ptrw[i] = frames[i].left;
		data_ptrw[i + frames_total] = frames[i].right;
	}

	godot_audio_sample_register_stream(
			itos(p_sample->stream->get_instance_id()).utf8().get_data(),
			data_ptrw,
			frames_total,
			loop_mode.utf8().get_data(),
			p_sample->loop_begin,
			p_sample->loop_end);
}

void AudioDriverWeb::unregister_sample(const Ref<AudioSample> &p_sample) {
	ERR_FAIL_COND_MSG(p_sample.is_null(), "Parameter p_sample is null.");
	ERR_FAIL_COND_MSG(p_sample->stream.is_null(), "Parameter p_sample->stream is null.");

	godot_audio_sample_unregister_stream(itos(p_sample->stream->get_instance_id()).utf8().get_data());
}

void AudioDriverWeb::start_sample_playback(const Ref<AudioSamplePlayback> &p_playback) {
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	ERR_FAIL_COND_MSG(p_playback->stream.is_null(), "Parameter p_playback->stream is null.");

	constexpr int real_max_channels = AudioServer::MAX_CHANNELS_PER_BUS * 2;
	PackedFloat32Array volume;
	volume.resize(real_max_channels);
	float *volume_ptrw = volume.ptrw();
	for (int i = 0; i < real_max_channels; i += 2) {
		if (p_playback->volume_vector.is_empty()) {
			volume_ptrw[i] = 0;
			volume_ptrw[i + 1] = 0;
		} else {
			const AudioFrame &frame = p_playback->volume_vector[i / 2];
			volume_ptrw[i] = frame.left;
			volume_ptrw[i + 1] = frame.right;
		}
	}
	godot_audio_sample_start(
			itos(p_playback->get_instance_id()).utf8().get_data(),
			itos(p_playback->stream->get_instance_id()).utf8().get_data(),
			AudioServer::get_bus_index(p_playback->bus),
			p_playback->offset,
			p_playback->pitch_scale,
			volume_ptrw);
}

void AudioDriverWeb::stop_sample_playback(const Ref<AudioSamplePlayback> &p_playback) {
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	godot_audio_sample_stop(itos(p_playback->get_instance_id()).utf8().get_data());
}

void AudioDriverWeb::set_sample_playback_pause(const Ref<AudioSamplePlayback> &p_playback, bool p_paused) {
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	godot_audio_sample_set_pause(itos(p_playback->get_instance_id()).utf8().get_data(), p_paused);
}

bool AudioDriverWeb::is_sample_playback_active(const Ref<AudioSamplePlayback> &p_playback) {
	ERR_FAIL_COND_V_MSG(p_playback.is_null(), false, "Parameter p_playback is null.");
	return godot_audio_sample_is_active(itos(p_playback->get_instance_id()).utf8().get_data()) != 0;
}

double AudioDriverWeb::get_sample_playback_position(const Ref<AudioSamplePlayback> &p_playback) {
	ERR_FAIL_COND_V_MSG(p_playback.is_null(), false, "Parameter p_playback is null.");
	return godot_audio_get_sample_playback_position(itos(p_playback->get_instance_id()).utf8().get_data());
}

void AudioDriverWeb::update_sample_playback_pitch_scale(const Ref<AudioSamplePlayback> &p_playback, float p_pitch_scale) {
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	godot_audio_sample_update_pitch_scale(
			itos(p_playback->get_instance_id()).utf8().get_data(),
			p_pitch_scale);
}

void AudioDriverWeb::set_sample_playback_bus_volumes_linear(const Ref<AudioSamplePlayback> &p_playback, const HashMap<String, Vector<AudioFrame>> &p_bus_volumes) {
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");

	constexpr int real_max_channels = AudioServer::MAX_CHANNELS_PER_BUS * 2;

	PackedInt32Array buses;
	buses.resize(p_bus_volumes.size());
	int32_t *buses_ptrw = buses.ptrw();
	PackedFloat32Array values;
	values.resize(p_bus_volumes.size() * AudioServer::MAX_CHANNELS_PER_BUS * 2);
	float *values_ptrw = values.ptrw();
	int idx = 0;
	for (KeyValue<String, Vector<AudioFrame>> pair : p_bus_volumes) {
		int bus_index = AudioServer::get_bus_index(pair.key);
		buses_ptrw[idx] = bus_index;
		ERR_FAIL_COND(pair.value.size() != AudioServer::MAX_CHANNELS_PER_BUS);
		for (int i = 0; i < real_max_channels; i += 2) {
			const AudioFrame &frame = pair.value[i / 2];
			values_ptrw[(idx * real_max_channels) + i] = frame.left;
			values_ptrw[(idx * real_max_channels) + i + 1] = frame.right;
		}
		idx++;
	}
	godot_audio_sample_set_volumes_linear(
			itos(p_playback->get_instance_id()).utf8().get_data(),
			buses_ptrw,
			buses.size(),
			values_ptrw,
			values.size());
}

void AudioDriverWeb::set_sample_bus_count(int p_count) {
	godot_audio_sample_bus_set_count(p_count);
}

void AudioDriverWeb::remove_sample_bus(int p_index) {
	godot_audio_sample_bus_remove(p_index);
}

void AudioDriverWeb::add_sample_bus(int p_at_pos) {
	godot_audio_sample_bus_add(p_at_pos);
}

void AudioDriverWeb::move_sample_bus(int p_bus, int p_to_pos) {
	godot_audio_sample_bus_move(p_bus, p_to_pos);
}

void AudioDriverWeb::set_sample_bus_send(int p_bus, const String &p_send) {
	godot_audio_sample_bus_set_send(p_bus, AudioServer::get_bus_index(p_send));
}

void AudioDriverWeb::set_sample_bus_volume_db(int p_bus, float p_volume_db) {
	godot_audio_sample_bus_set_volume_db(p_bus, p_volume_db);
}

void AudioDriverWeb::set_sample_bus_solo(int p_bus, bool p_enable) {
	godot_audio_sample_bus_set_solo(p_bus, p_enable);
}

void AudioDriverWeb::set_sample_bus_mute(int p_bus, bool p_enable) {
	godot_audio_sample_bus_set_mute(p_bus, p_enable);
}

#ifdef THREADS_ENABLED

/// AudioWorkletNode implementation (threads)
void AudioDriverWorklet::_audio_thread_func(void *p_data) {
	AudioDriverWorklet *driver = static_cast<AudioDriverWorklet *>(p_data);
	const int out_samples = memarr_len(driver->get_output_rb());
	const int in_samples = memarr_len(driver->get_input_rb());
	int wpos = 0;
	int to_write = out_samples;
	int rpos = 0;
	int to_read = 0;
	int32_t step = 0;
	while (!driver->quit) {
		if (to_read) {
			driver->lock();
			driver->_audio_driver_capture(rpos, to_read);
			godot_audio_worklet_state_add(driver->state, STATE_SAMPLES_IN, -to_read);
			driver->unlock();
			rpos += to_read;
			if (rpos >= in_samples) {
				rpos -= in_samples;
			}
		}
		if (to_write) {
			driver->lock();
			driver->_audio_driver_process(wpos, to_write);
			godot_audio_worklet_state_add(driver->state, STATE_SAMPLES_OUT, to_write);
			driver->unlock();
			wpos += to_write;
			if (wpos >= out_samples) {
				wpos -= out_samples;
			}
		}
		step = godot_audio_worklet_state_wait(driver->state, STATE_PROCESS, step, 1);
		to_write = out_samples - godot_audio_worklet_state_get(driver->state, STATE_SAMPLES_OUT);
		to_read = godot_audio_worklet_state_get(driver->state, STATE_SAMPLES_IN);
	}
}

Error AudioDriverWorklet::create(int &p_buffer_size, int p_channels) {
	if (!godot_audio_has_worklet()) {
		return ERR_UNAVAILABLE;
	}
	return (Error)godot_audio_worklet_create(p_channels);
}

void AudioDriverWorklet::start(float *p_out_buf, int p_out_buf_size, float *p_in_buf, int p_in_buf_size) {
	godot_audio_worklet_start(p_in_buf, p_in_buf_size, p_out_buf, p_out_buf_size, state);
	thread.start(_audio_thread_func, this);
}

void AudioDriverWorklet::lock() {
	mutex.lock();
}

void AudioDriverWorklet::unlock() {
	mutex.unlock();
}

void AudioDriverWorklet::finish_driver() {
	quit = true; // Ask thread to quit.
	thread.wait_to_finish();
}

#else // No threads.

/// AudioWorkletNode implementation (no threads)
Error AudioDriverWorklet::create(int &p_buffer_size, int p_channels) {
	if (!godot_audio_has_worklet()) {
		return ERR_UNAVAILABLE;
	}
	return (Error)godot_audio_worklet_create(p_channels);
}

void AudioDriverWorklet::start(float *p_out_buf, int p_out_buf_size, float *p_in_buf, int p_in_buf_size) {
	_audio_driver_process();
	godot_audio_worklet_start_no_threads(p_out_buf, p_out_buf_size, &_process_callback, p_in_buf, p_in_buf_size, &_capture_callback);
}

void AudioDriverWorklet::_process_callback(int p_pos, int p_samples) {
	AudioDriverWorklet *driver = static_cast<AudioDriverWorklet *>(AudioDriverManager::get_active_driver());
	driver->_audio_driver_process(p_pos, p_samples);
}

void AudioDriverWorklet::_capture_callback(int p_pos, int p_samples) {
	AudioDriverWorklet *driver = static_cast<AudioDriverWorklet *>(AudioDriverManager::get_active_driver());
	driver->_audio_driver_capture(p_pos, p_samples);
}

#endif // THREADS_ENABLED

/// ScriptProcessorNode implementation
void AudioDriverScriptProcessor::_process_callback() {
	AudioDriverScriptProcessor *driver = static_cast<AudioDriverScriptProcessor *>(AudioDriverManager::get_active_driver());
	driver->_audio_driver_capture();
	driver->_audio_driver_process();
}

Error AudioDriverScriptProcessor::create(int &p_buffer_samples, int p_channels) {
	if (!godot_audio_has_script_processor()) {
		return ERR_UNAVAILABLE;
	}
	return (Error)godot_audio_script_create(&p_buffer_samples, p_channels);
}

void AudioDriverScriptProcessor::start(float *p_out_buf, int p_out_buf_size, float *p_in_buf, int p_in_buf_size) {
	godot_audio_script_start(p_in_buf, p_in_buf_size, p_out_buf, p_out_buf_size, &_process_callback);
}


// --- Merged from platform/web/webmidi_driver.cpp ---
#include "platform/web/os_web.h"
// --- Inlined from platform/web/godot_midi.h ---
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

extern int godot_js_webmidi_open_midi_inputs(
		void (*p_callback)(int p_size, const char **p_connected_input_names),
		void (*p_on_midi_message)(int p_device_index, int p_status, const uint8_t *p_data, int p_data_len),
		const uint8_t *p_data_buffer,
		const int p_data_buffer_len);

extern void godot_js_webmidi_close_midi_inputs();

#ifdef __cplusplus
}
#endif

// --- End platform/web/godot_midi.h ---

#ifdef PROXY_TO_PTHREAD_ENABLED
#endif

void MIDIDriverWebMidi::initialize() {}

void MIDIDriverWebMidi::finalize()
{
	close();
}

Error MIDIDriverWebMidi::open()
{
	Error error = (Error)godot_js_webmidi_open_midi_inputs(
		&MIDIDriverWebMidi::set_input_names_callback, &MIDIDriverWebMidi::on_midi_message,
		_event_buffer, MIDIDriverWebMidi::MAX_EVENT_BUFFER_LENGTH);
	if (error == ERR_UNAVAILABLE) {
		ERR_PRINT("Web MIDI is not supported on this browser.");
	}
	return error;
}

void MIDIDriverWebMidi::close()
{
	MIDIDriver::clear_connected_inputs();
	godot_js_webmidi_close_midi_inputs();
}

void MIDIDriverWebMidi::set_input_names_callback(int p_size, const char** p_input_names)
{
	Vector<String> input_names;
	for (int i = 0; i < p_size; i++) {
		input_names.append(String::utf8(p_input_names[i]));
	}
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(MIDIDriverWebMidi::_set_input_names_callback).call_deferred(input_names);
		return;
	}
#endif

	_set_input_names_callback(input_names);
}

void MIDIDriverWebMidi::_set_input_names_callback(const Vector<String>& p_input_names)
{
	MIDIDriver::clear_connected_inputs();
	for (const String& input_name : p_input_names) {
		MIDIDriver::add_connected_input(input_name);
	}
}

void MIDIDriverWebMidi::on_midi_message(int p_device_index, int p_status, const uint8_t *p_data, int p_data_len) {
	PackedByteArray data;
	data.resize(p_data_len);
	uint8_t *data_ptr = data.ptrw();
	for (int i = 0; i < p_data_len; i++) {
		data_ptr[i] = p_data[i];
	}
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(MIDIDriverWebMidi::_on_midi_message).call_deferred(p_device_index, p_status, data, p_data_len);
		return;
	}
#endif
	_on_midi_message(p_device_index, p_status, data, p_data_len);
}

void MIDIDriverWebMidi::_on_midi_message(int p_device_index, int p_status, const PackedByteArray &p_data, int p_data_len) {
	MIDIDriver::send_event(p_device_index, p_status, p_data.ptr(), p_data_len);
}


// --- Merged from platform/web/http_client_web.cpp ---
// --- Inlined/Folded from platform/web/http_client_web.h ---
#include "core/io/stream_peer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	GODOT_JS_FETCH_STATE_REQUESTING = 0,
	GODOT_JS_FETCH_STATE_BODY = 1,
	GODOT_JS_FETCH_STATE_DONE = 2,
	GODOT_JS_FETCH_STATE_ERROR = -1,
} godot_js_fetch_state_t;

extern int godot_js_fetch_create(const char *p_method, const char *p_url, const char **p_headers, int p_headers_len, const uint8_t *p_body, int p_body_len);
extern int godot_js_fetch_read_headers(int p_id, void (*parse_callback)(int p_size, const char **p_headers, void *p_ref), void *p_ref);
extern int godot_js_fetch_read_chunk(int p_id, uint8_t *p_buf, int p_buf_size);
extern void godot_js_fetch_free(int p_id);
extern godot_js_fetch_state_t godot_js_fetch_state_get(int p_id);
extern int godot_js_fetch_http_status_get(int p_id);
extern int godot_js_fetch_is_chunked(int p_id);

#ifdef __cplusplus
}
#endif

class HTTPClientWeb : public HTTPClient {

private:
	int js_id = 0;
	Status status = STATUS_DISCONNECTED;

	// 64 KiB by default (favors fast download speeds at the cost of memory usage).
	int read_limit = 65536;

	String host;
	int port = -1;
	bool use_tls = false;

	int polled_response_code = 0;
	Vector<String> response_headers;
	Vector<uint8_t> response_buffer;

#ifdef DEBUG_ENABLED
	uint64_t last_polling_frame = 0;
#endif

	static void _parse_headers(int p_len, const char **p_headers, void *p_ref);

public:
	static HTTPClient *_create_func(bool p_notify_postinitialize);

	Error request(Method p_method, const String &p_url, const Vector<String> &p_headers, const uint8_t *p_body, int p_body_size) override;

	Error connect_to_host(const String &p_host, int p_port = -1, Ref<TLSOptions> p_tls_options = Ref<TLSOptions>()) override;
	void set_connection(const Ref<StreamPeer> &p_connection) override;
	Ref<StreamPeer> get_connection() const override;
	void close() override;
	Status get_status() const override;
	bool has_response() const override;
	bool is_response_chunked() const override;
	int get_response_code() const override;
	Error get_response_headers(List<String> *r_response) override;
	int64_t get_response_body_length() const override;
	PackedByteArray read_response_body_chunk() override;
	void set_blocking_mode(bool p_enable) override;
	bool is_blocking_mode_enabled() const override;
	void set_read_chunk_size(int p_size) override;
	int get_read_chunk_size() const override;
	Error poll() override;
	HTTPClientWeb();
	~HTTPClientWeb();
};


#ifdef DEBUG_ENABLED
#include "core/config/engine.h"
#endif

void HTTPClientWeb::_parse_headers(int p_len, const char **p_headers, void *p_ref) {
	HTTPClientWeb *client = static_cast<HTTPClientWeb *>(p_ref);
	for (int i = 0; i < p_len; i++) {
		client->response_headers.push_back(String::utf8(p_headers[i]));
	}
}

Error HTTPClientWeb::connect_to_host(const String &p_host, int p_port, Ref<TLSOptions> p_tls_options) {
	ERR_FAIL_COND_V(p_tls_options.is_valid() && p_tls_options->is_server(), ERR_INVALID_PARAMETER);

	close();

	port = p_port;
	use_tls = p_tls_options.is_valid();

	host = p_host;

	String host_lower = host.to_lower();
	if (host_lower.begins_with("http://")) {
		host = host.substr(7);
		use_tls = false;
	} else if (host_lower.begins_with("https://")) {
		use_tls = true;
		host = host.substr(8);
	}

	ERR_FAIL_COND_V(host.length() < HOST_MIN_LEN, ERR_INVALID_PARAMETER);

	if (port < 0) {
		if (use_tls) {
			port = PORT_HTTPS;
		} else {
			port = PORT_HTTP;
		}
	}

	status = host.is_valid_ip_address() ? STATUS_CONNECTING : STATUS_RESOLVING;

	return OK;
}

void HTTPClientWeb::set_connection(const Ref<StreamPeer> &p_connection) {
	ERR_FAIL_MSG("Accessing an HTTPClientWeb's StreamPeer is not supported for the Web platform.");
}

Ref<StreamPeer> HTTPClientWeb::get_connection() const {
	ERR_FAIL_V_MSG(Ref<RefCounted>(), "Accessing an HTTPClientWeb's StreamPeer is not supported for the Web platform.");
}

Error HTTPClientWeb::request(Method p_method, const String &p_url, const Vector<String> &p_headers, const uint8_t *p_body, int p_body_len) {
	ERR_FAIL_INDEX_V(p_method, METHOD_MAX, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_method == METHOD_TRACE || p_method == METHOD_CONNECT, ERR_UNAVAILABLE, "HTTP methods TRACE and CONNECT are not supported for the Web platform.");
	ERR_FAIL_COND_V(status != STATUS_CONNECTED, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(host.is_empty(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(port < 0, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!p_url.begins_with("/"), ERR_INVALID_PARAMETER);

	Error err = verify_headers(p_headers);
	if (err) {
		return err;
	}

	String url = (use_tls ? "https://" : "http://") + host + ":" + itos(port) + p_url;
	Vector<CharString> keeper;
	Vector<const char *> c_strings;
	for (int i = 0; i < p_headers.size(); i++) {
		keeper.push_back(p_headers[i].utf8());
		c_strings.push_back(keeper[i].get_data());
	}
	if (js_id) {
		godot_js_fetch_free(js_id);
	}
	js_id = godot_js_fetch_create(_methods[p_method], url.utf8().get_data(), c_strings.ptrw(), c_strings.size(), p_body, p_body_len);
	status = STATUS_REQUESTING;
	return OK;
}

void HTTPClientWeb::close() {
	host = "";
	port = -1;
	use_tls = false;
	status = STATUS_DISCONNECTED;
	polled_response_code = 0;
	response_headers.resize(0);
	response_buffer.resize(0);
	if (js_id) {
		godot_js_fetch_free(js_id);
		js_id = 0;
	}
}

HTTPClientWeb::Status HTTPClientWeb::get_status() const {
	return status;
}

bool HTTPClientWeb::has_response() const {
	return response_headers.size() > 0;
}

bool HTTPClientWeb::is_response_chunked() const {
	return godot_js_fetch_is_chunked(js_id);
}

int HTTPClientWeb::get_response_code() const {
	return polled_response_code;
}

Error HTTPClientWeb::get_response_headers(List<String> *r_response) {
	if (!response_headers.size()) {
		return ERR_INVALID_PARAMETER;
	}
	for (int i = 0; i < response_headers.size(); i++) {
		r_response->push_back(response_headers[i]);
	}
	response_headers.clear();
	return OK;
}

int64_t HTTPClientWeb::get_response_body_length() const {
	// Body length cannot be consistently retrieved from the web.
	// Reading the "content-length" value will return a meaningless value when the response is compressed,
	// as reading will return uncompressed chunks in any case, resulting in a mismatch between the detected
	// body size and the actual size returned by repeatedly calling read_response_body_chunk.
	// Additionally, while "content-length" is considered a safe CORS header, "content-encoding" is not,
	// so using the "content-encoding" to decide if "content-length" is meaningful is not an option either.
	// We simply must accept the fact that browsers are awful when it comes to networking APIs.
	// See GH-47597, and GH-79327.
	return -1;
}

PackedByteArray HTTPClientWeb::read_response_body_chunk() {
	ERR_FAIL_COND_V(status != STATUS_BODY, PackedByteArray());

	if (response_buffer.size() != read_limit) {
		response_buffer.resize(read_limit);
	}
	int read = godot_js_fetch_read_chunk(js_id, response_buffer.ptrw(), read_limit);

	// Check if the stream is over.
	godot_js_fetch_state_t state = godot_js_fetch_state_get(js_id);
	if (state == GODOT_JS_FETCH_STATE_DONE) {
		status = STATUS_DISCONNECTED;
	} else if (state != GODOT_JS_FETCH_STATE_BODY) {
		status = STATUS_CONNECTION_ERROR;
	}

	PackedByteArray chunk;
	if (!read) {
		return chunk;
	}
	chunk.resize(read);
	memcpy(chunk.ptrw(), response_buffer.ptr(), read);
	return chunk;
}

void HTTPClientWeb::set_blocking_mode(bool p_enable) {
	ERR_FAIL_COND_MSG(p_enable, "HTTPClientWeb blocking mode is not supported for the Web platform.");
}

bool HTTPClientWeb::is_blocking_mode_enabled() const {
	return false;
}

void HTTPClientWeb::set_read_chunk_size(int p_size) {
	read_limit = p_size;
}

int HTTPClientWeb::get_read_chunk_size() const {
	return read_limit;
}

Error HTTPClientWeb::poll() {
	switch (status) {
		case STATUS_DISCONNECTED:
			return ERR_UNCONFIGURED;

		case STATUS_RESOLVING:
			status = STATUS_CONNECTING;
			return OK;

		case STATUS_CONNECTING:
			status = STATUS_CONNECTED;
			return OK;

		case STATUS_CONNECTED:
			return OK;

		case STATUS_BODY: {
			godot_js_fetch_state_t state = godot_js_fetch_state_get(js_id);
			if (state == GODOT_JS_FETCH_STATE_DONE) {
				status = STATUS_DISCONNECTED;
			} else if (state != GODOT_JS_FETCH_STATE_BODY) {
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}
			return OK;
		}

		case STATUS_CONNECTION_ERROR:
			return ERR_CONNECTION_ERROR;

		case STATUS_REQUESTING: {
#ifdef DEBUG_ENABLED
			// forcing synchronous requests is not possible on the web
			if (last_polling_frame == Engine::get_process_frames()) {
				WARN_PRINT("HTTPClientWeb polled multiple times in one frame, "
						   "but request cannot progress more than once per "
						   "frame on the Web platform.");
			}
			last_polling_frame = Engine::get_process_frames();
#endif

			polled_response_code = godot_js_fetch_http_status_get(js_id);
			godot_js_fetch_state_t js_state = godot_js_fetch_state_get(js_id);
			if (js_state == GODOT_JS_FETCH_STATE_REQUESTING) {
				return OK;
			} else if (js_state == GODOT_JS_FETCH_STATE_ERROR) {
				// Fetch is in error state.
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}
			if (godot_js_fetch_read_headers(js_id, &_parse_headers, this)) {
				// Failed to parse headers.
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}
			status = STATUS_BODY;
			break;
		}

		default:
			ERR_FAIL_V(ERR_BUG);
	}
	return OK;
}

HTTPClient *HTTPClientWeb::_create_func(bool p_notify_postinitialize) {
	return static_cast<HTTPClient *>(ClassDB::creator<HTTPClientWeb>(p_notify_postinitialize));
}

HTTPClient *(*HTTPClient::_create)(bool p_notify_postinitialize) = HTTPClientWeb::_create_func;

HTTPClientWeb::HTTPClientWeb() {
}

HTTPClientWeb::~HTTPClientWeb() {
	close();
}


// --- Merged from platform/web/javascript_bridge_singleton.cpp ---
#include "platform/web/os_web.h"

#include "platform/web/os_web.h"

#include <emscripten.h>

#ifdef PROXY_TO_PTHREAD_ENABLED
#endif

extern "C" {
extern void godot_js_os_download_buffer(const uint8_t *p_buf, int p_buf_size, const char *p_name, const char *p_mime);
}

#ifdef JAVASCRIPT_EVAL_ENABLED

extern "C" {
typedef union {
	int64_t i;
	double r;
	void *p;
} godot_js_wrapper_ex;

typedef int (*GodotJSWrapperVariant2JSCallback)(const void **p_args, int p_pos, godot_js_wrapper_ex *r_val, void **p_lock);
typedef void (*GodotJSWrapperFreeLockCallback)(void **p_lock, int p_type);
extern int godot_js_wrapper_interface_get(const char *p_name);
extern int godot_js_wrapper_object_call(int p_id, const char *p_method, void **p_args, int p_argc, GodotJSWrapperVariant2JSCallback p_variant2js_callback, godot_js_wrapper_ex *p_cb_rval, void **p_lock, GodotJSWrapperFreeLockCallback p_lock_callback);
extern int godot_js_wrapper_object_get(int p_id, godot_js_wrapper_ex *p_val, const char *p_prop);
extern int godot_js_wrapper_object_getvar(int p_id, int p_type, godot_js_wrapper_ex *p_val);
extern int godot_js_wrapper_object_setvar(int p_id, int p_key_type, godot_js_wrapper_ex *p_key_ex, int p_val_type, godot_js_wrapper_ex *p_val_ex);
extern void godot_js_wrapper_object_set(int p_id, const char *p_name, int p_type, godot_js_wrapper_ex *p_val);
extern void godot_js_wrapper_object_unref(int p_id);
extern int godot_js_wrapper_create_cb(void *p_ref, void (*p_callback)(void *p_ref, int p_arg_id, int p_argc));
extern void godot_js_wrapper_object_set_cb_ret(int p_type, godot_js_wrapper_ex *p_val);
extern int godot_js_wrapper_create_object(const char *p_method, void **p_args, int p_argc, GodotJSWrapperVariant2JSCallback p_variant2js_callback, godot_js_wrapper_ex *p_cb_rval, void **p_lock, GodotJSWrapperFreeLockCallback p_lock_callback);
extern int godot_js_wrapper_object_is_buffer(int p_id);
extern int godot_js_wrapper_object_transfer_buffer(int p_id, void *p_byte_arr, void *p_byte_arr_write, void *(*p_callback)(void *p_ptr, void *p_ptr2, int p_len));
};

class JavaScriptObjectImpl : public JavaScriptObject {

private:
	friend class JavaScriptBridge;

	int _js_id = 0;
	Callable _callable;

	WASM_EXPORT static int _variant2js(const void **p_args, int p_pos, godot_js_wrapper_ex *r_val, void **p_lock);
	WASM_EXPORT static void _free_lock(void **p_lock, int p_type);
	WASM_EXPORT static Variant _js2variant(int p_type, godot_js_wrapper_ex *p_val);
	WASM_EXPORT static void *_alloc_variants(int p_size);
	WASM_EXPORT static void callback(void *p_ref, int p_arg_id, int p_argc);
	static void _callback(const JavaScriptObjectImpl *obj, Variant arg);

protected:
	bool _set(const String &p_name, const Variant &p_value) override;
	bool _get(const String &p_name, Variant &r_ret) const override;
	void _get_property_list(List<PropertyInfo> *p_list) const override;

public:
	Variant getvar(const Variant &p_key, bool *r_valid = nullptr) const override;
	void setvar(const Variant &p_key, const Variant &p_value, bool *r_valid = nullptr) override;
	Variant callp(const String &p_method, const Variant **p_args, int p_argc, Callable::CallError &r_error) override;
	JavaScriptObjectImpl() {}
	JavaScriptObjectImpl(int p_id) { _js_id = p_id; }
	~JavaScriptObjectImpl() {
		if (_js_id) {
			godot_js_wrapper_object_unref(_js_id);
		}
	}
};

bool JavaScriptObjectImpl::_set(const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND_V_MSG(!_js_id, false, "Invalid JS instance");
	const String name = p_name;
	godot_js_wrapper_ex exchange;
	void *lock = nullptr;
	const Variant *v = &p_value;
	int type = _variant2js((const void **)&v, 0, &exchange, &lock);
	godot_js_wrapper_object_set(_js_id, name.utf8().get_data(), type, &exchange);
	if (lock) {
		_free_lock(&lock, type);
	}
	return true;
}

bool JavaScriptObjectImpl::_get(const String &p_name, Variant &r_ret) const {
	ERR_FAIL_COND_V_MSG(!_js_id, false, "Invalid JS instance");
	const String name = p_name;
	godot_js_wrapper_ex exchange;
	int type = godot_js_wrapper_object_get(_js_id, &exchange, name.utf8().get_data());
	r_ret = _js2variant(type, &exchange);
	return true;
}

Variant JavaScriptObjectImpl::getvar(const Variant &p_key, bool *r_valid) const {
	if (r_valid) {
		*r_valid = false;
	}
	godot_js_wrapper_ex exchange;
	void *lock = nullptr;
	const Variant *v = &p_key;
	int prop_type = _variant2js((const void **)&v, 0, &exchange, &lock);
	int type = godot_js_wrapper_object_getvar(_js_id, prop_type, &exchange);
	if (lock) {
		_free_lock(&lock, prop_type);
	}
	if (type < 0) {
		return Variant();
	}
	if (r_valid) {
		*r_valid = true;
	}
	return _js2variant(type, &exchange);
}

void JavaScriptObjectImpl::setvar(const Variant &p_key, const Variant &p_value, bool *r_valid) {
	if (r_valid) {
		*r_valid = false;
	}
	godot_js_wrapper_ex kex, vex;
	void *klock = nullptr;
	void *vlock = nullptr;
	const Variant *kv = &p_key;
	const Variant *vv = &p_value;
	int ktype = _variant2js((const void **)&kv, 0, &kex, &klock);
	int vtype = _variant2js((const void **)&vv, 0, &vex, &vlock);
	int ret = godot_js_wrapper_object_setvar(_js_id, ktype, &kex, vtype, &vex);
	if (klock) {
		_free_lock(&klock, ktype);
	}
	if (vlock) {
		_free_lock(&vlock, vtype);
	}
	if (ret == 0 && r_valid) {
		*r_valid = true;
	}
}

void JavaScriptObjectImpl::_get_property_list(List<PropertyInfo> *p_list) const {
}

void JavaScriptObjectImpl::_free_lock(void **p_lock, int p_type) {
	ERR_FAIL_NULL_MSG(*p_lock, "No lock to free!");
	const Variant::Type type = (Variant::Type)p_type;
	switch (type) {
		case Variant::STRING: {
			CharString *cs = (CharString *)(*p_lock);
			memdelete(cs);
			*p_lock = nullptr;
		} break;
		default:
			ERR_FAIL_MSG("Unknown lock type to free. Likely a bug.");
	}
}

Variant JavaScriptObjectImpl::_js2variant(int p_type, godot_js_wrapper_ex *p_val) {
	Variant::Type type = (Variant::Type)p_type;
	switch (type) {
		case Variant::BOOL:
			return Variant((bool)p_val->i);
		case Variant::INT:
			return p_val->i;
		case Variant::FLOAT:
			return p_val->r;
		case Variant::STRING: {
			String out = String::utf8((const char *)p_val->p);
			free(p_val->p);
			return out;
		}
		case Variant::OBJECT: {
			return memnew(JavaScriptObjectImpl(p_val->i));
		}
		default:
			return Variant();
	}
}

int JavaScriptObjectImpl::_variant2js(const void **p_args, int p_pos, godot_js_wrapper_ex *r_val, void **p_lock) {
	const Variant **args = (const Variant **)p_args;
	const Variant *v = args[p_pos];
	Variant::Type type = v->get_type();
	switch (type) {
		case Variant::BOOL:
			r_val->i = v->operator bool() ? 1 : 0;
			break;
		case Variant::INT: {
			const int64_t tmp = v->operator int64_t();
			if (tmp >= 1LL << 31) {
				r_val->r = (double)tmp;
				return Variant::FLOAT;
			}
			r_val->i = v->operator int64_t();
		} break;
		case Variant::FLOAT:
			r_val->r = v->operator real_t();
			break;
		case Variant::STRING: {
			CharString *cs = memnew(CharString(v->operator String().utf8()));
			r_val->p = (void *)cs->get_data();
			*p_lock = (void *)cs;
		} break;
		case Variant::OBJECT: {
			JavaScriptObject *js_obj = Object::cast_to<JavaScriptObject>(v->operator Object *());
			r_val->i = js_obj != nullptr ? ((JavaScriptObjectImpl *)js_obj)->_js_id : 0;
		} break;
		default:
			break;
	}
	return type;
}

Variant JavaScriptObjectImpl::callp(const String &p_method, const Variant **p_args, int p_argc, Callable::CallError &r_error) {
	godot_js_wrapper_ex exchange;
	const String method = p_method;
	void *lock = nullptr;
	const int type = godot_js_wrapper_object_call(_js_id, method.utf8().get_data(), (void **)p_args, p_argc, &_variant2js, &exchange, &lock, &_free_lock);
	r_error.error = Callable::CallError::CALL_OK;
	if (type < 0) {
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}
	return _js2variant(type, &exchange);
}

void JavaScriptObjectImpl::callback(void *p_ref, int p_args_id, int p_argc) {
	const JavaScriptObjectImpl *obj = (JavaScriptObjectImpl *)p_ref;
	ERR_FAIL_COND_MSG(!obj->_callable.is_valid(), "JavaScript callback failed.");

	Array arg_arr;
	for (int i = 0; i < p_argc; i++) {
		godot_js_wrapper_ex exchange;
		exchange.i = i;
		int type = godot_js_wrapper_object_getvar(p_args_id, Variant::INT, &exchange);
		arg_arr.push_back(_js2variant(type, &exchange));
	}
	Variant arg = arg_arr;

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(JavaScriptObjectImpl::_callback).call_deferred(obj, arg);
		return;
	}
#endif

	_callback(obj, arg);
}

void JavaScriptObjectImpl::_callback(const JavaScriptObjectImpl *obj, Variant arg) {
	obj->_callable.call(arg);

	// Set return value
	godot_js_wrapper_ex exchange;
	void *lock = nullptr;
	Variant ret;
	const Variant *v = &ret;
	int type = _variant2js((const void **)&v, 0, &exchange, &lock);
	godot_js_wrapper_object_set_cb_ret(type, &exchange);
	if (lock) {
		_free_lock(&lock, type);
	}
}

Ref<JavaScriptObject> JavaScriptBridge::create_callback(const Callable &p_callable) {
	Ref<JavaScriptObjectImpl> out = memnew(JavaScriptObjectImpl);
	out->_callable = p_callable;
	out->_js_id = godot_js_wrapper_create_cb(out.ptr(), JavaScriptObjectImpl::callback);
	return out;
}

Ref<JavaScriptObject> JavaScriptBridge::get_interface(const String &p_interface) {
	int js_id = godot_js_wrapper_interface_get(p_interface.utf8().get_data());
	ERR_FAIL_COND_V_MSG(!js_id, Ref<JavaScriptObject>(), "No interface '" + p_interface + "' registered.");
	return Ref<JavaScriptObject>(memnew(JavaScriptObjectImpl(js_id)));
}

Variant JavaScriptBridge::_create_object_bind(const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	if (p_argcount < 1) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 1;
		return Ref<JavaScriptObject>();
	}
	if (!p_args[0]->is_string()) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		r_error.argument = 0;
		r_error.expected = Variant::STRING;
		return Ref<JavaScriptObject>();
	}
	godot_js_wrapper_ex exchange;
	const String object = *p_args[0];
	void *lock = nullptr;
	const Variant **args = p_argcount > 1 ? &p_args[1] : nullptr;
	const int type = godot_js_wrapper_create_object(object.utf8().get_data(), (void **)args, p_argcount - 1, &JavaScriptObjectImpl::_variant2js, &exchange, &lock, &JavaScriptObjectImpl::_free_lock);
	r_error.error = Callable::CallError::CALL_OK;
	if (type < 0) {
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Ref<JavaScriptObject>();
	}
	return JavaScriptObjectImpl::_js2variant(type, &exchange);
}

extern "C" {
union js_eval_ret {
	uint32_t b;
	double d;
	char *s;
};

extern int godot_js_eval(const char *p_js, int p_use_global_ctx, union js_eval_ret *p_union_ptr, void *p_byte_arr, void *p_byte_arr_write, void *(*p_callback)(void *p_ptr, void *p_ptr2, int p_len));
}

void *resize_PackedByteArray_and_open_write(void *p_arr, void *r_write, int p_len) {
	PackedByteArray *arr = (PackedByteArray *)p_arr;
	VectorWriteProxy<uint8_t> *write = (VectorWriteProxy<uint8_t> *)r_write;
	arr->resize(p_len);
	*write = arr->write;
	return arr->ptrw();
}

Variant JavaScriptBridge::eval(const String &p_code, bool p_use_global_exec_context) {
	union js_eval_ret js_data;
	PackedByteArray arr;
	VectorWriteProxy<uint8_t> arr_write;

	Variant::Type return_type = static_cast<Variant::Type>(godot_js_eval(p_code.utf8().get_data(), p_use_global_exec_context, &js_data, &arr, &arr_write, resize_PackedByteArray_and_open_write));

	switch (return_type) {
		case Variant::BOOL:
			return js_data.b;
		case Variant::FLOAT:
			return js_data.d;
		case Variant::STRING: {
			String str = String::utf8(js_data.s);
			free(js_data.s); // Must free the string allocated in JS.
			return str;
		}
		case Variant::PACKED_BYTE_ARRAY:
			arr_write = VectorWriteProxy<uint8_t>();
			return arr;
		default:
			return Variant();
	}
}

bool JavaScriptBridge::is_js_buffer(Ref<JavaScriptObject> p_js_obj) {
	Ref<JavaScriptObjectImpl> obj = p_js_obj;
	if (obj.is_null()) {
		return false;
	}
	return godot_js_wrapper_object_is_buffer(obj->_js_id);
}

PackedByteArray JavaScriptBridge::js_buffer_to_packed_byte_array(Ref<JavaScriptObject> p_js_obj) {
	ERR_FAIL_COND_V_MSG(!is_js_buffer(p_js_obj), PackedByteArray(), "The JavaScript object is not a buffer.");
	Ref<JavaScriptObjectImpl> obj = p_js_obj;

	PackedByteArray arr;
	VectorWriteProxy<uint8_t> arr_write;

	godot_js_wrapper_object_transfer_buffer(obj->_js_id, &arr, &arr_write, resize_PackedByteArray_and_open_write);

	arr_write = VectorWriteProxy<uint8_t>();
	return arr;
}

#endif // JAVASCRIPT_EVAL_ENABLED

void JavaScriptBridge::download_buffer(Vector<uint8_t> p_arr, const String &p_name, const String &p_mime) {
	godot_js_os_download_buffer(p_arr.ptr(), p_arr.size(), p_name.utf8().get_data(), p_mime.utf8().get_data());
}

bool JavaScriptBridge::pwa_needs_update() {
	return OS_Web::pwa_needs_update();
}

Error JavaScriptBridge::pwa_update() {
	return OS_Web::pwa_update();
}

void JavaScriptBridge::force_fs_sync() {
	OS_Web::force_fs_sync();
}

void register_web_api() {}


// --- Merged from platform/web/web_main.cpp ---
#include "platform/web/os_web.h"
#include "platform/web/os_web.h"
#include "platform/web/os_web.h"

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "main/main.h"

#ifdef TOOLS_ENABLED
#include "core/io/file_access.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h" // SceneTree only forward declares it.
#endif

#include <emscripten/emscripten.h>

#include <cstdlib>

#ifndef PROXY_TO_PTHREAD_ENABLED
static uint64_t target_ticks = 0;
#endif

static bool main_started = false;
static bool shutdown_complete = false;

void exit_callback() {
	if (!shutdown_complete) {
		return; // Still waiting.
	}
	if (main_started) {
		Main::cleanup();
		main_started = false;
	}
	int exit_code = OS::get_exit_code();
	OS::finalize();
	OS::finalize_core();
	godot_cleanup_profiler();
	emscripten_cancel_main_loop(); // We are exiting in this iteration.
	emscripten_force_exit(exit_code); // Exit runtime.
}

void cleanup_after_sync() {
	shutdown_complete = true;
}

void main_loop_callback() {
#ifndef PROXY_TO_PTHREAD_ENABLED
	uint64_t current_ticks = OS::get_ticks_usec();
#endif

	bool force_draw = DisplayServer::driver_as<DisplayServerWeb>()->check_size_force_redraw();
	if (force_draw) {
		Main::force_redraw();
#ifndef PROXY_TO_PTHREAD_ENABLED
	} else if (current_ticks < target_ticks) {
		return; // Skip frame.
#endif
	}

#ifndef PROXY_TO_PTHREAD_ENABLED
	int max_fps = Engine::get_max_fps();
	if (max_fps > 0) {
		if (current_ticks - target_ticks > 1000000) {
			// When the window loses focus, we stop getting updates and accumulate delay.
			// For this reason, if the difference is too big, we reset target ticks to the current ticks.
			target_ticks = current_ticks;
		}
		target_ticks += (uint64_t)(1000000 / max_fps);
	}
#endif

	if (OS_Web::main_loop_iterate()) {
		emscripten_cancel_main_loop(); // Cancel current loop and set the cleanup one.
		emscripten_set_main_loop(exit_callback, -1, false);
		godot_js_os_finish_async(cleanup_after_sync);
	}
}

void print_web_header() {
	// Emscripten.
	char *emscripten_version_char = godot_js_emscripten_get_version();
	String emscripten_version = vformat("Emscripten %s", emscripten_version_char);
	// `free()` is used here because it's not memory that was allocated by Godot.
	free(emscripten_version_char);

	// Build features.
	String thread_support = OS::has_feature("threads")
			? "multi-threaded"
			: "single-threaded";
	String extensions_support = OS::has_feature("web_extensions")
			? "GDExtension support"
			: "no GDExtension support";

	Vector<String> build_configuration = { emscripten_version, thread_support, extensions_support };
	print_line(vformat("Build configuration: %s.", String(", ").join(build_configuration)));
}

/// When calling main, it is assumed FS is setup and synced.
extern EMSCRIPTEN_KEEPALIVE int godot_web_main(int argc, char *argv[]) {
	godot_init_profiler();

	OS::initialize_core();
	OS::initialize();

	// We must override main when testing is enabled
	TEST_MAIN_OVERRIDE

	Error err = Main::setup(argv[0], argc - 1, &argv[1]);

	// Proper shutdown in case of setup failure.
	if (err != OK) {
		// Will only exit after sync.
		emscripten_set_main_loop(exit_callback, -1, false);
		godot_js_os_finish_async(cleanup_after_sync);
		if (err == ERR_HELP) { // Returned by --help and --version, so success.
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}

	print_web_header();

	main_started = true;

	// Ease up compatibility.
	ResourceLoader::set_abort_on_missing_resources(false);

	int ret = Main::start();
	OS::set_exit_code(ret);
	MainLoop::initialize();
#ifdef TOOLS_ENABLED
	if (Engine::is_project_manager_hint() && FileAccess::exists("/tmp/preload.zip")) {
		PackedStringArray ps;
		ps.push_back("/tmp/preload.zip");
		if (SceneTree::get_root()) {
			SceneTree::get_root()->emit_signal(SNAME("files_dropped"), ps);
		}
	}
#endif
	emscripten_set_main_loop(main_loop_callback, -1, false);
	// Immediately run the first iteration.
	// We are inside an animation frame, we want to immediately draw on the newly setup canvas.
	main_loop_callback();

	return OS::get_exit_code();
}


// --- Merged from platform/web/web_runtime.cpp ---
extern int godot_web_main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	return godot_web_main(argc, argv);
}
