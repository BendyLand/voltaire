#include "modules/modules_enabled.gen.h"

#if defined(MODULE_INTERACTIVE_MUSIC_ENABLED)
// --- Inlined from modules/interactive_music/audio_stream_interactive.h ---
#include "core/templates/local_vector.h"
#include "servers/audio/audio_stream.h"

class AudioStreamPlaybackInteractive;

class AudioStreamInteractive final : public AudioStream
{
public:
	enum Mode
	{
		MODE_CLIPS,
		MODE_PLAYLIST,
		MODE_SYNCHRONIZED,
	};

	enum TransitionFromTime
	{
		TRANSITION_FROM_TIME_IMMEDIATE,
		TRANSITION_FROM_TIME_NEXT_BEAT,
		TRANSITION_FROM_TIME_NEXT_BAR,
		TRANSITION_FROM_TIME_END,
		TRANSITION_FROM_TIME_MAX
	};

	enum TransitionToTime
	{
		TRANSITION_TO_TIME_SAME_POSITION,
		TRANSITION_TO_TIME_START,
		TRANSITION_TO_TIME_PREVIOUS_POSITION,
		TRANSITION_TO_TIME_MAX,
	};

	enum FadeMode
	{
		FADE_DISABLED,
		FADE_IN,
		FADE_OUT,
		FADE_CROSS,
		FADE_AUTOMATIC,
		FADE_MAX
	};

	enum AutoAdvanceMode
	{
		AUTO_ADVANCE_DISABLED,
		AUTO_ADVANCE_ENABLED,
		AUTO_ADVANCE_RETURN_TO_HOLD,
	};

	enum
	{
		CLIP_ANY = -1
	};

private:
	friend class AudioStreamPlaybackInteractive;

	enum
	{
		MAX_CLIPS = 63, // Because we use bitmasks for transition matching.
		MAX_TRANSITIONS = 63,
		MAX_STREAMS = 64,
		MAX_SYNC_STREAMS = 32,
	};

	struct Clip
	{
		String name;
		Ref<AudioStream> stream;

		AutoAdvanceMode auto_advance = AUTO_ADVANCE_DISABLED;
		int auto_advance_next_clip = 0;
	};

	struct Transition
	{
		TransitionFromTime from_time = TRANSITION_FROM_TIME_NEXT_BEAT;
		TransitionToTime to_time = TRANSITION_TO_TIME_START;
		FadeMode fade_mode = FADE_AUTOMATIC;
		float fade_beats = 1;
		bool use_filler_clip = false;
		int filler_clip = 0;
		bool hold_previous = false;
	};

	struct TransitionKey
	{
		uint32_t from_clip;
		uint32_t to_clip;

		bool operator==(const TransitionKey& p_key) const
		{
			return from_clip == p_key.from_clip && to_clip == p_key.to_clip;
		}

		TransitionKey(uint32_t p_from_clip = 0, uint32_t p_to_clip = 0)
		{
			from_clip = p_from_clip;
			to_clip = p_to_clip;
		}
	};

	struct TransitionKeyHasher
	{
		static _FORCE_INLINE_ uint32_t hash(const TransitionKey& p_key)
		{
			uint32_t h = hash_murmur3_one_32(p_key.from_clip);
			return hash_murmur3_one_32(p_key.to_clip, h);
		}
	};

	Mode mode = MODE_CLIPS;

	// Clips / transitions state (MODE_CLIPS)
	int sample_rate = 44100;
	bool stereo = true;
	int initial_clip = 0;
	int clip_count = 0;
	double time = 0;
	uint64_t version = 1; // Used to stop playback instances for incompatibility.
	LocalVector<Clip> clips;
	HashMap<TransitionKey, Transition, TransitionKeyHasher> transition_map;

	// Playlist & Synchronized state (MODE_PLAYLIST / MODE_SYNCHRONIZED)
	bool shuffle = false;
	bool loop = true;
	double fade_time = 0.3;
	int stream_count = 0;
	LocalVector<Ref<AudioStream>> audio_streams;
	LocalVector<float> audio_stream_volume_db;

	HashSet<AudioStreamPlaybackInteractive*> playbacks;

#ifdef TOOLS_ENABLED
	mutable String stream_name_cache;
	String _get_streams_hint() const;
#endif

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const;

	// CLIPS (MODE_CLIPS)
	void set_clip_count(int p_count);
	int get_clip_count() const;

	void set_initial_clip(int p_clip);
	int get_initial_clip() const;

	void set_clip_name(int p_clip, const String& p_name);
	String get_clip_name(int p_clip) const;

	void set_clip_stream(int p_clip, const Ref<AudioStream>& p_stream);
	Ref<AudioStream> get_clip_stream(int p_clip) const;

	void set_clip_auto_advance(int p_clip, AutoAdvanceMode p_mode);
	AutoAdvanceMode get_clip_auto_advance(int p_clip) const;

	void set_clip_auto_advance_next_clip(int p_clip, int p_index);
	int get_clip_auto_advance_next_clip(int p_clip) const;

	// TRANSITIONS (MODE_CLIPS)
	void add_transition(int p_from_clip, int p_to_clip, TransitionFromTime p_from_time,
		TransitionToTime p_to_time, FadeMode p_fade_mode, float p_fade_beats,
		bool p_use_filler_flip = false, int p_filler_clip = -1, bool p_hold_previous = false);
	TransitionFromTime get_transition_from_time(int p_from_clip, int p_to_clip) const;
	TransitionToTime get_transition_to_time(int p_from_clip, int p_to_clip) const;
	FadeMode get_transition_fade_mode(int p_from_clip, int p_to_clip) const;
	float get_transition_fade_beats(int p_from_clip, int p_to_clip) const;
	bool is_transition_using_filler_clip(int p_from_clip, int p_to_clip) const;
	int get_transition_filler_clip(int p_from_clip, int p_to_clip) const;
	bool is_transition_holding_previous(int p_from_clip, int p_to_clip) const;

	bool has_transition(int p_from_clip, int p_to_clip) const;
	void erase_transition(int p_from_clip, int p_to_clip);

	PackedInt32Array get_transition_list() const;

	// PLAYLIST & SYNCHRONIZED STREAMS
	void set_stream_count(int p_count);
	int get_stream_count() const;

	// PLAYLIST (MODE_PLAYLIST)
	void set_fade_time(float p_time);
	float get_fade_time() const;
	void set_shuffle(bool p_shuffle);
	bool get_shuffle() const;
	void set_loop(bool p_loop);
	void set_list_stream(int p_stream_index, Ref<AudioStream> p_stream);
	Ref<AudioStream> get_list_stream(int p_stream_index) const;

	// SYNCHRONIZED (MODE_SYNCHRONIZED)
	void set_sync_stream(int p_stream_index, Ref<AudioStream> p_stream);
	Ref<AudioStream> get_sync_stream(int p_stream_index) const;
	void set_sync_stream_volume(int p_stream_index, float p_db);
	float get_sync_stream_volume(int p_stream_index) const;

	// AudioStream overrides
	virtual double get_bpm() const override;
	virtual int get_beat_count() const override;
	virtual int get_bar_beats() const override;
	virtual bool has_loop() const override;
	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual double get_length() const override;
	virtual bool is_meta_stream() const override { return true; }

	AudioStreamInteractive();
};

class AudioStreamPlaybackInteractive final : public AudioStreamPlayback
{
	friend class AudioStreamInteractive;

private:
	enum
	{
		BUFFER_SIZE = 1024,
		MIX_BUFFER_SIZE = 128,
	};

	struct ClipState
	{
		Ref<AudioStream> stream;
		Ref<AudioStreamPlayback> playback;
		bool active = false;
		double fade_wait = 0; // Time to wait until fade kicks-in
		double fade_volume = 1.0;
		double fade_speed = 0; // Fade speed, negative or positive
		int auto_advance = -1;
		bool first_mix = true;
		double previous_position = 0;

		void reset_fade()
		{
			fade_wait = 0;
			fade_volume = 1.0;
			fade_speed = 0;
		}
	};

	struct ClipsPlayback
	{
		uint64_t version = 0;
		LocalVector<ClipState> states;
		int playback_current = -1;
		int return_memory = -1;
		int switch_request = -1;
	};

	struct PlaylistPlayback
	{
		LocalVector<Ref<AudioStreamPlayback>> playback;
		LocalVector<int> play_order;
		double stream_todo = 0.0;
		int fade_index = -1;
		double fade_volume = 1.0;
		int play_index = 0;
		double offset = 0.0;
		int loop_count = 0;
	};

	struct SyncPlayback
	{
		LocalVector<Ref<AudioStreamPlayback>> playback;
	};

	Ref<AudioStreamInteractive> stream;
	bool active = false;

	AudioFrame mix_buffer[BUFFER_SIZE];
	AudioFrame temp_buffer[BUFFER_SIZE];

	ClipsPlayback clips;
	PlaylistPlayback playlist;
	SyncPlayback sync;

	// Clips helpers
	void _mix_internal(int p_frames);
	void _mix_internal_state(int p_state_idx, int p_frames);
	void _queue(int p_to_clip_index, bool p_is_auto_advance);

	// Playlist / Synchronized helpers
	void _update_order();
	void _update_playback_instances();

	int _mix_clips(AudioFrame* p_buffer, float p_rate_scale, int p_frames);
	int _mix_playlist(AudioFrame* p_buffer, float p_rate_scale, int p_frames);
	int _mix_sync(AudioFrame* p_buffer, float p_rate_scale, int p_frames);

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;
	virtual int get_loop_count() const override;
	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;
	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) override;

	virtual void tag_used_streams() override;

	void switch_to_clip_by_name(const String& p_name);
	void switch_to_clip(int p_index);
	int get_current_clip_index() const;

	AudioStreamPlaybackInteractive() = default;
	~AudioStreamPlaybackInteractive();
};

// --- End modules/interactive_music/audio_stream_interactive.h ---
#include "modules/register_module_types.h"
#include "core/config/engine.h"
#include "core/math/math_funcs.h"

AudioStreamInteractive::AudioStreamInteractive() {}

void AudioStreamInteractive::set_mode(Mode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 3);
	if (mode == p_mode) {
		return;
	}

	AudioServer::lock();
	mode = p_mode;
	if (mode == MODE_SYNCHRONIZED && stream_count > MAX_SYNC_STREAMS) {
		stream_count = MAX_SYNC_STREAMS;
	}
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		if ((uint32_t)stream_count > audio_streams.size()) {
			audio_streams.resize(stream_count);
		}
		if (mode == MODE_SYNCHRONIZED && (uint32_t)stream_count > audio_stream_volume_db.size()) {
			uint32_t old_size = audio_stream_volume_db.size();
			audio_stream_volume_db.resize(stream_count);
			for (uint32_t i = old_size; i < (uint32_t)stream_count; i++) {
				audio_stream_volume_db[i] = 0.0f;
			}
		}
	}
	version++;
	for (AudioStreamPlaybackInteractive* E : playbacks) {
		E->stop();
		if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

AudioStreamInteractive::Mode AudioStreamInteractive::get_mode() const { return mode; }

Ref<AudioStreamPlayback> AudioStreamInteractive::instantiate_playback()
{
	Ref<AudioStreamPlaybackInteractive> pb;
	pb.instantiate();
	pb->stream = Ref<AudioStreamInteractive>(this);
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		pb->_update_playback_instances();
	}
	playbacks.insert(pb.ptr());
	return pb;
}

// CLIPS (MODE_CLIPS)

void AudioStreamInteractive::set_clip_count(int p_count)
{
	ERR_FAIL_COND(p_count < 0 || p_count > MAX_CLIPS);
	AudioDriverManager::get_active_driver()->lock();
	if ((uint32_t)p_count > clips.size()) {
		clips.resize(p_count);
	}
	clip_count = p_count;
	version++;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
	AudioDriverManager::get_active_driver()->unlock();
}

int AudioStreamInteractive::get_clip_count() const { return clip_count; }

void AudioStreamInteractive::set_initial_clip(int p_clip)
{
	ERR_FAIL_INDEX(p_clip, clip_count);
	initial_clip = p_clip;
}

int AudioStreamInteractive::get_initial_clip() const { return initial_clip; }

void AudioStreamInteractive::set_clip_name(int p_clip, const String& p_name)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].name = p_name;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
}

String AudioStreamInteractive::get_clip_name(int p_clip) const
{
	ERR_FAIL_COND_V(p_clip < -1 || p_clip >= MAX_CLIPS, String());
	if (p_clip == CLIP_ANY) {
		return RTR("All Clips");
	}
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].name : String();
}

void AudioStreamInteractive::set_clip_stream(int p_clip, const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	AudioDriverManager::get_active_driver()->lock();
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].stream = p_stream;
	version++;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
	AudioDriverManager::get_active_driver()->unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_clip_stream(int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, Ref<AudioStream>());
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].stream : Ref<AudioStream>();
}

void AudioStreamInteractive::set_clip_auto_advance(int p_clip, AutoAdvanceMode p_mode)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 3);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].auto_advance = p_mode;
}

AudioStreamInteractive::AutoAdvanceMode AudioStreamInteractive::get_clip_auto_advance(
	int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, AUTO_ADVANCE_DISABLED);
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].auto_advance : AUTO_ADVANCE_DISABLED;
}

void AudioStreamInteractive::set_clip_auto_advance_next_clip(int p_clip, int p_index)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].auto_advance_next_clip = p_index;
}

int AudioStreamInteractive::get_clip_auto_advance_next_clip(int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, -1);
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].auto_advance_next_clip : 0;
}

// TRANSITIONS (MODE_CLIPS)

bool AudioStreamInteractive::has_transition(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	return transition_map.has(tk);
}

void AudioStreamInteractive::erase_transition(int p_from_clip, int p_to_clip)
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND(!transition_map.has(tk));
	AudioDriverManager::get_active_driver()->lock();
	transition_map.erase(tk);
	AudioDriverManager::get_active_driver()->unlock();
}

PackedInt32Array AudioStreamInteractive::get_transition_list() const
{
	PackedInt32Array ret;

	for (const KeyValue<TransitionKey, Transition>& K : transition_map) {
		ret.push_back(K.key.from_clip);
		ret.push_back(K.key.to_clip);
	}
	return ret;
}

void AudioStreamInteractive::add_transition(int p_from_clip, int p_to_clip,
	TransitionFromTime p_from_time, TransitionToTime p_to_time, FadeMode p_fade_mode,
	float p_fade_beats, bool p_use_filler_flip, int p_filler_clip, bool p_hold_previous)
{
	ERR_FAIL_COND(p_from_clip < CLIP_ANY || p_from_clip >= clip_count);
	ERR_FAIL_COND(p_to_clip < CLIP_ANY || p_to_clip >= clip_count);
	ERR_FAIL_UNSIGNED_INDEX(p_from_time, TRANSITION_FROM_TIME_MAX);
	ERR_FAIL_UNSIGNED_INDEX(p_to_time, TRANSITION_TO_TIME_MAX);
	ERR_FAIL_UNSIGNED_INDEX(p_fade_mode, FADE_MAX);

	Transition tr;
	tr.from_time = p_from_time;
	tr.to_time = p_to_time;
	tr.fade_mode = p_fade_mode;
	tr.fade_beats = p_fade_beats;
	tr.use_filler_clip = p_use_filler_flip;
	tr.filler_clip = p_filler_clip;
	tr.hold_previous = p_hold_previous;

	TransitionKey tk(p_from_clip, p_to_clip);

	AudioDriverManager::get_active_driver()->lock();
	transition_map[tk] = tr;
	AudioDriverManager::get_active_driver()->unlock();
}

AudioStreamInteractive::TransitionFromTime AudioStreamInteractive::get_transition_from_time(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), TRANSITION_FROM_TIME_END);
	return transition_map[tk].from_time;
}

AudioStreamInteractive::TransitionToTime AudioStreamInteractive::get_transition_to_time(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), TRANSITION_TO_TIME_START);
	return transition_map[tk].to_time;
}

AudioStreamInteractive::FadeMode AudioStreamInteractive::get_transition_fade_mode(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), FADE_DISABLED);
	return transition_map[tk].fade_mode;
}

float AudioStreamInteractive::get_transition_fade_beats(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), -1);
	return transition_map[tk].fade_beats;
}

bool AudioStreamInteractive::is_transition_using_filler_clip(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), false);
	return transition_map[tk].use_filler_clip;
}

int AudioStreamInteractive::get_transition_filler_clip(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), -1);
	return transition_map[tk].filler_clip;
}

bool AudioStreamInteractive::is_transition_holding_previous(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), false);
	return transition_map[tk].hold_previous;
}

// PLAYLIST & SYNCHRONIZED STREAMS

void AudioStreamInteractive::set_stream_count(int p_count)
{
	int max_streams = (mode == MODE_SYNCHRONIZED) ? MAX_SYNC_STREAMS : MAX_STREAMS;
	ERR_FAIL_COND(p_count < 0 || p_count > max_streams);
	AudioServer::lock();
	if ((uint32_t)p_count > audio_streams.size()) {
		audio_streams.resize(p_count);
	}
	if ((uint32_t)p_count > audio_stream_volume_db.size()) {
		uint32_t old_size = audio_stream_volume_db.size();
		audio_stream_volume_db.resize(p_count);
		for (uint32_t i = old_size; i < (uint32_t)p_count; i++) {
			audio_stream_volume_db[i] = 0.0f;
		}
	}
	stream_count = p_count;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

int AudioStreamInteractive::get_stream_count() const { return stream_count; }

void AudioStreamInteractive::set_fade_time(float p_time) { fade_time = p_time; }

float AudioStreamInteractive::get_fade_time() const { return fade_time; }

void AudioStreamInteractive::set_shuffle(bool p_shuffle) { shuffle = p_shuffle; }

bool AudioStreamInteractive::get_shuffle() const { return shuffle; }

void AudioStreamInteractive::set_loop(bool p_loop) { loop = p_loop; }

void AudioStreamInteractive::set_list_stream(int p_stream_index, Ref<AudioStream> p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_stream_index, MAX_STREAMS);

	AudioServer::lock();
	if ((uint32_t)p_stream_index >= audio_streams.size()) {
		audio_streams.resize(p_stream_index + 1);
	}
	audio_streams[p_stream_index] = p_stream;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_list_stream(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_STREAMS, Ref<AudioStream>());
	return (uint32_t)p_stream_index < audio_streams.size() ? audio_streams[p_stream_index]
														   : Ref<AudioStream>();
}

