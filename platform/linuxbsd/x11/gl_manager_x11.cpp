/**************************************************************************/
/*  gl_manager_x11.cpp                                                    */
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

#include "core/os/main_loop.h"
#include "gl_manager_x11.h"

#if defined(X11_ENABLED) && defined(GLES3_ENABLED)

#include <thirdparty/glad/glad/glx.h>
#include "core/os/os.h"
#include "servers/display/display_server.h"

#ifdef SOWRAP_ENABLED
#include "dynwrappers/xrender-so_wrap.h"
#else
#include <X11/extensions/Xrender.h>
#endif

#include <unistd.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(
	Display*, GLXFBConfig, GLXContext, Bool, const int*);

// To prevent shadowing warnings
#undef glXCreateContextAttribsARB

struct GLManager_X11_Private
{
	::GLXContext glx_context;
};

GLManager_X11::GLDisplay::~GLDisplay()
{
	if (context) {
		// release_current();
		glXDestroyContext(x11_display, context->glx_context);
		memdelete(context);
		context = nullptr;
	}
}

static bool ctxErrorOccurred = false;

static int ctxErrorHandler(Display* dpy, XErrorEvent* ev)
{
	ctxErrorOccurred = true;
	return 0;
}

int GLManager_X11::_find_or_create_display(Display* p_x11_display)
{
	for (unsigned int n = 0; n < _displays.size(); n++) {
		const GLDisplay& d = _displays[n];
		if (d.x11_display == p_x11_display) {
			return n;
		}
	}

	// Ensure GLAD GLX function pointers are loaded for this display
	if (!gladLoaderLoadGLX(p_x11_display, DefaultScreen(p_x11_display))) {
		ERR_FAIL_V_MSG(-1, "Failed to initialize GLAD GLX.");
	}

	// create display entry
	GLDisplay d_temp;
	d_temp.x11_display = p_x11_display;
	_displays.push_back(d_temp);
	int new_display_id = _displays.size() - 1;
	GLDisplay& d = _displays[new_display_id];

	// Select valid FBConfig
	static const int visual_attribs[] = {GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True, None};

	int fbcount = 0;
	GLXFBConfig* fbc =
		glXChooseFBConfig(d.x11_display, DefaultScreen(d.x11_display), visual_attribs, &fbcount);
	if (!fbc || fbcount == 0) {
		static const int fallback_attribs[] = {GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE,
			GLX_WINDOW_BIT, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, True, None};
		fbc = glXChooseFBConfig(
			d.x11_display, DefaultScreen(d.x11_display), fallback_attribs, &fbcount);
	}

	ERR_FAIL_COND_V_MSG(!fbc || fbcount == 0, -1, "No matching GLX FBConfig found.");

	d.x_fbconfig = (void*)fbc[0];

	XVisualInfo* vi = glXGetVisualFromFBConfig(d.x11_display, fbc[0]);
	if (vi) {
		d.x_vi = *vi;
		XFree(vi);
	}
	XFree(fbc);

	// create context
	d.context = memnew(GLManager_X11_Private);
	d.context->glx_context = nullptr;

	Error err = _create_context(d);

	if (err != OK) {
		_displays.remove_at(new_display_id);
		return -1;
	}

	return new_display_id;
}

Error GLManager_X11::_create_context(GLDisplay& gl_display)
{
	ERR_FAIL_NULL_V(gl_display.x_fbconfig, ERR_INVALID_PARAMETER);

	if (!gl_display.context) {
		gl_display.context = memnew(GLManager_X11_Private);
	}
	gl_display.context->glx_context = nullptr;

	GLXFBConfig fb_config = (GLXFBConfig)gl_display.x_fbconfig;

	// Trap X11 errors to prevent default handler exit(1) on unsupported context flags
	ctxErrorOccurred = false;
	int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(ctxErrorHandler);

	// Try OpenGL 3.3 Core Profile first via ARB extension if supported
	if (glad_glXCreateContextAttribsARB) {
		int attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 3,
			GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None};

		gl_display.context->glx_context = glad_glXCreateContextAttribsARB(
			gl_display.x11_display, fb_config, nullptr, True, attribs);

		XSync(gl_display.x11_display, False);

		if (ctxErrorOccurred) {
			gl_display.context->glx_context = nullptr;
			ctxErrorOccurred = false;
		}
	}

	// Fall back to standard GLX 1.3 context creation if core profile creation fails
	if (!gl_display.context->glx_context) {
		ctxErrorOccurred = false;
		gl_display.context->glx_context =
			glXCreateNewContext(gl_display.x11_display, fb_config, GLX_RGBA_TYPE, nullptr, True);

		XSync(gl_display.x11_display, False);

		if (ctxErrorOccurred) {
			gl_display.context->glx_context = nullptr;
		}
	}

	XSetErrorHandler(oldHandler);

	ERR_FAIL_COND_V_MSG(
		!gl_display.context->glx_context, ERR_CANT_CREATE, "Could not create GLX context.");

	return OK;
}

