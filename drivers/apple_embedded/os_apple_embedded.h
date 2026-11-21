/**************************************************************************/
/*  os_apple_embedded.h                                                   */
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

#ifdef APPLE_EMBEDDED_ENABLED

#include "core/os/os.h"
#import "drivers/apple_embedded/apple_embedded.h"
#import "drivers/coreaudio/audio_driver_coreaudio.h"
#include "drivers/unix/os_unix.h"
#include "servers/audio/audio_server.h"
#include "servers/rendering/renderer_compositor.h"

#if defined(RD_ENABLED)
#include "servers/rendering/rendering_device.h"

#if defined(VULKAN_ENABLED)
#import "drivers/apple_embedded/rendering_context_driver_vulkan_apple_embedded.h"
#endif
#endif

class JoypadSDL;

class OS_AppleEmbedded final
{
public:
	OS_AppleEmbedded() = delete;
	~OS_AppleEmbedded() = delete;
	OS_AppleEmbedded(const OS_AppleEmbedded&) = delete;
	OS_AppleEmbedded& operator=(const OS_AppleEmbedded&) = delete;

	struct Data
	{
		AudioDriverCoreAudio audio_driver;
		AppleEmbedded* apple_embedded = nullptr;
#ifdef SDL_ENABLED
		JoypadSDL* joypad_sdl = nullptr;
#endif
		bool is_focused = false;
		String remote_fs_dir;
	};

	static inline Data* data = nullptr;

private:
	static HashMap<String, void*> dynamic_symbol_lookup_table;
	friend void register_dynamic_symbol(char* name, void* address);

	static CGFloat _weight_to_ct(int p_weight);
	static CGFloat _stretch_to_ct(int p_stretch);
	static String _get_default_fontname(const String& p_font_name);

	static _FORCE_INLINE_ String get_framework_executable(const String& p_path);

	static void deinitialize_modules();

public:
	static void setup();
	static void initialize_core();
	static void initialize();
	static void initialize_joypads();
	static void delete_main_loop();
	static void finalize();
	static void finalize_core();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static void initialize_modules();

	static bool iterate();

	static void start();

	static void alert(const String& p_alert, const String& p_title = "ALERT!");

	static Vector<String> get_system_fonts();
	static Vector<String> get_system_font_path_for_text(const String& p_font_name,
		const String& p_text, const String& p_locale = String(), const String& p_script = String(),
		int p_weight = 400, int p_stretch = 100, bool p_italic = false);
	static String get_system_font_path(
		const String& p_font_name, int p_weight = 400, int p_stretch = 100, bool p_italic = false);

	static Error open_dynamic_library(
		const String& p_path, void*& p_library_handle, GDExtensionData* p_data = nullptr);
	static Error close_dynamic_library(void* p_library_handle);
	static Error get_dynamic_library_symbol_handle(void* p_library_handle, const String& p_name,
		void*& p_symbol_handle, bool p_optional = false);

	static String get_distribution_name();
	static String get_version();
	static String get_model_name();

	static Error shell_open(const String& p_uri);

	static String get_user_data_dir(const String& p_user_dir);

	static String get_cache_path();
	static String get_temp_path();
	static String get_resource_dir();
	static String get_bundle_resource_dir();

	static String get_locale();
	static Vector<String> get_preferred_locales();

	static String get_unique_id();
	static String get_processor_name();

	static void vibrate_handheld(int p_duration_ms = 500, float p_amplitude = -1.0);

	static bool _check_internal_feature_support(const String& p_feature);

	static Error setup_remote_filesystem(const String& p_server_host, int p_port,
		const String& p_password, String& r_project_path);

	static void on_focus_out();
	static void on_focus_in();

	static void on_enter_background();
	static void on_exit_background();

	static Rect2 calculate_boot_screen_rect(
		const Size2& p_window_size, const Size2& p_imgrect_size);

	static bool request_permission(const String& p_name);
	static Vector<String> get_granted_permissions();

	static String get_platform_string(OS::PlatformString p_platform_string)
	{
		switch (p_platform_string) {
		case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_OPEN:
			return ETR("Open in Files");
		case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_SHOW:
			return ETR("Show in Files");
		default:
			return String();
		}
	}
};

#endif // APPLE_EMBEDDED_ENABLED
