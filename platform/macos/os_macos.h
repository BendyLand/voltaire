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

#include "drivers/unix/os_unix.h"

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

// --- Folded from platform/macos/display_server_macos.h ---
#if defined(MACOS_ENABLED)
// --- Inlined/Folded from platform/macos/display_server_macos_base.h ---
#include "core/os/thread_safe.h"
#include "core/typedefs.h" // IWYU pragma: keep. Prevent macOS `MAX`/`MIN` from being defined.
#include "servers/display/display_server.h"

#define FontVariation __FontVariation

#import <AppKit/AppKit.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

#undef FontVariation

class RenderingContextDriver;
class RenderingDevice;

class DisplayServerMacOSBase : public DisplayServerDriver {

	id tts = nullptr;
	IOPMAssertionID screen_keep_on_assertion = kIOPMNullAssertionID;

	struct LayoutInfo {
		String name;
		String code;
	};
	mutable Vector<LayoutInfo> kbd_layouts;
	mutable int current_layout = 0;
	mutable bool keyboard_layout_dirty = true;

	Callable system_theme_changed;

	constexpr static CGFloat HARDWARE_REFERENCE_LUMINANCE_NITS = 100.0f;

public:
	struct HDROutput {
		static constexpr float AUTO_MAX_LUMINANCE = -1.0f;

		bool requested = false;
		float max_luminance = AUTO_MAX_LUMINANCE;

		bool is_auto_max_luminance() const { return max_luminance < 0.0f; }
	};

protected:
	_THREAD_SAFE_CLASS_

	DisplayServerEnums::MouseMode mouse_mode = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	DisplayServerEnums::MouseMode mouse_mode_base = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	DisplayServerEnums::MouseMode mouse_mode_override = DisplayServerEnums::MOUSE_MODE_VISIBLE;
	bool mouse_mode_override_enabled = false;

	void _mouse_update_mode();
	virtual void _mouse_apply_mode(DisplayServerEnums::MouseMode p_prev_mode, DisplayServerEnums::MouseMode p_new_mode) = 0;

	String im_text;
	Point2i im_selection;

	DisplayServerEnums::CursorShape cursor_shape = DisplayServerEnums::CURSOR_ARROW;

	void initialize_tts() const;

	static CGDirectDisplayID _get_display_id_for_screen(NSScreen *p_screen);

	void _update_keyboard_layouts() const;
	static void _keyboard_layout_changed(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef user_info);

#if defined(RD_ENABLED)
	RenderingContextDriver *rendering_context = nullptr;
	RenderingDevice *rendering_device = nullptr;
#endif

	virtual HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) = 0;
	virtual const HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) const = 0;

	constexpr float _calculate_current_reference_luminance(CGFloat p_max_potential_edr_value, CGFloat p_max_edr_value) const;
	void _update_hdr_output(DisplayServerEnums::WindowID p_window, const HDROutput &p_hdr);

