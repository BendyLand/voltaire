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

#include "servers/display/display_server.h"

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
#include "core/os/os.h"
#include "servers/display/display_server.h"

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
#include "servers/display/display_server.h"
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

#include "core/os/os.h"
#include "core/io/ip.h"
#include "servers/display/display_server.h"

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
#include "servers/display/display_server.h"
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
#include "core/io/net_socket.h"

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
#include "core/os/os.h"
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
#include "servers/display/display_server.h"


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
#include "servers/display/display_server.h"

#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/os/os.h"
#include "servers/display/display_server.h"

#ifdef TOOLS_ENABLED
#include "core/io/file_access.h"
#include "scene/main/scene_tree.h"
#include "scene/main/scene_tree.h" // SceneTree only forward declares it.
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

// === Merged from platform/web/display_server_web.cpp ===
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

#include "servers/display/display_server.h"

#include "platform/web/dom_keys.inc"
#include "servers/display/display_server.h"

#include "core/os/os.h"
#include "servers/display/display_server.h"

#ifdef PROXY_TO_PTHREAD_ENABLED
#endif

#ifdef GLES3_ENABLED
#include "servers/display/display_server.h"
#endif

#include <emscripten.h>
#include <png.h>

#define DOM_BUTTON_LEFT 0
#define DOM_BUTTON_MIDDLE 1
#define DOM_BUTTON_RIGHT 2
#define DOM_BUTTON_XBUTTON1 3
#define DOM_BUTTON_XBUTTON2 4

// Window (canvas)
bool DisplayServerWeb::check_size_force_redraw() {
	bool size_changed = godot_js_display_size_update() != 0;
	if (size_changed && rect_changed_callback.is_valid()) {
		Size2i window_size = window_get_size();
		Variant size = Rect2i(Point2i(), window_size); // TODO use window_get_position if implemented.
		rect_changed_callback.call(size);
		emscripten_set_canvas_element_size(canvas_id, window_size.x, window_size.y);
	}
	return size_changed;
}

void DisplayServerWeb::fullscreen_change_callback(int p_fullscreen) {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_fullscreen_change_callback).call_deferred(p_fullscreen);
		return;
	}
#endif

	_fullscreen_change_callback(p_fullscreen);
}

void DisplayServerWeb::_fullscreen_change_callback(int p_fullscreen) {
	DisplayServerWeb *display = DisplayServer::driver_as<DisplayServerWeb>();
	if (p_fullscreen) {
		display->window_mode = DisplayServerEnums::WINDOW_MODE_FULLSCREEN;
	} else {
		display->window_mode = DisplayServerEnums::WINDOW_MODE_WINDOWED;
	}
}

// Drag and drop callback.
void DisplayServerWeb::drop_files_js_callback(const char **p_filev, int p_filec) {
	Vector<String> files;
	for (int i = 0; i < p_filec; i++) {
		files.push_back(String::utf8(p_filev[i]));
	}

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_drop_files_js_callback).call_deferred(files);
		return;
	}
#endif

	_drop_files_js_callback(files);
}

void DisplayServerWeb::_drop_files_js_callback(const Vector<String> &p_files) {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();
	if (!ds) {
		ERR_FAIL_MSG("Unable to drop files because the DisplayServer is not active");
	}
	if (!ds->drop_files_callback.is_valid()) {
		return;
	}
	Variant v_files = p_files;
	const Variant *v_args[1] = { &v_files };
	Variant ret;
	Callable::CallError ce;
	ds->drop_files_callback.callp((const Variant **)&v_args, 1, ret, ce);
	if (ce.error != Callable::CallError::CALL_OK) {
		ERR_PRINT(vformat("Failed to execute drop files callback: %s.", Variant::get_callable_error_text(ds->drop_files_callback, v_args, 1, ce)));
	}
}

// Web quit request callback.
void DisplayServerWeb::request_quit_callback() {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_request_quit_callback).call_deferred();
		return;
	}
#endif

	_request_quit_callback();
}

void DisplayServerWeb::_request_quit_callback() {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();
	if (ds && ds->window_event_callback.is_valid()) {
		Variant event = int(DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
		ds->window_event_callback.call(event);
	}
}

// Keys

void DisplayServerWeb::dom2godot_mod(Ref<InputEventWithModifiers> ev, int p_mod, Key p_keycode) {
	if (p_keycode != Key::SHIFT) {
		ev->set_shift_pressed(p_mod & 1);
	}
	if (p_keycode != Key::ALT) {
		ev->set_alt_pressed(p_mod & 2);
	}
	if (p_keycode != Key::CTRL) {
		ev->set_ctrl_pressed(p_mod & 4);
	}
	if (p_keycode != Key::META) {
		ev->set_meta_pressed(p_mod & 8);
	}
}

void DisplayServerWeb::key_callback(int p_pressed, int p_repeat, int p_modifiers) {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();
	JSKeyEvent &key_event = ds->key_event;

	const String code = String::utf8(key_event.code);
	const String key = String::utf8(key_event.key);

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_key_callback).call_deferred(code, key, p_pressed, p_repeat, p_modifiers);
		return;
	}
#endif

	_key_callback(code, key, p_pressed, p_repeat, p_modifiers);
}

void DisplayServerWeb::_key_callback(const String &p_key_event_code, const String &p_key_event_key, int p_pressed, int p_repeat, int p_modifiers) {
	// Resume audio context after input in case autoplay was denied.
	OS_Web::resume_audio();

	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();
	if (ds->ime_started) {
		return;
	}

	char32_t c = 0x00;
	String unicode = p_key_event_key;
	if (unicode.length() == 1) {
		c = unicode[0];
	}

	Key keycode = dom_code2godot_scancode(p_key_event_code.utf8().get_data(), p_key_event_key.utf8().get_data(), false);
	Key scancode = dom_code2godot_scancode(p_key_event_code.utf8().get_data(), p_key_event_key.utf8().get_data(), true);
	KeyLocation location = dom_code2godot_key_location(p_key_event_code.utf8().get_data());

	DisplayServerWeb::KeyEvent ke;

	ke.pressed = p_pressed;
	ke.echo = p_repeat;
	ke.raw = true;
	ke.keycode = fix_keycode(c, keycode);
	ke.physical_keycode = scancode;
	ke.key_label = fix_key_label(c, keycode);
	ke.unicode = fix_unicode(c);
	ke.location = location;
	ke.mod = p_modifiers;

	if (ds->key_event_pos >= ds->key_event_buffer.size()) {
		ds->key_event_buffer.resize(1 + ds->key_event_pos);
	}
	ds->key_event_buffer.write[ds->key_event_pos++] = ke;

	// Make sure to flush all events so we can call restricted APIs inside the event.
	Input::flush_buffered_events();
}

