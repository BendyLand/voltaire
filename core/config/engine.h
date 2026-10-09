/**************************************************************************/
/*  engine.h                                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"

class Engine
{
private:
	friend class Main;

	static inline uint64_t frames_drawn = 0;
	static inline uint32_t _frame_delay = 0;
	static inline uint64_t _frame_ticks = 0;
	static inline double _process_step = 0;

	static inline int ips = 60;
	static inline int user_ips = 60;
	static inline double physics_jitter_fix = 0.5;
	static inline double _fps = 1;
	static inline int _max_fps = 0;
	static inline int _audio_output_latency = 0;
	static inline double _time_scale = 1.0;
	static inline double _game_time_scale = 1.0;
	static inline double _user_time_scale = 1.0;
	static inline uint64_t _physics_frames = 0;
	static inline int max_physics_steps_per_frame = 8;
	static inline int max_user_physics_steps_per_frame = 8;
	static inline double _physics_interpolation_fraction = 0.0f;
	static inline bool abort_on_gpu_errors = false;
	static inline bool use_validation_layers = false;
	static inline bool generate_spirv_debug_info = false;
	static inline bool extra_gpu_memory_tracking = false;
#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
	static inline bool accurate_breadcrumbs = false;
#endif
	static inline int32_t gpu_idx = -1;

	static inline uint64_t _process_frames = 0;
	static inline bool _in_physics = false;

	static inline bool editor_hint = false;
	static inline bool project_manager_hint = false;
	static inline bool extension_reloading = false;
	static inline bool embedded_in_editor = false;
	static inline bool recovery_mode_hint = false;

	static inline bool _print_header = true;

	static inline String write_movie_path;
	static inline String shader_cache_path;

	static constexpr int SERVER_SYNC_FRAME_COUNT_WARNING = 5;
	static inline int server_syncs = 0;
	static inline bool frame_server_synced = false;

	static inline bool freeze_time_scale = false;

protected:
	static void _update_time_scale();

public:
	static void set_physics_ticks_per_second(int p_ips);
	static int get_physics_ticks_per_second();
	static int get_user_physics_ticks_per_second();

	static void set_max_physics_steps_per_frame(int p_max_physics_steps);
	static int get_max_physics_steps_per_frame();
	static int get_user_max_physics_steps_per_frame();

	static void set_physics_jitter_fix(double p_threshold);
	static double get_physics_jitter_fix();

	static void set_max_fps(int p_fps);
	static int get_max_fps();

	static void set_audio_output_latency(int p_msec);
	static int get_audio_output_latency();

	static double get_frames_per_second() { return _fps; }

	static uint64_t get_frames_drawn();

	static uint64_t get_physics_frames() { return _physics_frames; }

	static uint64_t get_process_frames() { return _process_frames; }

	static bool is_in_physics_frame() { return _in_physics; }

	static uint64_t get_frame_ticks() { return _frame_ticks; }

	static double get_process_step() { return _process_step; }

	static double get_physics_interpolation_fraction() { return _physics_interpolation_fraction; }

	static void set_time_scale(double p_scale);
	static double get_time_scale();
	static void set_user_time_scale(double p_scale);
	static double get_effective_time_scale();
	static double get_unfrozen_time_scale();

	static void set_print_to_stdout(bool p_enabled);
	static bool is_printing_to_stdout();

	static void set_print_error_messages(bool p_enabled);
	static bool is_printing_error_messages();
	static void print_header(const String& p_string);
	static void print_header_rich(const String& p_string);

	static void set_frame_delay(uint32_t p_msec);
	static uint32_t get_frame_delay();

#ifdef TOOLS_ENABLED
	_FORCE_INLINE_ static void set_editor_hint(bool p_enabled) { editor_hint = p_enabled; }

	_FORCE_INLINE_ static bool is_editor_hint() { return editor_hint; }

	_FORCE_INLINE_ static void set_project_manager_hint(bool p_enabled)
	{
		project_manager_hint = p_enabled;
	}

	_FORCE_INLINE_ static bool is_project_manager_hint() { return project_manager_hint; }

	_FORCE_INLINE_ static void set_extension_reloading_enabled(bool p_enabled)
	{
		extension_reloading = p_enabled;
	}

	_FORCE_INLINE_ static bool is_extension_reloading_enabled() { return extension_reloading; }

	_FORCE_INLINE_ static void set_recovery_mode_hint(bool p_enabled)
	{
		recovery_mode_hint = p_enabled;
	}

	_FORCE_INLINE_ static bool is_recovery_mode_hint() { return recovery_mode_hint; }
#else
	_FORCE_INLINE_ static void set_editor_hint(bool p_enabled) {}

	_FORCE_INLINE_ static bool is_editor_hint() { return false; }

	_FORCE_INLINE_ static void set_project_manager_hint(bool p_enabled) {}

	_FORCE_INLINE_ static bool is_project_manager_hint() { return false; }

	_FORCE_INLINE_ static void set_extension_reloading_enabled(bool p_enabled) {}

	_FORCE_INLINE_ static bool is_extension_reloading_enabled() { return false; }

	_FORCE_INLINE_ static void set_recovery_mode_hint(bool p_enabled) {}

	_FORCE_INLINE_ static bool is_recovery_mode_hint() { return false; }
#endif

	static String get_license_text();

	static void set_write_movie_path(const String& p_path);
	static String get_write_movie_path();

	static String get_architecture_name();

	static void set_shader_cache_path(const String& p_path);
	static String get_shader_cache_path();

	static bool is_abort_on_gpu_errors_enabled();
	static bool is_validation_layers_enabled();
	static bool is_generate_spirv_debug_info_enabled();
	static bool is_extra_gpu_memory_tracking_enabled();
#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
	static bool is_accurate_breadcrumbs_enabled();
#endif
	static int32_t get_gpu_index();

	static void increment_frames_drawn();
	static bool notify_frame_server_synced();

	static void set_freeze_time_scale(bool p_frozen);
	static void set_embedded_in_editor(bool p_enabled);
	static bool is_embedded_in_editor();

	Engine() = delete;
	~Engine() = delete;
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
};