public:
	virtual bool has_window(DisplayServerEnums::WindowID p_window) const { return p_window == DisplayServerEnums::MAIN_WINDOW_ID; }

	virtual bool tts_is_speaking() const override;
	virtual bool tts_is_paused() const override;
	virtual TypedArray<Dictionary> tts_get_voices() const override;

	virtual void tts_speak(const String &p_text, const String &p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false) override;
	virtual void tts_pause() override;
	virtual void tts_resume() override;
	virtual void tts_stop() override;

	void emit_system_theme_changed();
	virtual bool is_dark_mode_supported() const override;
	virtual bool is_dark_mode() const override;
	virtual Color get_accent_color() const override;
	virtual Color get_base_color() const override;
	virtual void set_system_theme_change_callback(const Callable &p_callable) override;

	virtual void send_window_event_by_id(DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual void mouse_set_mode(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode() const override;
	virtual void mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) override;
	virtual DisplayServerEnums::MouseMode mouse_get_mode_override() const override;
	virtual void mouse_set_mode_override_enabled(bool p_override_enabled) override;
	virtual bool mouse_is_mode_override_enabled() const override;

	virtual void clipboard_set(const String &p_text) override;
	virtual String clipboard_get() const override;
	virtual Ref<Image> clipboard_get_image() const override;
	virtual bool clipboard_has() const override;
	virtual bool clipboard_has_image() const override;

	virtual int get_primary_screen() const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual void screen_set_keep_on(bool p_enable) override;
	virtual bool screen_is_kept_on() const override;

	virtual int accessibility_should_increase_contrast() const override;
	virtual int accessibility_should_reduce_animation() const override;
	virtual int accessibility_should_reduce_transparency() const override;
	virtual int accessibility_screen_reader_active() const override;

	void update_im_text(const Point2i &p_selection, const String &p_text);
	virtual Point2i ime_get_selection() const override;
	virtual String ime_get_text() const override;

	virtual DisplayServerEnums::CursorShape cursor_get_shape() const override;

	virtual void beep() const override;

	virtual int keyboard_get_layout_count() const override;
	virtual int keyboard_get_current_layout() const override;
	virtual void keyboard_set_current_layout(int p_index) override;
	virtual String keyboard_get_layout_language(int p_index) const override;
	virtual String keyboard_get_layout_name(int p_index) const override;
	virtual Key keyboard_get_keycode_from_physical(Key p_keycode) const override;
	virtual Key keyboard_get_label_from_physical(Key p_keycode) const override;
	virtual void show_emoji_and_symbol_picker() const override;

	virtual void window_get_edr_values(DisplayServerEnums::WindowID p_window, CGFloat *r_max_potential_edr_value = nullptr, CGFloat *r_max_edr_value = nullptr) const = 0;
	virtual bool window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_request_hdr_output(const bool p_enable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual bool window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual float window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual float window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual float window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual float window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual float window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	DisplayServerMacOSBase();
	~DisplayServerMacOSBase();
};

// --- Folded from platform/macos/macos_quartz_core_spi.h ---
// Copyright 2014 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the OpenEmu Team nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import <Foundation/Foundation.h>
#import <Quartz/Quartz.h>

#include <cstdint>

// https://chromium.googlesource.com/chromium/src/+/refs/heads/main/ui/base/cocoa/remote_layer_api.h

// The CAContextID type identifies a CAContext across processes. This is the
// token that is passed from the process that is sharing the CALayer that it is
// rendering to the process that will be displaying that CALayer.
typedef uint32_t CAContextID;

// The CAContext has a static CAContextID which can be sent to another process.
// When a CALayerHost is created using that CAContextID in another process, the
// content displayed by that CALayerHost will be the content of the CALayer
// that is set as the |layer| property on the CAContext.
@interface CAContext : NSObject
+ (instancetype)contextWithCGSConnection:(CAContextID)contextId options:(NSDictionary *)optionsDict;
@property(readonly) CAContextID contextId;
@property(retain) CALayer *layer;
@end

// The CALayerHost is created in the process that will display the content
// being rendered by another process. Setting the |contextId| property on
// an object of this class will make this layer display the content of the
// CALayer that is set to the CAContext with that CAContextID in the layer
// sharing process.
@interface CALayerHost : CALayer
@property CAContextID contextId;
@end

// The CGSConnectionID is used to create the CAContext in the process that is
// going to share the CALayers that it is rendering to another process to
// display.
typedef uint32_t CGSConnectionID;
extern "C" CGSConnectionID CGSMainConnectionID(void);

extern "C" NSString *const kCAContextCIFilterBehavior;

// --- Folded from platform/macos/rendering_context_driver_vulkan_macos.h ---
#ifdef VULKAN_ENABLED

#include "drivers/vulkan/rendering_context_driver_vulkan.h"

#import <QuartzCore/CAMetalLayer.h>

class RenderingContextDriverVulkanMacOS : public RenderingContextDriverVulkan {
private:
	virtual const char *_get_platform_surface_extension() const override final;

protected:
	SurfaceID surface_create(const void *p_platform_data) override final;

public:
	struct WindowPlatformData {
		CAMetalLayer *const *layer_ptr;
	};

	RenderingContextDriverVulkanMacOS();
	~RenderingContextDriverVulkanMacOS();
};

#endif // VULKAN_ENABLED

// --- Folded from platform/macos/tts_macos.h ---
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"

#import <AppKit/AppKit.h>

#if __has_include(<AVFAudio/AVSpeechSynthesis.h>)
#import <AVFAudio/AVSpeechSynthesis.h>
#else
#import <AVFoundation/AVFoundation.h>
#endif

struct TTSUtterance;

@interface TTS_MacOS : NSObject <AVSpeechSynthesizerDelegate> {
	bool speaking;
	HashMap<id, int64_t> ids;

	id synth;
	List<TTSUtterance> queue;
}

- (void)pauseSpeaking;
- (void)resumeSpeaking;
- (void)stopSpeaking;
- (bool)isSpeaking;
- (bool)isPaused;
- (void)speak:(const String &)text voice:(const String &)voice volume:(int)volume pitch:(float)pitch rate:(float)rate utterance_id:(int64_t)utterance_id interrupt:(bool)interrupt;
- (Array)getVoices;
@end

#include "core/os/os.h"

#define FontVariation __FontVariation
#define BitMap _QDBitMap // Suppress deprecated QuickDraw definition.

#import <AppKit/AppKit.h>
#import <AppKit/NSCursor.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

#undef BitMap
#undef FontVariation

@class GodotWindow;
@class GodotContentView;
@class GodotWindowDelegate;
@class GodotButtonView;
@class GodotProgressView;

class InputEvent;
class InputEventWithModifiers;
class NativeMenuMacOS;

#ifdef TOOLS_ENABLED
class EmbeddedProcessMacOS;
#endif

#ifdef GLES3_ENABLED
class GLManagerLegacy_MacOS;
#ifdef ANGLE_ENABLED
class GLManagerANGLE_MacOS;
#endif
#endif

class DisplayServerMacOS : public DisplayServerMacOSBase {

public:
	struct KeyEvent {
		DisplayServerEnums::WindowID window_id = DisplayServerEnums::INVALID_WINDOW_ID;
		unsigned int macos_state = false;
		bool pressed = false;
		bool echo = false;
		bool raw = false;
		Key keycode = Key::NONE;
		Key physical_keycode = Key::NONE;
		Key key_label = Key::NONE;
		uint32_t unicode = 0;
		KeyLocation location = KeyLocation::UNSPECIFIED;
	};

	struct WindowData {
		GodotWindowDelegate *window_delegate;
		GodotWindow *window_object;
		GodotContentView *window_view;
		GodotButtonView *window_button_view;

		Vector<Vector2> mpath;

		CGDirectDisplayID display_id = -1;

		Point2i mouse_pos;
		DisplayServerEnums::WindowResizeEdge edge = DisplayServerEnums::WINDOW_EDGE_MAX;

		Size2i min_size;
		Size2i max_size;
		Size2i size;
		Vector2i wb_offset = Vector2i(14, 14);

		NSRect last_frame_rect = NSMakeRect(0, 0, 0, 0);
		NSRect pre_zoom_rect = NSMakeRect(0, 0, 0, 0);

		bool im_active = false;
		Size2i im_position;

		Callable rect_changed_callback;
		Callable event_callback;
		Callable input_event_callback;
		Callable input_text_callback;
		Callable drop_files_callback;

		ObjectID instance_id;
		bool fs_transition = false;
		bool initial_size = true;

		DisplayServerEnums::WindowID transient_parent = DisplayServerEnums::INVALID_WINDOW_ID;
		bool exclusive = false;
		HashSet<DisplayServerEnums::WindowID> transient_children;

		bool layered_window = false;
		bool fullscreen = false;
		bool exclusive_fullscreen = false;
		bool on_top = false;
		bool borderless = false;
		bool resize_disabled = false;
		bool no_min_btn = false;
		bool no_max_btn = false;
		bool no_focus = false;
		bool is_popup = false;
		bool mpass = false;
		bool focused = false;
		bool is_visible = true;
		bool extend_to_title = false;
		bool hide_from_capture = false;

		HDROutput hdr_output;

		Rect2i parent_safe_rect;
	};

	List<DisplayServerEnums::WindowID> popup_list;
	uint64_t time_since_popup = 0;

	GodotProgressView *dock_progress = nullptr;

private:
	id screen_observer = nil;

#if defined(GLES3_ENABLED)
	GLManagerLegacy_MacOS *gl_manager_legacy = nullptr;
#if defined(ANGLE_ENABLED)
	GLManagerANGLE_MacOS *gl_manager_angle = nullptr;
#endif
#endif
	String rendering_driver;

	struct WarpEvent {
		NSTimeInterval timestamp;
		NSPoint delta;
	};
	List<WarpEvent> warp_events;
	NSTimeInterval last_warp = 0;
	bool ignore_warp = false;

	Vector<KeyEvent> key_event_buffer;
	int key_event_pos = 0;

	id menu_delegate = nullptr;
	NativeMenuMacOS *native_menu = nullptr;

	CGEventSourceRef event_source;
	void _mouse_apply_mode(DisplayServerEnums::MouseMode p_prev_mode, DisplayServerEnums::MouseMode p_new_mode) override;

	HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) override;
	const HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) const override;

	bool drop_events = false;
	bool in_dispatch_input_event = false;

	DisplayServerEnums::WindowID window_mouseover_id = DisplayServerEnums::INVALID_WINDOW_ID;
	DisplayServerEnums::WindowID last_focused_window = DisplayServerEnums::INVALID_WINDOW_ID;
	DisplayServerEnums::WindowID window_id_counter = DisplayServerEnums::MAIN_WINDOW_ID;
	float display_max_scale = 1.f;
	mutable Point2i origin;
	mutable bool displays_arrangement_dirty = true;
	bool is_resizing = false;

	NSCursor *cursors[DisplayServerEnums::CURSOR_MAX];
	HashMap<DisplayServerEnums::CursorShape, Vector<Variant>> cursors_cache;

	HashMap<DisplayServerEnums::WindowID, WindowData> windows;

	struct IndicatorData {
		id delegate;
		id item;
	};

	DisplayServerEnums::IndicatorID indicator_id_counter = 0;
	HashMap<DisplayServerEnums::IndicatorID, IndicatorData> indicators;

	Callable help_search_callback;
	Callable help_action_callback;

	struct MenuCall {
		Variant tag;
		Callable callback;
	};
	List<MenuCall> deferred_menu_calls;

	DisplayServerEnums::WindowID _create_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, const Rect2i &p_rect);
	void _update_window_style(WindowData p_wd, DisplayServerEnums::WindowID p_window);

	void _update_displays_arrangement() const;
	Point2i _get_native_screen_position(int p_screen) const;
	static void _displays_arrangement_changed(CGDirectDisplayID display_id, CGDisplayChangeSummaryFlags flags, void *user_info);

	static void _dispatch_input_events(const Ref<InputEvent> &p_event);
	void _dispatch_input_event(const Ref<InputEvent> &p_event);
	void _push_input(const Ref<InputEvent> &p_event);
	void _process_key_events();

	static NSCursor *_cursor_from_selector(SEL p_selector, SEL p_fallback = nil);

	Error _file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, bool p_options_in_cb, DisplayServerEnums::WindowID p_window_id);