// Mouse

int DisplayServerWeb::mouse_button_callback(int p_pressed, int p_button, double p_x, double p_y, int p_modifiers) {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_mouse_button_callback).call_deferred(p_pressed, p_button, p_x, p_y, p_modifiers);
		return true;
	}
#endif

	return _mouse_button_callback(p_pressed, p_button, p_x, p_y, p_modifiers);
}

int DisplayServerWeb::_mouse_button_callback(int p_pressed, int p_button, double p_x, double p_y, int p_modifiers) {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();

	Point2 pos(p_x, p_y);
	Ref<InputEventMouseButton> ev;
	ev.instantiate();
	ev->set_position(pos);
	ev->set_global_position(pos);
	ev->set_pressed(p_pressed);
	dom2godot_mod(ev, p_modifiers, Key::NONE);

	switch (p_button) {
		case DOM_BUTTON_LEFT:
			ev->set_button_index(MouseButton::LEFT);
			break;
		case DOM_BUTTON_MIDDLE:
			ev->set_button_index(MouseButton::MIDDLE);
			break;
		case DOM_BUTTON_RIGHT:
			ev->set_button_index(MouseButton::RIGHT);
			break;
		case DOM_BUTTON_XBUTTON1:
			ev->set_button_index(MouseButton::MB_XBUTTON1);
			break;
		case DOM_BUTTON_XBUTTON2:
			ev->set_button_index(MouseButton::MB_XBUTTON2);
			break;
		default:
			return false;
	}

	if (p_pressed) {
		uint64_t diff = (OS::get_ticks_usec() / 1000) - ds->last_click_ms;

		if (ev->get_button_index() == ds->last_click_button_index) {
			if (diff < 400 && Point2(ds->last_click_pos).distance_to(ev->get_position()) < 5) {
				ds->last_click_ms = 0;
				ds->last_click_pos = Point2(-100, -100);
				ds->last_click_button_index = MouseButton::NONE;
				ev->set_double_click(true);
			}

		} else {
			ds->last_click_button_index = ev->get_button_index();
		}

		if (!ev->is_double_click()) {
			ds->last_click_ms += diff;
			ds->last_click_pos = ev->get_position();
		}
	}

	uint32_tget_mouse_button_mask();
	MouseButtonMask button_flag = mouse_button_to_mask(ev->get_button_index());
	if (ev->is_pressed()) {
		mask.set_flag(button_flag);
	} else if (mask.has_flag(button_flag)) {
		mask.clear_flag(button_flag);
	} else {
		// Received release event, but press was outside the canvas, so ignore.
		return false;
	}
	ev->set_button_mask(mask);

	Input::parse_input_event(ev);
	// Resume audio context after input in case autoplay was denied.
	OS_Web::resume_audio();

	// Make sure to flush all events so we can call restricted APIs inside the event.
	Input::flush_buffered_events();

	// Prevent multi-click text selection and wheel-click scrolling anchor.
	// Context menu is prevented through contextmenu event.
	return true;
}

void DisplayServerWeb::mouse_move_callback(double p_x, double p_y, double p_rel_x, double p_rel_y, int p_modifiers, double p_pressure) {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_mouse_move_callback).call_deferred(p_x, p_y, p_rel_x, p_rel_y, p_modifiers, p_pressure);
		return;
	}
#endif

	_mouse_move_callback(p_x, p_y, p_rel_x, p_rel_y, p_modifiers, p_pressure);
}

void DisplayServerWeb::_mouse_move_callback(double p_x, double p_y, double p_rel_x, double p_rel_y, int p_modifiers, double p_pressure) {
	uint32_tget_mouse_button_mask();
	// For motion outside the canvas, only read mouse movement if dragging
	// started inside the canvas; imitating desktop app behavior.
	if (!DisplayServer::driver_as<DisplayServerWeb>()->cursor_inside_canvas && input_mask.is_empty()) {
		return;
	}

	Point2 pos(p_x, p_y);
	Ref<InputEventMouseMotion> ev;
	ev.instantiate();
	dom2godot_mod(ev, p_modifiers, Key::NONE);
	ev->set_button_mask(input_mask);

	ev->set_position(pos);
	ev->set_global_position(pos);
	ev->set_pressure((float)p_pressure);

	ev->set_relative(Vector2(p_rel_x, p_rel_y));
	ev->set_relative_screen_position(ev->get_relative());
	ev->set_velocity(Input::get_last_mouse_velocity());
	ev->set_screen_velocity(ev->get_velocity());

	Input::parse_input_event(ev);
}

// Cursor
const char *DisplayServerWeb::godot2dom_cursor(DisplayServerEnums::CursorShape p_shape) {
	switch (p_shape) {
		case DisplayServerEnums::CURSOR_ARROW:
			return "default";
		case DisplayServerEnums::CURSOR_IBEAM:
			return "text";
		case DisplayServerEnums::CURSOR_POINTING_HAND:
			return "pointer";
		case DisplayServerEnums::CURSOR_CROSS:
			return "crosshair";
		case DisplayServerEnums::CURSOR_WAIT:
			return "wait";
		case DisplayServerEnums::CURSOR_BUSY:
			return "progress";
		case DisplayServerEnums::CURSOR_DRAG:
			return "grab";
		case DisplayServerEnums::CURSOR_CAN_DROP:
			return "grabbing";
		case DisplayServerEnums::CURSOR_FORBIDDEN:
			return "no-drop";
		case DisplayServerEnums::CURSOR_VSIZE:
			return "ns-resize";
		case DisplayServerEnums::CURSOR_HSIZE:
			return "ew-resize";
		case DisplayServerEnums::CURSOR_BDIAGSIZE:
			return "nesw-resize";
		case DisplayServerEnums::CURSOR_FDIAGSIZE:
			return "nwse-resize";
		case DisplayServerEnums::CURSOR_MOVE:
			return "move";
		case DisplayServerEnums::CURSOR_VSPLIT:
			return "row-resize";
		case DisplayServerEnums::CURSOR_HSPLIT:
			return "col-resize";
		case DisplayServerEnums::CURSOR_HELP:
			return "help";
		default:
			return "default";
	}
}

