/**************************************************************************/
/*  display_server_macos_embedded.h                                       */
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

#include "display_server_macos_base.h"

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

#include "core/error/error_list.h"
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