#ifdef TOOLS_ENABLED
	struct EmbeddedProcessData {
		EmbeddedProcessMacOS *process = nullptr;
		WindowData *wd = nullptr;
		CALayer *layer_host = nil;
	};
	HashMap<ProcessID, EmbeddedProcessData> embedded_processes;
#endif
	void _window_update_display_id(WindowData *p_wd);

public:
	void menu_callback(id p_sender);

	virtual bool has_window(DisplayServerEnums::WindowID p_window) const override;
	WindowData &get_window(DisplayServerEnums::WindowID p_window);

	NSImage *_convert_to_nsimg(Ref<Image> &p_image) const;
	Point2i _get_screens_origin() const;

	void set_menu_delegate(NSMenu *p_menu);

	void send_event(NSEvent *p_event);
	void send_window_event(const WindowData &p_wd, DisplayServerEnums::WindowEvent p_event) const;
	virtual void send_window_event_by_id(DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	void release_pressed_events();
	void sync_mouse_state();
	void get_key_modifier_state(unsigned int p_macos_state, Ref<InputEventWithModifiers> r_state) const;
	void update_mouse_pos(WindowData &p_wd, NSPoint p_location_in_window);
	void push_to_key_event_buffer(const KeyEvent &p_event);
	void pop_last_key_event();
	void set_last_focused_window(DisplayServerEnums::WindowID p_window);
	bool mouse_process_popups(bool p_close = false);
	void popup_open(DisplayServerEnums::WindowID p_window);
	void popup_close(DisplayServerEnums::WindowID p_window);
	void set_is_resizing(bool p_is_resizing);
	bool get_is_resizing() const;
	void reparent_check(DisplayServerEnums::WindowID p_window);
	DisplayServerEnums::WindowID _get_focused_window_or_popup() const;
	void mouse_enter_window(DisplayServerEnums::WindowID p_window);
	void mouse_exit_window(DisplayServerEnums::WindowID p_window);
	void update_presentation_mode();

	bool is_always_on_top_recursive(DisplayServerEnums::WindowID p_window) const;

	/**
	 * Get the display ID of a window.
	 */
	uint32_t window_get_display_id(DisplayServerEnums::WindowID p_window) const;
	void window_destroy(DisplayServerEnums::WindowID p_window);
	void window_resize(DisplayServerEnums::WindowID p_window, int p_width, int p_height);
	void window_set_custom_window_buttons(WindowData &p_wd, bool p_enabled);
	void set_window_per_pixel_transparency_enabled(bool p_enabled, DisplayServerEnums::WindowID p_window);

	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;
	virtual String get_name() const override;

	virtual void help_set_search_callbacks(const Callable &p_search_callback = Callable(), const Callable &p_action_callback = Callable()) override;
	Callable _help_get_search_callback() const;
	Callable _help_get_action_callback() const;

	virtual Error dialog_show(String p_title, String p_description, Vector<String> p_buttons, const Callable &p_callback) override;
	virtual Error dialog_input_text(String p_title, String p_description, String p_partial, const Callable &p_callback) override;

	virtual Error file_dialog_show(const String &p_title, const String &p_current_directory, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) override;
	virtual Error file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) override;

	bool update_mouse_wrap(WindowData &p_wd, NSPoint &r_delta, NSPoint &r_mpos, NSTimeInterval p_timestamp);
	virtual void warp_mouse(const Point2i &p_position) override;
	virtual Point2i mouse_get_position() const override;
	virtual uint32_t mouse_get_button_state() const override;

	virtual TypedArray<Rect2> get_display_cutouts(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i get_display_safe_area(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

	virtual int get_screen_count() const override;
	virtual int get_keyboard_focus_screen() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_max_scale() const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Color screen_get_pixel(const Point2i &p_position) const override;
	virtual Ref<Image> screen_get_image(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Ref<Image> screen_get_image_rect(const Rect2i &p_rect) const override;
	virtual Vector<int> get_window_list() const override;

	virtual DisplayServerEnums::WindowID create_sub_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i &p_rect = Rect2i(), bool p_exclusive = false, DisplayServerEnums::WindowID p_transient_parent = DisplayServerEnums::INVALID_WINDOW_ID) override;
	virtual void show_window(DisplayServerEnums::WindowID p_id) override;
	virtual void delete_sub_window(DisplayServerEnums::WindowID p_id) override;

	virtual DisplayServerEnums::WindowID window_get_active_popup() const override;
	virtual void window_set_popup_safe_rect(DisplayServerEnums::WindowID p_window, const Rect2i &p_rect) override;
	virtual Rect2i window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window) const override;

	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_title_size(const String &p_title, DisplayServerEnums::WindowID p_window) const override;
	virtual void window_set_mouse_passthrough(const Vector<Vector2> &p_region, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual Point2i window_get_position(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Point2i window_get_position_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) override;
	virtual void window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive) override;

	virtual void window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_max_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_min_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::WindowMode window_get_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool window_is_maximize_allowed(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_request_attention(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_move_to_foreground(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_focused(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual DisplayServerEnums::WindowID get_focused_window() const override;

	virtual bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool can_any_window_draw() const override;

	virtual void window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i &p_position) const override;

	virtual int64_t window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual ObjectID window_get_attached_instance_id(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void gl_window_make_current(DisplayServerEnums::WindowID p_window_id) override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

public:
	void update_screen_parameters();

	virtual bool window_maximize_on_title_dbl_click() const override;
	virtual bool window_minimize_on_title_dbl_click() const override;

	virtual void window_start_drag(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_window_buttons_offset(const Vector2i &p_offset, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Vector3i window_get_safe_title_margins(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	void cursor_update_shape();
	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override;
	virtual void cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW, const Vector2 &p_hotspot = Vector2()) override;

	virtual bool get_swap_cancel_ok() override;

	virtual void enable_for_stealing_focus(ProcessID pid) override;
#ifdef TOOLS_ENABLED
	Error embed_process_update(DisplayServerEnums::WindowID p_window, EmbeddedProcessMacOS *p_process);
	virtual Error request_close_embedded_process(ProcessID p_pid) override;
	virtual Error remove_embedded_process(ProcessID p_pid) override;
#endif

	void _process_events(bool p_pump);
	virtual void process_events() override;
	virtual void force_process_and_drop_events() override;

	virtual void release_rendering_thread() override;
	virtual void swap_buffers() override;

	virtual void set_native_icon(const String &p_filename) override;
	virtual void set_icon(const Ref<Image> &p_icon) override;

	virtual DisplayServerEnums::IndicatorID create_status_indicator(const Ref<Texture2D> &p_icon, const String &p_tooltip, const Callable &p_callback) override;
	virtual void status_indicator_set_icon(DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D> &p_icon) override;
	virtual void status_indicator_set_tooltip(DisplayServerEnums::IndicatorID p_id, const String &p_tooltip) override;
	virtual void status_indicator_set_menu(DisplayServerEnums::IndicatorID p_id, const RID &p_menu_rid) override;
	virtual void status_indicator_set_callback(DisplayServerEnums::IndicatorID p_id, const Callable &p_callback) override;
	virtual Rect2 status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const override;
	virtual void delete_status_indicator(DisplayServerEnums::IndicatorID p_id) override;

	virtual bool is_window_transparency_available() const override;

	void window_get_edr_values(DisplayServerEnums::WindowID p_window, CGFloat *r_max_potential_edr_value, CGFloat *r_max_edr_value) const override;

	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();

	static void register_macos_driver();

	DisplayServerMacOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerMacOS();
};

// --- Folded from platform/macos/gl_manager_macos_angle.h ---
#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#include "core/templates/vector.h"
#include "drivers/egl/egl_manager.h"
#include "servers/display/display_server_enums.h"

// Suppress redefinition conflicts
#define FontVariation __FontVariation
#define BitMap __BitMap

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreVideo/CoreVideo.h>

#undef BitMap
#undef FontVariation

class GLManagerANGLE_MacOS : public EGLManager {
private:
	virtual const char *_get_platform_extension_name() const override;
	virtual EGLenum _get_platform_extension_enum() const override;
	virtual EGLenum _get_platform_api_enum() const override;
	virtual Vector<EGLAttrib> _get_platform_display_attributes() const override;
	virtual Vector<EGLint> _get_platform_context_attribs() const override;

public:
	void window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height) {}

	GLManagerANGLE_MacOS() {}
	~GLManagerANGLE_MacOS() {}
};

#endif // MACOS_ENABLED && GLES3_ENABLED

// --- Folded from platform/macos/gl_manager_macos_legacy.h ---
#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#include "platform/macos/platform_gl.h"

#include "core/typedefs.h"
#include "core/templates/rb_map.h"
#include "servers/display/display_server_enums.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreVideo/CoreVideo.h>

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in macOS 10.14.

typedef CGLError (*CGLEnablePtr)(CGLContextObj ctx, CGLContextEnable pname);
typedef CGLError (*CGLSetParameterPtr)(CGLContextObj ctx, CGLContextParameter pname, const GLint *params);
typedef CGLContextObj (*CGLGetCurrentContextPtr)();

class GLManagerLegacy_MacOS {
	struct GLWindow {
		id window_view = nullptr;
		NSOpenGLContext *context = nullptr;
	};

	RBMap<DisplayServerEnums::WindowID, GLWindow> windows;

	NSOpenGLContext *shared_context = nullptr;
	DisplayServerEnums::WindowID current_window = DisplayServerEnums::INVALID_WINDOW_ID;

	Error create_context(GLWindow &win);

	bool framework_loaded = false;
	bool use_vsync = false;
	CGLEnablePtr CGLEnable = nullptr;
	CGLSetParameterPtr CGLSetParameter = nullptr;
	CGLGetCurrentContextPtr CGLGetCurrentContext = nullptr;

public:
	Error window_create(DisplayServerEnums::WindowID p_window_id, id p_view, int p_width, int p_height);
	void window_destroy(DisplayServerEnums::WindowID p_window_id);
	void window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height);

	void release_current();
	void swap_buffers();

	void window_make_current(DisplayServerEnums::WindowID p_window_id);

	void window_set_per_pixel_transparency_enabled(DisplayServerEnums::WindowID p_window_id, bool p_enabled);

	Error initialize();

	void set_use_vsync(bool p_use);
	bool is_using_vsync() const;

	NSOpenGLContext *get_context(DisplayServerEnums::WindowID p_window_id);

	GLManagerLegacy_MacOS();
	~GLManagerLegacy_MacOS();
};

GODOT_CLANG_WARNING_PUSH

#endif // MACOS_ENABLED && GLES3_ENABLED

// --- Folded from platform/macos/godot_core_cursor.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

constexpr int32_t GDCoreCursorWindowMove = 39;

// Expose private interface for CoreCursor.
@interface GodotCoreCursor : NSCursor

@property(readonly, nonatomic) int32_t _coreCursorType;

- (id)initWithType:(int32_t)type;

@end

// --- Folded from platform/macos/godot_open_save_delegate.h ---
#include "core/templates/hash_map.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

@interface GodotOpenSaveDelegate : NSObject <NSOpenSavePanelDelegate> {
	NSSavePanel *dialog;
	NSMutableArray *allowed_types;
	Vector<Vector<String>> preferred_types;

	HashMap<int, String> ctr_ids;
	Dictionary options;
	int cur_index;
	int ctr_id;

	String root;
}

- (void)makeAccessoryView:(NSSavePanel *)p_panel filters:(const Vector<String> &)p_filters options:(const TypedArray<Dictionary> &)p_options;
- (void)setFileTypes:(NSMutableArray *)p_allowed_types pref:(const Vector<Vector<String>> &)p_preftypes;
- (void)popupOptionAction:(id)p_sender;
- (void)popupCheckAction:(id)p_sender;
- (void)popupFileAction:(id)p_sender;
- (int)getIndex;
- (Dictionary)getSelection;
- (int)setDefaultInt:(const String &)p_name value:(int)p_value;
- (int)setDefaultBool:(const String &)p_name value:(bool)p_value;
- (void)setRootPath:(const String &)p_root_path;
- (String)validateFilename:(const String &)p_path;

@end

// --- Folded from platform/macos/godot_progress_view.h ---


#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotProgressView : NSView {
	DisplayServerEnums::ProgressState pr_state;
	float pr_value;
	float pr_offset;
}
- (void)setValue:(float)value;
- (void)setState:(DisplayServerEnums::ProgressState)state;
@end

// --- Folded from platform/macos/godot_status_item.h ---
#include "core/input/input_event.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotStatusItemDelegate : NSObject {
	Callable cb;
}

- (IBAction)click:(id)sender;

- (void)setCallback:(const Callable &)callback;

@end

// --- Folded from platform/macos/godot_window_delegate.h ---
#include "servers/display/display_server_enums.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

class DisplayServerMacOS;

@interface GodotWindowDelegate : NSObject <NSWindowDelegate>

- (void)setWindowID:(DisplayServerEnums::WindowID)wid;

- (instancetype)initWithDisplayServer:(DisplayServerMacOS *)p_ds;

@end

// --- Folded from platform/macos/godot_window.h ---
#include "servers/display/display_server_enums.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotWindow : NSWindow {
	DisplayServerEnums::WindowID window_id;
	NSTimeInterval anim_duration;
}

- (void)setWindowID:(DisplayServerEnums::WindowID)wid;
- (void)setAnimDuration:(NSTimeInterval)duration;

@end

// --- Folded from platform/macos/godot_button_view.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotButtonView : NSView {
	NSTrackingArea *tracking_area;
	NSPoint offset;
	CGFloat spacing;
	bool mouse_in_group;
	bool rtl;
	NSButton *close_button;
	NSButton *miniaturize_button;
	NSButton *zoom_button;
}

