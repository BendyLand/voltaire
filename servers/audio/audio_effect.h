/**************************************************************************/
/*  audio_effect.h                                                        */
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

#include "core/io/resource.h"
#include "core/math/audio_frame.h"
#include "core/types.h"

#include <type_traits>
#include <utility>

class AudioEffectInstance final : public RefCounted
{
public:
	using ProcessFn = void (*)(void* p_state, const AudioFrame* p_src, AudioFrame* p_dst, int p_frames);
	using ProcessSilenceFn = bool (*)(const void* p_state);
	using FreeFn = void (*)(void* p_state);

	void* state = nullptr;
	ProcessFn process_fn = nullptr;
	ProcessSilenceFn process_silence_fn = nullptr;
	FreeFn free_fn = nullptr;

private:
	const void* state_type_tag = nullptr;

	template <typename T, typename = void>
	struct HasProcessSilence : std::false_type {};

	template <typename T>
	struct HasProcessSilence<T, std::void_t<decltype(std::declval<const T&>().process_silence())>> : std::true_type {};

	template <typename T>
	static const void* _state_type_id()
	{
		static const char tag = 0;
		return &tag;
	}

	template <typename StateT>
	static void _default_process(void* p_state, const AudioFrame* p_src, AudioFrame* p_dst, int p_frames)
	{
		static_cast<StateT*>(p_state)->process(p_src, p_dst, p_frames);
	}

	template <typename StateT>
	static bool _default_process_silence(const void* p_state)
	{
		return static_cast<const StateT*>(p_state)->process_silence();
	}

	template <typename StateT>
	static void _default_free(void* p_state)
	{
		memdelete(static_cast<StateT*>(p_state));
	}

public:
	_ALWAYS_INLINE_ void process(
		const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
	{
		if (process_fn && state) {
			process_fn(state, p_src_frames, p_dst_frames, p_frame_count);
		}
	}

	_ALWAYS_INLINE_ bool process_silence() const
	{
		return (process_silence_fn && state) ? process_silence_fn(state) : false;
	}

	template <typename T>
	_ALWAYS_INLINE_ T* get_state() const
	{
		return (state_type_tag == _state_type_id<T>()) ? static_cast<T*>(state) : nullptr;
	}

	template <typename StateT>
	static Ref<AudioEffectInstance> create(
		StateT* p_state,
		ProcessFn p_process_fn = &_default_process<StateT>,
		ProcessSilenceFn p_process_silence_fn = nullptr,
		FreeFn p_free_fn = &_default_free<StateT>)
	{
		Ref<AudioEffectInstance> ins;
		ins.instantiate();
		ins->state = p_state;
		ins->state_type_tag = _state_type_id<StateT>();
		ins->process_fn = p_process_fn;
		if (p_process_silence_fn) {
			ins->process_silence_fn = p_process_silence_fn;
		}
		else if constexpr (HasProcessSilence<StateT>::value) {
			ins->process_silence_fn = &_default_process_silence<StateT>;
		}
		ins->free_fn = p_free_fn;
		return ins;
	}

	~AudioEffectInstance()
	{
		if (free_fn && state) {
			free_fn(state);
		}
	}
};

struct AudioEffectAmplifyConfig;
struct AudioEffectCaptureConfig;
struct AudioEffectChorusConfig;
struct AudioEffectCompressorConfig;
struct AudioEffectDelayConfig;
struct AudioEffectDistortionConfig;
struct AudioEffectEQConfig;
struct AudioEffectFilterConfig;
struct AudioEffectHardLimiterConfig;
struct AudioEffectLimiterConfig;
struct AudioEffectPannerConfig;
struct AudioEffectPhaserConfig;
struct AudioEffectPitchShiftConfig;
struct AudioEffectRecordConfig;
struct AudioEffectReverbConfig;
struct AudioEffectSpectrumAnalyzerConfig;
struct AudioEffectStereoEnhanceConfig;

class AudioEffect final : public Resource
{
public:
	enum Type
	{
		EFFECT_AMPLIFY,
		EFFECT_CAPTURE,
		EFFECT_CHORUS,
		EFFECT_COMPRESSOR,
		EFFECT_DELAY,
		EFFECT_DISTORTION,
		EFFECT_EQ,
		EFFECT_FILTER,
		EFFECT_HARD_LIMITER,
		EFFECT_LIMITER,
		EFFECT_PANNER,
		EFFECT_PHASER,
		EFFECT_PITCH_SHIFT,
		EFFECT_RECORD,
		EFFECT_REVERB,
		EFFECT_SPECTRUM_ANALYZER,
		EFFECT_STEREO_ENHANCE,
	};

private:
	Type effect_type = EFFECT_AMPLIFY;
	void* config = nullptr;