void AudioStreamInteractive::set_sync_stream(int p_stream_index, Ref<AudioStream> p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_stream_index, MAX_SYNC_STREAMS);

	AudioServer::lock();
	if ((uint32_t)p_stream_index >= audio_streams.size()) {
		audio_streams.resize(p_stream_index + 1);
	}
	audio_streams[p_stream_index] = p_stream;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_sync_stream(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_SYNC_STREAMS, Ref<AudioStream>());
	return (uint32_t)p_stream_index < audio_streams.size() ? audio_streams[p_stream_index]
														   : Ref<AudioStream>();
}

void AudioStreamInteractive::set_sync_stream_volume(int p_stream_index, float p_db)
{
	ERR_FAIL_INDEX(p_stream_index, MAX_SYNC_STREAMS);
	if ((uint32_t)p_stream_index >= audio_stream_volume_db.size()) {
		uint32_t old_size = audio_stream_volume_db.size();
		audio_stream_volume_db.resize(p_stream_index + 1);
		for (uint32_t i = old_size; i <= (uint32_t)p_stream_index; i++) {
			audio_stream_volume_db[i] = 0.0f;
		}
	}
	audio_stream_volume_db[p_stream_index] = p_db;
}

float AudioStreamInteractive::get_sync_stream_volume(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_SYNC_STREAMS, 0);
	return (uint32_t)p_stream_index < audio_stream_volume_db.size()
			   ? audio_stream_volume_db[p_stream_index]
			   : 0.0f;
}

// AudioStream virtual overrides

double AudioStreamInteractive::get_bpm() const
{
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				double bpm = audio_streams[i]->get_bpm();
				if (bpm != 0.0) {
					return bpm;
				}
			}
		}
	}
	return 0.0;
}

int AudioStreamInteractive::get_beat_count() const
{
	if (mode == MODE_SYNCHRONIZED) {
		int max_beats = 0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				max_beats = MAX(max_beats, audio_streams[i]->get_beat_count());
			}
		}
		return max_beats;
	}
	return 0;
}

int AudioStreamInteractive::get_bar_beats() const
{
	if (mode == MODE_SYNCHRONIZED) {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				int bar_beats = audio_streams[i]->get_bar_beats();
				if (bar_beats != 0) {
					return bar_beats;
				}
			}
		}
		return 0;
	}
	return AudioStream::get_bar_beats();
}

bool AudioStreamInteractive::has_loop() const
{
	switch (mode) {
	case MODE_PLAYLIST:
		return loop;
	case MODE_SYNCHRONIZED: {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid() && audio_streams[i]->has_loop()) {
				return true;
			}
		}
		return false;
	}
	case MODE_CLIPS:
	default:
		return false;
	}
}

double AudioStreamInteractive::get_length() const
{
	switch (mode) {
	case MODE_PLAYLIST: {
		double total_length = 0.0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				double bpm = audio_streams[i]->get_bpm();
				int beat_count = audio_streams[i]->get_beat_count();
				if (bpm > 0.0 && beat_count > 0) {
					total_length += beat_count * 60.0 / bpm;
				}
				else {
					total_length += audio_streams[i]->get_length();
				}
			}
		}
		return total_length;
	}
	case MODE_SYNCHRONIZED: {
		double max_length = 0.0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				max_length = MAX(max_length, audio_streams[i]->get_length());
			}
		}
		return max_length;
	}
	case MODE_CLIPS:
	default:
		return 0.0;
	}
}

#ifdef TOOLS_ENABLED

String AudioStreamInteractive::_get_streams_hint() const
{
	if (!stream_name_cache.is_empty()) {
		return stream_name_cache;
	}

	int count = MIN(clip_count, (int)clips.size());
	for (int i = 0; i < count; i++) {
		if (i > 0) {
			stream_name_cache += ",";
		}
		String n = String(clips[i].name).replace_char(',', ' ');

		if (n == "" && clips[i].stream.is_valid()) {
			if (!clips[i].stream->get_name().is_empty()) {
				n = clips[i].stream->get_name().replace_char(',', ' ');
			}
			else if (clips[i].stream->get_path().is_resource_file()) {
				n = clips[i].stream->get_path().get_file().replace_char(',', ' ');
			}
		}

		if (n == "") {
			n = "Clip " + itos(i);
		}

		stream_name_cache += n;
	}

	return stream_name_cache;
}

#endif

///////////////////////////////////////
// AudioStreamPlaybackInteractive

AudioStreamPlaybackInteractive::~AudioStreamPlaybackInteractive()
{
	if (stream.is_valid()) {
		stream->playbacks.erase(this);
	}
}

void AudioStreamPlaybackInteractive::stop()
{
	active = false;

	for (uint32_t i = 0; i < clips.states.size(); i++) {
		if (clips.states[i].playback.is_valid()) {
			clips.states[i].playback->stop();
		}
		clips.states[i].fade_speed = 0.0;
		clips.states[i].fade_volume = 0.0;
		clips.states[i].fade_wait = 0.0;
		clips.states[i].reset_fade();
		clips.states[i].active = false;
		clips.states[i].auto_advance = -1;
		clips.states[i].first_mix = true;
	}
	clips.playback_current = -1;

	for (uint32_t i = 0; i < playlist.playback.size(); i++) {
		if (playlist.playback[i].is_valid()) {
			playlist.playback[i]->stop();
		}
	}

	for (uint32_t i = 0; i < sync.playback.size(); i++) {
		if (sync.playback[i].is_valid()) {
			sync.playback[i]->stop();
		}
	}
}

void AudioStreamPlaybackInteractive::_update_order()
{
	int count = stream->stream_count;
	playlist.play_order.resize(count);
	for (int i = 0; i < count; i++) {
		playlist.play_order[i] = i;
	}

	if (stream->shuffle) {
		for (int i = 0; i < count; i++) {
			int swap_with = Math::rand() % uint32_t(count);
			SWAP(playlist.play_order[i], playlist.play_order[swap_with]);
		}
	}
}

void AudioStreamPlaybackInteractive::_update_playback_instances()
{
	stop();

	if (stream.is_null()) {
		return;
	}

	int count = stream->stream_count;
	if (stream->mode == AudioStreamInteractive::MODE_PLAYLIST) {
		playlist.playback.resize(count);
		for (int i = 0; i < count; i++) {
			if ((uint32_t)i < stream->audio_streams.size() && stream->audio_streams[i].is_valid()) {
				playlist.playback[i] = stream->audio_streams[i]->instantiate_playback();
			}
			else {
				playlist.playback[i].unref();
			}
		}
	}
	else if (stream->mode == AudioStreamInteractive::MODE_SYNCHRONIZED) {
		sync.playback.resize(count);
		for (int i = 0; i < count; i++) {
			if ((uint32_t)i < stream->audio_streams.size() && stream->audio_streams[i].is_valid()) {
				sync.playback[i] = stream->audio_streams[i]->instantiate_playback();
			}
			else {
				sync.playback[i].unref();
			}
		}
	}
}

void AudioStreamPlaybackInteractive::start(double p_from_pos)
{
	if (stream.is_null()) {
		return;
	}

	if (active) {
		stop();
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_CLIPS: {
		if (clips.states.size() < AudioStreamInteractive::MAX_CLIPS) {
			clips.states.resize(AudioStreamInteractive::MAX_CLIPS);
		}
		if (clips.version != stream->version) {
			for (int i = 0; i < AudioStreamInteractive::MAX_CLIPS; i++) {
				Ref<AudioStream> src_stream;
				if (i < stream->clip_count && (uint32_t)i < stream->clips.size()) {
					src_stream = stream->clips[i].stream;
				}
				if (clips.states[i].stream != src_stream) {
					clips.states[i].stream.unref();
					clips.states[i].playback.unref();

					clips.states[i].stream = src_stream;
					if (src_stream.is_valid()) {
						clips.states[i].playback = src_stream->instantiate_playback();
					}
				}
			}

			clips.version = stream->version;
		}

		int current = stream->initial_clip;
		if (current < 0 || current >= stream->clip_count) {
			return; // No playback possible.
		}
		if (clips.states[current].playback.is_null()) {
			return; // No playback possible.
		}
		active = true;

		_queue(current, false);
	} break;

	case AudioStreamInteractive::MODE_PLAYLIST: {
		if ((int)playlist.playback.size() != stream->stream_count) {
			_update_playback_instances();
		}

		p_from_pos = MAX(0.0, p_from_pos);

		float pl_length = stream->get_length();
		if (p_from_pos >= pl_length) {
			if (!stream->loop || pl_length <= 0.0f) {
				return; // No loop, end.
			}
			p_from_pos = Math::fmod((float)p_from_pos, pl_length);
		}

		_update_order();

		playlist.play_index = -1;

		double play_ofs = p_from_pos;
		for (int i = 0; i < stream->stream_count; i++) {
			int idx = playlist.play_order[i];
			if ((uint32_t)idx < stream->audio_streams.size() &&
				stream->audio_streams[idx].is_valid()) {
				double bpm = stream->audio_streams[idx]->get_bpm();
				int beat_count = stream->audio_streams[idx]->get_beat_count();
				double length;
				if (bpm > 0.0 && beat_count > 0) {
					length = beat_count * 60.0 / bpm;
				}
				else {
					length = stream->audio_streams[idx]->get_length();
				}
				if (play_ofs < length) {
					playlist.play_index = i;
					playlist.stream_todo = length - play_ofs;
					break;
				}
				else {
					play_ofs -= length;
				}
			}
		}

		if (playlist.play_index == -1) {
			return;
		}

		int first_idx = playlist.play_order[playlist.play_index];
		if ((uint32_t)first_idx >= playlist.playback.size() ||
			playlist.playback[first_idx].is_null()) {
			return;
		}

		playlist.playback[first_idx]->start(play_ofs);
		playlist.fade_index = -1;
		playlist.loop_count = 0;
		playlist.offset = p_from_pos;

		active = true;
	} break;

	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		if ((int)sync.playback.size() != stream->stream_count) {
			_update_playback_instances();
		}

		for (int i = 0; i < stream->stream_count; i++) {
			if ((uint32_t)i < sync.playback.size() && sync.playback[i].is_valid()) {
				sync.playback[i]->start(p_from_pos);
				active = true;
			}
		}
	} break;
	}
}

void AudioStreamPlaybackInteractive::_queue(int p_to_clip_index, bool p_is_auto_advance)
{
	ERR_FAIL_INDEX(p_to_clip_index, stream->clip_count);
	ERR_FAIL_COND(clips.states[p_to_clip_index].playback.is_null());

	if (clips.playback_current == -1) {
		// Nothing to do, start.
		int current = p_to_clip_index;
		ClipState& state = clips.states[current];
		state.active = true;
		state.fade_wait = 0;
		state.fade_volume = 1.0;
		state.fade_speed = 0;
		state.first_mix = true;

		state.playback->start(0);

		clips.playback_current = current;

		if (stream->clips[current].auto_advance == AudioStreamInteractive::AUTO_ADVANCE_ENABLED &&
			stream->clips[current].auto_advance_next_clip >= 0 &&
			stream->clips[current].auto_advance_next_clip < stream->clip_count &&
			stream->clips[current].auto_advance_next_clip != current) {
			// prepare auto advance
			state.auto_advance = stream->clips[current].auto_advance_next_clip;
		}
		return;
	}

	for (int i = 0; i < stream->clip_count; i++) {
		if (i == clips.playback_current || i == p_to_clip_index) {
			continue;
		}
		if (clips.states[i].active &&
			clips.states[i].fade_wait > 0) { // Waiting to kick in, terminate because change of plans.
			clips.states[i].playback->stop();
			clips.states[i].reset_fade();
			clips.states[i].active = false;
		}
	}

	ClipState& from_state = clips.states[clips.playback_current];
	ClipState& to_state = clips.states[p_to_clip_index];

	AudioStreamInteractive::Transition transition; // Use an empty transition by default

	AudioStreamInteractive::TransitionKey tkeys[4] = {
		AudioStreamInteractive::TransitionKey(clips.playback_current, p_to_clip_index),
		AudioStreamInteractive::TransitionKey(
			clips.playback_current, AudioStreamInteractive::CLIP_ANY),
		AudioStreamInteractive::TransitionKey(AudioStreamInteractive::CLIP_ANY, p_to_clip_index),
		AudioStreamInteractive::TransitionKey(
			AudioStreamInteractive::CLIP_ANY, AudioStreamInteractive::CLIP_ANY)};

	for (int i = 0; i < 4; i++) {
		if (stream->transition_map.has(tkeys[i])) {
			transition = stream->transition_map[tkeys[i]];
			break;
		}
	}

	if (transition.fade_mode == AudioStreamInteractive::FADE_AUTOMATIC) {
		// Adjust automatic mode based on context.
		if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_START) {
			transition.fade_mode = AudioStreamInteractive::FADE_OUT;
		}
		else {
			transition.fade_mode = AudioStreamInteractive::FADE_CROSS;
		}
	}

	if (p_is_auto_advance) {
		transition.from_time = AudioStreamInteractive::TRANSITION_FROM_TIME_END;
		if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_SAME_POSITION) {
			transition.to_time = AudioStreamInteractive::TRANSITION_TO_TIME_START;
		}
	}

	// Prepare the fadeout
	float current_pos = from_state.playback->get_playback_position();

	float src_fade_wait = 0;
	float dst_seek_to = 0;
	float fade_speed = 0;
	bool src_no_loop = false;

	if (from_state.stream->get_bpm()) {
		// Check if source speed has BPM, if so, transition syncs to BPM
		float beat_sec = 60 / float(from_state.stream->get_bpm());
		switch (transition.from_time) {
		case AudioStreamInteractive::TRANSITION_FROM_TIME_IMMEDIATE: {
			src_fade_wait = 0;
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_NEXT_BEAT: {
			float remainder = Math::fmod(current_pos, beat_sec);
			src_fade_wait = beat_sec - remainder;
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_NEXT_BAR: {
			if (from_state.stream->get_bar_beats() > 0) {
				float bar_sec = beat_sec * from_state.stream->get_bar_beats();
				float remainder = Math::fmod(current_pos, bar_sec);
				src_fade_wait = bar_sec - remainder;
			}
			else {
				// Stream does not have a number of beats per bar - avoid NaN, and play immediately.
				src_fade_wait = 0;
			}
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_END: {
			float end = from_state.stream->get_beat_count() > 0
							? float(from_state.stream->get_beat_count() * beat_sec)
							: from_state.stream->get_length();
			if (end == 0) {
				// Stream does not have a length.
				src_fade_wait = 0;
			}
			else {
				src_fade_wait = end - current_pos;
			}

			if (!from_state.stream->has_loop()) {
				src_no_loop = true;
			}

		} break;
		default: {
		}
		}
		// Fade speed also aligned to BPM
		fade_speed = 1.0 / (transition.fade_beats * beat_sec);
	}
	else {
		// Source has no BPM, so just simple transition.
		if (transition.from_time == AudioStreamInteractive::TRANSITION_FROM_TIME_END &&
			from_state.stream->get_length() > 0) {
			float end = from_state.stream->get_length();
			src_fade_wait = end - current_pos;
			if (!from_state.stream->has_loop()) {
				src_no_loop = true;
			}
		}
		else {
			src_fade_wait = 0;
		}
		fade_speed = 1.0 / transition.fade_beats;
	}

	if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_PREVIOUS_POSITION &&
		to_state.stream->get_length() > 0.0) {
		dst_seek_to = to_state.previous_position;
	}
	else if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_SAME_POSITION &&
			 transition.from_time != AudioStreamInteractive::TRANSITION_FROM_TIME_END &&
			 to_state.stream->get_length() > 0.0) {
		// Seeking to basically same position as when we start fading.
		dst_seek_to = current_pos + src_fade_wait;
		float end;
		if (to_state.stream->get_bpm() > 0 && to_state.stream->get_beat_count()) {
			float beat_sec = 60 / float(to_state.stream->get_bpm());
			end = to_state.stream->get_beat_count() * beat_sec;
		}
		else {
			end = to_state.stream->get_length();
		}

		if (dst_seek_to > end) {
			// Seeking too far away.
			dst_seek_to = 0; // past end, loop to beginning.
		}

	}
	else {
		// Seek to Start
		dst_seek_to = 0.0;
	}

	if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
		transition.fade_mode == AudioStreamInteractive::FADE_IN) {
		if (src_no_loop) {
			// If there is no fade in the source stream, then let it continue until it ends.
			from_state.fade_wait = 0;
			from_state.fade_speed = 0;
		}
		else {
			// Otherwise force a very quick fade to avoid clicks
			from_state.fade_wait = src_fade_wait;
			from_state.fade_speed = 1.0 / -0.001;
		}
	}
	else {
		// Regular fade.
		from_state.fade_wait = src_fade_wait;
		from_state.fade_speed = -fade_speed;
	}
	// keep volume, since it may have been fading in from something else.

	to_state.playback->start(dst_seek_to);
	to_state.active = true;
	to_state.fade_volume = 0.0;
	to_state.first_mix = true;

	int auto_advance_to = -1;

	if (stream->clips[p_to_clip_index].auto_advance ==
		AudioStreamInteractive::AUTO_ADVANCE_ENABLED) {
		int next_clip = stream->clips[p_to_clip_index].auto_advance_next_clip;
		if (next_clip >= 0 && next_clip < (int)stream->clip_count &&
			clips.states[next_clip].playback.is_valid() && next_clip != p_to_clip_index &&
			(!transition.use_filler_clip || next_clip != transition.filler_clip)) {
			auto_advance_to = next_clip;
		}
	}

	if (clips.return_memory != -1 && stream->clips[p_to_clip_index].auto_advance ==
										 AudioStreamInteractive::AUTO_ADVANCE_RETURN_TO_HOLD) {
		auto_advance_to = clips.return_memory;
		clips.return_memory = -1;
	}

	if (transition.hold_previous) {
		clips.return_memory = clips.playback_current;
	}

	if (transition.use_filler_clip && transition.filler_clip >= 0 &&
		transition.filler_clip < (int)stream->clip_count &&
		clips.states[transition.filler_clip].playback.is_valid() &&
		clips.playback_current != transition.filler_clip &&
		p_to_clip_index != transition.filler_clip) {
		ClipState& filler_state = clips.states[transition.filler_clip];

		filler_state.playback->start(0);
		filler_state.active = true;

		// Filler state does not fade (bake fade in the audio clip if you want fading).
		filler_state.fade_volume = 1.0;
		filler_state.fade_speed = 0.0;

		filler_state.fade_wait = src_fade_wait;
		filler_state.first_mix = true;

		float filler_end;
		if (filler_state.stream->get_bpm() > 0 && filler_state.stream->get_beat_count() > 0) {
			float filler_beat_sec = 60 / float(filler_state.stream->get_bpm());
			filler_end = filler_beat_sec * filler_state.stream->get_beat_count();
		}
		else {
			filler_end = filler_state.stream->get_length();
		}

		if (!filler_state.stream->has_loop()) {
			src_no_loop = true;
		}

		if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
			transition.fade_mode == AudioStreamInteractive::FADE_OUT) {
			// No fading, immediately start at full volume.
			to_state.fade_volume = 0.0;
			to_state.fade_speed = 1.0; // start at full volume, as filler is meant as a transition.
		}
		else {
			// Fade enable, prepare fade.
			to_state.fade_volume = 0.0;
			to_state.fade_speed = fade_speed;
		}

		to_state.fade_wait = src_fade_wait + filler_end;

	}
	else {
		to_state.fade_wait = src_fade_wait;

		if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
			transition.fade_mode == AudioStreamInteractive::FADE_OUT) {
			to_state.fade_volume = 1.0;
			to_state.fade_speed = 0.0;
		}
		else {
			to_state.fade_volume = 0.0;
			to_state.fade_speed = fade_speed;
		}

		to_state.auto_advance = auto_advance_to;
	}
}

