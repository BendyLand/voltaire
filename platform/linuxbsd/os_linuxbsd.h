/**************************************************************************/
/*  os_linuxbsd.h                                                         */
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

#include "core/input/input_event.h"
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
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/audio/audio_server.h"

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
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "core/templates/vector.h"

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
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/audio/audio_server.h"

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
#include "drivers/unix/os_unix.h"

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

#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"

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
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/display/display_server_enums.h"
#include "core/types.h"

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
#include "core/os/thread.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
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