bool DisplayServerWeb::tts_is_speaking() const {
	return godot_js_tts_is_speaking();
}

bool DisplayServerWeb::tts_is_paused() const {
	return godot_js_tts_is_paused();
}

void DisplayServerWeb::update_voices_callback(int p_size, const char **p_voice) {
	Vector<String> voices;
	for (int i = 0; i < p_size; i++) {
		voices.append(String::utf8(p_voice[i]));
	}

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_update_voices_callback).call_deferred(voices);
		return;
	}
#endif

	_update_voices_callback(voices);
}

void DisplayServerWeb::_update_voices_callback(const Vector<String> &p_voices) {
	DisplayServer::driver_as<DisplayServerWeb>()->voices.clear();
	for (int i = 0; i < p_voices.size(); i++) {
		Vector<String> tokens = p_voices[i].split(";", true, 2);
		if (tokens.size() == 2) {
			Dictionary voice_d;
			voice_d["name"] = tokens[1];
			voice_d["id"] = tokens[1];
			voice_d["language"] = tokens[0];
			DisplayServer::driver_as<DisplayServerWeb>()->voices.push_back(voice_d);
		}
	}
}

TypedArray<Dictionary> DisplayServerWeb::tts_get_voices() const {
	godot_js_tts_get_voices(update_voices_callback);
	return voices;
}

void DisplayServerWeb::tts_speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	if (p_interrupt) {
		tts_stop();
	}

	if (p_text.is_empty()) {
		tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, p_utterance_id);
		return;
	}

	CharString string = p_text.utf8();
	utterance_ids[p_utterance_id] = string;

	godot_js_tts_speak(string.get_data(), p_voice.utf8().get_data(), CLAMP(p_volume, 0, 100), CLAMP(p_pitch, 0.f, 2.f), CLAMP(p_rate, 0.1f, 10.f), p_utterance_id, DisplayServerWeb::js_utterance_callback);
}

void DisplayServerWeb::tts_pause() {
	godot_js_tts_pause();
}

void DisplayServerWeb::tts_resume() {
	godot_js_tts_resume();
}

void DisplayServerWeb::tts_stop() {
	for (const KeyValue<int64_t, CharString> &E : utterance_ids) {
		tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, E.key);
	}
	utterance_ids.clear();
	godot_js_tts_stop();
}

void DisplayServerWeb::js_utterance_callback(int p_event, int64_t p_id, int p_pos) {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_js_utterance_callback).call_deferred(p_event, p_id, p_pos);
		return;
	}
#endif

	_js_utterance_callback(p_event, p_id, p_pos);
}

void DisplayServerWeb::_js_utterance_callback(int p_event, int64_t p_id, int p_pos) {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();
	if (ds->utterance_ids.has(p_id)) {
		int pos = 0;
		if ((DisplayServerEnums::TTSUtteranceEvent)p_event == DisplayServerEnums::TTS_UTTERANCE_BOUNDARY) {
			// Convert position from UTF-8 to UTF-32.
			const CharString &string = ds->utterance_ids[p_id];
			for (int i = 0; i < MIN(p_pos, string.length()); i++) {
				uint8_t c = string[i];
				if ((c & 0xe0) == 0xc0) {
					i += 1;
				} else if ((c & 0xf0) == 0xe0) {
					i += 2;
				} else if ((c & 0xf8) == 0xf0) {
					i += 3;
				}
				pos++;
			}
		} else if ((DisplayServerEnums::TTSUtteranceEvent)p_event != DisplayServerEnums::TTS_UTTERANCE_STARTED) {
			ds->utterance_ids.erase(p_id);
		}
		ds->tts_post_utterance_event((DisplayServerEnums::TTSUtteranceEvent)p_event, p_id, pos);
	}
}

void DisplayServerWeb::cursor_set_shape(DisplayServerEnums::CursorShape p_shape) {
	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);
	if (cursor_shape == p_shape) {
		return;
	}
	cursor_shape = p_shape;
	godot_js_display_cursor_set_shape(godot2dom_cursor(cursor_shape));
}

DisplayServerEnums::CursorShape DisplayServerWeb::cursor_get_shape() const {
	return cursor_shape;
}

void DisplayServerWeb::cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape, const Vector2 &p_hotspot) {
	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);
	if (p_cursor.is_valid()) {
		Ref<Image> image = _get_cursor_image_from_resource(p_cursor, p_hotspot);
		ERR_FAIL_COND(image.is_null());
		Vector2i texture_size = image->get_size();

		if (image->get_format() != Image::FORMAT_RGBA8) {
			image->convert(Image::FORMAT_RGBA8);
		}

		png_image png_meta;
		memset(&png_meta, 0, sizeof png_meta);
		png_meta.version = PNG_IMAGE_VERSION;
		png_meta.width = texture_size.width;
		png_meta.height = texture_size.height;
		png_meta.format = PNG_FORMAT_RGBA;

		PackedByteArray png;
		size_t len;
		PackedByteArray data = image->get_data();
		ERR_FAIL_COND(!png_image_write_get_memory_size(png_meta, len, 0, data.ptr(), 0, nullptr));

		png.resize(len);
		ERR_FAIL_COND(!png_image_write_to_memory(&png_meta, png.ptrw(), &len, 0, data.ptr(), 0, nullptr));

		godot_js_display_cursor_set_custom_shape(godot2dom_cursor(p_shape), png.ptr(), len, p_hotspot.x, p_hotspot.y);

	} else {
		godot_js_display_cursor_set_custom_shape(godot2dom_cursor(p_shape), nullptr, 0, 0, 0);
	}

	cursor_set_shape(cursor_shape);
}