- (void)initButtons:(CGFloat)button_spacing offset:(NSPoint)button_offset rtl:(bool)is_rtl;
- (void)displayButtons;
- (void)setOffset:(NSPoint)button_offset;
- (NSPoint)getOffset;

@end

// --- Folded from platform/macos/godot_content_view.h ---
#include "core/input/input_event.h"
#include "core/typedefs.h"
#include "servers/display/display_server_enums.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#if defined(GLES3_ENABLED)
#import <AppKit/NSOpenGLView.h>
#define RootView NSOpenGLView
#else
#define RootView NSView
#endif

#import <QuartzCore/CAMetalLayer.h>

@interface GodotContentLayerDelegate : NSObject <CALayerDelegate> {
	DisplayServerEnums::WindowID window_id;
	bool need_redraw;
}

- (void)setWindowID:(DisplayServerEnums::WindowID)wid;
- (void)setNeedRedraw:(bool)redraw;

@end

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in macOS 10.14.

@interface GodotContentView : RootView <NSTextInputClient> {
	DisplayServerEnums::WindowID window_id;
	NSTrackingArea *tracking_area;
	NSMutableAttributedString *marked_text;
	bool ime_input_event_in_progress;
	bool mouse_down_control;
	bool ignore_momentum_scroll;
	bool last_pen_inverted;
	bool ime_suppress_next_keyup;
	id layer_delegate;
	NSMutableSet<NSString *> *registered_observers;
}