XVisualInfo GLManager_X11::get_vi(Display* p_display, Error& r_error)
{
	int display_id = _find_or_create_display(p_display);
	if (display_id < 0) {
		r_error = FAILED;
		return XVisualInfo();
	}
	r_error = OK;
	return _displays[display_id].x_vi;
}

Error GLManager_X11::open_display(Display* p_display)
{
	int gldisplay_id = _find_or_create_display(p_display);
	if (gldisplay_id < 0) {
		return ERR_CANT_CREATE;
	}
	else {
		return OK;
	}
}

Error GLManager_X11::window_create(DisplayServerEnums::WindowID p_window_id, ::Window p_window,
	Display* p_display, int p_width, int p_height)
{
	// make sure vector is big enough...
	// we can mirror the external vector, it is simpler
	// to keep the IDs identical for fast lookup
	if (p_window_id >= (int)_windows.size()) {
		_windows.resize(p_window_id + 1);
	}

	GLWindow& win = _windows[p_window_id];
	win.in_use = true;
	win.window_id = p_window_id;
	win.width = p_width;
	win.height = p_height;
	win.x11_window = p_window;
	win.gldisplay_id = _find_or_create_display(p_display);

	if (win.gldisplay_id == -1) {
		return FAILED;
	}

	// the display could be invalid .. check NYI
	GLDisplay& gl_display = _displays[win.gldisplay_id];
	::Display* x11_display = gl_display.x11_display;
	::Window& x11_window = win.x11_window;

	if (!glXMakeCurrent(x11_display, x11_window, gl_display.context->glx_context)) {
		ERR_PRINT("glXMakeCurrent failed");
	}

	_internal_set_current_window(&win);

	return OK;
}

void GLManager_X11::_internal_set_current_window(GLWindow* p_win)
{
	_current_window = p_win;

	// quick access to x info
	_x_windisp.x11_window = _current_window->x11_window;
	const GLDisplay& disp = get_current_display();
	_x_windisp.x11_display = disp.x11_display;
}

void GLManager_X11::window_resize(
	DisplayServerEnums::WindowID p_window_id, int p_width, int p_height)
{
	get_window(p_window_id).width = p_width;
	get_window(p_window_id).height = p_height;
}

void GLManager_X11::window_destroy(DisplayServerEnums::WindowID p_window_id)
{
	GLWindow& win = get_window(p_window_id);
	win.in_use = false;

	if (_current_window == &win) {
		_current_window = nullptr;
		_x_windisp.x11_display = nullptr;
		_x_windisp.x11_window = -1;
	}
}

void GLManager_X11::release_current()
{
	if (!_current_window) {
		return;
	}

	if (!glXMakeCurrent(_x_windisp.x11_display, None, nullptr)) {
		ERR_PRINT("glXMakeCurrent failed");
	}
	_current_window = nullptr;
}

void GLManager_X11::window_make_current(DisplayServerEnums::WindowID p_window_id)
{
	if (p_window_id == -1) {
		return;
	}

	GLWindow& win = _windows[p_window_id];
	if (!win.in_use) {
		return;
	}

	// noop
	if (&win == _current_window) {
		return;
	}

	const GLDisplay& disp = get_display(win.gldisplay_id);

	if (!glXMakeCurrent(disp.x11_display, win.x11_window, disp.context->glx_context)) {
		ERR_PRINT("glXMakeCurrent failed");
	}

	_internal_set_current_window(&win);
}