// Mouse mode
void DisplayServerWeb::_mouse_update_mode() {
	DisplayServerEnums::MouseMode wanted_mouse_mode = mouse_mode_override_enabled
			? mouse_mode_override
			: mouse_mode_base;

	ERR_FAIL_COND_MSG(wanted_mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED || wanted_mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN, "DisplayServerEnums::MOUSE_MODE_CONFINED is not supported for the Web platform.");
	if (wanted_mouse_mode == mouse_get_mode()) {
		return;
	}

	if (wanted_mouse_mode == DisplayServerEnums::MOUSE_MODE_VISIBLE) {
		godot_js_display_cursor_set_visible(1);
		godot_js_display_cursor_lock_set(0);

	} else if (wanted_mouse_mode == DisplayServerEnums::MOUSE_MODE_HIDDEN) {
		godot_js_display_cursor_set_visible(0);
		godot_js_display_cursor_lock_set(0);

	} else if (wanted_mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
		godot_js_display_cursor_set_visible(1);
		godot_js_display_cursor_lock_set(1);
	}
}

void DisplayServerWeb::mouse_set_mode(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);

	if (mouse_mode_override_enabled) {
		mouse_mode_base = p_mode;
		// No need to update, as override is enabled.
		return;
	}
	if (p_mode == mouse_mode_base && p_mode == mouse_get_mode()) {
		// No need to update, as it is currently set as the correct mode.
		return;
	}

	mouse_mode_base = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerWeb::mouse_get_mode() const {
	if (godot_js_display_cursor_is_hidden()) {
		return DisplayServerEnums::MOUSE_MODE_HIDDEN;
	}

	if (godot_js_display_cursor_is_locked()) {
		return DisplayServerEnums::MOUSE_MODE_CAPTURED;
	}
	return DisplayServerEnums::MOUSE_MODE_VISIBLE;
}

void DisplayServerWeb::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);

	if (!mouse_mode_override_enabled) {
		mouse_mode_override = p_mode;
		// No need to update, as override is not enabled.
		return;
	}
	if (p_mode == mouse_mode_override && p_mode == mouse_get_mode()) {
		// No need to update, as it is currently set as the correct mode.
		return;
	}

	mouse_mode_override = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerWeb::mouse_get_mode_override() const {
	return mouse_mode_override;
}

void DisplayServerWeb::mouse_set_mode_override_enabled(bool p_override_enabled) {
	if (p_override_enabled == mouse_mode_override_enabled) {
		return;
	}
	mouse_mode_override_enabled = p_override_enabled;
	_mouse_update_mode();
}

bool DisplayServerWeb::mouse_is_mode_override_enabled() const {
	return mouse_mode_override_enabled;
}

Point2i DisplayServerWeb::mouse_get_position() const {
	return Input::get_mouse_position();
}

// Wheel
int DisplayServerWeb::mouse_wheel_callback(int p_delta_mode, double p_delta_x, double p_delta_y) {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_mouse_wheel_callback).call_deferred(p_delta_mode, p_delta_x, p_delta_y);
		return true;
	}
#endif

	return _mouse_wheel_callback(p_delta_mode, p_delta_x, p_delta_y);
}

int DisplayServerWeb::_mouse_wheel_callback(int p_delta_mode, double p_delta_x, double p_delta_y) {
	if (!godot_js_display_canvas_is_focused() && !godot_js_is_ime_focused()) {
		if (DisplayServer::driver_as<DisplayServerWeb>()->cursor_inside_canvas) {
			godot_js_display_canvas_focus();
		} else {
			return false;
		}
	}

	Ref<InputEventMouseButton> ev;
	ev.instantiate();
	ev->set_position(Input::get_mouse_position());
	ev->set_global_position(ev->get_position());

	ev->set_shift_pressed(Input::is_key_pressed(Key::SHIFT));
	ev->set_alt_pressed(Input::is_key_pressed(Key::ALT));
	ev->set_ctrl_pressed(Input::is_key_pressed(Key::CTRL));
	ev->set_meta_pressed(Input::is_key_pressed(Key::META));

	enum DeltaMode {
		DELTA_MODE_PIXEL = 0,
		DELTA_MODE_LINE = 1,
		DELTA_MODE_PAGE = 2,
	};
	const float MOUSE_WHEEL_PIXEL_FACTOR = 0.03f;
	const float MOUSE_WHEEL_LINE_FACTOR = 0.3f;
	const float MOUSE_WHEEL_PAGE_FACTOR = 1.0f;
	float mouse_wheel_factor;

	switch (p_delta_mode) {
		case DELTA_MODE_PIXEL: {
			mouse_wheel_factor = MOUSE_WHEEL_PIXEL_FACTOR;
		} break;
		case DELTA_MODE_LINE: {
			mouse_wheel_factor = MOUSE_WHEEL_LINE_FACTOR;
		} break;
		case DELTA_MODE_PAGE: {
			mouse_wheel_factor = MOUSE_WHEEL_PAGE_FACTOR;
		} break;
	}

	if (p_delta_y < 0) {
		ev->set_button_index(MouseButton::WHEEL_UP);
		ev->set_factor(-p_delta_y * mouse_wheel_factor);
	} else if (p_delta_y > 0) {
		ev->set_button_index(MouseButton::WHEEL_DOWN);
		ev->set_factor(p_delta_y * mouse_wheel_factor);
	} else if (p_delta_x > 0) {
		ev->set_button_index(MouseButton::WHEEL_LEFT);
		ev->set_factor(p_delta_x * mouse_wheel_factor);
	} else if (p_delta_x < 0) {
		ev->set_button_index(MouseButton::WHEEL_RIGHT);
		ev->set_factor(-p_delta_x * mouse_wheel_factor);
	} else {
		return false;
	}

	MouseButtonMask button_flag = mouse_button_to_mask(ev->get_button_index());
	uint32_tget_mouse_button_mask();
	button_mask.set_flag(button_flag);

	ev->set_pressed(true);
	ev->set_button_mask(button_mask);
	Input::parse_input_event(ev.ptr());

	Ref<InputEventMouseButton> release = ev->duplicate();
	release->set_pressed(false);
	release->set_button_mask(Input::get_mouse_button_mask());
	Input::parse_input_event(release.ptr());

	return true;
}

// Touch
void DisplayServerWeb::touch_callback(int p_type, int p_count) {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_touch_callback).call_deferred(p_type, p_count);
		return;
	}
#endif

	_touch_callback(p_type, p_count);
}

void DisplayServerWeb::_touch_callback(int p_type, int p_count) {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();

	const JSTouchEvent &touch_event = ds->touch_event;
	for (int i = 0; i < p_count; i++) {
		Point2 point(touch_event.coords[i * 2], touch_event.coords[i * 2 + 1]);
		if (p_type == 2) {
			// touchmove
			Ref<InputEventScreenDrag> ev;
			ev.instantiate();
			ev->set_index(touch_event.identifier[i]);
			ev->set_position(point);

			Point2 &prev = ds->touches[i];
			ev->set_relative(ev->get_position() - prev);
			ev->set_relative_screen_position(ev->get_relative());
			prev = ev->get_position();

			Input::parse_input_event(ev);
		} else {
			// touchstart/touchend
			Ref<InputEventScreenTouch> ev;

			// Resume audio context after input in case autoplay was denied.
			OS_Web::resume_audio();

			ev.instantiate();
			ev->set_index(touch_event.identifier[i]);
			ev->set_position(point);
			ev->set_pressed(p_type == 0);
			ds->touches[i] = point;

			Input::parse_input_event(ev);

			// Make sure to flush all events so we can call restricted APIs inside the event.
			Input::flush_buffered_events();
		}
	}
}

bool DisplayServerWeb::is_touchscreen_available() const {
	return godot_js_display_touchscreen_is_available() || (Input::is_initialized() && Input::is_emulating_touch_from_mouse());
}

// Virtual Keyboard
void DisplayServerWeb::vk_input_text_callback(const char *p_text, int p_cursor) {
	String text = String::utf8(p_text);

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_vk_input_text_callback).call_deferred(text, p_cursor);
		return;
	}
#endif

	_vk_input_text_callback(text, p_cursor);
}

void DisplayServerWeb::_vk_input_text_callback(const String &p_text, int p_cursor) {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();
	if (!ds || !ds->input_text_callback.is_valid()) {
		return;
	}
	// Call input_text
	ds->input_text_callback.call(p_text, true);
	// Insert key right to reach position.
	Ref<InputEventKey> k;
	for (int i = 0; i < p_cursor; i++) {
		k.instantiate();
		k->set_pressed(true);
		k->set_echo(false);
		k->set_keycode(Key::RIGHT);
		Input::parse_input_event(k.ptr());
		k.instantiate();
		k->set_pressed(false);
		k->set_echo(false);
		k->set_keycode(Key::RIGHT);
		Input::parse_input_event(k.ptr());
	}
}

void DisplayServerWeb::virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect, DisplayServerEnums::VirtualKeyboardType p_type, int p_max_input_length, int p_cursor_start, int p_cursor_end) {
	godot_js_display_vk_show(p_existing_text.utf8().get_data(), p_type, p_cursor_start, p_cursor_end);
}

void DisplayServerWeb::virtual_keyboard_hide() {
	godot_js_display_vk_hide();
}

void DisplayServerWeb::window_blur_callback() {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_window_blur_callback).call_deferred();
		return;
	}
#endif

	_window_blur_callback();
}

void DisplayServerWeb::_window_blur_callback() {
	Input::release_pressed_events();
}

// Gamepad
void DisplayServerWeb::gamepad_callback(int p_index, int p_connected, const char *p_id, const char *p_guid) {
	String id = p_id;
	String guid = p_guid;

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_gamepad_callback).call_deferred(p_index, p_connected, id, guid);
		return;
	}
#endif

	_gamepad_callback(p_index, p_connected, id, guid);
}

void DisplayServerWeb::_gamepad_callback(int p_index, int p_connected, const String &p_id, const String &p_guid) {
	(void)p_index;
	(void)p_id;
	(void)p_guid;
	if (p_connected) {
		DisplayServer::driver_as<DisplayServerWeb>()->gamepad_count += 1;
	} else {
		DisplayServer::driver_as<DisplayServerWeb>()->gamepad_count -= 1;
	}
}

// IME.
void DisplayServerWeb::ime_callback(int p_type, const char *p_text) {
	String text = String::utf8(p_text);

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_ime_callback).call_deferred(p_type, text);
		return;
	}
#endif

	_ime_callback(p_type, text);
}

void DisplayServerWeb::_ime_callback(int p_type, const String &p_text) {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();
	// Resume audio context after input in case autoplay was denied.
	OS_Web::resume_audio();

	switch (p_type) {
		case 0: {
			// IME start.
			ds->ime_text = String();
			ds->ime_selection = Vector2i();
			for (int i = ds->key_event_pos - 1; i >= 0; i--) {
				// Delete last raw keydown event from query.
				if (ds->key_event_buffer[i].pressed && ds->key_event_buffer[i].raw) {
					ds->key_event_buffer.remove_at(i);
					ds->key_event_pos--;
					break;
				}
			}
			if (MainLoop::is_active()) {
				MainLoop::notification(MainLoop::NOTIFICATION_OS_IME_UPDATE);
			}
			ds->ime_started = true;
		} break;
		case 1: {
			// IME update.
			if (ds->ime_active && ds->ime_started) {
				ds->ime_text = p_text;
				ds->ime_selection = Vector2i(ds->ime_text.length(), ds->ime_text.length());
				if (MainLoop::is_active()) {
					MainLoop::notification(MainLoop::NOTIFICATION_OS_IME_UPDATE);
				}
			}
		} break;
		case 2: {
			// IME commit.
			if (ds->ime_active && ds->ime_started) {
				ds->ime_started = false;

				ds->ime_text = String();
				ds->ime_selection = Vector2i();
				if (MainLoop::is_active()) {
					MainLoop::notification(MainLoop::NOTIFICATION_OS_IME_UPDATE);
				}

				String text = p_text;
				for (int i = 0; i < text.length(); i++) {
					DisplayServerWeb::KeyEvent ke;

					ke.pressed = true;
					ke.echo = false;
					ke.raw = false;
					ke.keycode = Key::NONE;
					ke.physical_keycode = Key::NONE;
					ke.key_label = Key::NONE;
					ke.unicode = text[i];
					ke.mod = 0;

					if (ds->key_event_pos >= ds->key_event_buffer.size()) {
						ds->key_event_buffer.resize(1 + ds->key_event_pos);
					}
					ds->key_event_buffer.write[ds->key_event_pos++] = ke;
				}
			}
		} break;
		default:
			break;
	}

	ds->process_keys();
	Input::flush_buffered_events();
}

void DisplayServerWeb::window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window) {
	ime_active = p_active;
	godot_js_set_ime_active(p_active);
}

void DisplayServerWeb::window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window) {
	godot_js_set_ime_position(p_pos.x, p_pos.y);
}

Point2i DisplayServerWeb::ime_get_selection() const {
	return ime_selection;
}

String DisplayServerWeb::ime_get_text() const {
	return ime_text;
}

void DisplayServerWeb::process_joypads() {
	int32_t pads = godot_js_input_gamepad_sample_count();
	int32_t s_btns_num = 0;
	int32_t s_axes_num = 0;
	int32_t s_standard = 0;
	float s_btns[16];
	float s_axes[10];
	for (int idx = 0; idx < pads; idx++) {
		int err = godot_js_input_gamepad_sample_get(idx, s_btns, &s_btns_num, s_axes, &s_axes_num, &s_standard);
		if (err) {
			continue;
		}
		for (int b = 0; b < s_btns_num; b++) {
			// Buttons 6 and 7 in the standard mapping need to be
			// axis to be handled as JoyAxis::TRIGGER by Godot.
			if (s_standard && (b == 6)) {
				Input::joy_axis(idx, JoyAxis::TRIGGER_LEFT, s_btns[b]);
			} else if (s_standard && (b == 7)) {
				Input::joy_axis(idx, JoyAxis::TRIGGER_RIGHT, s_btns[b]);
			} else {
				Input::joy_button(idx, (JoyButton)b, s_btns[b]);
			}
		}
		for (int a = 0; a < s_axes_num; a++) {
			Input::joy_axis(idx, (JoyAxis)a, s_axes[a]);
		}
	}
}

Vector<String> DisplayServerWeb::get_rendering_drivers_func() {
	Vector<String> drivers;
#ifdef GLES3_ENABLED
	drivers.push_back("opengl3");
#endif
	return drivers;
}

// Clipboard
void DisplayServerWeb::update_clipboard_callback(const char *p_text) {
	String text = String::utf8(p_text);

#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_update_clipboard_callback).call_deferred(text);
		return;
	}
#endif

	_update_clipboard_callback(text);
}

void DisplayServerWeb::_update_clipboard_callback(const String &p_text) {
	DisplayServer::driver_as<DisplayServerWeb>()->clipboard = p_text;
}

void DisplayServerWeb::clipboard_set(const String &p_text) {
	clipboard = p_text;
	int err = godot_js_display_clipboard_set(p_text.utf8().get_data());
	ERR_FAIL_COND_MSG(err, "Clipboard API is not supported.");
}

String DisplayServerWeb::clipboard_get() const {
	godot_js_display_clipboard_get(update_clipboard_callback);
	return clipboard;
}

void DisplayServerWeb::send_window_event_callback(int p_notification) {
#ifdef PROXY_TO_PTHREAD_ENABLED
	if (!Thread::is_main_thread()) {
		callable_mp_static(DisplayServerWeb::_send_window_event_callback).call_deferred(p_notification);
		return;
	}
#endif

	_send_window_event_callback(p_notification);
}

void DisplayServerWeb::_send_window_event_callback(int p_notification) {
	DisplayServerWeb *ds = DisplayServer::driver_as<DisplayServerWeb>();
	if (!ds) {
		return;
	}
	if (p_notification == DisplayServerEnums::WINDOW_EVENT_MOUSE_ENTER || p_notification == DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT) {
		ds->cursor_inside_canvas = p_notification == DisplayServerEnums::WINDOW_EVENT_MOUSE_ENTER;
	}
	if (godot_js_is_ime_focused() && (p_notification == DisplayServerEnums::WINDOW_EVENT_FOCUS_IN || p_notification == DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT)) {
		return;
	}
	if (ds->window_event_callback.is_valid()) {
		Variant event = int(p_notification);
		ds->window_event_callback.call(event);
	}
}

void DisplayServerWeb::set_icon(const Ref<Image> &p_icon) {
	if (p_icon.is_valid()) {
		ERR_FAIL_COND(p_icon->get_width() <= 0 || p_icon->get_height() <= 0);

		Ref<Image> icon = p_icon;
		if (icon->is_compressed()) {
			icon = icon->duplicate();
			ERR_FAIL_COND(icon->decompress() != OK);
		}
		if (icon->get_format() != Image::FORMAT_RGBA8) {
			if (icon == p_icon) {
				icon = icon->duplicate();
			}
			icon->convert(Image::FORMAT_RGBA8);
		}

		png_image png_meta;
		memset(&png_meta, 0, sizeof png_meta);
		png_meta.version = PNG_IMAGE_VERSION;
		png_meta.width = icon->get_width();
		png_meta.height = icon->get_height();
		png_meta.format = PNG_FORMAT_RGBA;

		PackedByteArray png;
		size_t len;
		PackedByteArray data = icon->get_data();
		ERR_FAIL_COND(!png_image_write_get_memory_size(png_meta, len, 0, data.ptr(), 0, nullptr));

		png.resize(len);
		ERR_FAIL_COND(!png_image_write_to_memory(&png_meta, png.ptrw(), &len, 0, data.ptr(), 0, nullptr));

		godot_js_display_window_icon_set(png.ptr(), len);
	} else {
		godot_js_display_window_icon_set(nullptr, 0);
	}
}

void DisplayServerWeb::_dispatch_input_event(const Ref<InputEvent> &p_event) {
	Callable cb = DisplayServer::driver_as<DisplayServerWeb>()->input_event_callback;
	if (cb.is_valid()) {
		cb.call(p_event);
	}
}

DisplayServerDriver *DisplayServerWeb::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_window_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Point2i *p_position, const Size2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	return memnew(DisplayServerWeb(p_rendering_driver, p_window_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error));
}

DisplayServerWeb::DisplayServerWeb(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_window_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Point2i *p_position, const Size2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	r_error = OK; // Always succeeds for now.

	NativeMenu::initialize();

	// Ensure the canvas ID.
	godot_js_config_canvas_id_get(canvas_id, 256);

	// Handle contextmenu, webglcontextlost
	godot_js_display_setup_canvas(p_resolution.x, p_resolution.y, (p_window_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_window_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN), OS::is_hidpi_allowed() ? 1 : 0);

	// Check if it's windows.
	swap_cancel_ok = godot_js_display_is_swap_ok_cancel() == 1;

	// Expose method for requesting quit.
	godot_js_os_request_quit_cb(request_quit_callback);

#ifdef GLES3_ENABLED
	bool webgl2_inited = false;
	if (godot_js_display_has_webgl(2)) {
		EmscriptenWebGLContextAttributes attributes;
		emscripten_webgl_init_context_attributes(&attributes);
		attributes.alpha = OS::is_layered_allowed();
		attributes.antialias = false;
		attributes.majorVersion = 2;
		attributes.explicitSwapControl = true;

		webgl_ctx = emscripten_webgl_create_context(canvas_id, &attributes);
		webgl2_inited = webgl_ctx && emscripten_webgl_make_context_current(webgl_ctx) == EMSCRIPTEN_RESULT_SUCCESS;
	}
	if (webgl2_inited) {
		if (!emscripten_webgl_enable_extension(webgl_ctx, "OVR_multiview2")) {
			print_verbose("Failed to enable WebXR extension.");
		}
		RasterizerGLES3::make_current(false);

	} else {
		OS::alert(
				"Your browser seems not to support WebGL 2.\n\n"
				"If possible, consider updating your browser version and video card drivers.",
				"Unable to initialize WebGL 2 video driver");
		RasterizerDummy::make_current();
	}
#else
	RasterizerDummy::make_current();
#endif

	// JS Input interface (js/libs/library_godot_input.js)
	godot_js_input_mouse_button_cb(&DisplayServerWeb::mouse_button_callback);
	godot_js_input_mouse_move_cb(&DisplayServerWeb::mouse_move_callback);
	godot_js_input_mouse_wheel_cb(&DisplayServerWeb::mouse_wheel_callback);
	godot_js_input_touch_cb(&DisplayServerWeb::touch_callback, touch_event.identifier, touch_event.coords);
	godot_js_input_key_cb(&DisplayServerWeb::key_callback, key_event.code, key_event.key);
	godot_js_input_paste_cb(&DisplayServerWeb::update_clipboard_callback);
	godot_js_input_drop_files_cb(&DisplayServerWeb::drop_files_js_callback);
	godot_js_input_gamepad_cb(&DisplayServerWeb::gamepad_callback);
	godot_js_set_ime_cb(&DisplayServerWeb::ime_callback, &DisplayServerWeb::key_callback, key_event.code, key_event.key);

	// JS Display interface (js/libs/library_godot_display.js)
	godot_js_display_fullscreen_cb(&DisplayServerWeb::fullscreen_change_callback);
	godot_js_display_window_blur_cb(&DisplayServerWeb::window_blur_callback);
	godot_js_display_notification_cb(&DisplayServerWeb::send_window_event_callback,
			DisplayServerEnums::WINDOW_EVENT_MOUSE_ENTER,
			DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT,
			DisplayServerEnums::WINDOW_EVENT_FOCUS_IN,
			DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT);
	godot_js_display_vk_cb(&DisplayServerWeb::vk_input_text_callback);

	Input::set_event_dispatch_function(_dispatch_input_event);
}

DisplayServerWeb::~DisplayServerWeb() {
	NativeMenu::finalize();
#ifdef GLES3_ENABLED
	if (webgl_ctx) {
		emscripten_webgl_commit_frame();
		emscripten_webgl_destroy_context(webgl_ctx);
	}
#endif
}

bool DisplayServerWeb::has_feature(DisplayServerEnums::Feature p_feature) const {
	switch (p_feature) {
#ifndef DISABLE_DEPRECATED
		case DisplayServerEnums::FEATURE_GLOBAL_MENU: {
			return NativeMenu::is_initialized() && NativeMenu::has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
		} break;
#endif
		//case DisplayServerEnums::FEATURE_HIDPI:
		case DisplayServerEnums::FEATURE_ICON:
		case DisplayServerEnums::FEATURE_CLIPBOARD:
		case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
		case DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE:
		case DisplayServerEnums::FEATURE_MOUSE:
		case DisplayServerEnums::FEATURE_TOUCHSCREEN:
			return true;
		//case DisplayServerEnums::FEATURE_MOUSE_WARP:
		//case DisplayServerEnums::FEATURE_NATIVE_DIALOG:
		//case DisplayServerEnums::FEATURE_NATIVE_DIALOG_INPUT:
		//case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE:
		//case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA:
		//case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_MIME:
		//case DisplayServerEnums::FEATURE_NATIVE_ICON:
		//case DisplayServerEnums::FEATURE_WINDOW_TRANSPARENCY:
		//case DisplayServerEnums::FEATURE_KEEP_SCREEN_ON:
		//case DisplayServerEnums::FEATURE_ORIENTATION:
		case DisplayServerEnums::FEATURE_IME:
			// IME does not work with experimental VK support.
			return godot_js_display_vk_available() == 0;
		case DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD:
			return godot_js_display_vk_available() != 0;
		case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH:
			return godot_js_display_tts_available() != 0;
		default:
			return false;
	}
}

void DisplayServerWeb::register_web_driver() {
	DisplayServer::register_create_function("web", create_func, get_rendering_drivers_func);
}

String DisplayServerWeb::get_name() const {
	return "web";
}

int DisplayServerWeb::get_screen_count() const {
	return 1;
}

int DisplayServerWeb::get_primary_screen() const {
	return 0;
}

Point2i DisplayServerWeb::screen_get_position(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Point2i());

	return Point2i(0, 0); // TODO offsetX/Y?
}

Size2i DisplayServerWeb::screen_get_size(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Size2i());

	int size[2];
	godot_js_display_screen_size_get(size, size + 1);
	return Size2(size[0], size[1]);
}

Rect2i DisplayServerWeb::screen_get_usable_rect(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	int size[2];
	godot_js_display_window_size_get(size, size + 1);
	return Rect2i(0, 0, size[0], size[1]);
}

int DisplayServerWeb::screen_get_dpi(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 72);

	return godot_js_display_screen_dpi_get();
}

float DisplayServerWeb::screen_get_scale(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 1.0f);

	return godot_js_display_pixel_ratio_get();
}

float DisplayServerWeb::screen_get_refresh_rate(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, SCREEN_REFRESH_RATE_FALLBACK);

	return SCREEN_REFRESH_RATE_FALLBACK; // Web doesn't have much of a need for the screen refresh rate, and there's no native way to do so.
}

Vector<DisplayServerEnums::WindowID> DisplayServerWeb::get_window_list() const {
	Vector<DisplayServerEnums::WindowID> ret;
	ret.push_back(DisplayServerEnums::MAIN_WINDOW_ID);
	return ret;
}

DisplayServerEnums::WindowID DisplayServerWeb::get_window_at_screen_position(const Point2i &p_position) const {
	return DisplayServerEnums::MAIN_WINDOW_ID;
}

void DisplayServerWeb::window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window) {
	window_attached_instance_id = p_instance;
}

ObjectID DisplayServerWeb::window_get_attached_instance_id(DisplayServerEnums::WindowID p_window) const {
	return window_attached_instance_id;
}

void DisplayServerWeb::window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	rect_changed_callback = p_callable;
}

void DisplayServerWeb::window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	window_event_callback = p_callable;
}

void DisplayServerWeb::window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	input_event_callback = p_callable;
}

void DisplayServerWeb::window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	input_text_callback = p_callable;
}

void DisplayServerWeb::window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	drop_files_callback = p_callable;
}

void DisplayServerWeb::window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window) {
	godot_js_display_window_title_set(p_title.utf8().get_data());
}

int DisplayServerWeb::window_get_current_screen(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, DisplayServerEnums::INVALID_SCREEN);
	return 0;
}

void DisplayServerWeb::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window) {
	// Not implemented.
}

Point2i DisplayServerWeb::window_get_position(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

Point2i DisplayServerWeb::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

void DisplayServerWeb::window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window) {
	// Not supported.
}

void DisplayServerWeb::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) {
	// Not supported.
}

void DisplayServerWeb::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported.
}

Size2i DisplayServerWeb::window_get_max_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerWeb::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported.
}

Size2i DisplayServerWeb::window_get_min_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerWeb::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	godot_js_display_desired_size_set(p_size.x, p_size.y);
}

Size2i DisplayServerWeb::window_get_size(DisplayServerEnums::WindowID p_window) const {
	int size[2];
	godot_js_display_window_size_get(size, size + 1);
	return Size2i(size[0], size[1]);
}

Size2i DisplayServerWeb::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return window_get_size(p_window);
}

void DisplayServerWeb::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window) {
	if (window_mode == p_mode) {
		return;
	}

	switch (p_mode) {
		case DisplayServerEnums::WINDOW_MODE_WINDOWED: {
			if (window_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN) {
				godot_js_display_fullscreen_exit();
			}
			window_mode = DisplayServerEnums::WINDOW_MODE_WINDOWED;
		} break;
		case DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN:
		case DisplayServerEnums::WINDOW_MODE_FULLSCREEN: {
			int result = godot_js_display_fullscreen_request();
			ERR_FAIL_COND_MSG(result, "The request was denied. Remember that enabling fullscreen is only possible from an input callback for the Web platform.");
		} break;
		case DisplayServerEnums::WINDOW_MODE_MAXIMIZED:
		case DisplayServerEnums::WINDOW_MODE_MINIMIZED:
			// DisplayServerEnums::WindowMode MAXIMIZED and MINIMIZED are not supported in Web platform.
			break;
		default:
			break;
	}
}

DisplayServerEnums::WindowMode DisplayServerWeb::window_get_mode(DisplayServerEnums::WindowID p_window) const {
	return window_mode;
}

bool DisplayServerWeb::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window) const {
	return false;
}

void DisplayServerWeb::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window) {
	// Not supported.
}

bool DisplayServerWeb::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window) const {
	return false;
}

void DisplayServerWeb::window_request_attention(DisplayServerEnums::WindowID p_window) {
	// Not supported.
}

void DisplayServerWeb::window_move_to_foreground(DisplayServerEnums::WindowID p_window) {
	// Not supported.
}

bool DisplayServerWeb::window_is_focused(DisplayServerEnums::WindowID p_window) const {
	return true;
}

bool DisplayServerWeb::window_can_draw(DisplayServerEnums::WindowID p_window) const {
	return true;
}

bool DisplayServerWeb::can_any_window_draw() const {
	return true;
}

DisplayServerEnums::VSyncMode DisplayServerWeb::window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const {
	return DisplayServerEnums::VSYNC_ENABLED;
}

void DisplayServerWeb::process_events() {
	process_keys();
	Input::flush_buffered_events();

	if (gamepad_count > 0) {
		if (godot_js_input_gamepad_sample() == OK) {
			process_joypads();
		}
	}
}

void DisplayServerWeb::process_keys() {
	for (int i = 0; i < key_event_pos; i++) {
		const DisplayServerWeb::KeyEvent &ke = key_event_buffer[i];

		Ref<InputEventKey> ev;
		ev.instantiate();
		ev->set_pressed(ke.pressed);
		ev->set_echo(ke.echo);
		ev->set_keycode(ke.keycode);
		ev->set_physical_keycode(ke.physical_keycode);
		ev->set_key_label(ke.key_label);
		ev->set_unicode(ke.unicode);
		ev->set_location(ke.location);
		if (ke.raw) {
			dom2godot_mod(ev, ke.mod, ke.keycode);
		}

		Input::parse_input_event(ev);
	}
	key_event_pos = 0;
}

int DisplayServerWeb::get_current_video_driver() const {
	return 1;
}

bool DisplayServerWeb::get_swap_cancel_ok() {
	return swap_cancel_ok;
}

void DisplayServerWeb::swap_buffers() {
#ifdef GLES3_ENABLED
	if (webgl_ctx) {
		emscripten_webgl_commit_frame();
	}
#endif
}