- (void)processScrollEvent:(NSEvent *)event button:(MouseButton)button factor:(double)factor;
- (void)processPanEvent:(NSEvent *)event dx:(double)dx dy:(double)dy;
- (void)processMouseEvent:(NSEvent *)event index:(MouseButton)index pressed:(bool)pressed outofstream:(bool)outofstream;
- (void)setWindowID:(DisplayServerEnums::WindowID)wid;
- (void)updateLayerDelegate;
- (void)cancelComposition;

@end

GODOT_CLANG_WARNING_POP

// --- Folded from platform/macos/display_server_macos_embedded.h ---
@class CAContext;
@class CALayer;
class InputEvent;
class NativeMenu;

#ifdef GLES3_ENABLED
class GLManagerEmbedded;
#endif

struct DisplayServerMacOSEmbeddedState {
	/*! Default to a scale of 2.0, which is the most common. */
	float screen_max_scale = 2.0f;
	float screen_dpi = 96.0f;
	/*! Scale for window displaying embedded content */
	float screen_window_scale = 2.0f;
	/*! The display ID of the window which is displaying the embedded process content. */
	uint32_t display_id = -1;

	/*! The current maximum EDR value for the display. */
	double screen_max_edr = 1.0;
	/*! The maximum possible EDR value for the display. */
	double screen_max_potential_edr = 1.0;

	void serialize(PackedByteArray &r_data);
	Error deserialize(const PackedByteArray &p_data);

	_FORCE_INLINE_ bool operator==(const DisplayServerMacOSEmbeddedState &p_other) const {
		return screen_max_scale == p_other.screen_max_scale && screen_dpi == p_other.screen_dpi && screen_window_scale == p_other.screen_window_scale && display_id == p_other.display_id && screen_max_edr == p_other.screen_max_edr && screen_max_potential_edr == p_other.screen_max_potential_edr;
	}
};

/// "Embedded" as in "Embedded in the Godot editor window".
class DisplayServerMacOSEmbedded : public DisplayServerMacOSBase {

	DisplayServerMacOSEmbeddedState state;

	HashMap<DisplayServerEnums::WindowID, ObjectID> window_attached_instance_id;

	HashMap<DisplayServerEnums::WindowID, Callable> window_event_callbacks;
	HashMap<DisplayServerEnums::WindowID, Callable> window_resize_callbacks;
	HashMap<DisplayServerEnums::WindowID, Callable> input_event_callbacks;
	HashMap<DisplayServerEnums::WindowID, Callable> input_text_callbacks;

	DisplayServerEnums::WindowID window_id_counter = DisplayServerEnums::MAIN_WINDOW_ID;

	bool transparent = false;

	CAContext *ca_context = nullptr;
	// Either be a CAMetalLayer or a CALayer depending on the rendering driver.
	CALayer *layer = nullptr;
#ifdef GLES3_ENABLED
	GLManagerEmbedded *gl_manager = nullptr;
#endif

	String rendering_driver;

	HDROutput hdr_output;

	HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) override;
	const HDROutput &_get_hdr_output(DisplayServerEnums::WindowID p_window) const override;

	Point2i ime_last_position;

	// Convert source pixel size to render pixel size, accounting for the
	// difference between display scale and render scale when hiDPI is disabled.
	Size2i _source_to_render_size(const Size2i &p_source_size) const;

	void _mouse_apply_mode(DisplayServerEnums::MouseMode p_prev_mode, DisplayServerEnums::MouseMode p_new_mode) override;

	struct Joy {
		String name;
		uint64_t timestamp = 0;

		Joy() = default;
		Joy(const String &p_name) :
				name(p_name) {}
	};
	HashMap<int, Joy> joysticks;

public:
	static void register_embedded_driver();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();

	void _window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);

	// MARK: - Events

	virtual void process_events() override;

	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	static void _dispatch_input_events(const Ref<InputEvent> &p_event);
	void send_input_event(const Ref<InputEvent> &p_event, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const;
	void send_input_text(const String &p_text, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const;
	virtual void send_window_event_by_id(DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	void _window_callback(const Callable &p_callable, const Variant &p_arg) const;

	// MARK: - Mouse
	virtual void warp_mouse(const Point2i &p_position) override;
	virtual Point2i mouse_get_position() const override;
	virtual uint32_t mouse_get_button_state() const override;

	// MARK: - Window

	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;
	virtual String get_name() const override;

	virtual int get_screen_count() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Vector<DisplayServerEnums::WindowID> get_window_list() const override;

	virtual DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i &p_position) const override;

	virtual void window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual ObjectID window_get_attached_instance_id(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual Point2i window_get_position(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Point2i window_get_position_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) override;

	virtual void window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_max_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_min_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::WindowMode window_get_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool window_is_maximize_allowed(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_request_attention(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_move_to_foreground(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_focused(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual float screen_get_max_scale() const override;

	virtual bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool can_any_window_draw() const override;

	virtual void window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override;
	virtual void cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW, const Vector2 &p_hotspot = Vector2()) override;

	void window_get_edr_values(DisplayServerEnums::WindowID p_window, CGFloat *r_max_potential_edr_value, CGFloat *r_max_edr_value) const override;
	void update_screen_parameters();

	void set_state(const DisplayServerMacOSEmbeddedState &p_state);
	virtual void swap_buffers() override;

	DisplayServerMacOSEmbedded(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, Error &r_error);
	~DisplayServerMacOSEmbedded();
};

// --- Folded from platform/macos/embedded_debugger.h ---
#include "core/templates/hash_map.h"

class DisplayServerMacOSEmbedded;

class EmbeddedDebugger final
{
#ifdef DEBUG_ENABLED
	typedef Error (*ParseMessageFunc)(const Array& p_args);

	struct Data
	{
		DisplayServerMacOSEmbedded* ds = nullptr;
		HashMap<String, ParseMessageFunc> parse_message_handlers;
	};

	static inline Data* data = nullptr;

	static void _init_parse_message_handlers();

	static Error _msg_window_size(const Array& p_args);
	static Error _msg_mouse_set_mode(const Array& p_args);
	static Error _msg_event(const Array& p_args);
	static Error _msg_win_event(const Array& p_args);
	static Error _msg_notification(const Array& p_args);
	static Error _msg_ime_update(const Array& p_args);
	static Error _msg_ds_state(const Array& p_args);
#endif

public:
	EmbeddedDebugger() = delete;

	static void initialize(DisplayServerMacOSEmbedded* p_ds);
	static void deinitialize();

#ifdef DEBUG_ENABLED
	static Error parse_message(
		void* p_user, const String& p_msg, const Array& p_args, bool& r_captured);
#endif
};

// --- Folded from platform/macos/embedded_gl_manager.h ---
#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#include "core/typedefs.h"
#include "core/math/vector2i.h"
#include "core/templates/rb_map.h"
#include "servers/display/display_server_enums.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreVideo/CoreVideo.h>

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in macOS 10.14.

typedef CGLContextObj (*CGLGetCurrentContextPtr)();
typedef CGLError (*CGLTexImageIOSurface2DPtr)(CGLContextObj ctx, GLenum target, GLenum internal_format,
		GLsizei width, GLsizei height, GLenum format, GLenum type, IOSurfaceRef ioSurface, GLuint plane);
typedef const char *(*CGLErrorStringPtr)(CGLError);

class GLManagerEmbedded {
	/// @brief The number of framebuffers to create for each window.
	///
	/// Triple-buffering is used to avoid stuttering.
	static constexpr uint32_t BUFFER_COUNT = 3;

	// The display ID for which vsync is used. If this value is -1, vsync is disabled.
	constexpr static uint32_t INVALID_DISPLAY_ID = static_cast<uint32_t>(-1);

	struct FrameBuffer {
		IOSurfaceRef surface = nullptr;
		unsigned int tex = 0;
		unsigned int fbo = 0;
	};

	struct GLWindow {
		uint32_t width = 0;
		uint32_t height = 0;
		CALayer *layer = nullptr;
		NSOpenGLContext *context = nullptr;
		FrameBuffer framebuffers[BUFFER_COUNT];
		uint32_t current_fb = 0;
		bool is_valid = false;

		void destroy_framebuffers();

		~GLWindow() { destroy_framebuffers(); }
	};

	RBMap<DisplayServerEnums::WindowID, GLWindow> windows;
	typedef RBMap<DisplayServerEnums::WindowID, GLWindow>::Element GLWindowElement;

	NSOpenGLContext *shared_context = nullptr;
	DisplayServerEnums::WindowID current_window = DisplayServerEnums::INVALID_WINDOW_ID;

	Error create_context(GLWindow &p_win);

	bool framework_loaded = false;
	CGLGetCurrentContextPtr CGLGetCurrentContext = nullptr;
	CGLTexImageIOSurface2DPtr CGLTexImageIOSurface2D = nullptr;
	CGLErrorStringPtr CGLErrorString = nullptr;

	uint32_t display_id = INVALID_DISPLAY_ID;
	CVDisplayLinkRef display_link = nullptr;
	bool vsync_enabled = false;
	bool display_link_running = false;
	dispatch_semaphore_t display_semaphore = nullptr;

	void create_display_link();
	void release_display_link();

public:
	Error window_create(DisplayServerEnums::WindowID p_window_id, CALayer *p_layer, int p_width, int p_height);
	void window_destroy(DisplayServerEnums::WindowID p_window_id);
	void window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height);
	Size2i window_get_size(DisplayServerEnums::WindowID p_window_id) const;

	void set_display_id(uint32_t p_display_id);
	void set_vsync_enabled(bool p_enabled);
	bool is_vsync_enabled() const { return vsync_enabled; }

	void release_current();
	void swap_buffers();

	void window_make_current(DisplayServerEnums::WindowID p_window_id);

	Error initialize();

	GLManagerEmbedded();
	~GLManagerEmbedded();
};

GODOT_CLANG_WARNING_PUSH

#endif // MACOS_ENABLED && GLES3_ENABLED

// --- Folded from platform/macos/key_mapping_macos.h ---
#include "core/os/keyboard.h"

class KeyMappingMacOS {
	KeyMappingMacOS() {}

	static bool is_numpad_key(unsigned int p_key);

public:
	static void initialize();

	// Mappings input.
	static Key translate_key(unsigned int p_key);
	static unsigned int unmap_key(Key p_key);
	static Key remap_key(unsigned int p_key, unsigned int p_state, bool p_unicode);
	static KeyLocation translate_location(unsigned int p_key);

	// Mapping for menu shortcuts.
	static String keycode_get_native_string(Key p_keycode);
	static unsigned int keycode_get_native_mask(Key p_keycode);
};

// --- Folded from platform/macos/native_menu_macos.h ---
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "servers/display/display_server.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

class NativeMenuMacOS : public NativeMenuDriver {

	struct MenuData {
		NSMenu *menu = nullptr;

		Callable open_cb;
		Callable close_cb;
		bool is_open = false;
		bool is_system = false;
	};

	mutable RID_PtrOwner<MenuData> menus;
	HashMap<NSMenu *, RID> menu_lookup;

	NSMenu *main_menu_ns = nullptr;
	NSMenu *application_menu_ns = nullptr;
	NSMenu *window_menu_ns = nullptr;
	NSMenu *help_menu_ns = nullptr;
	NSMenu *dock_menu_ns = nullptr;

	RID main_menu;
	RID application_menu;
	RID window_menu;
	RID help_menu;
	RID dock_menu;

	void _update_align(MenuData *p_md);

	int _get_system_menu_start(const NSMenu *p_menu) const;
	int _get_system_menu_count(const NSMenu *p_menu) const;
	bool _is_menu_opened(NSMenu *p_menu) const;
	NSMenuItem *_menu_add_item(NSMenu *p_menu, const String &p_label, Key p_accel, int p_index, int *r_out);

public:
	void _register_system_menus(NSMenu *p_main_menu, NSMenu *p_application_menu, NSMenu *p_window_menu, NSMenu *p_help_menu, NSMenu *p_dock_menu);
	NSMenu *_get_dock_menu();

	void _menu_need_update(NSMenu *p_menu);
	void _menu_open(NSMenu *p_menu);
	void _menu_close(NSMenu *p_menu);
	void _menu_close_cb(const RID &p_rid);

	virtual bool has_feature(Feature p_feature) const override;

	virtual bool has_system_menu(SystemMenus p_menu_id) const override;
	virtual RID get_system_menu(SystemMenus p_menu_id) const override;

	virtual RID create_menu() override;
	virtual bool has_menu(const RID &p_rid) const override;
	virtual void free_menu(const RID &p_rid) override;

	NSMenu *get_native_menu_handle(const RID &p_rid);

	virtual String get_system_menu_text(SystemMenus p_menu_id) const override;
	virtual void set_system_menu_text(SystemMenus p_menu_id, const String &p_name) override;

	virtual Size2 get_size(const RID &p_rid) const override;
	virtual void popup(const RID &p_rid, const Vector2i &p_position) override;

	virtual void set_interface_direction(const RID &p_rid, bool p_is_rtl) override;
	virtual void set_popup_open_callback(const RID &p_rid, const Callable &p_callback) override;
	virtual Callable get_popup_open_callback(const RID &p_rid) const override;
	virtual void set_popup_close_callback(const RID &p_rid, const Callable &p_callback) override;
	virtual Callable get_popup_close_callback(const RID &p_rid) const override;
	virtual void set_minimum_width(const RID &p_rid, float p_width) override;
	virtual float get_minimum_width(const RID &p_rid) const override;

	virtual bool is_opened(const RID &p_rid) const override;

	virtual int add_submenu_item(const RID &p_rid, const String &p_label, const RID &p_submenu_rid, const Variant &p_tag = Variant(), int p_index = -1) override;
	virtual int add_item(const RID &p_rid, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_check_item(const RID &p_rid, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_icon_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_icon_check_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_radio_check_item(const RID &p_rid, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_icon_radio_check_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_multistate_item(const RID &p_rid, const String &p_label, int p_max_states, int p_default_state, const Callable &p_callback = Callable(), const Callable &p_key_callback = Callable(), const Variant &p_tag = Variant(), Key p_accel = Key::NONE, int p_index = -1) override;
	virtual int add_separator(const RID &p_rid, int p_index = -1) override;

	virtual int find_item_index_with_text(const RID &p_rid, const String &p_text) const override;
	virtual int find_item_index_with_tag(const RID &p_rid, const Variant &p_tag) const override;

	virtual bool is_item_checked(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_indeterminate(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_checkable(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_radio_checkable(const RID &p_rid, int p_idx) const override;
	virtual Callable get_item_callback(const RID &p_rid, int p_idx) const override;
	virtual Callable get_item_key_callback(const RID &p_rid, int p_idx) const override;
	virtual Variant get_item_tag(const RID &p_rid, int p_idx) const override;
	virtual String get_item_text(const RID &p_rid, int p_idx) const override;
	virtual RID get_item_submenu(const RID &p_rid, int p_idx) const override;
	virtual Key get_item_accelerator(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_disabled(const RID &p_rid, int p_idx) const override;
	virtual bool is_item_hidden(const RID &p_rid, int p_idx) const override;
	virtual String get_item_tooltip(const RID &p_rid, int p_idx) const override;
	virtual int get_item_state(const RID &p_rid, int p_idx) const override;
	virtual int get_item_max_states(const RID &p_rid, int p_idx) const override;
	virtual Ref<Texture2D> get_item_icon(const RID &p_rid, int p_idx) const override;
	virtual int get_item_indentation_level(const RID &p_rid, int p_idx) const override;

	virtual void set_item_checked(const RID &p_rid, int p_idx, bool p_checked) override;
	virtual void set_item_indeterminate(const RID &p_rid, int p_idx, bool p_cindeterminate) override;
	virtual void set_item_checkable(const RID &p_rid, int p_idx, bool p_checkable) override;
	virtual void set_item_radio_checkable(const RID &p_rid, int p_idx, bool p_checkable) override;
	virtual void set_item_callback(const RID &p_rid, int p_idx, const Callable &p_callback) override;
	virtual void set_item_key_callback(const RID &p_rid, int p_idx, const Callable &p_key_callback) override;
	virtual void set_item_hover_callbacks(const RID &p_rid, int p_idx, const Callable &p_callback) override;
	virtual void set_item_tag(const RID &p_rid, int p_idx, const Variant &p_tag) override;
	virtual void set_item_text(const RID &p_rid, int p_idx, const String &p_text) override;
	virtual void set_item_submenu(const RID &p_rid, int p_idx, const RID &p_submenu_rid) override;
	virtual void set_item_accelerator(const RID &p_rid, int p_idx, Key p_keycode) override;
	virtual void set_item_disabled(const RID &p_rid, int p_idx, bool p_disabled) override;
	virtual void set_item_hidden(const RID &p_rid, int p_idx, bool p_hidden) override;
	virtual void set_item_tooltip(const RID &p_rid, int p_idx, const String &p_tooltip) override;
	virtual void set_item_state(const RID &p_rid, int p_idx, int p_state) override;
	virtual void set_item_max_states(const RID &p_rid, int p_idx, int p_max_states) override;
	virtual void set_item_icon(const RID &p_rid, int p_idx, const Ref<Texture2D> &p_icon) override;
	virtual void set_item_indentation_level(const RID &p_rid, int p_idx, int p_level) override;
	virtual int set_item_index(const RID &p_rid, int p_idx, int p_target_idx) override;

	virtual int get_item_count(const RID &p_rid) const override;
	virtual bool is_system_menu(const RID &p_rid) const override;

	virtual void remove_item(const RID &p_rid, int p_idx) override;
	virtual void clear(const RID &p_rid) override;

	NativeMenuMacOS();
	~NativeMenuMacOS();
};

// --- Folded from platform/macos/godot_menu_delegate.h ---
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface GodotMenuDelegate : NSObject <NSMenuDelegate> {
}

- (void)doNothing:(id)sender;

@end

// --- Folded from platform/macos/godot_menu_item.h ---
#include "core/io/image.h"
#include "core/os/keyboard.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#define MENU_TAG_START 0x00004447
#define MENU_TAG_END 0xFFFF4447

enum GlobalMenuCheckType {
	CHECKABLE_TYPE_NONE,
	CHECKABLE_TYPE_CHECK_BOX,
	CHECKABLE_TYPE_RADIO_BUTTON,
};

@interface GodotMenuItem : NSObject {
@public
	Callable callback;
	Callable key_callback;
	Callable hover_callback;
	Variant meta;
	Key accel;
	GlobalMenuCheckType checkable_type;
	bool checked;
	bool enabled;
	bool indeterminate;
	int max_states;
	int state;
	int indent;
	Ref<Image> img;
}

@end
#endif // defined(MACOS_ENABLED)
// --- End platform/macos/display_server_macos.h ---