void GLManager_X11::swap_buffers()
{
	if (!_current_window) {
		return;
	}
	if (!_current_window->in_use) {
		WARN_PRINT("current window not in use!");
		return;
	}

	// Drain and inspect events right here before letting GLX block in poll()
	while (XPending(_x_windisp.x11_display)) {
		XEvent ev;
		XNextEvent(_x_windisp.x11_display, &ev);

		if (ev.type == ClientMessage) {
			Atom wm_protocols = XInternAtom(_x_windisp.x11_display, "WM_PROTOCOLS", True);
			Atom wm_delete = XInternAtom(_x_windisp.x11_display, "WM_DELETE_WINDOW", True);
			if (ev.xclient.message_type == wm_protocols &&
				(Atom)ev.xclient.data.l[0] == wm_delete) {
				fprintf(stderr, "[GLX SWAP] Intercepted WM_DELETE_WINDOW before swap! Exiting.\n");
				fflush(stderr);
				extern bool s_quit_requested;
				s_quit_requested = true;
				if (OS::is_initialized() && MainLoop::is_active()) {
					MainLoop::finalize();
				}
				return; // Do NOT call glXSwapBuffers!
			}
		}
		else if (ev.type == DestroyNotify || ev.type == UnmapNotify) {
			fprintf(stderr, "[GLX SWAP] Window unmapped or destroyed (type=%d). Exiting.\n", ev.type);
			fflush(stderr);
			extern bool s_quit_requested;
			s_quit_requested = true;
			if (OS::is_initialized() && MainLoop::is_active()) {
				MainLoop::finalize();
			}
			return; // Do NOT call glXSwapBuffers!
		}
	}

	// On X11, when enabled, transparency is always active, so clear alpha manually.
	if (OS::is_layered_allowed()) {
		if (!DisplayServer::window_get_flag(
				DisplayServerEnums::WINDOW_FLAG_TRANSPARENT, _current_window->window_id)) {
			glColorMask(false, false, false, true);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			glColorMask(true, true, true, true);
		}
	}

	static uint64_t swap_count = 0;
	swap_count++;

	int pending_gl = XPending(_x_windisp.x11_display);

	// Do the swap
	glXSwapBuffers(_x_windisp.x11_display, _x_windisp.x11_window);
}

