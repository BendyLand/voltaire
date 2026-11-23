/**************************************************************************/
/*  os_macos.h                                                            */
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

// --- Inlined/Folded from platform/macos/crash_handler_macos.h ---
class CrashHandler {
	bool disabled;

public:
	void initialize();

	void disable();
	bool is_disabled() const { return disabled; }

	CrashHandler();
	~CrashHandler();
};

#include "core/input/input_event.h"
#include "core/templates/rb_map.h"
#import "drivers/coreaudio/audio_driver_coreaudio.h"
// --- Inlined/Folded from drivers/coremidi/midi_driver_coremidi.h ---
#ifdef COREMIDI_ENABLED

#include "core/os/os.h"
#include "core/os/mutex.h"
#include "core/templates/vector.h"

#import <CoreMIDI/CoreMIDI.h>

#include <cstdio>

class MIDIDriverCoreMidi final
{
	struct InputConnection
	{
		InputConnection() = default;
		InputConnection(int p_device_index, MIDIEndpointRef p_source);
		MIDIDriver::Parser parser;
		MIDIEndpointRef source;
	};

public:
	struct Data
	{
		MIDIClientRef client = 0;
		MIDIPortRef port_in = 0;
		Vector<InputConnection*> connected_sources;
		Mutex mutex;
		bool core_midi_closed = false;
	};

	static inline Data* data = nullptr;

private:
	static void read(
		const MIDIPacketList* packet_list, void* read_proc_ref_con, void* src_conn_ref_con);

public:
	static void initialize();
	static void finalize();
	static Error open();
	static void close();

	MIDIDriverCoreMidi() = delete;
	MIDIDriverCoreMidi(const MIDIDriverCoreMidi&) = delete;
	MIDIDriverCoreMidi& operator=(const MIDIDriverCoreMidi&) = delete;
	~MIDIDriverCoreMidi() = delete;
};

#endif // COREMIDI_ENABLED
#include "drivers/unix/os_unix.h"
#include "servers/audio/audio_server.h"

class JoypadSDL;

class OS_MacOS : public OS_Unix {
#ifdef COREAUDIO_ENABLED
	AudioDriverCoreAudio audio_driver;
#endif

	CrashHandler crash_handler;

	List<String> launch_service_args;

	CGFloat _weight_to_ct(int p_weight) const;
	CGFloat _stretch_to_ct(int p_stretch) const;
	String _get_default_fontname(const String &p_font_name) const;

	static _FORCE_INLINE_ String get_framework_executable(const String &p_path);

protected:
	const char *execpath = nullptr;
	int argc = 0;
	char **argv = nullptr;

#ifdef SDL_ENABLED
	JoypadSDL *joypad_sdl = nullptr;
#endif
	CFRunLoopTimerRef wait_timer = nil;

	virtual void initialize_core() override;
	virtual void initialize() override;
	virtual void finalize() override;

	virtual void initialize_joypads() override;

	void delete_main_loop();

public:
	static inline const char *headless_args[] = {
		"--headless",
		"-h",
		"--help",
		"/?",
		"--version",
		"--dump-gdextension-interface",
		"--dump-extension-api",
		"--dump-gdextension-interface-json",
		"--dump-extension-api-with-docs",
		"--validate-extension-api",
		"--convert-3to4",
		"--validate-conversion-3to4",
		"--doctool",
		"--test",
	};

	virtual void add_frame_delay(bool p_can_draw, bool p_wake_for_events) override;

	virtual void set_cmdline_platform_args(const List<String> &p_args);
	virtual List<String> get_cmdline_platform_args() const override;

	virtual void load_shell_environment() const override;

	virtual String get_name() const override;
	virtual String get_distribution_name() const override;
	virtual String get_version() const override;
	virtual String get_version_alias() const override;

	virtual void alert(const String &p_alert, const String &p_title = "ALERT!") override;