void AudioStreamPlaybackInteractive::seek(double p_time)
{
	if (stream.is_null()) {
		return;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		stop();
		start(p_time);
		break;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid()) {
				sync.playback[i]->seek(p_time);
			}
		}
	} break;
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		// Seek not supported in clips mode.
		break;
	}
}

int AudioStreamPlaybackInteractive::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (stream.is_null()) {
		return 0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return _mix_playlist(p_buffer, p_rate_scale, p_frames);
	case AudioStreamInteractive::MODE_SYNCHRONIZED:
		return _mix_sync(p_buffer, p_rate_scale, p_frames);
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return _mix_clips(p_buffer, p_rate_scale, p_frames);
	}
}

int AudioStreamPlaybackInteractive::_mix_clips(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (active && clips.version != stream->version) {
		stop();
	}

	if (clips.switch_request != -1) {
		if (active) {
			_queue(clips.switch_request, false);
		}
		clips.switch_request = -1;
	}

	if (!active) {
		return 0;
	}

	int todo = p_frames;

	while (todo) {
		int to_mix = MIN(todo, BUFFER_SIZE);
		_mix_internal(to_mix);
		for (int i = 0; i < to_mix; i++) {
			p_buffer[i] = mix_buffer[i];
		}
		p_buffer += to_mix;
		todo -= to_mix;
	}

	return p_frames;
}

void AudioStreamPlaybackInteractive::_mix_internal(int p_frames)
{
	for (int i = 0; i < p_frames; i++) {
		mix_buffer[i] = AudioFrame(0, 0);
	}

	bool any_active = false;
	int count = MIN(stream->clip_count, (int)clips.states.size());
	for (int i = 0; i < count; i++) {
		if (!clips.states[i].active) {
			continue;
		}

		_mix_internal_state(i, p_frames);

		any_active = clips.states[i].active || any_active;
	}
	if (!any_active) {
		active = false;
		clips.playback_current = -1;
	}
}

void AudioStreamPlaybackInteractive::_mix_internal_state(int p_state_idx, int p_frames)
{
	ClipState& state = clips.states[p_state_idx];
	double mix_rate = double(AudioServer::get_mix_rate());
	double frame_inc = 1.0 / mix_rate;

	int from_frame = 0;
	int queue_next = -1;

	if (state.first_mix) {
		// Did not start mixing yet, wait.
		double mix_time = p_frames * frame_inc;
		if (state.fade_wait < mix_time) {
			// time to start!
			from_frame = state.fade_wait * mix_rate;
			state.fade_wait = 0;
			if (state.fade_speed == 0.0) {
				queue_next = state.auto_advance;
			}
			clips.playback_current = p_state_idx;
			state.first_mix = false;
		}
		else {
			// This is for fade in of new stream.
			state.fade_wait -= mix_time;
			return; // Nothing to do
		}
	}

	state.previous_position = state.playback->get_playback_position();
	state.playback->mix(temp_buffer + from_frame, 1.0, p_frames - from_frame);

	double frame_fade_inc = state.fade_speed * frame_inc;
	for (int i = from_frame; i < p_frames; i++) {
		if (state.fade_wait) {
			// This is for fade out of existing stream;
			state.fade_wait -= frame_inc;
			if (state.fade_wait < 0.0) {
				state.fade_wait = 0.0;
			}
		}
		else if (frame_fade_inc > 0) {
			state.fade_volume += frame_fade_inc;
			if (state.fade_volume >= 1.0) {
				state.fade_speed = 0.0;
				frame_fade_inc = 0.0;
				state.fade_volume = 1.0;
				queue_next = state.auto_advance;
			}
		}
		else if (frame_fade_inc < 0.0) {
			state.fade_volume += frame_fade_inc;
			if (state.fade_volume <= 0.0) {
				state.fade_speed = 0.0;
				frame_fade_inc = 0.0;
				state.fade_volume = 0.0;
				state.playback->stop(); // Stop playback and break, no point to continue mixing
				break;
			}
		}

		mix_buffer[i] += temp_buffer[i] * state.fade_volume;
		state.previous_position += frame_inc;
	}

	if (!state.playback->is_playing()) {
		// It finished because it either reached end or faded out, so deactivate and continue.
		state.active = false;
	}
	if (queue_next != -1) {
		_queue(queue_next, true);
	}
}

int AudioStreamPlaybackInteractive::_mix_playlist(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (!active) {
		return 0;
	}

	double time_dec = (1.0 / AudioServer::get_mix_rate());
	double fade_dec = (1.0 / stream->fade_time) / AudioServer::get_mix_rate();

	int todo = p_frames;

	while (todo) {
		int to_mix = MIN(todo, MIX_BUFFER_SIZE);

		playlist.playback[playlist.play_order[playlist.play_index]]->mix(mix_buffer, 1.0, to_mix);
		if (playlist.fade_index != -1) {
			playlist.playback[playlist.fade_index]->mix(temp_buffer, 1.0, to_mix);
		}

		playlist.offset += time_dec * to_mix;

		for (int i = 0; i < to_mix; i++) {
			*p_buffer = mix_buffer[i];
			playlist.stream_todo -= time_dec;
			if (playlist.stream_todo < 0) {
				// find next stream.
				int prev = playlist.play_order[playlist.play_index];

				for (int j = 0; j < stream->stream_count; j++) {
					playlist.play_index++;
					if (playlist.play_index >= stream->stream_count) {
						// No loop, exit.
						if (!stream->loop) {
							for (int k = i; k < todo - i; k++) {
								p_buffer[k] = AudioFrame(0, 0);
							}
							todo = to_mix;
							active = false;
							break;
						}

						_update_order();
						playlist.play_index = 0;
						playlist.loop_count++;
						playlist.offset = time_dec * (to_mix - i);
					}

					if (playlist.playback[playlist.play_order[playlist.play_index]].is_valid()) {
						break;
					}
				}

				if (!active) {
					break;
				}

				if (playlist.playback[playlist.play_order[playlist.play_index]].is_null()) {
					todo = to_mix; // Weird error.
					active = false;
					break;
				}

				bool restart = true;
				if (prev == playlist.play_order[playlist.play_index]) {
					// Went back to the same one, continue loop (if it loops) or restart if it does
					// not.
					if (stream->audio_streams[prev]->has_loop()) {
						restart = false;
					}
					playlist.fade_index = -1;
				}
				else {
					// Move current mixed data to fade buffer (temp_buffer).
					for (int j = i; j < to_mix; j++) {
						temp_buffer[j] = mix_buffer[j];
					}

					playlist.fade_index = prev;
					playlist.fade_volume = 1.0;
				}

				int idx = playlist.play_order[playlist.play_index];

				if (restart) {
					playlist.playback[idx]->start(0); // No loop, just cold-restart.
					playlist.playback[idx]->mix(
						mix_buffer + i, 1.0, to_mix - i); // Fill rest of mix buffer
				}

				// Update fade todo.
				double bpm = stream->audio_streams[idx]->get_bpm();
				int beat_count = stream->audio_streams[idx]->get_beat_count();

				if (bpm > 0.0 && beat_count > 0) {
					playlist.stream_todo = beat_count * 60.0 / bpm;
				}
				else {
					playlist.stream_todo = stream->audio_streams[idx]->get_length();
				}
			}

			if (playlist.fade_index != -1) {
				*p_buffer += temp_buffer[i] * playlist.fade_volume;
				playlist.fade_volume -= fade_dec;
				if (playlist.fade_volume <= 0.0) {
					playlist.playback[playlist.fade_index]->stop();
					playlist.fade_index = -1;
				}
			}

			p_buffer++;
		}

		todo -= to_mix;
	}

	return p_frames;
}

int AudioStreamPlaybackInteractive::_mix_sync(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (!active) {
		return 0;
	}

	int todo = p_frames;
	int count = MIN(stream->stream_count, (int)sync.playback.size());

	bool any_active = false;
	while (todo) {
		int to_mix = MIN(todo, MIX_BUFFER_SIZE);

		bool first = true;
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				float db = (uint32_t)i < stream->audio_stream_volume_db.size()
							   ? stream->audio_stream_volume_db[i]
							   : 0.0f;
				float volume = Math::db_to_linear(db);
				if (first) {
					sync.playback[i]->mix(p_buffer, p_rate_scale, to_mix);
					for (int j = 0; j < to_mix; j++) {
						p_buffer[j] *= volume;
					}
					first = false;
					any_active = true;
				}
				else {
					sync.playback[i]->mix(mix_buffer, p_rate_scale, to_mix);
					for (int j = 0; j < to_mix; j++) {
						p_buffer[j] += mix_buffer[j] * volume;
					}
				}
			}
		}

		if (first) {
			// Nothing mixed, put zeroes.
			for (int j = 0; j < to_mix; j++) {
				p_buffer[j] = AudioFrame(0, 0);
			}
		}

		p_buffer += to_mix;
		todo -= to_mix;
	}

	if (!any_active) {
		active = false;
	}
	return p_frames;
}

void AudioStreamPlaybackInteractive::tag_used_streams()
{
	if (stream.is_null()) {
		return;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		if (active && (uint32_t)playlist.play_index < playlist.play_order.size()) {
			int idx = playlist.play_order[playlist.play_index];
			if ((uint32_t)idx < stream->audio_streams.size() &&
				stream->audio_streams[idx].is_valid() && (uint32_t)idx < playlist.playback.size() &&
				playlist.playback[idx].is_valid()) {
				stream->audio_streams[idx]->tag_used(
					playlist.playback[idx]->get_playback_position());
			}
		}
		stream->tag_used(0);
		break;

	case AudioStreamInteractive::MODE_SYNCHRONIZED:
		if (active) {
			int count = MIN(stream->stream_count, (int)sync.playback.size());
			for (int i = 0; i < count; i++) {
				if (sync.playback[i].is_valid() && sync.playback[i]->is_playing() &&
					(uint32_t)i < stream->audio_streams.size() &&
					stream->audio_streams[i].is_valid()) {
					stream->audio_streams[i]->tag_used(sync.playback[i]->get_playback_position());
				}
			}
			stream->tag_used(0);
		}
		break;

	case AudioStreamInteractive::MODE_CLIPS:
	default: {
		int count = MIN(stream->clip_count, (int)clips.states.size());
		for (int i = 0; i < count; i++) {
			if (clips.states[i].active && !clips.states[i].first_mix &&
				clips.states[i].playback->is_playing()) {
				clips.states[i].stream->tag_used(
					clips.states[i].playback->get_playback_position());
			}
		}
		stream->tag_used(0);
	} break;
	}
}

void AudioStreamPlaybackInteractive::switch_to_clip_by_name(const String& p_name)
{
	if (p_name == String()) {
		clips.switch_request = -1;
		return;
	}

	ERR_FAIL_COND_MSG(stream.is_null(), "Attempted to switch while not playing back any stream.");

	for (int i = 0; i < stream->get_clip_count(); i++) {
		if (stream->get_clip_name(i) == p_name) {
			clips.switch_request = i;
			return;
		}
	}
	ERR_FAIL_MSG("Clip not found: " + String(p_name));
}

void AudioStreamPlaybackInteractive::switch_to_clip(int p_index)
{
	clips.switch_request = p_index;
}

int AudioStreamPlaybackInteractive::get_current_clip_index() const
{
	return clips.playback_current;
}

int AudioStreamPlaybackInteractive::get_loop_count() const
{
	if (stream.is_null()) {
		return 0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return playlist.loop_count;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		int min_loops = 0;
		bool min_loops_found = false;
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				int loops = sync.playback[i]->get_loop_count();
				if (!min_loops_found || loops < min_loops) {
					min_loops = loops;
					min_loops_found = true;
				}
			}
		}
		return min_loops;
	}
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return 0;
	}
}

double AudioStreamPlaybackInteractive::get_playback_position() const
{
	if (stream.is_null()) {
		return 0.0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return playlist.offset;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		float max_pos = 0;
		bool pos_found = false;
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				float pos = sync.playback[i]->get_playback_position();
				if (!pos_found || pos > max_pos) {
					max_pos = pos;
					pos_found = true;
				}
			}
		}
		return max_pos;
	}
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return 0.0;
	}
}

bool AudioStreamPlaybackInteractive::is_playing() const { return active; }

void initialize_interactive_music_module(ModuleInitializationLevel p_level) {}

void uninitialize_interactive_music_module(ModuleInitializationLevel p_level)
{
	// Nothing to do here.
}
#endif // defined(MODULE_INTERACTIVE_MUSIC_ENABLED)


// --- Merged from modules/mp3/audio_stream_mp3.cpp ---
#if defined(MODULE_MP3_ENABLED)
#define DR_MP3_FLOAT_OUTPUT
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO

#include <thirdparty/dr_libs/dr_bridge.h>
#include "core/io/resource_importer.h"
#include "core/io/resource_saver.h"
#include "modules/register_module_types.h"
// --- Inlined from modules/mp3/audio_stream_mp3.h ---
#include <thirdparty/dr_libs/dr_mp3.h>
#include "servers/audio/audio_stream.h"

class AudioStreamMP3;

class AudioStreamPlaybackMP3 : public AudioStreamPlaybackResampled
{
	enum
	{
		FADE_SIZE = 256
	};

	AudioFrame loop_fade[FADE_SIZE];
	int loop_fade_remaining = FADE_SIZE;

	bool looping_override = false;
	bool looping = false;
	drmp3 mp3d = {};
	uint32_t frames_mixed = 0;
	bool active = false;
	int loops = 0;

	friend class AudioStreamMP3;

	Ref<AudioStreamMP3> mp3_stream;

	bool _is_sample = false;
	Ref<AudioSamplePlayback> sample_playback;

protected:
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; // times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual void set_is_sample(bool p_is_sample) override;
	virtual bool get_is_sample() const override;
	virtual Ref<AudioSamplePlayback> get_sample_playback() const override;
	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;

	AudioStreamPlaybackMP3() {}

	~AudioStreamPlaybackMP3();
};

class AudioStreamMP3 final : public AudioStream
{
	friend class AudioStreamPlaybackMP3;

	TightLocalVector<uint8_t> data;

	float sample_rate = 1.0;
	int channels = 1;
	float length = 0.0;
	bool loop = false;
	float loop_offset = 0.0;

	double bpm = 0;
	int beat_count = 0;
	int bar_beats = 4;

protected:

public:
	static Ref<AudioStreamMP3> load_from_buffer(const Vector<uint8_t>& p_stream_data);
	static Ref<AudioStreamMP3> load_from_file(const String& p_path);

	void set_loop(bool p_enable);
	virtual bool has_loop() const override;

	void set_loop_offset(double p_seconds);
	double get_loop_offset() const;

	void set_bpm(double p_bpm);
	virtual double get_bpm() const override;

	void set_beat_count(int p_beat_count);
	virtual int get_beat_count() const override;

	void set_bar_beats(int p_bar_beats);
	virtual int get_bar_beats() const override;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;

	void set_data(const Vector<uint8_t>& p_data);
	Vector<uint8_t> get_data() const;

	virtual double get_length() const override;

	virtual bool is_monophonic() const override;

	virtual bool can_be_sampled() const override { return true; }

	virtual Ref<AudioSample> generate_sample() const override;
};

#include "core/io/resource_importer.h"

class ResourceImporterMP3 final : public ResourceImporter
{
public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

#ifdef TOOLS_ENABLED
	virtual bool has_advanced_options() const override;
	virtual void show_advanced_options(const String& p_path) override;
#endif

	virtual bool can_import_threaded() const override { return true; }

	ResourceImporterMP3() = default;
};

// --- End modules/mp3/audio_stream_mp3.h ---
#include "core/io/file_access.h"

int AudioStreamPlaybackMP3::_mix_internal(AudioFrame* p_buffer, int p_frames)
{
	if (!active) {
		return 0;
	}

	int todo = p_frames;

	int frames_mixed_this_step = p_frames;

	int beat_length_frames = -1;
	bool use_loop = looping_override ? looping : mp3_stream->loop;

	bool beat_loop = use_loop && mp3_stream->get_bpm() > 0 && mp3_stream->get_beat_count() > 0;
	if (beat_loop) {
		beat_length_frames =
			mp3_stream->get_beat_count() * mp3_stream->sample_rate * 60 / mp3_stream->get_bpm();
	}

	while (todo && active) {
		drmp3d_sample_t buf_frame[2];

		int samples_mixed = drmp3_read_pcm_frames_f32(&mp3d, 1, buf_frame);

		if (samples_mixed) {
			p_buffer[p_frames - todo] = AudioFrame(buf_frame[0], buf_frame[mp3d.channels - 1]);
			if (loop_fade_remaining < FADE_SIZE) {
				p_buffer[p_frames - todo] +=
					loop_fade[loop_fade_remaining] *
					(float(FADE_SIZE - loop_fade_remaining) / float(FADE_SIZE));
				loop_fade_remaining++;
			}
			--todo;
			++frames_mixed;

			if (beat_loop && (int)frames_mixed >= beat_length_frames) {
				for (int i = 0; i < FADE_SIZE; i++) {
					samples_mixed = drmp3_read_pcm_frames_f32(&mp3d, 1, buf_frame);
					loop_fade[i] = AudioFrame(buf_frame[0], buf_frame[mp3d.channels - 1]);
					if (!samples_mixed) {
						break;
					}
				}
				loop_fade_remaining = 0;
				seek(mp3_stream->loop_offset);
				loops++;
			}
		}

		else {
			// EOF
			if (use_loop) {
				seek(mp3_stream->loop_offset);
				loops++;
			}
			else {
				frames_mixed_this_step = p_frames - todo;
				// fill remainder with silence
				for (int i = p_frames - todo; i < p_frames; i++) {
					p_buffer[i] = AudioFrame(0, 0);
				}
				active = false;
				todo = 0;
			}
		}
	}
	return frames_mixed_this_step;
}

float AudioStreamPlaybackMP3::get_stream_sampling_rate() { return mp3_stream->sample_rate; }

void AudioStreamPlaybackMP3::start(double p_from_pos)
{
	active = true;
	seek(p_from_pos);
	loops = 0;
	begin_resample();
}

void AudioStreamPlaybackMP3::stop() { active = false; }

bool AudioStreamPlaybackMP3::is_playing() const { return active; }

int AudioStreamPlaybackMP3::get_loop_count() const { return loops; }

double AudioStreamPlaybackMP3::get_playback_position() const
{
	return double(frames_mixed) / mp3_stream->sample_rate;
}

void AudioStreamPlaybackMP3::seek(double p_time)
{
	if (!active) {
		return;
	}

	if (p_time >= mp3_stream->get_length()) {
		p_time = 0;
	}

	frames_mixed = uint32_t(mp3_stream->sample_rate * p_time);
	drmp3_seek_to_pcm_frame(&mp3d, (uint64_t)frames_mixed);
}

void AudioStreamPlaybackMP3::tag_used_streams() { mp3_stream->tag_used(get_playback_position()); }

void AudioStreamPlaybackMP3::set_is_sample(bool p_is_sample) { _is_sample = p_is_sample; }

bool AudioStreamPlaybackMP3::get_is_sample() const { return _is_sample; }

Ref<AudioSamplePlayback> AudioStreamPlaybackMP3::get_sample_playback() const
{
	return sample_playback;
}

void AudioStreamPlaybackMP3::set_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	sample_playback = p_playback;
	if (sample_playback.is_valid()) {
		sample_playback->stream_playback = Ref<AudioStreamPlayback>(this);
	}
}

AudioStreamPlaybackMP3::~AudioStreamPlaybackMP3() { drmp3_uninit(&mp3d); }

Ref<AudioStreamPlayback> AudioStreamMP3::instantiate_playback()
{
	Ref<AudioStreamPlaybackMP3> mp3s;

	ERR_FAIL_COND_V_MSG(data.is_empty(), mp3s,
		"This AudioStreamMP3 does not have an audio file assigned "
		"to it. AudioStreamMP3 should not be created from the "
		"inspector or with `.new()`. Instead, load an audio file.");

	mp3s.instantiate();
	mp3s->mp3_stream = Ref<AudioStreamMP3>(this);

	int success = drmp3_init_memory(
		&mp3s->mp3d, data.ptr(), data.size(), (drmp3_allocation_callbacks*)&dr_alloc_calls);

	mp3s->frames_mixed = 0;
	mp3s->active = false;
	mp3s->loops = 0;

	ERR_FAIL_COND_V(!success, Ref<AudioStreamPlaybackMP3>());

	return mp3s;
}

void AudioStreamMP3::set_data(const Vector<uint8_t>& p_data)
{
	drmp3* mp3d = memnew(drmp3);
	int success = drmp3_init_memory(
		mp3d, p_data.ptr(), p_data.size(), (drmp3_allocation_callbacks*)&dr_alloc_calls);
	if (!success || mp3d->sampleRate == 0) {
		memdelete(mp3d);
		ERR_FAIL_MSG("Failed to decode mp3 file. Make sure it is a valid mp3 audio file.");
	}

	channels = mp3d->channels;
	sample_rate = mp3d->sampleRate;
	length = float(drmp3_get_pcm_frame_count(mp3d)) / (mp3d->sampleRate);

	drmp3_uninit(mp3d);
	memdelete(mp3d);

	data = p_data;
}

Vector<uint8_t> AudioStreamMP3::get_data() const { return Vector<uint8_t>(data); }

void AudioStreamMP3::set_loop(bool p_enable) { loop = p_enable; }

bool AudioStreamMP3::has_loop() const { return loop; }

void AudioStreamMP3::set_loop_offset(double p_seconds) { loop_offset = p_seconds; }

double AudioStreamMP3::get_loop_offset() const { return loop_offset; }

double AudioStreamMP3::get_length() const { return length; }

bool AudioStreamMP3::is_monophonic() const { return false; }

void AudioStreamMP3::set_bpm(double p_bpm)
{
	ERR_FAIL_COND(p_bpm < 0);
	bpm = p_bpm;
	emit_changed();
}

double AudioStreamMP3::get_bpm() const { return bpm; }

void AudioStreamMP3::set_beat_count(int p_beat_count)
{
	ERR_FAIL_COND(p_beat_count < 0);
	beat_count = p_beat_count;
	emit_changed();
}

int AudioStreamMP3::get_beat_count() const { return beat_count; }

void AudioStreamMP3::set_bar_beats(int p_bar_beats)
{
	ERR_FAIL_COND(p_bar_beats < 0);
	bar_beats = p_bar_beats;
	emit_changed();
}

int AudioStreamMP3::get_bar_beats() const { return bar_beats; }

Ref<AudioSample> AudioStreamMP3::generate_sample() const
{
	Ref<AudioSample> sample;
	sample.instantiate();
	sample->loop_mode =
		loop ? AudioSample::LoopMode::LOOP_FORWARD : AudioSample::LoopMode::LOOP_DISABLED;
	sample->loop_begin = loop_offset;
	sample->loop_end = 0;
	return sample
;
}

Ref<AudioStreamMP3> AudioStreamMP3::load_from_buffer(const Vector<uint8_t>& p_stream_data)
{
	Ref<AudioStreamMP3> mp3_stream;
	mp3_stream.instantiate();
	mp3_stream->set_data(p_stream_data);
	ERR_FAIL_COND_V_MSG(mp3_stream->get_data().is_empty(), Ref<AudioStreamMP3>(),
		"MP3 decoding failed. Check that your data is a valid MP3 audio stream.");
	return mp3_stream;
}

Ref<AudioStreamMP3> AudioStreamMP3::load_from_file(const String& p_path)
{
	const Vector<uint8_t> stream_data = FileAccess::get_file_as_bytes(p_path);
	ERR_FAIL_COND_V_MSG(
		stream_data.is_empty(), Ref<AudioStreamMP3>(), vformat("Cannot open file '%s'.", p_path));
	return load_from_buffer(stream_data);
}

String ResourceImporterMP3::get_importer_name() const { return "mp3"; }

String ResourceImporterMP3::get_visible_name() const { return "MP3"; }

void ResourceImporterMP3::get_recognized_extensions(List<String>* p_extensions) const
{
#ifndef DR_MP3_ONLY_MP3
	p_extensions->push_back("mp1");
	p_extensions->push_back("mp2");
#endif
	p_extensions->push_back("mp3");
}

String ResourceImporterMP3::get_save_extension() const { return "mp3str"; }

String ResourceImporterMP3::get_resource_type() const { return "AudioStreamMP3"; }

int ResourceImporterMP3::get_preset_count() const { return 0; }

String ResourceImporterMP3::get_preset_name(int p_idx) const { return String(); }

#ifdef TOOLS_ENABLED
bool ResourceImporterMP3::has_advanced_options() const { return false; }

void ResourceImporterMP3::show_advanced_options(const String& p_path) {}
#endif

void ResourceImporterMP3::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}

#ifdef TOOLS_ENABLED
#endif

void initialize_mp3_module(ModuleInitializationLevel p_level)
{
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		Ref<ResourceImporterMP3> mp3_import;
		mp3_import.instantiate();
		ResourceFormatImporter::add_importer(mp3_import);
	}
#endif
}

void uninitialize_mp3_module(ModuleInitializationLevel p_level) {}
#endif // defined(MODULE_MP3_ENABLED)


// --- Merged from modules/ogg/ogg_packet_sequence.cpp ---
#if defined(MODULE_OGG_ENABLED)
// --- Inlined/Folded from modules/ogg/ogg_packet_sequence.h ---
#include <ogg/ogg.h>
#include "core/io/resource.h"

class OggPacketSequencePlayback;

class OggPacketSequence : public Resource
{
	friend class OggPacketSequencePlayback;

	// List of pages, each of which is a list of packets on that page. The innermost
	// PackedByteArrays contain complete ogg packets.
	Vector<Vector<PackedByteArray>> page_data;

	// List of the granule position for each page.
	Vector<uint64_t> page_granule_positions;

	// The page after the current last page. Similar semantics to an end() iterator.
	int64_t end_page = 0;

	uint64_t data_version = 0;

	float sampling_rate = 0;
	float length = 0;

protected:

public:
	// Pushes information about all the pages that ended on this page.
	// This should be called for each page, even for pages that no packets ended on.
	void push_page(int64_t p_granule_pos, const Vector<PackedByteArray>& p_data);

	void set_packet_granule_positions(const PackedInt64Array& p_granule_positions);
	PackedInt64Array get_packet_granule_positions() const;

	// Sets a sampling rate associated with this object. OggPacketSequence doesn't understand
	// codecs, so this value is naively stored as a convenience.
	void set_sampling_rate(float p_sampling_rate);

	// Returns a sampling rate previously set by set_sampling_rate().
	float get_sampling_rate() const;

	// Returns a length previously set by set_length().
	float get_length() const;

	// Returns the granule position of the last page in this sequence.
	int64_t get_final_granule_pos() const;

	Ref<OggPacketSequencePlayback> instantiate_playback();

	OggPacketSequence() {}

	virtual ~OggPacketSequence() {}
};

class OggPacketSequencePlayback : public RefCounted
{
	friend class OggPacketSequence;

	Ref<OggPacketSequence> ogg_packet_sequence;

	mutable int64_t page_cursor = 0;
	mutable int32_t packet_cursor = 0;

	mutable ogg_packet* packet;

	uint64_t data_version = 0;

	mutable int64_t packetno = 0;

	// Recursive bisection search for the correct page.
	uint32_t seek_page_internal(
		int64_t granule, uint32_t after_page_inclusive, uint32_t before_page_inclusive);

public:
	// Calling functions must not modify this packet.
	// Returns true on success, false on error or if there is no next packet.
	bool next_ogg_packet(ogg_packet** p_packet) const;

	// Seeks to the page such that the previous page has a granule position less than or equal to
	// this value, and the current page has a granule position greater than this value. Returns true
	// on success, false on failure.
	bool seek_page(int64_t p_granule_pos);

	// Gets the current page number.
	int64_t get_page_number() const;

	// Moves to a specific page in the stream.
	// Returns true on success, false if the page number is out of bounds.
	bool set_page_number(int64_t p_page_number);

	OggPacketSequencePlayback();
	virtual ~OggPacketSequencePlayback();
};
#include "modules/register_module_types.h"

void OggPacketSequence::push_page(int64_t p_granule_pos, const Vector<PackedByteArray>& p_data)
{
	Vector<PackedByteArray> data_stored;
	for (int i = 0; i < p_data.size(); i++) {
		data_stored.push_back(p_data[i]);
	}
	page_granule_positions.push_back(p_granule_pos);
	page_data.push_back(data_stored);
	data_version++;
}

void OggPacketSequence::set_packet_granule_positions(const PackedInt64Array& p_granule_positions)
{
	data_version++; // Update the data version so old playbacks know that they can't rely on us
					// anymore.
	page_granule_positions.clear();
	for (int page_idx = 0; page_idx < p_granule_positions.size(); page_idx++) {
		int64_t granule_pos = p_granule_positions[page_idx];
		page_granule_positions.push_back(granule_pos);
	}
}

PackedInt64Array OggPacketSequence::get_packet_granule_positions() const
{
	PackedInt64Array ret;
	for (int64_t granule_pos : page_granule_positions) {
		ret.push_back(granule_pos);
	}
	return ret;
}

void OggPacketSequence::set_sampling_rate(float p_sampling_rate)
{
	sampling_rate = p_sampling_rate;
}

float OggPacketSequence::get_sampling_rate() const { return sampling_rate; }

int64_t OggPacketSequence::get_final_granule_pos() const
{
	if (!page_granule_positions.is_empty()) {
		return page_granule_positions[page_granule_positions.size() - 1];
	}
	return -1;
}

float OggPacketSequence::get_length() const
{
	int64_t granule_pos = get_final_granule_pos();
	if (granule_pos < 0) {
		return 0;
	}
	return granule_pos / sampling_rate;
}

Ref<OggPacketSequencePlayback> OggPacketSequence::instantiate_playback()
{
	Ref<OggPacketSequencePlayback> playback;
	playback.instantiate();
	playback->ogg_packet_sequence = Ref<OggPacketSequence>(this);
	playback->data_version = data_version;

	return playback;
}


bool OggPacketSequencePlayback::next_ogg_packet(ogg_packet** p_packet) const
{
	ERR_FAIL_COND_V(data_version != ogg_packet_sequence->data_version, false);
	ERR_FAIL_COND_V(ogg_packet_sequence->page_data.is_empty(), false);
	ERR_FAIL_COND_V(ogg_packet_sequence->page_granule_positions.is_empty(), false);
	ERR_FAIL_COND_V(page_cursor >= ogg_packet_sequence->page_data.size(), false);

	// Move on to the next page if need be. This happens first to help simplify seek logic.
	while (packet_cursor >= ogg_packet_sequence->page_data[page_cursor].size()) {
		packet_cursor = 0;
		page_cursor++;
		if (page_cursor >= ogg_packet_sequence->page_data.size()) {
			return false;
		}
	}

	ERR_FAIL_COND_V(page_cursor >= ogg_packet_sequence->page_data.size(), false);

	packet->b_o_s = page_cursor == 0 && packet_cursor == 0;
	packet->e_o_s = page_cursor == ogg_packet_sequence->page_data.size() - 1 &&
					packet_cursor == ogg_packet_sequence->page_data[page_cursor].size() - 1;
	packet->granulepos = packet_cursor == ogg_packet_sequence->page_data[page_cursor].size() - 1
							 ? ogg_packet_sequence->page_granule_positions[page_cursor]
							 : -1;
	packet->packetno = packetno++;
	packet->bytes = ogg_packet_sequence->page_data[page_cursor][packet_cursor].size();
	packet->packet =
		(unsigned char*)(ogg_packet_sequence->page_data[page_cursor][packet_cursor].ptr());

	*p_packet = packet;

	packet_cursor++;

	return true;
}

uint32_t OggPacketSequencePlayback::seek_page_internal(
	int64_t granule, uint32_t after_page_inclusive, uint32_t before_page_inclusive)
{
	// FIXME: This function needs better corner case handling.
	if (before_page_inclusive == after_page_inclusive) {
		return before_page_inclusive;
	}
	uint32_t actual_middle_page =
		after_page_inclusive + (before_page_inclusive - after_page_inclusive) / 2;
	// Complicating the bisection search algorithm, the middle page might not have a packet that
	// ends on it, which means it might not have a correct granule position. Find a nearby page that
	// does have a packet ending on it.
	uint32_t bisection_page = -1;
	// Don't include before_page_inclusive because that always succeeds and will cause infinite
	// recursion later.
	for (uint32_t test_page = actual_middle_page; test_page < before_page_inclusive; test_page++) {
		if (ogg_packet_sequence->page_data[test_page].size() > 0) {
			bisection_page = test_page;
			break;
		}
	}
	// Check if we have to go backwards.
	if (bisection_page == (unsigned int)-1) {
		for (uint32_t test_page = actual_middle_page; test_page >= after_page_inclusive;
			 test_page--) {
			if (ogg_packet_sequence->page_data[test_page].size() > 0) {
				bisection_page = test_page;
				break;
			}
		}
	}
	if (bisection_page == (unsigned int)-1) {
		return -1;
	}

	int64_t bisection_granule_pos = ogg_packet_sequence->page_granule_positions[bisection_page];
	if (granule > bisection_granule_pos) {
		return seek_page_internal(granule, bisection_page + 1, before_page_inclusive);
	}
	else {
		return seek_page_internal(granule, after_page_inclusive, bisection_page);
	}
}

bool OggPacketSequencePlayback::seek_page(int64_t p_granule_pos)
{
	int correct_page =
		seek_page_internal(p_granule_pos, 0, ogg_packet_sequence->page_data.size() - 1);
	if (correct_page == -1) {
		return false;
	}

	packet_cursor = 0;
	page_cursor = correct_page;

	// Don't pretend subsequent packets are contiguous with previous ones.
	packetno = 0;

	return true;
}

int64_t OggPacketSequencePlayback::get_page_number() const { return page_cursor; }

bool OggPacketSequencePlayback::set_page_number(int64_t p_page_number)
{
	if (p_page_number >= 0 && p_page_number < ogg_packet_sequence->page_data.size()) {
		page_cursor = p_page_number;
		packet_cursor = 0;
		packetno = 0;
		return true;
	}
	return false;
}

OggPacketSequencePlayback::OggPacketSequencePlayback() { packet = new ogg_packet(); }

OggPacketSequencePlayback::~OggPacketSequencePlayback() { delete packet; }

void initialize_ogg_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_ogg_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_OGG_ENABLED)


// --- Merged from modules/vorbis/audio_stream_ogg_vorbis.cpp ---
#if defined(MODULE_VORBIS_ENABLED)
#include <ogg/ogg.h>
#include "core/io/resource_importer.h"
#include "core/io/resource_saver.h"
#include "modules/register_module_types.h"
// --- Inlined from modules/vorbis/audio_stream_ogg_vorbis.h ---
#include <vorbis/codec.h>

#include "servers/audio/audio_stream.h"

class AudioStreamOggVorbis;

class AudioStreamPlaybackOggVorbis : public AudioStreamPlaybackResampled
{
	uint32_t frames_mixed = 0;
	bool active = false;
	bool looping_override = false;
	bool looping = false;
	int loops = 0;

	enum
	{
		FADE_SIZE = 256
	};

	AudioFrame loop_fade[FADE_SIZE];
	int loop_fade_remaining = FADE_SIZE;

	vorbis_info info;
	vorbis_comment comment;
	vorbis_dsp_state dsp_state;
	vorbis_block block;

	bool info_is_allocated = false;
	bool comment_is_allocated = false;
	bool dsp_state_is_allocated = false;
	bool block_is_allocated = false;

	bool ready = false;

	bool have_samples_left = false;
	bool have_packets_left = false;

	friend class AudioStreamOggVorbis;

	Ref<OggPacketSequence> vorbis_data;
	Ref<OggPacketSequencePlayback> vorbis_data_playback;
	Ref<AudioStreamOggVorbis> vorbis_stream;

	bool _is_sample = false;
	Ref<AudioSamplePlayback> sample_playback;

	int _mix_frames(AudioFrame* p_buffer, int p_frames);
	int _mix_frames_vorbis(AudioFrame* p_buffer, int p_frames);

	// Allocates vorbis data structures. Returns true upon success, false on failure.
	bool _alloc_vorbis();

protected:
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; // times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual void set_is_sample(bool p_is_sample) override;
	virtual bool get_is_sample() const override;
	virtual Ref<AudioSamplePlayback> get_sample_playback() const override;
	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;

	AudioStreamPlaybackOggVorbis() {}

	~AudioStreamPlaybackOggVorbis();
};

class AudioStreamOggVorbis final : public AudioStream
{
	friend class AudioStreamPlaybackOggVorbis;

	int channels = 1;
	double length = 0.0;
	bool loop = false;
	double loop_offset = 0.0;

	// Performs a seek to the beginning of the stream, should not be called during playback!
	// Also causes allocation and deallocation.
	void maybe_update_info();

	Ref<OggPacketSequence> packet_sequence;

	double bpm = 0;
	int beat_count = 0;
	int bar_beats = 4;

public:
	static Ref<AudioStreamOggVorbis> load_from_file(const String& p_path);
	static Ref<AudioStreamOggVorbis> load_from_buffer(const Vector<uint8_t>& p_stream_data);

	void set_loop(bool p_enable);
	virtual bool has_loop() const override;

	void set_loop_offset(double p_seconds);
	double get_loop_offset() const;

	void set_bpm(double p_bpm);
	virtual double get_bpm() const override;

	void set_beat_count(int p_beat_count);
	virtual int get_beat_count() const override;

	void set_bar_beats(int p_bar_beats);
	virtual int get_bar_beats() const override;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;

	void set_packet_sequence(Ref<OggPacketSequence> p_packet_sequence);
	Ref<OggPacketSequence> get_packet_sequence() const;

	virtual double get_length() const override; // if supported, otherwise return 0

	virtual bool is_monophonic() const override;

	virtual bool can_be_sampled() const override { return true; }

	virtual Ref<AudioSample> generate_sample() const override;
};

#include "core/io/resource_importer.h"

class ResourceImporterOggVorbis final : public ResourceImporter
{
protected:

public:
#ifdef TOOLS_ENABLED
	virtual bool has_advanced_options() const override;
	virtual void show_advanced_options(const String& p_path) override;
#endif

#ifndef DISABLE_DEPRECATED
	static Ref<AudioStreamOggVorbis> load_from_file(const String& p_path);
	static Ref<AudioStreamOggVorbis> load_from_buffer(const Vector<uint8_t>& p_stream_data);
#endif

	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;
	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

	virtual bool can_import_threaded() const override { return true; }

	ResourceImporterOggVorbis();
};

// --- End modules/vorbis/audio_stream_ogg_vorbis.h ---
#include "core/io/file_access.h"
#include "core/templates/rb_map.h"

int AudioStreamPlaybackOggVorbis::_mix_internal(AudioFrame* p_buffer, int p_frames)
{
	ERR_FAIL_COND_V(!ready, 0);

	if (!active) {
		return 0;
	}

	int todo = p_frames;

	int beat_length_frames = -1;
	bool use_loop = looping_override ? looping : vorbis_stream->loop;

	if (use_loop && vorbis_stream->get_bpm() > 0 && vorbis_stream->get_beat_count() > 0) {
		beat_length_frames = vorbis_stream->get_beat_count() * vorbis_data->get_sampling_rate() *
							 60 / vorbis_stream->get_bpm();
	}

	while (todo > 0 && active) {
		AudioFrame* buffer = p_buffer;
		buffer += p_frames - todo;

		int to_mix = todo;
		if (beat_length_frames >= 0 && (beat_length_frames - (int)frames_mixed) < to_mix) {
			to_mix = MAX(0, beat_length_frames - (int)frames_mixed);
		}

		int mixed = _mix_frames_vorbis(buffer, to_mix);
		ERR_FAIL_COND_V(mixed < 0, 0);
		todo -= mixed;
		frames_mixed += mixed;

		if (loop_fade_remaining < FADE_SIZE) {
			int to_fade = loop_fade_remaining + MIN(FADE_SIZE - loop_fade_remaining, mixed);
			for (int i = loop_fade_remaining; i < to_fade; i++) {
				buffer[i - loop_fade_remaining] +=
					loop_fade[i] * (float(FADE_SIZE - i) / float(FADE_SIZE));
			}
			loop_fade_remaining = to_fade;
		}

		if (beat_length_frames >= 0) {
			/**
			 * Length determined by beat length
			 * This code is commented out because, in practice, it is preferred that the fade
			 * is done by the transitioner and this stream just goes on until it ends while fading
			out.
			 *
			 * End fade implementation is left here for reference in case at some point this feature
			 * is desired.

			if (!beat_loop && (int)frames_mixed > beat_length_frames - FADE_SIZE) {
				print_line("beat length fade/after mix?");
				//No loop, just fade and finish
				for (int i = 0; i < mixed; i++) {
					int idx = frames_mixed + i - mixed;
					buffer[i] *= 1.0 - float(MAX(0, (idx - (beat_length_frames - FADE_SIZE)))) /
			float(FADE_SIZE);
				}
				if ((int)frames_mixed == beat_length_frames) {
					for (int i = p_frames - todo; i < p_frames; i++) {
						p_buffer[i] = AudioFrame(0, 0);
					}
					active = false;
					break;
				}
			}
			else
			**/

			if (use_loop && beat_length_frames <= (int)frames_mixed) {
				// End of file when doing beat-based looping. <= used instead of == because importer
				// editing
				if (!have_packets_left && !have_samples_left) {
					// Nothing remaining, so do nothing.
					loop_fade_remaining = FADE_SIZE;
				}
				else {
					// Add some loop fade;
					int faded_mix = _mix_frames_vorbis(loop_fade, FADE_SIZE);

					for (int i = faded_mix; i < FADE_SIZE; i++) {
						// In case lesss was mixed, pad with zeros
						loop_fade[i] = AudioFrame(0, 0);
					}
					loop_fade_remaining = 0;
				}

				seek(vorbis_stream->loop_offset);
				loops++;
				// We still have buffer to fill, start from this element in the next iteration.
				continue;
			}
		}

		if (!have_packets_left && !have_samples_left) {
			// Actual end of file!
			bool is_not_empty = mixed > 0 || vorbis_stream->get_length() > 0;
			if (use_loop && is_not_empty) {
				// loop

				seek(vorbis_stream->loop_offset);
				loops++;
				// We still have buffer to fill, start from this element in the next iteration.

			}
			else {
				for (int i = p_frames - todo; i < p_frames; i++) {
					p_buffer[i] = AudioFrame(0, 0);
				}
				active = false;
			}
		}
	}
	return p_frames - todo;
}

int AudioStreamPlaybackOggVorbis::_mix_frames_vorbis(AudioFrame* p_buffer, int p_frames)
{
	ERR_FAIL_COND_V(!ready, p_frames);
	if (!have_samples_left) {
		ogg_packet* packet = nullptr;
		int err;

		if (!vorbis_data_playback->next_ogg_packet(&packet)) {
			have_packets_left = false;
			WARN_PRINT("ran out of packets in stream");
			return -1;
		}

		err = vorbis_synthesis(&block, packet);
		ERR_FAIL_COND_V_MSG(err != 0, p_frames, "Error during vorbis synthesis " + itos(err));

		err = vorbis_synthesis_blockin(&dsp_state, &block);
		ERR_FAIL_COND_V_MSG(
			err != 0, p_frames, "Error during vorbis block processing " + itos(err));

		have_packets_left = !packet->e_o_s;
	}

	float** pcm; // Accessed with pcm[channel_idx][sample_idx].

	int frames = vorbis_synthesis_pcmout(&dsp_state, &pcm);
	if (frames > p_frames) {
		frames = p_frames;
		have_samples_left = true;
	}
	else {
		have_samples_left = false;
	}

	if (info.channels > 1) {
		for (int frame = 0; frame < frames; frame++) {
			p_buffer[frame].left = pcm[0][frame];
			p_buffer[frame].right = pcm[1][frame];
		}
	}
	else {
		for (int frame = 0; frame < frames; frame++) {
			p_buffer[frame].left = pcm[0][frame];
			p_buffer[frame].right = pcm[0][frame];
		}
	}
	vorbis_synthesis_read(&dsp_state, frames);
	return frames;
}

float AudioStreamPlaybackOggVorbis::get_stream_sampling_rate()
{
	return vorbis_data->get_sampling_rate();
}

bool AudioStreamPlaybackOggVorbis::_alloc_vorbis()
{
	vorbis_info_init(&info);
	info_is_allocated = true;
	vorbis_comment_init(&comment);
	comment_is_allocated = true;

	ERR_FAIL_COND_V(vorbis_data.is_null(), false);
	vorbis_data_playback = vorbis_data->instantiate_playback();

	ogg_packet* packet;
	int err;

	for (int i = 0; i < 3; i++) {
		if (!vorbis_data_playback->next_ogg_packet(&packet)) {
			WARN_PRINT("Not enough packets to parse header");
			return false;
		}

		err = vorbis_synthesis_headerin(&info, &comment, packet);
		ERR_FAIL_COND_V_MSG(err != 0, false, "Error parsing header");
	}

	err = vorbis_synthesis_init(&dsp_state, &info);
	ERR_FAIL_COND_V_MSG(err != 0, false, "Error initializing dsp state");
	dsp_state_is_allocated = true;

	err = vorbis_block_init(&dsp_state, &block);
	ERR_FAIL_COND_V_MSG(err != 0, false, "Error initializing block");
	block_is_allocated = true;

	ready = true;

	return true;
}

void AudioStreamPlaybackOggVorbis::start(double p_from_pos)
{
	ERR_FAIL_COND(!ready);
	loop_fade_remaining = FADE_SIZE;
	active = true;
	seek(p_from_pos);
	loops = 0;
	begin_resample();
}

void AudioStreamPlaybackOggVorbis::stop() { active = false; }

bool AudioStreamPlaybackOggVorbis::is_playing() const { return active; }

int AudioStreamPlaybackOggVorbis::get_loop_count() const { return loops; }

double AudioStreamPlaybackOggVorbis::get_playback_position() const
{
	return double(frames_mixed) / (double)vorbis_data->get_sampling_rate();
}

void AudioStreamPlaybackOggVorbis::tag_used_streams()
{
	vorbis_stream->tag_used(get_playback_position());
}

void AudioStreamPlaybackOggVorbis::seek(double p_time)
{
	ERR_FAIL_COND(!ready);
	ERR_FAIL_COND(vorbis_stream.is_null());
	if (!active) {
		return;
	}

	if (p_time >= vorbis_stream->get_length()) {
		p_time = 0;
	}

	frames_mixed = uint32_t(vorbis_data->get_sampling_rate() * p_time);

	const int64_t desired_sample = p_time * get_stream_sampling_rate();

	if (!vorbis_data_playback->seek_page(desired_sample)) {
		WARN_PRINT("seek failed");
		return;
	}

	// We want to start decoding before the page that we expect the sample to be in (the sample may
	// be part of a partial packet across page boundaries). Otherwise, the decoder may not have
	// synchronized before reaching the sample.
	int64_t start_page_number = vorbis_data_playback->get_page_number() - 1;
	if (start_page_number < 0) {
		start_page_number = 0;
	}

	while (true) {
		ogg_packet* packet;
		int err;

		// We start at an unknown granule position.
		int64_t granule_pos = -1;

		// Decode data until we get to the desired sample or notice that we have read past it.
		vorbis_data_playback->set_page_number(start_page_number);
		vorbis_synthesis_restart(&dsp_state);

		while (true) {
			if (!vorbis_data_playback->next_ogg_packet(&packet)) {
				WARN_PRINT_ONCE("Seeking beyond limits");
				return;
			}

			err = vorbis_synthesis(&block, packet);
			if (err != OV_ENOTAUDIO) {
				ERR_FAIL_COND_MSG(err != 0, "Error during vorbis synthesis " + itos(err) + ".");

				err = vorbis_synthesis_blockin(&dsp_state, &block);
				ERR_FAIL_COND_MSG(
					err != 0, "Error during vorbis block processing " + itos(err) + ".");

				int samples_out = vorbis_synthesis_pcmout(&dsp_state, nullptr);

				if (granule_pos < 0) {
					// We don't know where we are yet, so just keep on decoding.
					err = vorbis_synthesis_read(&dsp_state, samples_out);
					ERR_FAIL_COND_MSG(
						err != 0, "Error during vorbis read updating " + itos(err) + ".");
				}
				else if (granule_pos + samples_out >= desired_sample) {
					// Our sample is in this block. Skip the beginning of the block up to the
					// sample, then return.
					int skip_samples = (int)(desired_sample - granule_pos);
					err = vorbis_synthesis_read(&dsp_state, skip_samples);
					ERR_FAIL_COND_MSG(
						err != 0, "Error during vorbis read updating " + itos(err) + ".");
					have_samples_left = skip_samples < samples_out;
					have_packets_left = !packet->e_o_s;
					return;
				}
				else {
					// Our sample is not in this block. Skip it.
					err = vorbis_synthesis_read(&dsp_state, samples_out);
					ERR_FAIL_COND_MSG(
						err != 0, "Error during vorbis read updating " + itos(err) + ".");
					granule_pos += samples_out;
				}
			}
			if (packet->granulepos != -1) {
				// We found an update to our granule position.
				granule_pos = packet->granulepos;
				if (granule_pos > desired_sample) {
					// We've read past our sample. We need to start on an earlier page.
					if (start_page_number == 0) {
						// We didn't find the sample even reading from the beginning.
						have_samples_left = false;
						have_packets_left = !packet->e_o_s;
						return;
					}
					start_page_number--;
					break;
				}
			}
			if (packet->e_o_s) {
				// We've reached the end of the stream and didn't find our sample.
				have_samples_left = false;
				have_packets_left = false;
				return;
			}
		}
	}
}

void AudioStreamPlaybackOggVorbis::set_is_sample(bool p_is_sample) { _is_sample = p_is_sample; }

bool AudioStreamPlaybackOggVorbis::get_is_sample() const { return _is_sample; }

Ref<AudioSamplePlayback> AudioStreamPlaybackOggVorbis::get_sample_playback() const
{
	return sample_playback;
}

void AudioStreamPlaybackOggVorbis::set_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	sample_playback = p_playback;
	if (sample_playback.is_valid()) {
		sample_playback->stream_playback = Ref<AudioStreamPlayback>(this);
	}
}

AudioStreamPlaybackOggVorbis::~AudioStreamPlaybackOggVorbis()
{
	if (block_is_allocated) {
		vorbis_block_clear(&block);
	}
	if (dsp_state_is_allocated) {
		vorbis_dsp_clear(&dsp_state);
	}
	if (comment_is_allocated) {
		vorbis_comment_clear(&comment);
	}
	if (info_is_allocated) {
		vorbis_info_clear(&info);
	}
}

Ref<AudioStreamPlayback> AudioStreamOggVorbis::instantiate_playback()
{
	Ref<AudioStreamPlaybackOggVorbis> ovs;

	ERR_FAIL_COND_V(packet_sequence.is_null(), nullptr);

	ovs.instantiate();
	ovs->vorbis_stream = Ref<AudioStreamOggVorbis>(this);
	ovs->vorbis_data = packet_sequence;
	ovs->frames_mixed = 0;
	ovs->active = false;
	ovs->loops = 0;
	if (ovs->_alloc_vorbis()) {
		return ovs;
	}
	// Failed to allocate data structures.
	return nullptr;
}

void AudioStreamOggVorbis::set_packet_sequence(Ref<OggPacketSequence> p_packet_sequence)
{
	packet_sequence = p_packet_sequence;
	if (packet_sequence.is_valid()) {
		maybe_update_info();
	}
}

Ref<OggPacketSequence> AudioStreamOggVorbis::get_packet_sequence() const { return packet_sequence; }

void AudioStreamOggVorbis::set_loop(bool p_enable) { loop = p_enable; }

bool AudioStreamOggVorbis::has_loop() const { return loop; }

void AudioStreamOggVorbis::set_loop_offset(double p_seconds) { loop_offset = p_seconds; }

double AudioStreamOggVorbis::get_loop_offset() const { return loop_offset; }

double AudioStreamOggVorbis::get_length() const
{
	ERR_FAIL_COND_V(packet_sequence.is_null(), 0);
	return packet_sequence->get_length();
}

void AudioStreamOggVorbis::set_bpm(double p_bpm)
{
	ERR_FAIL_COND(p_bpm < 0);
	bpm = p_bpm;
	emit_changed();
}

double AudioStreamOggVorbis::get_bpm() const { return bpm; }

void AudioStreamOggVorbis::set_beat_count(int p_beat_count)
{
	ERR_FAIL_COND(p_beat_count < 0);
	beat_count = p_beat_count;
	emit_changed();
}

int AudioStreamOggVorbis::get_beat_count() const { return beat_count; }

void AudioStreamOggVorbis::set_bar_beats(int p_bar_beats)
{
	ERR_FAIL_COND(p_bar_beats < 2);
	bar_beats = p_bar_beats;
	emit_changed();
}

int AudioStreamOggVorbis::get_bar_beats() const { return bar_beats; }

bool AudioStreamOggVorbis::is_monophonic() const { return false; }

Ref<AudioStreamOggVorbis> AudioStreamOggVorbis::load_from_buffer(
	const Vector<uint8_t>& p_stream_data)
{
	Ref<AudioStreamOggVorbis> ogg_vorbis_stream;
	ogg_vorbis_stream.instantiate();

	Ref<OggPacketSequence> ogg_packet_sequence;
	ogg_packet_sequence.instantiate();

	ogg_stream_state stream_state;
	ogg_sync_state sync_state;
	ogg_page page;
	ogg_packet packet;
	bool initialized_stream = false;

	ogg_sync_init(&sync_state);
	const long OGG_SYNC_BUFFER_SIZE = 8192;
	int err;
	size_t cursor = 0;
	size_t packet_count = 0;
	bool done = false;
	while (!done) {
		err = ogg_sync_check(&sync_state);
		ERR_FAIL_COND_V_MSG(err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));
		while (ogg_sync_pageout(&sync_state, &page) != 1) {
			if (cursor >= size_t(p_stream_data.size())) {
				done = true;
				break;
			}
			err = ogg_sync_check(&sync_state);
			ERR_FAIL_COND_V_MSG(
				err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));
			char* sync_buf = ogg_sync_buffer(&sync_state, OGG_SYNC_BUFFER_SIZE);
			err = ogg_sync_check(&sync_state);
			ERR_FAIL_COND_V_MSG(
				err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));
			size_t copy_size = p_stream_data.size() - cursor;
			if (copy_size > OGG_SYNC_BUFFER_SIZE) {
				copy_size = OGG_SYNC_BUFFER_SIZE;
			}
			memcpy(sync_buf, &p_stream_data[cursor], copy_size);
			ogg_sync_wrote(&sync_state, copy_size);
			cursor += copy_size;
			err = ogg_sync_check(&sync_state);
			ERR_FAIL_COND_V_MSG(
				err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));
		}
		if (done) {
			break;
		}
		err = ogg_sync_check(&sync_state);
		ERR_FAIL_COND_V_MSG(err != 0, Ref<AudioStreamOggVorbis>(), "Ogg sync error " + itos(err));

		// Have a page now.
		if (!initialized_stream) {
			if (ogg_stream_init(&stream_state, ogg_page_serialno(&page))) {
				ERR_FAIL_V_MSG(
					Ref<AudioStreamOggVorbis>(), "Failed allocating memory for Ogg Vorbis stream.");
			}
			initialized_stream = true;
		}
		ogg_stream_pagein(&stream_state, &page);
		err = ogg_stream_check(&stream_state);
		ERR_FAIL_COND_V_MSG(err != 0, Ref<AudioStreamOggVorbis>(), "Ogg stream error " + itos(err));
		int desync_iters = 0;

		RBMap<uint64_t, Vector<Vector<uint8_t>>> sorted_packets;
		int64_t granule_pos = 0;

		while (true) {
			err = ogg_stream_packetout(&stream_state, &packet);
			if (err == -1) {
				// According to the docs this is usually recoverable, but don't sit here spinning
				// forever.
				desync_iters++;
				WARN_PRINT_ONCE("Desync during ogg import.");
				ERR_FAIL_COND_V_MSG(desync_iters > 100, Ref<AudioStreamOggVorbis>(),
					"Packet sync issue during Ogg import");
				continue;
			}
			else if (err == 0) {
				// Not enough data to fully reconstruct a packet. Go on to the next page.
				break;
			}
			if (packet_count == 0 && vorbis_synthesis_idheader(&packet) == 0) {
				print_verbose("Found a non-vorbis-header packet in a header position");
				// Clearly this logical stream is not a vorbis stream, so destroy it and try again
				// with the next page.
				if (initialized_stream) {
					ogg_stream_clear(&stream_state);
					initialized_stream = false;
				}
				break;
			}
			if (packet.granulepos > granule_pos) {
				granule_pos = packet.granulepos;
			}

			if (packet.bytes > 0) {
				PackedByteArray data;
				data.resize(packet.bytes);
				memcpy(data.ptrw(), packet.packet, packet.bytes);
				sorted_packets[granule_pos].push_back(data);
				packet_count++;
			}
		}
		Vector<Vector<uint8_t>> packet_data;
		for (const KeyValue<uint64_t, Vector<Vector<uint8_t>>>& pair : sorted_packets) {
			for (const Vector<uint8_t>& packets : pair.value) {
				packet_data.push_back(packets);
			}
		}
		if (initialized_stream && packet_data.size() > 0) {
			ogg_packet_sequence->push_page(ogg_page_granulepos(&page), packet_data);
		}
	}
	if (initialized_stream) {
		ogg_stream_clear(&stream_state);
	}
	ogg_sync_clear(&sync_state);

	if (ogg_packet_sequence->get_packet_granule_positions().is_empty()) {
		ERR_FAIL_V_MSG(Ref<AudioStreamOggVorbis>(),
			"Ogg Vorbis decoding failed. Check that your data is a valid Ogg Vorbis audio stream.");
	}

	ogg_vorbis_stream->set_packet_sequence(ogg_packet_sequence);

	return ogg_vorbis_stream;
}

Ref<AudioStreamOggVorbis> AudioStreamOggVorbis::load_from_file(const String& p_path)
{
	const Vector<uint8_t> stream_data = FileAccess::get_file_as_bytes(p_path);
	ERR_FAIL_COND_V_MSG(stream_data.is_empty(), Ref<AudioStreamOggVorbis>(),
		vformat("Cannot open file '%s'.", p_path));
	return load_from_buffer(stream_data);
}

void AudioStreamOggVorbis::maybe_update_info() {}

Ref<AudioSample> AudioStreamOggVorbis::generate_sample() const { return Ref<AudioSample>(); }

String ResourceImporterOggVorbis::get_importer_name() const { return "oggvorbisstr"; }

String ResourceImporterOggVorbis::get_visible_name() const { return "Ogg Vorbis"; }

void ResourceImporterOggVorbis::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("ogg");
}

String ResourceImporterOggVorbis::get_save_extension() const { return "oggvorbisstr"; }

String ResourceImporterOggVorbis::get_resource_type() const { return "AudioStreamOggVorbis"; }

#ifdef TOOLS_ENABLED
bool ResourceImporterOggVorbis::has_advanced_options() const { return false; }

void ResourceImporterOggVorbis::show_advanced_options(const String& p_path) {}
#endif

#ifndef DISABLE_DEPRECATED
Ref<AudioStreamOggVorbis> ResourceImporterOggVorbis::load_from_buffer(
	const Vector<uint8_t>& p_stream_data)
{
	return AudioStreamOggVorbis::load_from_buffer(p_stream_data);
}

Ref<AudioStreamOggVorbis> ResourceImporterOggVorbis::load_from_file(const String& p_path)
{
	return AudioStreamOggVorbis::load_from_file(p_path);
}
#endif

ResourceImporterOggVorbis::ResourceImporterOggVorbis() {}

int ResourceImporterOggVorbis::get_preset_count() const { return 0; }

String ResourceImporterOggVorbis::get_preset_name(int p_preset) const { return String(); }

void ResourceImporterOggVorbis::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}

#ifdef TOOLS_ENABLED
#endif

void initialize_vorbis_module(ModuleInitializationLevel p_level)
{
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		Ref<ResourceImporterOggVorbis> ogg_vorbis_importer;
		ogg_vorbis_importer.instantiate();
		ResourceFormatImporter::add_importer(ogg_vorbis_importer);
	}
#endif
}

void uninitialize_vorbis_module(ModuleInitializationLevel p_level) {}
#endif // defined(MODULE_VORBIS_ENABLED)


// --- Merged from modules/theora/video_stream_theora.cpp ---
#if defined(MODULE_THEORA_ENABLED)
// --- Inlined from modules/theora/video_stream_theora.h ---
#include "core/io/resource_loader.h"

class ResourceFormatLoaderTheora final : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
};

#ifdef TOOLS_ENABLED
#include <theora/theoraenc.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include "core/io/file_access.h"
#include "servers/audio/audio_server.h"
#include "main/main.h"

class MovieWriterOGV final : public MovieWriter
{
	uint32_t mix_rate = 48000;
	AudioServer::SpeakerMode speaker_mode = AudioServer::SPEAKER_MODE_STEREO;
	String base_path;
	uint32_t frame_count = 0;
	uint32_t fps = 0;
	uint32_t audio_ch = 0;
	uint32_t audio_frames = 0;

	Ref<FileAccess> f;

	// Vorbis quality -0.1 to 1 (-0.1 yields smallest files but lowest fidelity; 1 yields highest
	// fidelity but large files. '0.2' is a reasonable default).
	float audio_quality = 0.5;

	// Bitrate target for Theora video.
	int video_bitrate = 0;

	// Theora quality selector from 0 to 1.0 (0 yields smallest files but lowest video quality. 1.0
	// yields highest fidelity but large files).
	float video_quality = 0.75;

	// Video stream keyframe frequency (one every N frames).
	ogg_uint32_t keyframe_frequency = 64;

	// Sets the encoder speed level. Higher speed levels favor quicker encoding over better quality
	// per bit. Depending on the encoding mode, and the internal algorithms used, quality may
	// actually improve with higher speeds, but in this case bitrate will also likely increase. The
	// maximum value, and the meaning of each value, are implementation-specific and may change
	// depending on the current encoding mode.
	int speed = 4;

	// Take physical pages, weld into a logical stream of packets.
	ogg_stream_state to;

	// Take physical pages, weld into a logical stream of packets.
	ogg_stream_state vo;

	// Theora encoding context.
	th_enc_ctx* td;

	// Theora bitstream information.
	th_info ti;

	// Theora comment information.
	th_comment tc;

	// Vorbis bitstream information.
	vorbis_info vi;

	// Vorbis comment information.
	vorbis_comment vc;

	// Central working state for the packet->PCM decoder.
	vorbis_dsp_state vd;

	// Local working space for packet->PCM decode.
	vorbis_block vb;

	// Video buffer.
	uint8_t *y, *u, *v;
	th_ycbcr_buffer ycbcr;

	bool audio_flag = false;
	bool video_flag = false;
	ogg_page audio_page;
	ogg_page video_page;
	ogg_page backup_page;
	unsigned int backup_page_size = 0;
	unsigned char* backup_page_data = nullptr;

	void write_to_file(bool p_finish = false);
	void push_audio(const int32_t* p_audio_data);
	void push_video(const Ref<Image>& p_image);
	void pull_audio(bool p_last = false);
	void pull_video(bool p_last = false);
	void save_page(ogg_page page);
	void restore_page(ogg_page* page);

	inline int ilog(unsigned _v)
	{
		int ret;
		for (ret = 0; _v; ret++) {
			_v >>= 1;
		}
		return ret;
	}

protected:
	virtual uint32_t get_audio_mix_rate() const override;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const override;
	virtual void get_supported_extensions(List<String>* r_extensions) const override;

	virtual Error write_begin(
		const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path) override;
	virtual Error write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data) override;
	virtual void write_end() override;

	virtual bool handles_file(const String& p_path) const override;

public:
	MovieWriterOGV();
};
#endif // TOOLS_ENABLED

// --- End modules/theora/video_stream_theora.h ---
#include "modules/register_module_types.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"

#include <theora/theoradec.h>
#include <thirdparty/misc/yuv2rgb.h>
#include <vorbis/codec.h>

struct VideoStreamPlayback::TheoraData
{
	Image::Format format = Image::Format::FORMAT_L8;
	Vector<uint8_t> frame_data;
	int frames_pending = 0;
	Ref<FileAccess> file;
	String file_name;
	Point2i size;
	Rect2i region;

	float* audio_buffer = nullptr;
	int audio_buffer_size = 0;
	int audio_ptr_start = 0;
	int audio_ptr_end = 0;

	bool theora_eos = false;
	bool vorbis_eos = false;

	ogg_sync_state oy = {};
	ogg_stream_state vo = {};
	ogg_stream_state to = {};
	th_info ti = {};
	th_comment tc = {};
	th_dec_ctx* td = nullptr;
	vorbis_info vi = {};
	vorbis_dsp_state vd = {};
	vorbis_block vb = {};
	vorbis_comment vc = {};
	th_pixel_fmt px_fmt = TH_PF_420;
	double frame_duration = 0;
	double stream_length = 0;
	int64_t stream_data_offset = 0;
	int64_t stream_data_size = 0;

	int pp_level_max = 0;
	int pp_level = 0;
	int pp_inc = 0;

	bool playing = false;
	bool paused = false;

	bool dup_frame = false;
	bool has_video = false;
	bool has_audio = false;
	bool video_ready = false;
	bool video_done = false;
	bool audio_done = false;

	double time = 0;
	double next_frame_time = 0;
	double current_frame_time = 0;
	double delay_compensation = 0;

	Ref<ImageTexture> texture;
	int audio_track = 0;

	int buffer_data()
	{
		char* buffer = ogg_sync_buffer(&oy, 4096);
		uint64_t bytes = file->get_buffer((uint8_t*)buffer, 4096);
		ogg_sync_wrote(&oy, bytes);
		return bytes;
	}

	int queue_page(ogg_page* page)
	{
		ogg_stream_pagein(&to, page);
		if (to.e_o_s) {
			theora_eos = true;
		}
		if (has_audio) {
			ogg_stream_pagein(&vo, page);
			if (vo.e_o_s) {
				vorbis_eos = true;
			}
		}
		return 0;
	}

	int read_page(ogg_page* page)
	{
		int ret = 0;
		while (ret <= 0) {
			ret = ogg_sync_pageout(&oy, page);
			if (ret <= 0) {
				int bytes = buffer_data();
				if (bytes == 0) {
					return 0;
				}
			}
		}
		return ret;
	}

	double get_page_time(ogg_page* page)
	{
		uint64_t granulepos = ogg_page_granulepos(page);
		int page_serialno = ogg_page_serialno(page);
		double page_time = -1;

		if (page_serialno == to.serialno) {
			page_time = th_granule_time(td, granulepos);
		}
		if (has_audio && page_serialno == vo.serialno) {
			page_time = vorbis_granule_time(&vd, granulepos);
		}

		return page_time;
	}

	int feed_pages()
	{
		int pages = 0;
		ogg_page og;

		while (pages == 0) {
			while (ogg_sync_pageout(&oy, &og) > 0) {
				queue_page(&og);
				pages++;
			}
			if (pages == 0) {
				int bytes = buffer_data();
				if (bytes == 0) {
					break;
				}
			}
		}

		return pages;
	}

	int64_t seek_streams(
		double p_time, int64_t& cur_video_granulepos, int64_t& cur_audio_granulepos)
	{
		const int64_t min_seek = 512 * 1024;
		int64_t target_video_granulepos;
		int64_t target_audio_granulepos;
		double target_time = 0;
		int64_t seek_pos;

		if (p_time == -1) {
			seek_pos = MAX(stream_data_size - min_seek, stream_data_offset);
			target_video_granulepos = INT64_MAX;
			target_audio_granulepos = INT64_MAX;
		}
		else {
			int64_t video_frame = (int64_t)(p_time / frame_duration);
			target_video_granulepos = MAX(1LL, video_frame - (1LL << ti.keyframe_granule_shift))
									  << ti.keyframe_granule_shift;
			target_audio_granulepos = 0;
			seek_pos = MAX(((target_video_granulepos >> ti.keyframe_granule_shift) - 1) *
							   frame_duration * stream_data_size / stream_length,
				stream_data_offset);
			target_time = th_granule_time(td, target_video_granulepos);
			if (has_audio) {
				target_audio_granulepos = video_frame * frame_duration * vi.rate;
				target_time = MIN(target_time, vorbis_granule_time(&vd, target_audio_granulepos));
			}
		}

		int64_t video_seek_pos = seek_pos;
		int64_t audio_seek_pos = seek_pos;
		double backtrack_time = 0;
		bool video_catch = false;
		bool audio_catch = false;
		int64_t last_video_granule_seek_pos = seek_pos;
		int64_t last_audio_granule_seek_pos = seek_pos;

		cur_video_granulepos = -1;
		cur_audio_granulepos = -1;

		while (!video_catch || (has_audio && !audio_catch)) {
			if (seek_pos < stream_data_offset) {
				seek_pos = stream_data_offset;
			}
			file->seek(seek_pos);
			ogg_sync_reset(&oy);

			backtrack_time = 0;
			last_video_granule_seek_pos = seek_pos;
			last_audio_granule_seek_pos = seek_pos;
			while (!video_catch || (has_audio && !audio_catch)) {
				ogg_page page;
				uint64_t last_seek_pos = file->get_position() - oy.fill + oy.returned;
				int ret = read_page(&page);
				if (ret <= 0) {
					if (seek_pos < stream_data_offset) {
						return -1;
					}
					seek_pos -= min_seek;
					break;
				}
				int64_t cur_granulepos = ogg_page_granulepos(&page);
				if (cur_granulepos >= 0) {
					int page_serialno = ogg_page_serialno(&page);
					if (!video_catch && page_serialno == to.serialno) {
						if (cur_granulepos >= target_video_granulepos) {
							video_catch = true;
							if (cur_video_granulepos < 0) {
								backtrack_time = MAX(backtrack_time,
									1 + th_granule_time(td, cur_granulepos) - target_time);
							}
						}
						else {
							video_seek_pos = last_video_granule_seek_pos;
							cur_video_granulepos = cur_granulepos;
						}
						last_video_granule_seek_pos = last_seek_pos;
					}
					if ((has_audio && !audio_catch) && page_serialno == vo.serialno) {
						if (cur_granulepos >= target_audio_granulepos) {
							audio_catch = true;
							if (cur_audio_granulepos < 0) {
								backtrack_time = MAX(backtrack_time,
									1 + vorbis_granule_time(&vd, cur_granulepos) - target_time);
							}
						}
						else {
							audio_seek_pos = last_audio_granule_seek_pos;
							cur_audio_granulepos = cur_granulepos;
						}
						last_audio_granule_seek_pos = last_seek_pos;
					}
				}
			}
			if (backtrack_time > 0) {
				if (seek_pos <= stream_data_offset) {
					break;
				}
				int64_t delta_seek = MAX(backtrack_time * stream_data_size / stream_length, min_seek);
				seek_pos -= delta_seek;
			}
			video_catch = cur_video_granulepos != -1;
			audio_catch = cur_audio_granulepos != -1;
		}

		if (cur_video_granulepos < (1LL << ti.keyframe_granule_shift)) {
			video_seek_pos = stream_data_offset;
			cur_video_granulepos = 1LL << ti.keyframe_granule_shift;
		}
		if (has_audio) {
			if (cur_audio_granulepos == -1) {
				audio_seek_pos = stream_data_offset;
				cur_audio_granulepos = 0;
			}
			seek_pos = MIN(video_seek_pos, audio_seek_pos);
		}
		else {
			seek_pos = video_seek_pos;
		}

		return seek_pos;
	}