Error GLManager_X11::initialize(Display* p_display)
{
	if (!gladLoaderLoadGLX(p_display, XScreenNumberOfScreen(XDefaultScreenOfDisplay(p_display)))) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

void GLManager_X11::set_use_vsync(bool p_use)
{
	// we need an active window to get a display to set the vsync
	if (!_current_window) {
		return;
	}
	const GLDisplay& disp = get_current_display();

	int val = p_use ? 1 : 0;
	if (GLAD_GLX_MESA_swap_control)
{
		glXSwapIntervalMESA(val);
	}
	else if (GLAD_GLX_SGI_swap_control) {
		glXSwapIntervalSGI(val);
	}
	else if (GLAD_GLX_EXT_swap_control) {
		GLXDrawable drawable = glXGetCurrentDrawable();
		glXSwapIntervalEXT(disp.x11_display, drawable, val);
	}
	else {
		WARN_PRINT_ONCE("Could not set V-Sync mode, as changing V-Sync mode is not supported by "
						"the graphics driver.");
		return;
	}
	use_vsync = p_use;
}

bool GLManager_X11::is_using_vsync() const { return use_vsync; }

void* GLManager_X11::get_glx_context(DisplayServerEnums::WindowID p_window_id)
{
	if (p_window_id == -1) {
		return nullptr;
	}

	const GLWindow& win = _windows[p_window_id];
	const GLDisplay& disp = get_display(win.gldisplay_id);

	return (void*)disp.context->glx_context;
}

VisualID GLManager_X11::get_glx_visualid(DisplayServerEnums::WindowID p_window_id)
{
	if (p_window_id == -1) {
		return (VisualID)0;
	}

	const GLWindow& win = _windows[p_window_id];
	const GLDisplay& disp = get_display(win.gldisplay_id);

	return disp.x_vi.visualid;
}

void* GLManager_X11::get_glx_fbconfig(DisplayServerEnums::WindowID p_window_id)
{
	if (p_window_id == -1) {
		return nullptr;
	}

	const GLWindow& win = _windows[p_window_id];
	const GLDisplay& disp = get_display(win.gldisplay_id);

	return disp.x_fbconfig;
}

GLManager_X11::GLManager_X11(const Vector2i& p_size, ContextType p_context_type)
{
	context_type = p_context_type;

	double_buffer = false;
	direct_render = false;
	glx_minor = glx_major = 0;
	use_vsync = false;
	_current_window = nullptr;
}

GLManager_X11::~GLManager_X11() { release_current(); }

#endif // X11_ENABLED && GLES3_ENABLED

// ============================================================================
// Merged from platform/linuxbsd/x11/gl_manager_x11_egl.cpp
// ============================================================================

#include "platform/linuxbsd/x11/gl_manager_x11.h"

#if defined(X11_ENABLED) && defined(GLES3_ENABLED)

const char *GLManagerEGL_X11::_get_platform_extension_name() const {
	return "EGL_KHR_platform_x11";
}

EGLenum GLManagerEGL_X11::_get_platform_extension_enum() const {
	return EGL_PLATFORM_X11_KHR;
}

Vector<EGLAttrib> GLManagerEGL_X11::_get_platform_display_attributes() const {
	return Vector<EGLAttrib>();
}

EGLenum GLManagerEGL_X11::_get_platform_api_enum() const {
	return EGL_OPENGL_ES_API;
}

Vector<EGLint> GLManagerEGL_X11::_get_platform_context_attribs() const {
	Vector<EGLint> ret;
	ret.push_back(EGL_CONTEXT_CLIENT_VERSION);
	ret.push_back(3);
	ret.push_back(EGL_NONE);

	return ret;
}

#endif // X11_ENABLED && GLES3_ENABLED

// ============================================================================
// Merged from platform/linuxbsd/x11/detect_prime_x11.cpp
// ============================================================================

#if defined(X11_ENABLED) && defined(GLES3_ENABLED)

#define glXCreateContextAttribsARB glad_glXCreateContextAttribsARB

#include <platform_gl.h>
#include <thirdparty/glad/glad/glx.h>
#include "core/core_globals.h"
#include "core/string/print_string.h"
#include "core/types.h"
#include "platform/linuxbsd/x11/gl_manager_x11.h"

#ifdef SOWRAP_ENABLED
#include "x11/dynwrappers/xlib-so_wrap.h"
#else
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(
	Display*, GLXFBConfig, GLXContext, Bool, const int*);

// To prevent shadowing warnings
#undef glGetString

int silent_error_handler(Display* display, XErrorEvent* error)
{
	static char message[1024];
	XGetErrorText(display, error->error_code, message, sizeof(message));
	print_verbose(vformat("XServer error: %s"
						  "\n   Major opcode of failed request: %d"
						  "\n   Serial number of failed request: %d"
						  "\n   Current serial number in output stream: %d",
		String::utf8(message), (uint64_t)error->request_code, (uint64_t)error->minor_code,
		(uint64_t)error->serial));

	quick_exit(1);
	return 0;
}

// Runs inside a child. Exiting will not quit the engine.
void DetectPrimeX11::create_context()
{
	XSetErrorHandler(&silent_error_handler);

	Display* x11_display = XOpenDisplay(nullptr);
	Window x11_window;
	GLXContext glx_context;

	static int visual_attribs[] = {GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER, true, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
		GLX_DEPTH_SIZE, 24, None};

	if (gladLoaderLoadGLX(
			x11_display, XScreenNumberOfScreen(XDefaultScreenOfDisplay(x11_display))) == 0) {
		print_verbose("Unable to load GLX, GPU detection skipped.");
		quick_exit(1);
	}
	int fbcount;
	GLXFBConfig fbconfig = nullptr;
	XVisualInfo* vi = nullptr;

	XSetWindowAttributes swa;
	swa.event_mask = StructureNotifyMask;
	swa.border_pixel = 0;
	unsigned long valuemask = CWBorderPixel | CWColormap | CWEventMask;

	GLXFBConfig* fbc =
		glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs, &fbcount);
	if (!fbc) {
		quick_exit(1);
	}

	vi = glXGetVisualFromFBConfig(x11_display, fbc[0]);

	fbconfig = fbc[0];

	static int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB,
		3, GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, GLX_CONTEXT_FLAGS_ARB,
		GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, None};

	glx_context = glXCreateContextAttribsARB(x11_display, fbconfig, nullptr, true, context_attribs);

	swa.colormap =
		XCreateColormap(x11_display, RootWindow(x11_display, vi->screen), vi->visual, AllocNone);
	x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi->screen), 0, 0, 10, 10, 0,
		vi->depth, InputOutput, vi->visual, valuemask, &swa);

	if (!x11_window) {
		quick_exit(1);
	}

	glXMakeCurrent(x11_display, x11_window, glx_context);
	XFree(vi);
}

