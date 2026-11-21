/**************************************************************************/
/*  audio_server.h                                                        */
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

#include <atomic>
#include "core/math/audio_frame.h"
#include "core/templates/safe_list.h"
#include "servers/audio/audio_effect.h"
// --- Folded from servers/audio/audio_filter_sw.h ---
#include "core/typedefs.h"

class AudioFilterSW {
public:
	struct Coeffs {
		double a1 = 0.0;
		double a2 = 0.0;
		double b0 = 0.0;
		double b1 = 0.0;
		double b2 = 0.0;
	};

	enum Mode {
		BANDPASS,
		HIGHPASS,
		LOWPASS,
		NOTCH,
		PEAK,
		BANDLIMIT,
		LOWSHELF,
		HIGHSHELF
	};

	class Processor { // Simple filter processor.
		AudioFilterSW *filter = nullptr;
		Coeffs coeffs;
		// History.
		float ha1 = 0.0f;
		float ha2 = 0.0f;
		float hb1 = 0.0f;
		float hb2 = 0.0f;
		Coeffs incr_coeffs;

	public:
		void set_filter(AudioFilterSW *p_filter, bool p_clear_history = true);
		void process(float *p_samples, int p_amount, int p_stride = 1, bool p_interpolate = false);
		void update_coeffs(int p_interp_buffer_len = 0);
		_ALWAYS_INLINE_ void process_one(float &p_sample);
		_ALWAYS_INLINE_ void process_one_interp(float &p_sample);

		Processor();
	};

private:
	float cutoff = 5000.0f;
	float resonance = 0.5f;
	float gain = 1.0f;
	float sampling_rate = 44100.0f;
	int stages = 1;
	Mode mode = LOWPASS;

public:
	float get_response(float p_freq, Coeffs *p_coeffs);

	void set_mode(Mode p_mode);
	void set_cutoff(float p_cutoff);
	void set_resonance(float p_resonance);
	void set_gain(float p_gain);
	void set_sampling_rate(float p_srate);
	void set_stages(int p_stages); //adjust for multiple stages

	void prepare_coefficients(Coeffs *p_coeffs);

	AudioFilterSW() {}
};

/* inline methods */

void AudioFilterSW::Processor::process_one(float &p_sample) {
	float pre = p_sample;
	p_sample = (p_sample * coeffs.b0 + hb1 * coeffs.b1 + hb2 * coeffs.b2 + ha1 * coeffs.a1 + ha2 * coeffs.a2);
	ha2 = ha1;
	hb2 = hb1;
	hb1 = pre;
	ha1 = p_sample;
}

void AudioFilterSW::Processor::process_one_interp(float &p_sample) {
	float pre = p_sample;
	p_sample = (p_sample * coeffs.b0 + hb1 * coeffs.b1 + hb2 * coeffs.b2 + ha1 * coeffs.a1 + ha2 * coeffs.a2);
	ha2 = ha1;
	hb2 = hb1;
	hb1 = pre;
	ha1 = p_sample;

	coeffs.b0 += incr_coeffs.b0;
	coeffs.b1 += incr_coeffs.b1;
	coeffs.b2 += incr_coeffs.b2;
	coeffs.a1 += incr_coeffs.a1;
	coeffs.a2 += incr_coeffs.a2;
}

// --- End servers/audio/audio_filter_sw.h ---

class AudioDriverDummy;
class AudioSample;
class AudioStream;
class AudioStreamWAV;
class AudioStreamPlayback;
class AudioSamplePlayback;

class AudioDriver
{
	uint64_t _last_mix_time = 0;
	uint64_t _last_mix_frames = 0;

#ifdef DEBUG_ENABLED
	SafeNumeric<uint64_t> prof_ticks;
	SafeNumeric<uint64_t> prof_time;
#endif

protected:
	Vector<int32_t> input_buffer;
	unsigned int input_position = 0;
	unsigned int input_size = 0;

	void audio_server_process(int p_frames, int32_t* p_buffer, bool p_update_mix_time = true);
	void update_mix_time(int p_frames);
	void input_buffer_init(int driver_buffer_frames);
	void input_buffer_write(int32_t sample);

	int _get_configured_mix_rate();

#ifdef DEBUG_ENABLED
	void start_counting_ticks();
	void stop_counting_ticks();
#else
	_FORCE_INLINE_ void start_counting_ticks() {}

	_FORCE_INLINE_ void stop_counting_ticks() {}
#endif

public:
	double get_time_since_last_mix(); // useful for video -> audio sync
	double get_time_to_next_mix();

	enum SpeakerMode
	{
		SPEAKER_MODE_STEREO,
		SPEAKER_SURROUND_31,
		SPEAKER_SURROUND_51,
		SPEAKER_SURROUND_71,
	};

	// Virtual API to implement.

	virtual const char* get_name() const = 0;

	virtual Error init() = 0;
	virtual void start() = 0;
	virtual int get_mix_rate() const = 0;

	virtual int get_input_mix_rate() const { return get_mix_rate(); }

	virtual SpeakerMode get_speaker_mode() const = 0;

	virtual float get_latency() { return 0; }

	virtual void lock() = 0;
	virtual void unlock() = 0;
	virtual void finish() = 0;

	virtual PackedStringArray get_output_device_list();
	virtual String get_output_device();

	virtual void set_output_device(const String& p_name) {}

	virtual Error input_start() { return FAILED; }

	virtual Error input_stop() { return FAILED; }

	virtual PackedStringArray get_input_device_list();

	virtual String get_input_device() { return "Default"; }

	virtual void set_input_device(const String& p_name) {}

	//

	SpeakerMode get_speaker_mode_by_total_channels(int p_channels) const;
	int get_total_channels_by_speaker_mode(SpeakerMode) const;

	Vector<int32_t> get_input_buffer() { return input_buffer; }

	unsigned int get_input_position() { return input_position; }

	unsigned int get_input_size() { return input_size; }

#ifdef DEBUG_ENABLED
	uint64_t get_profiling_time() const { return prof_time.get(); }

	void reset_profiling_time() { prof_time.set(0); }
#endif

	// Samples handling.
	virtual bool is_stream_registered_as_sample(const Ref<AudioStream>& p_stream) const
	{
		return false;
	}

	virtual void register_sample(const Ref<AudioSample>& p_sample) {}

	virtual void unregister_sample(const Ref<AudioSample>& p_sample) {}

	virtual void start_sample_playback(const Ref<AudioSamplePlayback>& p_playback) {}

	virtual void stop_sample_playback(const Ref<AudioSamplePlayback>& p_playback) {}

	virtual void set_sample_playback_pause(
		const Ref<AudioSamplePlayback>& p_playback, bool p_paused)
	{
	}

	virtual bool is_sample_playback_active(const Ref<AudioSamplePlayback>& p_playback)
	{
		return false;
	}

	virtual double get_sample_playback_position(const Ref<AudioSamplePlayback>& p_playback)
	{
		return false;
	}

	virtual void update_sample_playback_pitch_scale(
		const Ref<AudioSamplePlayback>& p_playback, float p_pitch_scale = 0.0f)
	{
	}

	virtual void set_sample_playback_bus_volumes_linear(const Ref<AudioSamplePlayback>& p_playback,
		const HashMap<String, Vector<AudioFrame>>& p_bus_volumes)
	{
	}

	virtual void set_sample_bus_count(int p_count) {}

	virtual void remove_sample_bus(int p_bus) {}

	virtual void add_sample_bus(int p_at_pos = -1) {}

	virtual void move_sample_bus(int p_bus, int p_to_pos) {}

	virtual void set_sample_bus_send(int p_bus, const String& p_send) {}

	virtual void set_sample_bus_volume_db(int p_bus, float p_volume_db) {}

	virtual void set_sample_bus_solo(int p_bus, bool p_enable) {}

	virtual void set_sample_bus_mute(int p_bus, bool p_enable) {}

	AudioDriver() {}

	virtual ~AudioDriver() {}
};

class AudioDriverManager final
{
	enum
	{
		MAX_DRIVERS = 10
	};

	static AudioDriver* drivers[MAX_DRIVERS];
	static int driver_count;

	static AudioDriverDummy dummy_driver;
	static AudioDriver* active_driver;

public:
	static const int DEFAULT_MIX_RATE = 44100;

	static void add_driver(AudioDriver* p_driver);
	static void initialize(int p_driver);
	static int get_driver_count();
	static AudioDriver* get_driver(int p_driver);

	_FORCE_INLINE_ static AudioDriver* get_active_driver() { return active_driver; }
	static void set_active_driver(AudioDriver* p_driver);
	_FORCE_INLINE_ static AudioDriverDummy* get_dummy_driver() { return &dummy_driver; }

	AudioDriverManager() = delete;
};

class AudioBusLayout;

class AudioServer final
{
public:
	// re-expose this here, as AudioDriver is not exposed to script
	enum SpeakerMode
	{
		SPEAKER_MODE_STEREO,
		SPEAKER_SURROUND_31,
		SPEAKER_SURROUND_51,
		SPEAKER_SURROUND_71,
	};

	enum PlaybackType
	{
		PLAYBACK_TYPE_DEFAULT,
		PLAYBACK_TYPE_STREAM,
		PLAYBACK_TYPE_SAMPLE,
		PLAYBACK_TYPE_MAX
	};

	enum
	{
		AUDIO_DATA_INVALID_ID = -1,
		MAX_CHANNELS_PER_BUS = 4,
		MAX_BUSES_PER_PLAYBACK = 6,
		LOOKAHEAD_BUFFER_SIZE = 64,
	};

	typedef void (*AudioCallback)(void* p_userdata);

private:

	struct Bus
	{
		String name;
		bool solo = false;
		bool mute = false;
		bool bypass = false;

		bool soloed = false;

		// Each channel is a stereo pair.
		struct Channel
		{
			bool used = false;
			bool active = false;
			AudioFrame peak_volume = AudioFrame(AUDIO_MIN_PEAK_DB, AUDIO_MIN_PEAK_DB);
			Vector<AudioFrame> buffer;
			Vector<Ref<AudioEffectInstance>> effect_instances;
			uint64_t last_mix_with_audio = 0;

			Channel() {}
		};

		Vector<Channel> channels;

		struct Effect
		{
			Ref<AudioEffect> effect;
			bool enabled = false;
#ifdef DEBUG_ENABLED
			uint64_t prof_time = 0;
#endif
		};

		Vector<Effect> effects;
		float volume_db = 0.0f;
		String send;
		int index_cache = 0;
	};

	struct AudioStreamPlaybackBusDetails
	{
		bool bus_active[MAX_BUSES_PER_PLAYBACK] = {};
		String bus[MAX_BUSES_PER_PLAYBACK];
		AudioFrame volume[MAX_BUSES_PER_PLAYBACK][MAX_CHANNELS_PER_BUS];
	};

	struct AudioStreamPlaybackListNode
	{
		// The state machine for audio stream playbacks is as follows:
		// 1. The playback is created and added to the playback list in the playing state.
		// 2. The playback is (maybe) paused, and the state is set to FADE_OUT_TO_PAUSE.
		// 2.1. The playback is mixed after being paused, and the audio server thread atomically
		// sets the state to PAUSED after performing a brief fade-out.
		// 3. The playback is (maybe) deleted, and the state is set to FADE_OUT_TO_DELETION.
		// 3.1. The playback is mixed after being deleted, and the audio server thread atomically
		// sets the state to AWAITING_DELETION after performing a brief fade-out. 		NOTE: The playback
		// is not deallocated at this time because allocation and deallocation are not
		// realtime-safe.
		// 4. The playback is removed and deallocated on the main thread using the SafeList
		// maybe_cleanup method.
		enum PlaybackState
		{
			PAUSED = 0,	 // Paused. Keep this stream playback around though so it can be restarted.
			PLAYING = 1, // Playing. Fading may still be necessary if volume changes!
			FADE_OUT_TO_PAUSE = 2,	  // About to pause.
			FADE_OUT_TO_DELETION = 3, // About to stop.
			AWAITING_DELETION = 4,
		};

		// If zero or positive, a place in the stream to seek to during the next mix.
		SafeNumeric<float> setseek;
		SafeNumeric<float> pitch_scale;
		SafeNumeric<float> highshelf_gain;
		SafeNumeric<float>
			attenuation_filter_cutoff_hz; // This isn't used unless highshelf_gain is nonzero.
		AudioFilterSW::Processor filter_process[8];
		// Updating this ref after the list node is created breaks consistency guarantees, don't do
		// it!
		Ref<AudioStreamPlayback> stream_playback;
		// Playback state determines the fate of a particular AudioStreamListNode during the mix
		// step. Must be atomically replaced.
		std::atomic<PlaybackState> state = AWAITING_DELETION;
		// This data should only ever be modified by an atomic replacement of the pointer.
		std::atomic<AudioStreamPlaybackBusDetails*> bus_details = nullptr;
		// Previous bus details should only be accessed on the audio thread.
		AudioStreamPlaybackBusDetails* prev_bus_details = nullptr;
		// The next few samples are stored here so we have some time to fade audio out if it ends
		// abruptly at the beginning of the next mix.
		AudioFrame lookahead[LOOKAHEAD_BUFFER_SIZE];
	};

	struct CallbackItem
	{
		AudioCallback callback;
		void* userdata = nullptr;
	};

	struct Data
	{
		uint64_t mix_time = 0;
		int mix_size = 0;

		uint32_t buffer_size = 0;
		uint64_t mix_count = 0;
		uint64_t mix_frames = 0;
#ifdef DEBUG_ENABLED
		SafeNumeric<uint64_t> prof_time;
#endif

		float channel_disable_threshold_db = 0.0f;
		uint32_t channel_disable_frames = 0;

		int channel_count = 0;
		int to_mix = 0;

		float playback_speed_scale = 1.0f;

		bool tag_used_audio_streams = false;

#ifdef DEBUG_ENABLED
		bool debug_mute = false;
#endif // DEBUG_ENABLED

		bool input_device_active = false;
		int input_buffer_ofs = 0;

		SafeList<AudioStreamPlaybackListNode*> playback_list;
		SafeList<AudioStreamPlaybackBusDetails*> bus_details_graveyard;
		SafeList<AudioStreamPlaybackBusDetails*> bus_details_graveyard_frame_old;

		Vector<Vector<AudioFrame>> temp_buffer; // temp_buffer for each level
		Vector<AudioFrame> mix_buffer;
		Vector<Bus*> buses;
		HashMap<String, Bus*> bus_map;

		SafeList<CallbackItem*> update_callback_list;
		SafeList<CallbackItem*> mix_callback_list;
		SafeList<CallbackItem*> listener_changed_callback_list;

		LocalVector<Ref<AudioSamplePlayback>> sample_playback_list;
	};

	static inline Data* data = nullptr;

	static void _delete_stream_playback(Ref<AudioStreamPlayback> p_playback);
	static void _delete_stream_playback_list_node(AudioStreamPlaybackListNode* p_node);

	static void _cleanup_lists();
	static void _update_bus_effects(int p_bus);
	static void init_channels_and_buffers();

	static void _mix_step();
	static void _mix_step_for_channel(AudioFrame* p_out_buf, AudioFrame* p_source_buf,
		AudioFrame p_vol_start, AudioFrame p_vol_final, float p_attenuation_filter_cutoff_hz,
		float p_highshelf_gain, AudioFilterSW::Processor* p_processor_l,
		AudioFilterSW::Processor* p_processor_r);

	// Should only be called on the main thread.
	static AudioStreamPlaybackListNode* _find_playback_list_node(Ref<AudioStreamPlayback> p_playback);

	friend class AudioDriver;
	static void _driver_process(int p_frames, int32_t* p_buffer);

protected:

public:
	static _FORCE_INLINE_ int get_channel_count()
	{
		switch (get_speaker_mode()) {
		case SPEAKER_MODE_STEREO:
			return 1;
		case SPEAKER_SURROUND_31:
			return 2;
		case SPEAKER_SURROUND_51:
			return 3;
		case SPEAKER_SURROUND_71:
			return 4;
		}
		ERR_FAIL_V(1);
	}

	// Do not use from outside audio thread.
	static bool thread_has_channel_mix_buffer(int p_bus, int p_buffer);
	static AudioFrame* thread_get_channel_mix_buffer(int p_bus, int p_buffer);
	static int thread_get_mix_buffer_size();
	static int thread_find_bus_index(const String& p_name);

#ifdef DEBUG_ENABLED
	static void set_debug_mute(bool p_mute);
	static bool get_debug_mute();
#endif // DEBUG_ENABLED

	static void set_bus_count(int p_count);
	static int get_bus_count();

	static void remove_bus(int p_index);
	static void add_bus(int p_at_pos = -1);

	static void move_bus(int p_bus, int p_to_pos);

	static void set_bus_name(int p_bus, const String& p_name);
	static String get_bus_name(int p_bus);
	static int get_bus_index(const String& p_bus_name);

	static int get_bus_channels(int p_bus);

	static void set_bus_volume_db(int p_bus, float p_volume_db);
	static float get_bus_volume_db(int p_bus);

	static void set_bus_volume_linear(int p_bus, float p_volume_linear);
	static float get_bus_volume_linear(int p_bus);

	static void set_bus_send(int p_bus, const String& p_send);
	static String get_bus_send(int p_bus);

	static void set_bus_solo(int p_bus, bool p_enable);
	static bool is_bus_solo(int p_bus);

	static void set_bus_mute(int p_bus, bool p_enable);
	static bool is_bus_mute(int p_bus);

	static void set_bus_bypass_effects(int p_bus, bool p_enable);
	static bool is_bus_bypassing_effects(int p_bus);

	static void add_bus_effect(int p_bus, const Ref<AudioEffect>& p_effect, int p_at_pos = -1);
	static void remove_bus_effect(int p_bus, int p_effect);

	static int get_bus_effect_count(int p_bus);
	static Ref<AudioEffect> get_bus_effect(int p_bus, int p_effect);
	static Ref<AudioEffectInstance> get_bus_effect_instance(int p_bus, int p_effect, int p_channel = 0);

	static void swap_bus_effects(int p_bus, int p_effect, int p_by_effect);

	static void set_bus_effect_enabled(int p_bus, int p_effect, bool p_enabled);
	static bool is_bus_effect_enabled(int p_bus, int p_effect);

	static float get_bus_peak_volume_left_db(int p_bus, int p_channel);
	static float get_bus_peak_volume_right_db(int p_bus, int p_channel);

	static bool is_bus_channel_active(int p_bus, int p_channel);

	static void set_playback_speed_scale(float p_scale);
	static float get_playback_speed_scale();

	// Convenience method.
	static void start_playback_stream(Ref<AudioStreamPlayback> p_playback, const String& p_bus,
		Vector<AudioFrame> p_volume_db_vector, float p_start_time = 0, float p_pitch_scale = 1);
	// Expose all parameters.
	static void start_playback_stream(Ref<AudioStreamPlayback> p_playback,
		const HashMap<String, Vector<AudioFrame>>& p_bus_volumes, float p_start_time = 0,
		float p_pitch_scale = 1, float p_highshelf_gain = 0, float p_attenuation_cutoff_hz = 0);
	static void stop_playback_stream(Ref<AudioStreamPlayback> p_playback);

	static void set_playback_bus_exclusive(
		Ref<AudioStreamPlayback> p_playback, const String& p_bus, Vector<AudioFrame> p_volumes);
	static void set_playback_bus_volumes_linear(Ref<AudioStreamPlayback> p_playback,
		const HashMap<String, Vector<AudioFrame>>& p_bus_volumes);
	static void set_playback_all_bus_volumes_linear(
		Ref<AudioStreamPlayback> p_playback, Vector<AudioFrame> p_volumes);
	static void set_playback_pitch_scale(Ref<AudioStreamPlayback> p_playback, float p_pitch_scale);
	static void set_playback_paused(Ref<AudioStreamPlayback> p_playback, bool p_paused);
	static void set_playback_highshelf_params(
		Ref<AudioStreamPlayback> p_playback, float p_gain, float p_attenuation_cutoff_hz);

	static bool is_playback_active(Ref<AudioStreamPlayback> p_playback);
	static float get_playback_position(Ref<AudioStreamPlayback> p_playback);
	static bool is_playback_paused(Ref<AudioStreamPlayback> p_playback);

	static uint64_t get_mix_count();
	static uint64_t get_mixed_frames();

	static String get_driver_name();

	static void notify_listener_changed();

	static void init();
	static void finish();
	static void update();
	static void load_default_bus_layout();

	/* MISC config */

	static void lock();
	static void unlock();

	static SpeakerMode get_speaker_mode();
	static float get_mix_rate();
	static float get_input_mix_rate();

	static float read_output_peak_db();

	static double get_output_latency();
	static double get_time_to_next_mix();
	static double get_time_since_last_mix();

	static void add_listener_changed_callback(AudioCallback p_callback, void* p_userdata);
	static void remove_listener_changed_callback(AudioCallback p_callback, void* p_userdata);

	static void add_update_callback(AudioCallback p_callback, void* p_userdata);
	static void remove_update_callback(AudioCallback p_callback, void* p_userdata);

	static void add_mix_callback(AudioCallback p_callback, void* p_userdata);
	static void remove_mix_callback(AudioCallback p_callback, void* p_userdata);

	static void set_bus_layout(const Ref<AudioBusLayout>& p_bus_layout);
	static Ref<AudioBusLayout> generate_bus_layout();

	static PackedStringArray get_output_device_list();
	static String get_output_device();
	static void set_output_device(const String& p_name);

	static PackedStringArray get_input_device_list();
	static String get_input_device();
	static void set_input_device(const String& p_name);
	static Error set_input_device_active(bool p_is_active);
	static int get_input_frames_available();
	static int get_input_buffer_length_frames();
	static PackedVector2Array get_input_frames(int p_frames);

	static void set_enable_tagging_used_audio_streams(bool p_enable);

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);
#endif

	static PlaybackType get_default_playback_type();

	static bool is_stream_registered_as_sample(const Ref<AudioStream>& p_stream);
	static void register_stream_as_sample(const Ref<AudioStream>& p_stream);
	static void unregister_stream_as_sample(const Ref<AudioStream>& p_stream);
	static void register_sample(const Ref<AudioSample>& p_sample);
	static void unregister_sample(const Ref<AudioSample>& p_sample);
	static void start_sample_playback(const Ref<AudioSamplePlayback>& p_playback);
	static void stop_sample_playback(const Ref<AudioSamplePlayback>& p_playback);
	static void set_sample_playback_pause(const Ref<AudioSamplePlayback>& p_playback, bool p_paused);
	static bool is_sample_playback_active(const Ref<AudioSamplePlayback>& p_playback);
	static double get_sample_playback_position(const Ref<AudioSamplePlayback>& p_playback);
	static void update_sample_playback_pitch_scale(
		const Ref<AudioSamplePlayback>& p_playback, float p_pitch_scale = 0.0f);

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	AudioServer() = delete;
	AudioServer(const AudioServer&) = delete;
	AudioServer& operator=(const AudioServer&) = delete;
	~AudioServer() = delete;
};

class AudioBusLayout : public Resource
{
	friend class AudioServer;

	struct Bus
	{
		String name;
		bool solo = false;
		bool mute = false;
		bool bypass = false;

		struct Effect
		{
			Ref<AudioEffect> effect;
			bool enabled = false;
		};

		Vector<Effect> effects;

		float volume_db = 0.0f;
		String send;

		Bus() {}
	};
	Vector<Bus> buses;

public:
	AudioBusLayout();
};

// --- Folded from servers/audio/audio_driver_dummy.h ---
#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"


class AudioDriverDummy : public AudioDriver {
	Thread thread;
	Mutex mutex;

	int32_t *samples_in = nullptr;

	static void thread_func(void *p_udata);

	uint32_t buffer_frames = 4096;
	int32_t mix_rate = -1;
	SpeakerMode speaker_mode = SPEAKER_MODE_STEREO;

	int channels;

	SafeFlag active;
	SafeFlag exit_thread;

	bool use_threads = true;

public:
	virtual const char *get_name() const override {
		return "Dummy";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

	void set_use_threads(bool p_use_threads);
	void set_speaker_mode(SpeakerMode p_mode);
	void set_mix_rate(int p_rate);

	uint32_t get_channels() const;

	void mix_audio(int p_frames, int32_t *p_buffer);

	AudioDriverDummy();
	~AudioDriverDummy() {}
};

// --- End servers/audio/audio_driver_dummy.h ---

// --- Folded from servers/audio/audio_rb_resampler.h ---
#include "core/math/audio_frame.h"
#include "core/templates/safe_refcount.h"
#include "core/typedefs.h"

struct AudioRBResampler {
	uint32_t rb_bits;
	uint32_t rb_len;
	uint32_t rb_mask;
	uint32_t read_buff_len;
	uint32_t channels;
	uint32_t src_mix_rate;
	uint32_t target_mix_rate;
	double playback_speed = 1.0;

	SafeNumeric<int> rb_read_pos;
	SafeNumeric<int> rb_write_pos;

	int32_t offset; //contains the fractional remainder of the resampler
	enum {
		MIX_FRAC_BITS = 13,
		MIX_FRAC_LEN = (1 << MIX_FRAC_BITS),
		MIX_FRAC_MASK = MIX_FRAC_LEN - 1,
	};

	float *read_buf = nullptr;
	float *rb = nullptr;

	template <int C>
	uint32_t _resample(AudioFrame *p_dest, int p_todo, int32_t p_increment);

public:
	_FORCE_INLINE_ void flush() {
		rb_read_pos.set(0);
		rb_write_pos.set(0);
		offset = 0;
	}

	_FORCE_INLINE_ bool is_ready() const {
		return rb != nullptr;
	}

	_FORCE_INLINE_ int get_total() const {
		return rb_len - 1;
	}

	_FORCE_INLINE_ int get_writer_space() const {
		int space, r, w;

		r = rb_read_pos.get();
		w = rb_write_pos.get();

		if (r == w) {
			space = rb_len - 1;
		} else if (w < r) {
			space = r - w - 1;
		} else {
			space = (rb_len - w) + (r - 1);
		}

		return space;
	}

	_FORCE_INLINE_ int get_reader_space() const {
		int space, r, w;

		r = rb_read_pos.get();
		w = rb_write_pos.get();

		if (r == w) {
			space = 0;
		} else if (w < r) {
			space = rb_len - r + w;
		} else {
			space = w - r;
		}

		return space;
	}

	_FORCE_INLINE_ bool has_data() const {
		return rb && rb_read_pos.get() != rb_write_pos.get();
	}

	_FORCE_INLINE_ float *get_write_buffer() { return read_buf; }
	_FORCE_INLINE_ void write(uint32_t p_frames) {
		ERR_FAIL_COND(p_frames >= rb_len);

		int wp = rb_write_pos.get();

		switch (channels) {
			case 1: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[wp] = read_buf[i];
					wp = (wp + 1) & rb_mask;
				}
			} break;
			case 2: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[(wp << 1) + 0] = read_buf[(i << 1) + 0];
					rb[(wp << 1) + 1] = read_buf[(i << 1) + 1];
					wp = (wp + 1) & rb_mask;
				}
			} break;
			case 4: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[(wp << 2) + 0] = read_buf[(i << 2) + 0];
					rb[(wp << 2) + 1] = read_buf[(i << 2) + 1];
					rb[(wp << 2) + 2] = read_buf[(i << 2) + 2];
					rb[(wp << 2) + 3] = read_buf[(i << 2) + 3];
					wp = (wp + 1) & rb_mask;
				}
			} break;
			case 6: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[(wp * 6) + 0] = read_buf[(i * 6) + 0];
					rb[(wp * 6) + 1] = read_buf[(i * 6) + 1];
					rb[(wp * 6) + 2] = read_buf[(i * 6) + 2];
					rb[(wp * 6) + 3] = read_buf[(i * 6) + 3];
					rb[(wp * 6) + 4] = read_buf[(i * 6) + 4];
					rb[(wp * 6) + 5] = read_buf[(i * 6) + 5];
					wp = (wp + 1) & rb_mask;
				}
			} break;
			case 8: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[(wp << 3) + 0] = read_buf[(i << 3) + 0];
					rb[(wp << 3) + 1] = read_buf[(i << 3) + 1];
					rb[(wp << 3) + 2] = read_buf[(i << 3) + 2];
					rb[(wp << 3) + 3] = read_buf[(i << 3) + 3];
					rb[(wp << 3) + 4] = read_buf[(i << 3) + 4];
					rb[(wp << 3) + 5] = read_buf[(i << 3) + 5];
					rb[(wp << 3) + 6] = read_buf[(i << 3) + 6];
					rb[(wp << 3) + 7] = read_buf[(i << 3) + 7];
					wp = (wp + 1) & rb_mask;
				}
			} break;
		}

		rb_write_pos.set(wp);
	}

	int get_channel_count() const;

	Error setup(int p_channels, int p_src_mix_rate, int p_target_mix_rate, int p_buffer_msec, int p_minbuff_needed = -1);
	void clear();
	bool mix(AudioFrame *p_dest, int p_frames);
	int get_num_of_ready_frames();
	void set_playback_speed(double p_playback_speed);
	double get_playback_speed() const;

	AudioRBResampler();
	~AudioRBResampler();
};

// --- End servers/audio/audio_rb_resampler.h ---