	void video_write(th_ycbcr_buffer yuv)
	{
		uint8_t* w = frame_data.ptrw();
		char* dst = (char*)w;
		uint32_t y_offset = region.position.y * yuv[0].stride + region.position.x;
		uint32_t uv_offset = 0;

		if (px_fmt == TH_PF_444) {
			uv_offset += region.position.y * yuv[1].stride + region.position.x;
			yuv444_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}
		else if (px_fmt == TH_PF_422) {
			uv_offset += region.position.y * yuv[1].stride + region.position.x / 2;
			yuv422_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}
		else if (px_fmt == TH_PF_420) {
			uv_offset += region.position.y * yuv[1].stride / 2 + region.position.x / 2;
			yuv420_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}

		Ref<Image> img;
		img.instantiate(region.size.x, region.size.y, false, Image::FORMAT_RGBA8, frame_data);
		texture->update(img);
	}

	void clear()
	{
		if (!file.is_null()) {
			file.unref();
		}
		if (has_audio) {
			vorbis_block_clear(&vb);
			vorbis_dsp_clear(&vd);
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			ogg_stream_clear(&vo);
			if (audio_buffer_size) {
				memdelete_arr(audio_buffer);
			}
		}
		if (has_video) {
			th_decode_free(td);
			th_comment_clear(&tc);
			th_info_clear(&ti);
			ogg_stream_clear(&to);
			ogg_sync_clear(&oy);
		}

		audio_buffer = nullptr;
		playing = false;
		has_video = false;
		has_audio = false;
		theora_eos = false;
		vorbis_eos = false;
	}

	void find_streams(th_setup_info*& ts)
	{
		ogg_stream_state test;
		ogg_packet op;
		ogg_page og;
		int stateflag = 0;
		int audio_track_skip = audio_track;

		while (!stateflag) {
			int ret = buffer_data();
			if (!ret) {
				break;
			}
			while (ogg_sync_pageout(&oy, &og) > 0) {
				if (!ogg_page_bos(&og)) {
					queue_page(&og);
					stateflag = 1;
					break;
				}

				ogg_stream_init(&test, ogg_page_serialno(&og));
				ogg_stream_pagein(&test, &og);
				ogg_stream_packetout(&test, &op);

				if (!has_video && th_decode_headerin(&ti, &tc, &ts, &op) >= 0) {
					memcpy(&to, &test, sizeof(test));
					has_video = true;
				}
				else if (!has_audio && vorbis_synthesis_headerin(&vi, &vc, &op) >= 0) {
					if (audio_track_skip) {
						vorbis_info_clear(&vi);
						vorbis_comment_clear(&vc);
						ogg_stream_clear(&test);
						vorbis_info_init(&vi);
						vorbis_comment_init(&vc);
						audio_track_skip--;
					}
					else {
						memcpy(&vo, &test, sizeof(test));
						has_audio = true;
					}
				}
				else {
					ogg_stream_clear(&test);
				}
			}
		}
	}

	void read_headers(th_setup_info*& ts)
	{
		ogg_packet op;
		int theora_header_packets = 1;
		int vorbis_header_packets = 1;

		while (theora_header_packets < 3 || (has_audio && vorbis_header_packets < 3)) {
			while (theora_header_packets < 3 && ogg_stream_packetout(&to, &op) > 0) {
				if (th_decode_headerin(&ti, &tc, &ts, &op) > 0) {
					theora_header_packets++;
				}
			}

			while (has_audio && vorbis_header_packets < 3 && ogg_stream_packetout(&vo, &op) > 0) {
				if (!vorbis_synthesis_headerin(&vi, &vc, &op)) {
					vorbis_header_packets++;
				}
			}

			if (theora_header_packets < 3 || (has_audio && vorbis_header_packets < 3)) {
				ogg_page page;
				if (read_page(&page)) {
					queue_page(&page);
				}
				else {
					fprintf(stderr, "End of file while searching for codec headers.\n");
					break;
				}
			}
		}

		has_video = theora_header_packets == 3;
		has_audio = vorbis_header_packets == 3;
	}

	double get_time() const
	{
		return time - delay_compensation;
	}

	_FORCE_INLINE_ bool send_audio(AudioMixCallback p_callback, void* p_udata)
	{
		if (audio_ptr_end > 0) {
			int mixed = p_callback
				? p_callback(
					  p_udata, &audio_buffer[audio_ptr_start * vi.channels], audio_ptr_end - audio_ptr_start)
				: (audio_ptr_end - audio_ptr_start);
			audio_ptr_start += mixed;
			if (audio_ptr_start == audio_ptr_end) {
				audio_ptr_start = 0;
				audio_ptr_end = 0;
			}
			else {
				return false;
			}
		}
		return true;
	}
};

void VideoStreamPlayback::set_file(const String& p_file)
{
	ERR_FAIL_COND(data->playing);
	th_setup_info* ts = nullptr;

	data->clear();

	data->file = FileAccess::open(p_file, FileAccess::READ);
	ERR_FAIL_COND_MSG(data->file.is_null(), "Cannot open file '" + p_file + "'.");

	data->file_name = p_file;

	ogg_sync_init(&data->oy);

	vorbis_info_init(&data->vi);
	vorbis_comment_init(&data->vc);

	th_comment_init(&data->tc);
	th_info_init(&data->ti);

	memset(&data->to, 0, sizeof(data->to));
	memset(&data->vo, 0, sizeof(data->vo));

	data->find_streams(ts);
	data->read_headers(ts);

	if (!data->has_audio) {
		vorbis_comment_clear(&data->vc);
		vorbis_info_clear(&data->vi);
		if (!ogg_stream_check(&data->vo)) {
			ogg_stream_clear(&data->vo);
		}
	}

	if (!data->has_video) {
		th_setup_free(ts);
		th_comment_clear(&data->tc);
		th_info_clear(&data->ti);
		if (!ogg_stream_check(&data->to)) {
			ogg_stream_clear(&data->to);
		}
		data->file.unref();

		vorbis_comment_clear(&data->vc);
		vorbis_info_clear(&data->vi);
		if (!ogg_stream_check(&data->vo)) {
			ogg_stream_clear(&data->vo);
		}

		data->has_video = false;
		data->has_audio = false;
		ERR_FAIL_MSG("File '" + p_file + "' has no video stream.");
	}

	data->td = th_decode_alloc(&data->ti, ts);
	th_setup_free(ts);
	data->px_fmt = data->ti.pixel_fmt;
	switch (data->ti.pixel_fmt) {
	case TH_PF_420:
	case TH_PF_422:
	case TH_PF_444:
		break;
	default:
		WARN_PRINT(" video\n  (UNKNOWN Chroma sampling!)\n");
		break;
	}
	th_decode_ctl(
		data->td, TH_DECCTL_GET_PPLEVEL_MAX, &data->pp_level_max, sizeof(data->pp_level_max));
	data->pp_level = 0;
	th_decode_ctl(data->td, TH_DECCTL_SET_PPLEVEL, &data->pp_level, sizeof(data->pp_level));
	data->pp_inc = 0;

	data->size.x = data->ti.frame_width;
	data->size.y = data->ti.frame_height;
	data->region.position.x = data->ti.pic_x;
	data->region.position.y = data->ti.pic_y;
	data->region.size.x = data->ti.pic_width;
	data->region.size.y = data->ti.pic_height;

	Ref<Image> img = Image::create_empty(
		data->region.size.x, data->region.size.y, false, Image::FORMAT_RGBA8);
	data->texture->set_image(img);
	data->frame_data.resize(data->region.size.x * data->region.size.y * 4);

	data->frame_duration = (double)data->ti.fps_denominator / data->ti.fps_numerator;

	if (data->has_audio) {
		vorbis_synthesis_init(&data->vd, &data->vi);
		vorbis_block_init(&data->vd, &data->vb);
		data->audio_buffer_size = MIN(data->vi.channels, 8) * 1024;
		data->audio_buffer = memnew_arr(float, data->audio_buffer_size);
	}

	data->stream_data_offset = data->file->get_position() - data->oy.fill + data->oy.returned;
	data->stream_data_size = data->file->get_length() - data->stream_data_offset;

	int64_t seek_pos = MAX(data->stream_data_offset, (int64_t)data->file->get_length() - 64 * 1024);
	int64_t video_granulepos = INT64_MAX;
	int64_t audio_granulepos = INT64_MAX;
	data->file->seek(seek_pos);
	seek_pos = data->seek_streams(-1, video_granulepos, audio_granulepos);
	data->file->seek(seek_pos);
	ogg_sync_reset(&data->oy);

	data->stream_length = 0;
	ogg_page page;
	while (data->read_page(&page) > 0) {
		data->stream_length = MAX(data->stream_length, data->get_page_time(&page));
	}

	seek(0);
}

Ref<Texture2D> VideoStreamPlayback::get_texture() const { return data->texture; }

void VideoStreamPlayback::update(double p_delta)
{
	if (data->file.is_null()) {
		return;
	}

	if (!data->playing || data->paused) {
		return;
	}

	data->time += p_delta;

	double comp_time = data->get_time();
	bool audio_ready = false;

	while ((!audio_ready && !data->audio_done) || (!data->video_ready && !data->video_done)) {
		ogg_packet op;

		while (!audio_ready && !data->audio_done) {
			if (!data->send_audio(mix_callback, mix_udata)) {
				audio_ready = true;
				break;
			}

			float** pcm;
			int ret = vorbis_synthesis_pcmout(&data->vd, &pcm);
			if (ret > 0) {
				int frames_read = 0;
				while (frames_read < ret) {
					int m = MIN(data->audio_buffer_size / data->vi.channels, ret - frames_read);
					int count = 0;
					for (int j = 0; j < m; j++) {
						for (int i = 0; i < data->vi.channels; i++) {
							data->audio_buffer[count++] = pcm[i][frames_read + j];
						}
					}
					frames_read += m;
					data->audio_ptr_end = m;
					if (!data->send_audio(mix_callback, mix_udata)) {
						audio_ready = true;
						break;
					}
				}
				vorbis_synthesis_read(&data->vd, frames_read);
			}
			else {
				if (ogg_stream_packetout(&data->vo, &op) > 0) {
					if (vorbis_synthesis(&data->vb, &op) == 0) {
						vorbis_synthesis_blockin(&data->vd, &data->vb);
					}
				}
				else {
					data->audio_done = data->vorbis_eos;
					break;
				}
			}
		}

		while (!data->video_ready && !data->video_done) {
			if (ogg_stream_packetout(&data->to, &op) > 0) {
				if (op.granulepos >= 0) {
					th_decode_ctl(
						data->td, TH_DECCTL_SET_GRANPOS, &op.granulepos, sizeof(op.granulepos));
				}
				int64_t videobuf_granulepos;
				int ret = th_decode_packetin(data->td, &op, &videobuf_granulepos);
				if (ret == 0 || ret == TH_DUPFRAME) {
					data->next_frame_time = th_granule_time(data->td, videobuf_granulepos);
					if (data->next_frame_time > comp_time) {
						data->dup_frame = (ret == TH_DUPFRAME);
						data->video_ready = true;
					}
					else {
						data->pp_inc = data->pp_level > 0 ? -1 : 0;
					}
				}
			}
			else {
				data->video_done = data->theora_eos;
				break;
			}
		}

		if (!data->video_ready || !audio_ready) {
			int ret = data->feed_pages();
			if (ret == 0) {
				data->vorbis_eos = true;
				data->theora_eos = true;
				break;
			}
		}

		double tdiff = data->next_frame_time - comp_time;
		if (tdiff > data->ti.fps_denominator * 0.25 / data->ti.fps_numerator) {
			data->pp_inc = data->pp_level < data->pp_level_max ? 1 : 0;
		}
		else if (tdiff < data->ti.fps_denominator * 0.05 / data->ti.fps_numerator) {
			data->pp_inc = data->pp_level > 0 ? -1 : 0;
		}
	}

	if (!data->video_ready && data->video_done && data->audio_done) {
		stop();
		return;
	}

	if (data->video_ready && comp_time >= data->current_frame_time) {
		if (!data->dup_frame) {
			th_ycbcr_buffer yuv;
			th_decode_ycbcr_out(data->td, yuv);
			data->video_write(yuv);
		}
		data->dup_frame = false;
		data->video_ready = false;
		data->current_frame_time = data->next_frame_time;
	}
}

void VideoStreamPlayback::stop()
{
	data->playing = false;
	seek(0);
}

void VideoStreamPlayback::play()
{
	if (!data->has_video) {
		return;
	}
	if (!data->playing) {
		data->time = 0;
	}
	data->playing = true;
}

bool VideoStreamPlayback::is_playing() const { return data->playing; }

void VideoStreamPlayback::set_paused(bool p_paused) { data->paused = p_paused; }

bool VideoStreamPlayback::is_paused() const { return data->paused; }

double VideoStreamPlayback::get_length() const { return data->stream_length; }

double VideoStreamPlayback::get_playback_position() const { return data->get_time(); }

void VideoStreamPlayback::seek(double p_time)
{
	if (data->file.is_null()) {
		return;
	}
	if (p_time >= data->stream_length) {
		return;
	}

	data->video_ready = false;
	data->next_frame_time = 0;
	data->current_frame_time = -1;
	data->dup_frame = false;
	data->video_done = false;
	data->audio_done = !data->has_audio;
	data->theora_eos = false;
	data->vorbis_eos = false;
	data->audio_ptr_start = 0;
	data->audio_ptr_end = 0;

	ogg_stream_reset(&data->to);
	if (data->has_audio) {
		ogg_stream_reset(&data->vo);
		vorbis_synthesis_restart(&data->vd);
	}

	int64_t seek_pos;
	int64_t video_granulepos;
	int64_t audio_granulepos;
	seek_pos = data->seek_streams(p_time, video_granulepos, audio_granulepos);
	if (seek_pos < 0) {
		return;
	}
	data->file->seek(seek_pos);
	ogg_sync_reset(&data->oy);

	data->time = p_time;

	double last_audio_time = 0;
	double last_video_time = 0;
	bool first_frame_decoded = false;
	bool start_audio = (audio_granulepos == 0);
	bool start_video = (video_granulepos == (1LL << data->ti.keyframe_granule_shift));
	bool keyframe_found = false;
	uint64_t current_frame = 0;

	while ((data->has_audio && last_audio_time < p_time) || (last_video_time <= p_time)) {
		ogg_packet op;
		if (data->feed_pages() == 0) {
			break;
		}
		while (data->has_audio && last_audio_time < p_time && ogg_stream_packetout(&data->vo, &op) > 0) {
			if (start_audio) {
				if (vorbis_synthesis(&data->vb, &op) == 0) {
					vorbis_synthesis_blockin(&data->vd, &data->vb);
					float** pcm;
					int samples_left = ceil((p_time - last_audio_time) * data->vi.rate);
					int samples_read = vorbis_synthesis_pcmout(&data->vd, &pcm);
					int samples_consumed = MIN(samples_left, samples_read);
					vorbis_synthesis_read(&data->vd, samples_consumed);
					last_audio_time += (double)samples_consumed / data->vi.rate;
				}
			}
			else if (op.granulepos >= audio_granulepos) {
				last_audio_time = vorbis_granule_time(&data->vd, op.granulepos);
				if (vorbis_synthesis_trackonly(&data->vb, &op) == 0) {
					vorbis_synthesis_blockin(&data->vd, &data->vb);
				}
				start_audio = true;
			}
		}
		while (last_video_time <= p_time && ogg_stream_packetout(&data->to, &op) > 0) {
			if (!start_video &&
				(op.granulepos >= video_granulepos ||
					video_granulepos == (1LL << data->ti.keyframe_granule_shift))) {
				if (op.granulepos > 0) {
					current_frame = th_granule_frame(data->td, op.granulepos);
				}
				start_video = true;
			}
			if (start_video) {
				if (!keyframe_found && th_packet_iskeyframe(&op)) {
					keyframe_found = true;
					int64_t cur_granulepos = (current_frame + 1) << data->ti.keyframe_granule_shift;
					th_decode_ctl(
						data->td, TH_DECCTL_SET_GRANPOS, &cur_granulepos, sizeof(cur_granulepos));
				}
				if (keyframe_found) {
					int64_t videobuf_granulepos;
					if (op.granulepos >= 0) {
						th_decode_ctl(
							data->td, TH_DECCTL_SET_GRANPOS, &op.granulepos, sizeof(op.granulepos));
					}
					int ret = th_decode_packetin(data->td, &op, &videobuf_granulepos);
					if (ret == 0 || ret == TH_DUPFRAME) {
						last_video_time = th_granule_time(data->td, videobuf_granulepos);
						first_frame_decoded = true;
					}
				}
				else {
					current_frame++;
				}
			}
		}
	}

	if (first_frame_decoded) {
		if (is_playing()) {
			th_ycbcr_buffer yuv;
			th_decode_ycbcr_out(data->td, yuv);
			data->video_write(yuv);
			data->current_frame_time = last_video_time;
		}
		else {
			data->next_frame_time = data->current_frame_time;
			data->video_ready = true;
		}
	}
}

int VideoStreamPlayback::get_channels() const
{
	_channel_count = data->vi.channels;
	return data->vi.channels;
}

void VideoStreamPlayback::set_audio_track(int p_idx) { data->audio_track = p_idx; }

int VideoStreamPlayback::get_mix_rate() const { return data->vi.rate; }

VideoStreamPlayback::VideoStreamPlayback()
{
	data = memnew(TheoraData);
	data->texture.instantiate();
}

VideoStreamPlayback::~VideoStreamPlayback()
{
	data->clear();
	memdelete(data);
	data = nullptr;
}

Ref<VideoStreamPlayback> VideoStream::instantiate_playback()
{
	Ref<VideoStreamPlayback> pb = memnew(VideoStreamPlayback);
	pb->set_audio_track(audio_track);
	pb->set_file(file);
	return pb;
}

Ref<Resource> ResourceFormatLoaderTheora::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		if (r_error) {
			*r_error = ERR_CANT_OPEN;
		}
		return Ref<Resource>();
	}

	Ref<VideoStream> stream;
	stream.instantiate();
	stream->set_file(p_path);

	if (r_error) {
		*r_error = OK;
	}

	return stream;
}

void ResourceFormatLoaderTheora::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("ogv");
}

bool ResourceFormatLoaderTheora::handles_type(const String& p_type) const
{
	return p_type == "VideoStream";
}

String ResourceFormatLoaderTheora::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("ogv")) {
		return "VideoStream";
	}
	return "";
}

#ifdef TOOLS_ENABLED
#include <cstdlib>

// For reference, see:
// - https://stackoverflow.com/a/9467305
// - https://en.wikipedia.org/wiki/YCbCr#Approximate_8-bit_matrices_for_BT.601

static void _rgb2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgb, size_t width, size_t height, size_t pixel_size) {
	size_t uvpos = 0;
	size_t i = 0;
	for (size_t line = 0; line < height; line += 2) {
		for (size_t x = 0; x < width; x += 2) {
			uint8_t r = rgb[pixel_size * i];
			uint8_t g = rgb[pixel_size * i + 1];
			uint8_t b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

			u[uvpos] = ((-38 * r + -74 * g + 112 * b) >> 8) + 128;
			v[uvpos] = ((112 * r + -94 * g + -18 * b) >> 8) + 128;
			uvpos++;

			r = rgb[pixel_size * i];
			g = rgb[pixel_size * i + 1];
			b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
		}
		for (size_t x = 0; x < width; x += 1) {
			uint8_t r = rgb[pixel_size * i];
			uint8_t g = rgb[pixel_size * i + 1];
			uint8_t b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
		}
	}
}

static void rgb2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgb, size_t width, size_t height) {
	_rgb2yuv420(y, u, v, rgb, width, height, 3);
}

static void rgba2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgba, size_t width, size_t height) {
	_rgb2yuv420(y, u, v, rgba, width, height, 4);
}

#include <cstdlib>

void MovieWriterOGV::push_audio(const int32_t* p_audio_data)
{
	// Read and process more audio.
	float** vorbis_buffer = vorbis_analysis_buffer(&vd, audio_frames);

	// Deinterleave samples.
	uint32_t count = 0;
	for (uint32_t i = 0; i < audio_frames; i++) {
		for (uint32_t j = 0; j < audio_ch; j++) {
			vorbis_buffer[j][i] = p_audio_data[count] / 2147483647.f;
			count++;
		}
	}

	vorbis_analysis_wrote(&vd, audio_frames);
}

void MovieWriterOGV::pull_audio(bool p_last)
{
	ogg_packet op;

	while (vorbis_analysis_blockout(&vd, &vb) > 0) {
		// Analysis, assume we want to use bitrate management.
		vorbis_analysis(&vb, nullptr);
		vorbis_bitrate_addblock(&vb);

		// Weld packets into the bitstream.
		while (vorbis_bitrate_flushpacket(&vd, &op) > 0) {
			ogg_stream_packetin(&vo, &op);
		}
	}

	if (p_last) {
		vorbis_analysis_wrote(&vd, 0);
		pull_audio();
	}
}

void MovieWriterOGV::push_video(const Ref<Image>& p_image)
{
	PackedByteArray data = p_image->get_data();
	if (p_image->get_format() == Image::FORMAT_RGBA8) {
		rgba2yuv420(y, u, v, data.ptrw(), p_image->get_width(), p_image->get_height());
	}
	else {
		rgb2yuv420(y, u, v, data.ptrw(), p_image->get_width(), p_image->get_height());
	}
	th_encode_ycbcr_in(td, ycbcr);
}

void MovieWriterOGV::pull_video(bool p_last)
{
	ogg_packet op;

	int ret = 0;
	do {
		ret = th_encode_packetout(td, p_last, &op);
		if (ret > 0) {
			ogg_stream_packetin(&to, &op);
		}
	} while (ret > 0);
}

uint32_t MovieWriterOGV::get_audio_mix_rate() const { return mix_rate; }

AudioServer::SpeakerMode MovieWriterOGV::get_audio_speaker_mode() const { return speaker_mode; }

bool MovieWriterOGV::handles_file(const String& p_path) const
{
	return p_path.has_extension("ogv");
}

void MovieWriterOGV::get_supported_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("ogv");
}

Error MovieWriterOGV::write_begin(
	const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path)
{
	ERR_FAIL_COND_V_MSG((p_movie_size.width & 1) || (p_movie_size.height & 1), ERR_UNAVAILABLE,
		"Both video dimensions must be even.");
	base_path = p_base_path.get_basename();
	if (base_path.is_relative_path()) {
		base_path = "res://" + base_path;
	}
	base_path += ".ogv";

	f = FileAccess::open(base_path, FileAccess::WRITE_READ);
	ERR_FAIL_COND_V(f.is_null(), ERR_CANT_OPEN);

	fps = p_fps;

	audio_ch = 2;
	switch (speaker_mode) {
	case AudioServer::SPEAKER_MODE_STEREO:
		audio_ch = 2;
		break;
	case AudioServer::SPEAKER_SURROUND_31:
		audio_ch = 4;
		break;
	case AudioServer::SPEAKER_SURROUND_51:
		audio_ch = 6;
		break;
	case AudioServer::SPEAKER_SURROUND_71:
		audio_ch = 8;
		break;
	}
	audio_frames = mix_rate / fps;

	// Set up Ogg output streams.
	srand(time(nullptr));
	ogg_stream_init(&to, rand()); // Video.
	ogg_stream_init(&vo, rand()); // Audio.

	// Initialize Vorbis audio encoding.
	vorbis_info_init(&vi);
	int ret = vorbis_encode_init_vbr(&vi, audio_ch, mix_rate, audio_quality);
	ERR_FAIL_COND_V_MSG(ret, ERR_UNAVAILABLE,
		"The Ogg Vorbis encoder couldn't set up a mode according to the requested quality or "
		"bitrate.");

	vorbis_comment_init(&vc);
	vorbis_analysis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);

	// Set up Theora encoder.
	// Theora has a divisible-by-16 restriction for the encoded frame size
	// scale the picture size up to the nearest /16 and calculate offsets.
	int pic_w = p_movie_size.width;
	int pic_h = p_movie_size.height;
	int frame_w = (pic_w + 15) & ~0xF;
	int frame_h = (pic_h + 15) & ~0xF;
	// Force the offsets to be even so that chroma samples line up like we expect.
	int pic_x = (frame_w - pic_w) / 2 & ~1;
	int pic_y = (frame_h - pic_h) / 2 & ~1;

	y = (uint8_t*)memalloc(pic_w * pic_h);
	u = (uint8_t*)memalloc(pic_w * pic_h / 4);
	v = (uint8_t*)memalloc(pic_w * pic_h / 4);

	// We submit the buffer using the size of the picture region.
	// libtheora will pad the picture region out to the full frame size for us,
	// whether we pass in a full frame or not.
	ycbcr[0].width = pic_w;
	ycbcr[0].height = pic_h;
	ycbcr[0].stride = pic_w;
	ycbcr[0].data = y;
	ycbcr[1].width = pic_w / 2;
	ycbcr[1].height = pic_h / 2;
	ycbcr[1].stride = pic_w / 2;
	ycbcr[1].data = u;
	ycbcr[2].width = pic_w / 2;
	ycbcr[2].height = pic_h / 2;
	ycbcr[2].stride = pic_w / 2;
	ycbcr[2].data = v;

	th_info_init(&ti);
	ti.frame_width = frame_w;
	ti.frame_height = frame_h;
	ti.pic_width = pic_w;
	ti.pic_height = pic_h;
	ti.pic_x = pic_x;
	ti.pic_y = pic_y;
	ti.fps_numerator = fps;
	ti.fps_denominator = 1;
	ti.aspect_numerator = 1;
	ti.aspect_denominator = 1;
	ti.colorspace = TH_CS_UNSPECIFIED;
	// Account for the Ogg page overhead.
	// This is 1 byte per 255 for lacing values, plus 26 bytes per 4096 bytes for
	// the page header, plus approximately 1/2 byte per packet (not accounted for here).
	ti.target_bitrate = (int)(64870 * (ogg_int64_t)video_bitrate >> 16);
	ti.quality = video_quality * 63;
	ti.pixel_fmt = TH_PF_420;
	td = th_encode_alloc(&ti);
	th_info_clear(&ti);
	ERR_FAIL_NULL_V_MSG(td, ERR_UNCONFIGURED,
		"Couldn't create a Theora encoder instance. Check that the video parameters are valid.");

	// Setting just the granule shift only allows power-of-two keyframe spacing.
	// Set the actual requested spacing.
	ret = th_encode_ctl(td, TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE, &keyframe_frequency,
		sizeof(keyframe_frequency));
	if (ret < 0) {
		ERR_PRINT("Couldn't set keyframe interval.");
	}

	// Speed should also be set after the current encoder mode is established,
	// since the available speed levels may change depending on the encoder mode.
	if (speed >= 0) {
		int speed_max;
		ret = th_encode_ctl(td, TH_ENCCTL_GET_SPLEVEL_MAX, &speed_max, sizeof(speed_max));
		if (ret < 0) {
			WARN_PRINT("Couldn't determine maximum speed level.");
			speed_max = 0;
		}
		ret = th_encode_ctl(td, TH_ENCCTL_SET_SPLEVEL, &speed, sizeof(speed));
		if (ret < 0) {
			if (ret < 0) {
				WARN_PRINT(vformat("Couldn't set speed level to %d of %d.", speed, speed_max));
			}
			if (speed > speed_max) {
				WARN_PRINT(vformat("Setting speed level to %d instead.", speed_max));
			}
			ret = th_encode_ctl(td, TH_ENCCTL_SET_SPLEVEL, &speed_max, sizeof(speed_max));
			if (ret < 0) {
				WARN_PRINT(vformat("Couldn't set speed level to %d of %d.", speed_max, speed_max));
			}
		}
	}

	// Write the bitstream header packets with proper page interleave.
	th_comment_init(&tc);
	// The first packet will get its own page automatically.
	ogg_packet op;
	if (th_encode_flushheader(td, &tc, &op) <= 0) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Theora library error.");
	}

	ogg_stream_packetin(&to, &op);
	if (ogg_stream_pageout(&to, &video_page) != 1) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
	}
	f->store_buffer(video_page.header, video_page.header_len);
	f->store_buffer(video_page.body, video_page.body_len);

	// Create the remaining Theora headers.
	while (true) {
		ret = th_encode_flushheader(td, &tc, &op);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Theora library error.");
		}
		else if (ret == 0) {
			break;
		}
		ogg_stream_packetin(&to, &op);
	}

	// Vorbis streams start with 3 standard header packets.
	ogg_packet id;
	ogg_packet comment;
	ogg_packet code;
	if (vorbis_analysis_headerout(&vd, &vc, &id, &comment, &code) < 0) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Vorbis library error.");
	}

	// ID header is automatically placed in its own page.
	ogg_stream_packetin(&vo, &id);
	if (ogg_stream_pageout(&vo, &audio_page) != 1) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
	}
	f->store_buffer(audio_page.header, audio_page.header_len);
	f->store_buffer(audio_page.body, audio_page.body_len);

	// Append remaining Vorbis header packets.
	ogg_stream_packetin(&vo, &comment);
	ogg_stream_packetin(&vo, &code);

	// Flush the rest of our headers. This ensures the actual data in each stream will start on a
	// new page, as per spec.
	while (true) {
		ret = ogg_stream_flush(&to, &video_page);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
		}
		else if (ret == 0) {
			break;
		}
		f->store_buffer(video_page.header, video_page.header_len);
		f->store_buffer(video_page.body, video_page.body_len);
	}

	while (true) {
		ret = ogg_stream_flush(&vo, &audio_page);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
		}
		else if (ret == 0) {
			break;
		}
		f->store_buffer(audio_page.header, audio_page.header_len);
		f->store_buffer(audio_page.body, audio_page.body_len);
	}

	return OK;
}

// The order of the operations has been chosen so we're one frame behind writing to the stream so we
// can put the eos mark in the last frame. Flushing streams to the file every X frames is done to
// improve audio/video page interleaving thus avoiding large runs of video or audio pages.
Error MovieWriterOGV::write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data)
{
	ERR_FAIL_COND_V(f.is_null() || td == nullptr, ERR_UNCONFIGURED);

	frame_count++;

	pull_audio();
	pull_video();

	if ((frame_count % 8) == 0) {
		write_to_file();
	}

	push_audio(p_audio_data);
	push_video(p_image);

	return OK;
}

void MovieWriterOGV::save_page(ogg_page page)
{
	unsigned int page_size = page.header_len + page.body_len;
	if (page_size > backup_page_size) {
		backup_page_data = (unsigned char*)memrealloc(backup_page_data, page_size);
		backup_page_size = page_size;
	}
	backup_page.header = backup_page_data;
	backup_page.header_len = page.header_len;
	backup_page.body = backup_page_data + page.header_len;
	backup_page.body_len = page.body_len;
	memcpy(backup_page.header, page.header, page.header_len);
	memcpy(backup_page.body, page.body, page.body_len);
}

void MovieWriterOGV::restore_page(ogg_page* page)
{
	page->header = backup_page.header;
	page->header_len = backup_page.header_len;
	page->body = backup_page.body;
	page->body_len = backup_page.body_len;
}

// The added complexity here is because we have to ensure pages are written in ascending timestamp
// order. libOgg doesn't allow checking the next page granulepos without requesting the page, and
// once requested it can't be returned, thus, we need to save it so that it doesn't get erased by
// the next `ogg_stream_packetin` call.
void MovieWriterOGV::write_to_file(bool p_finish)
{
	if (audio_flag) {
		restore_page(&audio_page);
	}
	else {
		audio_flag = ogg_stream_flush(&vo, &audio_page);
	}
	if (video_flag) {
		restore_page(&video_page);
	}
	else {
		video_flag = ogg_stream_flush(&to, &video_page);
	}

	bool finishing = p_finish && (audio_flag || video_flag);
	while (finishing || (audio_flag && video_flag)) {
		double audiotime = vorbis_granule_time(&vd, ogg_page_granulepos(&audio_page));
		double videotime = th_granule_time(td, ogg_page_granulepos(&video_page));
		bool video_first = audiotime >= videotime;

		if (video_flag && video_first) {
			// Flush a video page.
			f->store_buffer(video_page.header, video_page.header_len);
			f->store_buffer(video_page.body, video_page.body_len);
			video_flag = ogg_stream_flush(&to, &video_page) > 0;
		}
		else {
			// Flush an audio page.
			f->store_buffer(audio_page.header, audio_page.header_len);
			f->store_buffer(audio_page.body, audio_page.body_len);
			audio_flag = ogg_stream_flush(&vo, &audio_page) > 0;
		}
		finishing = p_finish && (audio_flag || video_flag);
	}

	if (video_flag) {
		save_page(video_page);
	}
	else if (audio_flag) {
		save_page(audio_page);
	}
}

void MovieWriterOGV::write_end()
{
	pull_audio(true);
	pull_video(true);
	write_to_file(true);

	th_encode_free(td);

	ogg_stream_clear(&vo);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);

	ogg_stream_clear(&to);
	th_comment_clear(&tc);

	memfree(y);
	memfree(u);
	memfree(v);

	if (backup_page_data != nullptr) {
		memfree(backup_page_data);
	}

	if (f.is_valid()) {
		f.unref();
	}
}



MovieWriterOGV::MovieWriterOGV() {}
#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED
#endif

static Ref<ResourceFormatLoaderTheora> resource_loader_theora;
#ifdef TOOLS_ENABLED
static MovieWriterOGV* writer_ogv = nullptr;
#endif

void initialize_theora_module(ModuleInitializationLevel p_level)
{
	switch (p_level) {
	case MODULE_INITIALIZATION_LEVEL_SERVERS: {
#ifdef TOOLS_ENABLED
		writer_ogv = memnew(MovieWriterOGV);
		MovieWriter::add_writer(writer_ogv);
#endif
	} break;

	case MODULE_INITIALIZATION_LEVEL_SCENE: {
		resource_loader_theora.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_theora, true);
	} break;
	default:
		break;
	}
}

void uninitialize_theora_module(ModuleInitializationLevel p_level)
{
	switch (p_level) {
	case MODULE_INITIALIZATION_LEVEL_SCENE: {
		ResourceLoader::remove_resource_format_loader(resource_loader_theora);
		resource_loader_theora.unref();
	} break;

	case MODULE_INITIALIZATION_LEVEL_SERVERS: {
#ifdef TOOLS_ENABLED
		memdelete(writer_ogv);
#endif
	} break;
	default:
		break;
	}
}
#endif // defined(MODULE_THEORA_ENABLED)