int DetectPrimeX11::detect_prime()
{
	pid_t p;
	int priorities[2] = {};
	String vendors[2];
	String renderers[2];

	vendors[0] = "Unknown";
	vendors[1] = "Unknown";
	renderers[0] = "Unknown";
	renderers[1] = "Unknown";

	for (int i = 0; i < 2; ++i) {
		int fdset[2];

		if (pipe(fdset) == -1) {
			print_verbose("Failed to pipe(), using default GPU");
			return 0;
		}

		// Fork so the driver initialization can crash without taking down the engine.
		p = fork();

		if (p > 0) {
			// Main thread

			int stat_loc = 0;
			char string[201];
			string[200] = '\0';

			close(fdset[1]);

			waitpid(p, &stat_loc, 0);

			if (!stat_loc) {
				// No need to do anything complicated here. Anything less than
				// PIPE_BUF will be delivered in one read() call.
				// Leave it 'Unknown' otherwise.
				if (read(fdset[0], string, sizeof(string) - 1) > 0) {
					vendors[i] = string;
					renderers[i] = string + strlen(string) + 1;
				}
			}

			close(fdset[0]);

		}
		else {
			// In child, exit() here will not quit the engine.

			// Prevent false leak reports as we will not be properly
			// cleaning up these processes, and fork() makes a copy
			// of all globals.
			CoreGlobals::leak_reporting_enabled = false;

			char string[201];

			close(fdset[0]);

			if (i) {
				setenv("DRI_PRIME", "1", 1);
			}

			create_context();

			PFNGLGETSTRINGPROC glGetString =
				(PFNGLGETSTRINGPROC)glXGetProcAddressARB((GLubyte*)"glGetString");
			if (!glGetString) {
				print_verbose("Unable to get glGetString, GPU detection skipped.");
				quick_exit(1);
			}

			const char* vendor = (const char*)glGetString(GL_VENDOR);
			const char* renderer = (const char*)glGetString(GL_RENDERER);

			unsigned int vendor_len = strlen(vendor) + 1;
			unsigned int renderer_len = strlen(renderer) + 1;

			if (vendor_len + renderer_len >= sizeof(string)) {
				renderer_len = 200 - vendor_len;
			}

			memcpy(&string, vendor, vendor_len);
			memcpy(&string[vendor_len], renderer, renderer_len);

			if (write(fdset[1], string, vendor_len + renderer_len) == -1) {
				print_verbose("Couldn't write vendor/renderer string.");
			}
			close(fdset[1]);

			// The function quick_exit() is used because exit() will call destructors on static
			// objects copied by fork(). These objects will be freed anyway when the process
			// finishes execution.
			quick_exit(0);
		}
	}

	int preferred = 0;
	int priority = 0;

	if (vendors[0] == vendors[1]) {
		print_verbose("Only one GPU found, using default.");
		return 0;
	}

	for (int i = 1; i >= 0; --i) {
		const Vendor* v = vendor_map;
		while (v->glxvendor) {
			if (v->glxvendor == vendors[i]) {
				priorities[i] = v->priority;

				if (v->priority >= priority) {
					priority = v->priority;
					preferred = i;
				}
			}
			++v;
		}
	}

	print_verbose("Found renderers:");
	for (int i = 0; i < 2; ++i) {
		print_verbose(
			"Renderer " + itos(i) + ": " + renderers[i] + " with priority: " + itos(priorities[i]));
	}

	print_verbose("Using renderer: " + renderers[preferred]);
	return preferred;
}

#endif // X11_ENABLED && GLES3_ENABLED

#ifdef GLX_CONTEXT_MAJOR_VERSION_ARB
#undef GLX_CONTEXT_MAJOR_VERSION_ARB
#endif
#ifdef GLX_CONTEXT_MINOR_VERSION_ARB
#undef GLX_CONTEXT_MINOR_VERSION_ARB
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif
