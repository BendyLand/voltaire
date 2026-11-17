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

#include "audio_driver_opensl.h"

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
