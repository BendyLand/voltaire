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
// --- Folded from drivers/apple_embedded/apple_embedded.h ---
#import <CoreHaptics/CoreHaptics.h>

class AppleEmbedded
{

private:
	CHHapticEngine* haptic_engine API_AVAILABLE(ios(13)) = nullptr;

	CHHapticEngine* get_haptic_engine_instance() API_AVAILABLE(ios(13));
	void start_haptic_engine();
	void stop_haptic_engine();

public:
	mem_unique_ptr<Object> obj;
	static void alert(const char* p_alert, const char* p_title);

	bool supports_haptic_engine();
	void vibrate_haptic_engine(float p_duration_seconds, float p_amplitude);

	String get_model() const;
	String get_rate_url(int p_app_id) const;

	AppleEmbedded();
};

// --- Folded from drivers/apple_embedded/display_layer_apple_embedded.h ---
#import <QuartzCore/CAMetalLayer.h>

@protocol GDTDisplayLayer <NSObject>

- (void)startRenderDisplayLayer;
- (void)stopRenderDisplayLayer;
- (void)initializeDisplayLayer;
- (void)layoutDisplayLayer;

@end
// --- End drivers/apple_embedded/apple_embedded.h ---
#import "drivers/coreaudio/audio_driver_coreaudio.h"
#include "drivers/unix/os_unix.h"
#include "servers/audio/audio_server.h"
#include "servers/rendering/renderer.h"

#if defined(RD_ENABLED)
#include "servers/rendering/rendering_device.h"

#if defined(VULKAN_ENABLED)

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


// --- Folded from drivers/apple_embedded/godot_view_apple_embedded.h ---
#import <UIKit/UIKit.h>

class String;

@class GDTView;
@protocol GDTDisplayLayer;
@protocol GDTViewRendererProtocol;

@protocol GDTViewDelegate

- (BOOL)godotViewFinishedSetup:(GDTView *)view;

@end

@interface GDTView : UIView

@property(assign, nonatomic) id<GDTViewRendererProtocol> renderer;
@property(assign, nonatomic) id<GDTViewDelegate> delegate;

@property(assign, readonly, nonatomic) BOOL isActive;

@property(assign, nonatomic) BOOL useCADisplayLink;
@property(strong, readonly, nonatomic) CALayer<GDTDisplayLayer> *renderingLayer;
@property(assign, readonly, nonatomic) BOOL canRender;

@property(assign, nonatomic) float preferredFrameRate;

// Can be extended by subclasses
- (void)godot_commonInit;

// Implemented in subclasses
- (CALayer<GDTDisplayLayer> *)initializeRenderingForDriver:(NSString *)driverName;

- (void)startRendering;
- (void)stopRendering;

@end

// Implemented in subclasses
extern GDTView *GDTViewCreate();

// --- Folded from drivers/apple_embedded/godot_view_renderer.h ---
#import <UIKit/UIKit.h>

@protocol GDTViewRendererProtocol <NSObject>

@property(assign, readonly, nonatomic) BOOL hasFinishedSetup;

- (BOOL)setupView:(UIView *)view;
- (void)renderOnView:(UIView *)view;

@end

@interface GDTViewRenderer : NSObject <GDTViewRendererProtocol>

@end
// --- End drivers/apple_embedded/godot_view_apple_embedded.h ---


// --- Folded from drivers/apple_embedded/godot_view_controller.h ---
#import <UIKit/UIKit.h>

@class GDTView;
@class GDTKeyboardInputView;

@interface GDTViewController : UIViewController

@property(nonatomic, readonly, strong) GDTView *godotView;
@property(nonatomic, readonly, strong) GDTKeyboardInputView *keyboardView;

#ifdef IOS_ENABLED
- (void)propagateUIPreferencesToRootViewController;
#endif

@end
// --- End drivers/apple_embedded/godot_view_controller.h ---


// --- Folded from drivers/apple_embedded/app_delegate_service.h ---
#import <UIKit/UIKit.h>

@class GDTViewController;

@interface GDTAppDelegateService : NSObject <UIApplicationDelegate, UIWindowSceneDelegate>

@property(strong, class, nonatomic) GDTViewController *viewController;

@end
// --- End drivers/apple_embedded/app_delegate_service.h ---


// --- Folded from drivers/apple_embedded/godot_app_delegate.h ---
#import <UIKit/UIKit.h>

typedef NSObject<UIApplicationDelegate, UIWindowSceneDelegate> GDTAppDelegateServiceProtocol;

@interface GDTApplicationDelegate : NSObject <UIApplicationDelegate, UIWindowSceneDelegate>

@property(class, readonly, strong) NSArray<GDTAppDelegateServiceProtocol *> *services;

+ (void)addService:(GDTAppDelegateServiceProtocol *)service;

@end

// --- Folded from drivers/apple_embedded/main_utilities.h ---
void change_to_launch_dir(char **p_args);

int process_args(int p_argc, char **p_args, char **r_args);
// --- End drivers/apple_embedded/godot_app_delegate.h ---


// --- Folded from drivers/apple_embedded/display_server_apple_embedded.h ---
#include "core/os/os.h"
#include "servers/display/display_server.h"

#if defined(RD_ENABLED)
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/rendering_device.h"

#if defined(VULKAN_ENABLED)
// --- Inlined/Folded from drivers/apple_embedded/rendering_context_driver_vulkan_apple_embedded.h ---
#ifdef VULKAN_ENABLED

#include "drivers/vulkan/rendering_context_driver_vulkan.h"

#import <QuartzCore/CAMetalLayer.h>

class RenderingContextDriverVulkanAppleEmbedded : public RenderingContextDriverVulkan {
private:
	virtual const char *_get_platform_surface_extension() const override final;

protected:
	SurfaceID surface_create(const void *p_platform_data) override final;

public:
	struct WindowPlatformData {
		CAMetalLayer *const *layer_ptr;
	};

	RenderingContextDriverVulkanAppleEmbedded();
	~RenderingContextDriverVulkanAppleEmbedded();
};

#endif // VULKAN_ENABLED

#include <drivers/vulkan/godot_vulkan.h>
#endif // VULKAN_ENABLED

#if defined(METAL_ENABLED)
#import "drivers/metal/rendering_context_driver_metal.h"
#endif // METAL_ENABLED
#endif // RD_ENABLED

#if defined(GLES3_ENABLED)
#include "drivers/gles3/rasterizer_gles3.h"
#endif // GLES3_ENABLED

#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>

class InputEvent;
/// "Embedded" as in "Embedded Device".
class DisplayServerAppleEmbedded : public DisplayServerDriver {

	_THREAD_SAFE_CLASS_

#if defined(RD_ENABLED)
	RenderingContextDriver *rendering_context = nullptr;
	RenderingDevice *rendering_device = nullptr;
#endif

	id tts = nullptr;

	DisplayServerEnums::ScreenOrientation screen_orientation;

	ObjectID window_attached_instance_id;

	Callable window_event_callback;
	Callable window_resize_callback;
	Callable input_event_callback;
	Callable input_text_callback;

	Callable system_theme_changed;

	int virtual_keyboard_height = 0;

	void perform_event(const Ref<InputEvent> &p_event);

	void initialize_tts() const;

	bool edr_requested = false;
	void _update_hdr_output(bool edr_headroom_changed);
	float _calculate_current_reference_luminance() const;

protected:
	virtual bool _screen_hdr_is_supported() const { return false; }
	virtual float _screen_potential_edr_headroom() const { return 1.0f; }
	virtual float _screen_current_edr_headroom() const { return 1.0f; }
	float hardware_reference_luminance_nits = 100.0f;

	DisplayServerAppleEmbedded(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerAppleEmbedded();

public:
	String rendering_driver;

	static Vector<String> get_rendering_drivers_func();

	// MARK: - Events

	virtual void process_events() override;

	virtual void window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual void window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;

	static void _dispatch_input_events(const Ref<InputEvent> &p_event);
	void send_input_event(const Ref<InputEvent> &p_event) const;
	void send_input_text(const String &p_text) const;
	void send_window_event(DisplayServerEnums::WindowEvent p_event) const;
	void _window_callback(const Callable &p_callable, const Variant &p_arg) const;

	void emit_system_theme_changed();

	// MARK: - Input

	// MARK: Touches and Apple Pencil

	void touch_press(int p_idx, int p_x, int p_y, bool p_pressed, bool p_double_click);
	void touch_drag(int p_idx, int p_prev_x, int p_prev_y, int p_x, int p_y, float p_pressure, Vector2 p_tilt);
	void touches_canceled(int p_idx);

	// MARK: Keyboard

	void key(Key p_key, char32_t p_char, Key p_unshifted, Key p_physical, NSInteger p_modifier, bool p_pressed, KeyLocation p_location);
	bool is_keyboard_active() const;

	// MARK: Motion

	void update_gravity(const Vector3 &p_gravity);
	void update_accelerometer(const Vector3 &p_accelerometer);
	void update_magnetometer(const Vector3 &p_magnetometer);
	void update_gyroscope(const Vector3 &p_gyroscope);

	// MARK: -

	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const override;

	virtual bool tts_is_speaking() const override;
	virtual bool tts_is_paused() const override;
	virtual TypedArray<Dictionary> tts_get_voices() const override;

	virtual void tts_speak(const String &p_text, const String &p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false) override;
	virtual void tts_pause() override;
	virtual void tts_resume() override;
	virtual void tts_stop() override;

	virtual bool is_dark_mode_supported() const override;
	virtual bool is_dark_mode() const override;
	virtual void set_system_theme_change_callback(const Callable &p_callable) override;

	virtual Rect2i get_display_safe_area(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

	virtual int get_screen_count() const override;
	virtual int get_primary_screen() const override;
	virtual Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

	virtual Vector<DisplayServerEnums::WindowID> get_window_list() const override;

	virtual DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i &p_position) const override;

	virtual int64_t window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

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
	virtual void window_move_to_foreground(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_focused(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual float screen_get_max_scale() const override;

	virtual void screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen) override;
	virtual DisplayServerEnums::ScreenOrientation screen_get_orientation(int p_screen) const override;

	virtual bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual bool can_any_window_draw() const override;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_vsync_mode) const override;

	virtual bool is_touchscreen_available() const override;

	virtual void virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect, DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start, int p_cursor_end) override;
	virtual void virtual_keyboard_hide() override;

	void virtual_keyboard_set_height(int height);
	virtual int virtual_keyboard_get_height() const override;
	virtual bool has_hardware_keyboard() const override;

	virtual void clipboard_set(const String &p_text) override;
	virtual String clipboard_get() const override;

	virtual void screen_set_keep_on(bool p_enable) override;
	virtual bool screen_is_kept_on() const override;

	// MARK: - HDR / EDR

	void current_edr_headroom_changed();
	virtual bool window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual void window_request_hdr_output(const bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual bool window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual bool window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual float window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual float window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual void window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	virtual float window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;
	virtual float window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	virtual float window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override;

	void resize_window(CGSize size);
	virtual void swap_buffers() override {}

	virtual void set_native_icon(const String &p_filename) override;
	virtual void set_icon(const Ref<Image> &p_icon) override;
};

// --- Folded from drivers/apple_embedded/godot_keyboard_input_view.h ---
#import <UIKit/UIKit.h>

@interface GDTKeyboardInputView : UITextView

- (BOOL)becomeFirstResponderWithString:(NSString *)existingString cursorStart:(NSInteger)start cursorEnd:(NSInteger)end;

@end

// --- Folded from drivers/apple_embedded/key_mapping_apple_embedded.h ---
#include "core/os/keyboard.h"

#import <UIKit/UIKit.h>

class KeyMappingAppleEmbedded {
	KeyMappingAppleEmbedded() {}

public:
	static void initialize();
	static Key remap_key(CFIndex p_keycode);
	static KeyLocation key_location(CFIndex p_keycode);
};

// --- Folded from drivers/apple_embedded/tts_apple_embedded.h ---
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"

#if __has_include(<AVFAudio/AVSpeechSynthesis.h>)
#import <AVFAudio/AVSpeechSynthesis.h>
#else
#import <AVFoundation/AVFoundation.h>
#endif

struct TTSUtterance;

@interface GDTTTS : NSObject <AVSpeechSynthesizerDelegate> {
	bool speaking;
	HashMap<id, int64_t> ids;

	AVSpeechSynthesizer *av_synth;
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
// --- End drivers/apple_embedded/display_server_apple_embedded.h ---

#endif // APPLE_EMBEDDED_ENABLED