	void _init_config(Type p_type);
	void _free_config();

public:
	Type get_effect_type() const { return effect_type; }
	void set_effect_type(Type p_type);

	void* get_config_ptr() const { return config; }

	AudioEffectAmplifyConfig* get_amplify_config() const
	{
		return effect_type == EFFECT_AMPLIFY ? static_cast<AudioEffectAmplifyConfig*>(config) : nullptr;
	}
	AudioEffectCaptureConfig* get_capture_config() const
	{
		return effect_type == EFFECT_CAPTURE ? static_cast<AudioEffectCaptureConfig*>(config) : nullptr;
	}
	AudioEffectChorusConfig* get_chorus_config() const
	{
		return effect_type == EFFECT_CHORUS ? static_cast<AudioEffectChorusConfig*>(config) : nullptr;
	}
	AudioEffectCompressorConfig* get_compressor_config() const
	{
		return effect_type == EFFECT_COMPRESSOR ? static_cast<AudioEffectCompressorConfig*>(config) : nullptr;
	}
	AudioEffectDelayConfig* get_delay_config() const
	{
		return effect_type == EFFECT_DELAY ? static_cast<AudioEffectDelayConfig*>(config) : nullptr;
	}
	AudioEffectDistortionConfig* get_distortion_config() const
	{
		return effect_type == EFFECT_DISTORTION ? static_cast<AudioEffectDistortionConfig*>(config) : nullptr;
	}
	AudioEffectEQConfig* get_eq_config() const
	{
		return effect_type == EFFECT_EQ ? static_cast<AudioEffectEQConfig*>(config) : nullptr;
	}
	AudioEffectFilterConfig* get_filter_config() const
	{
		return effect_type == EFFECT_FILTER ? static_cast<AudioEffectFilterConfig*>(config) : nullptr;
	}
	AudioEffectHardLimiterConfig* get_hard_limiter_config() const
	{
		return effect_type == EFFECT_HARD_LIMITER ? static_cast<AudioEffectHardLimiterConfig*>(config) : nullptr;
	}
	AudioEffectLimiterConfig* get_limiter_config() const
	{
		return effect_type == EFFECT_LIMITER ? static_cast<AudioEffectLimiterConfig*>(config) : nullptr;
	}
	AudioEffectPannerConfig* get_panner_config() const
	{
		return effect_type == EFFECT_PANNER ? static_cast<AudioEffectPannerConfig*>(config) : nullptr;
	}
	AudioEffectPhaserConfig* get_phaser_config() const
	{
		return effect_type == EFFECT_PHASER ? static_cast<AudioEffectPhaserConfig*>(config) : nullptr;
	}
	AudioEffectPitchShiftConfig* get_pitch_shift_config() const
	{
		return effect_type == EFFECT_PITCH_SHIFT ? static_cast<AudioEffectPitchShiftConfig*>(config) : nullptr;
	}
	AudioEffectRecordConfig* get_record_config() const
	{
		return effect_type == EFFECT_RECORD ? static_cast<AudioEffectRecordConfig*>(config) : nullptr;
	}
	AudioEffectReverbConfig* get_reverb_config() const
	{
		return effect_type == EFFECT_REVERB ? static_cast<AudioEffectReverbConfig*>(config) : nullptr;
	}
	AudioEffectSpectrumAnalyzerConfig* get_spectrum_analyzer_config() const
	{
		return effect_type == EFFECT_SPECTRUM_ANALYZER ? static_cast<AudioEffectSpectrumAnalyzerConfig*>(config) : nullptr;
	}
	AudioEffectStereoEnhanceConfig* get_stereo_enhance_config() const
	{
		return effect_type == EFFECT_STEREO_ENHANCE ? static_cast<AudioEffectStereoEnhanceConfig*>(config) : nullptr;
	}

	Ref<AudioEffectInstance> instantiate();

	AudioEffect(Type p_type = EFFECT_AMPLIFY);
	AudioEffect(const AudioEffect&) = delete;
	AudioEffect& operator=(const AudioEffect&) = delete;
	~AudioEffect();
};