	virtual Error open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data = nullptr) override;

	virtual String get_config_path() const override;
	virtual String get_data_path() const override;
	virtual String get_cache_path() const override;
	virtual String get_temp_path() const override;
	virtual String get_bundle_resource_dir() const override;
	virtual String get_bundle_icon_path() const override;
	virtual String get_bundle_icon_name() const override;
	virtual String get_godot_dir_name() const override;

	virtual String get_system_dir(SystemDir p_dir, bool p_shared_storage = true) const override;

	virtual Error shell_open(const String &p_uri) override;
	virtual Error shell_show_in_file_manager(String p_path, bool p_open_folder) override;

	virtual String get_locale() const override;
	virtual Vector<String> get_preferred_locales() const override;

	virtual Vector<String> get_system_fonts() const override;
	virtual String get_system_font_path(const String &p_font_name, int p_weight = 400, int p_stretch = 100, bool p_italic = false) const override;
	virtual Vector<String> get_system_font_path_for_text(const String &p_font_name, const String &p_text, const String &p_locale = String(), const String &p_script = String(), int p_weight = 400, int p_stretch = 100, bool p_italic = false) const override;
	virtual String get_executable_path() const override;
	virtual Error create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id = nullptr, bool p_open_console = false) override;
	virtual Error create_instance(const List<String> &p_arguments, ProcessID *r_child_id = nullptr) override;
	virtual Error open_with_program(const String &p_program_path, const List<String> &p_paths) override;
	virtual bool is_process_running(const ProcessID &p_pid) const override;

	virtual String get_unique_id() const override;
	virtual String get_processor_name() const override;

	virtual String get_model_name() const override;

	virtual bool is_sandboxed() const override;
	virtual bool request_permission(const String &p_name) override;
	virtual Vector<String> get_granted_permissions() const override;
	virtual void revoke_granted_permissions() override;

#ifdef TOOLS_ENABLED
	static bool is_debugger_attached();
	void wait_for_debugger(uint32_t p_msec);
#endif

	virtual bool _check_internal_feature_support(const String &p_feature) override;

	virtual void disable_crash_handler() override;
	virtual bool is_disable_crash_handler() const override;

	virtual Error move_to_trash(const String &p_path) override;

	virtual String get_system_ca_certificates() override;
	virtual OS::PreferredTextureFormat get_preferred_texture_format() const override;

	virtual void run() = 0;

	virtual String get_platform_string(PlatformString p_platform_string) const override {
		switch (p_platform_string) {
			case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_OPEN:
				return ETR("Open in Finder");
			case OS::PlatformString::PLATFORM_STRING_FILE_MANAGER_SHOW:
				return ETR("Show in Finder");
			default:
				return OS::get_platform_string(p_platform_string);
		}
	}

	OS_MacOS(const char *p_execpath, int p_argc, char **p_argv);
};

class OS_MacOS_NSApp : public OS_MacOS {
	id delegate = nullptr;
	bool should_terminate = false;
	bool main_started = false;

	CFRunLoopObserverRef pre_wait_observer = nil;

	void terminate();

public:
	void start_main(); // Initializes and runs Godot main loop.
	void cleanup();
	bool os_should_terminate() const { return should_terminate; }
	int get_cmd_argc() const { return argc; }

	virtual void run() override;

	OS_MacOS_NSApp(const char *p_execpath, int p_argc, char **p_argv);
};

class OS_MacOS_Headless : public OS_MacOS {
public:
	virtual void run() override;

	OS_MacOS_Headless(const char *p_execpath, int p_argc, char **p_argv);
};

#ifdef TOOLS_ENABLED

class OS_MacOS_Embedded : public OS_MacOS {
public:
	virtual void run() override;

	OS_MacOS_Embedded(const char *p_execpath, int p_argc, char **p_argv);
};

#endif

// --- Folded from platform/macos/dir_access_macos.h ---
#if defined(UNIX_ENABLED)

#include "drivers/unix/dir_access_unix.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class DirAccessMacOS : public DirAccessUnix {

protected:
	virtual String fix_unicode_name(const char *p_name) const override;

	virtual int get_drive_count() override;
	virtual String get_drive(int p_drive) override;

	virtual bool is_hidden(const String &p_name) override;
	virtual bool is_case_sensitive(const String &p_path) const override;

	virtual String get_filesystem_type() const override;

	virtual bool is_bundle(const String &p_file) const override;
};

#endif // UNIX ENABLED

// --- Folded from platform/macos/godot_application.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <IOKit/hidsystem/ev_keymap.h>

@class GodotApplicationDelegate;

@interface GodotApplication : NSApplication

extern "C" GodotApplication *GodotApp;

@property(readonly, nonatomic) GodotApplicationDelegate *godotDelegate;

- (GodotApplication *)init;

- (void)activateApplication;
@end

// --- Folded from platform/macos/godot_application_delegate.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

class OS_MacOS_NSApp;

@interface GodotApplicationDelegate : NSObject <NSUserInterfaceItemSearching, NSApplicationDelegate, NSMenuItemValidation>

- (GodotApplicationDelegate *)initWithOS:(OS_MacOS_NSApp *)os;

- (bool)getHighContrast;
- (bool)getReduceMotion;
- (bool)getReduceTransparency;
- (bool)getVoiceOver;
@end
