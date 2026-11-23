/**************************************************************************/
/*  main.h                                                                */
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

#include "core/os/thread.h"
#include "core/typedefs.h"

template <typename T>
class Vector;

class Main {
	enum CLIOptionAvailability {
		CLI_OPTION_AVAILABILITY_EDITOR,
		CLI_OPTION_AVAILABILITY_TEMPLATE_DEBUG,
		CLI_OPTION_AVAILABILITY_TEMPLATE_RELEASE,
		CLI_OPTION_AVAILABILITY_TEMPLATE_UNSAFE,
		CLI_OPTION_AVAILABILITY_HIDDEN,
	};

	static void print_header(bool p_rich);
	static void print_help_copyright(const char *p_notice);
	static void print_help_title(const char *p_title);
	static void print_help_option(const char *p_option, const char *p_description, CLIOptionAvailability p_availability = CLI_OPTION_AVAILABILITY_TEMPLATE_RELEASE);
	static String format_help_option(const char *p_option);
	static void print_help(const char *p_binary);
	static uint64_t last_ticks;
	static uint32_t hide_print_fps_attempts;
	static uint32_t frames;
	static uint32_t frame;
	static bool force_redraw_requested;
	static int iterating;

public:
	static bool is_cmdline_tool();
#ifdef TOOLS_ENABLED
	enum CLIScope {
		CLI_SCOPE_TOOL, // Editor and project manager.
		CLI_SCOPE_PROJECT,
	};
	static const Vector<String> &get_forwardable_cli_arguments(CLIScope p_scope);
#endif

	static int test_entrypoint(int argc, char *argv[], bool &tests_need_run);
	static Error setup(const char *execpath, int argc, char *argv[], bool p_second_phase = true);
	static Error setup2(bool p_show_boot_logo = true); // The thread calling setup2() will effectively become the main thread.
	static String get_locale_override();
	static void setup_boot_logo();
	static Error test_setup();
	static void test_cleanup();
	static int start();

	static bool iteration();
	static void force_redraw();

	static bool is_iterating();

	static void cleanup(bool p_force = false);
};

// Test main override is for the testing behavior.
#define TEST_MAIN_OVERRIDE \
	bool run_test = false; \
	int return_code = Main::test_entrypoint(argc, argv, run_test); \
	if (run_test) { \
		godot_cleanup_profiler(); \
		return return_code; \
	}

#define TEST_MAIN_PARAM_OVERRIDE(argc, argv) \
	bool run_test = false; \
	int return_code = Main::test_entrypoint(argc, argv, run_test); \
	if (run_test) { \
		godot_cleanup_profiler(); \
		return return_code; \
	}

// --- Folded from core/register_core_types.h ---
void register_core_types();
void register_core_settings();
void register_core_extensions();
void register_early_core_singletons();
void register_core_singletons();
void unregister_core_types();
void unregister_core_extensions();

// --- Folded from drivers/register_driver_types.h ---
void register_core_driver_types();
void unregister_core_driver_types();

void register_driver_types();
void unregister_driver_types();

// --- Folded from scene/register_scene_types.h ---
void register_scene_types();
void unregister_scene_types();
void register_scene_singletons();

// --- Folded from servers/register_server_types.h ---
void register_server_types();
void unregister_server_types();

// --- Folded from servers/movie_writer/movie_writer.h ---
#include "core/io/image.h"
#include "core/templates/local_vector.h"
#include "servers/audio/audio_server.h"

class MovieWriter
{
	uint64_t fps = 0;
	uint64_t mix_rate = 0;
	uint32_t audio_channels = 0;

	// The output resolution, which can differ from the window size.
	// Used as a base for resizing all subsequent frames if their resolution differs.
	Vector2i movie_size;

	float cpu_time = 0.0f;
	float gpu_time = 0.0f;
	uint64_t encoding_time_usec = 0;

	String project_name;

	LocalVector<int32_t> audio_mix_buffer;

	enum
	{
		MAX_WRITERS = 8
	};

	static MovieWriter* writers[];
	static uint32_t writer_count;

protected:
	virtual uint32_t get_audio_mix_rate() const;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const;

	virtual Error write_begin(
		const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path);
	virtual Error write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data);
	virtual void write_end();


public:
	virtual bool handles_file(const String& p_path) const;
	virtual void get_supported_extensions(List<String>* r_extensions) const;

	static void add_writer(MovieWriter* p_writer);
	static MovieWriter* find_writer_for_file(const String& p_file);

	void begin(const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path);
	void add_frame();

	static void set_extensions_hint();

	void end();
};

// --- Folded from servers/movie_writer/movie_writer_pngwav.h ---


class MovieWriterPNGWAV : public MovieWriter {

	enum {
		MAX_TRAILING_ZEROS = 8 // more than 10 days at 60fps, no hard drive can put up with this anyway :)
	};

	uint32_t mix_rate = 48000;
	AudioServer::SpeakerMode speaker_mode = AudioServer::SPEAKER_MODE_STEREO;
	String base_path;
	uint32_t frame_count = 0;
	uint32_t fps = 0;

	uint32_t audio_block_size = 0;

	Ref<FileAccess> f_wav;
	uint32_t wav_data_size_pos = 0;

	String zeros_str(uint32_t p_index);

protected:
	virtual uint32_t get_audio_mix_rate() const override;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const override;
	virtual void get_supported_extensions(List<String> *r_extensions) const override;

	virtual Error write_begin(const Size2i &p_movie_size, uint32_t p_fps, const String &p_base_path) override;
	virtual Error write_frame(const Ref<Image> &p_image, const int32_t *p_audio_data) override;
	virtual void write_end() override;

	virtual bool handles_file(const String &p_path) const override;

public:
	MovieWriterPNGWAV();
};

// --- End servers/movie_writer/movie_writer_pngwav.h ---
