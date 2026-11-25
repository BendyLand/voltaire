/**************************************************************************/
/*  os_visionos.mm                                                        */
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

// --- Inlined from platform/visionos/os_visionos.h ---
#ifdef VISIONOS_ENABLED

#import "servers/display/display_server.h"

class OS_VisionOS final
{
public:
	OS_VisionOS() = delete;
	~OS_VisionOS() = delete;
	OS_VisionOS(const OS_VisionOS&) = delete;
	OS_VisionOS& operator=(const OS_VisionOS&) = delete;

	static void setup();
	static String get_name();
};

#endif // VISIONOS_ENABLED

// --- Folded from platform/visionos/display_server_visionos.h ---
#include "servers/display/display_server.h"

class DisplayServerVisionOS : public DisplayServerAppleEmbedded {

	_THREAD_SAFE_CLASS_

	DisplayServerVisionOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerVisionOS();

public:
	static void register_visionos_driver();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);

	virtual String get_name() const override;

	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

protected:
	virtual bool _screen_hdr_is_supported() const override;
	virtual float _screen_potential_edr_headroom() const override;
	virtual float _screen_current_edr_headroom() const override;
};

// --- Folded from platform/visionos/display_layer_visionos.h ---
#include "servers/display/display_server.h"

@interface GDTMetalLayer : CAMetalLayer <GDTDisplayLayer>
@end
// --- End platform/visionos/os_visionos.h ---

// --- Inlined from platform/visionos/os_visionos.h ---
#ifdef VISIONOS_ENABLED

#import "servers/display/display_server.h"

class OS_VisionOS final
{
public:
	OS_VisionOS() = delete;
	~OS_VisionOS() = delete;
	OS_VisionOS(const OS_VisionOS&) = delete;
	OS_VisionOS& operator=(const OS_VisionOS&) = delete;

	static void setup();
	static String get_name();
};

#endif // VISIONOS_ENABLED

// --- Folded from platform/visionos/display_server_visionos.h ---
#include "servers/display/display_server.h"

class DisplayServerVisionOS : public DisplayServerAppleEmbedded {

	_THREAD_SAFE_CLASS_

	DisplayServerVisionOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerVisionOS();

public:
	static void register_visionos_driver();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);

	virtual String get_name() const override;

	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

protected:
	virtual bool _screen_hdr_is_supported() const override;
	virtual float _screen_potential_edr_headroom() const override;
	virtual float _screen_current_edr_headroom() const override;
};

// --- Folded from platform/visionos/display_layer_visionos.h ---
#include "servers/display/display_server.h"

@interface GDTMetalLayer : CAMetalLayer <GDTDisplayLayer>
@end
// --- End platform/visionos/os_visionos.h ---

#ifdef VISIONOS_ENABLED

void OS_VisionOS::setup() {
	OS_AppleEmbedded::setup();
	DisplayServerVisionOS::register_visionos_driver();
}

String OS_VisionOS::get_name() {
	return "visionOS";
}

String OS::get_name() {
	return OS_VisionOS::get_name();
}

#endif // VISIONOS_ENABLED

// === Merged from platform/visionos/display_layer_visionos.mm ===
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

// --- Inlined from platform/visionos/os_visionos.h ---
#ifdef VISIONOS_ENABLED

#import "servers/display/display_server.h"

class OS_VisionOS final
{
public:
	OS_VisionOS() = delete;
	~OS_VisionOS() = delete;
	OS_VisionOS(const OS_VisionOS&) = delete;
	OS_VisionOS& operator=(const OS_VisionOS&) = delete;

	static void setup();
	static String get_name();
};

#endif // VISIONOS_ENABLED

// --- Folded from platform/visionos/display_server_visionos.h ---
#include "servers/display/display_server.h"

class DisplayServerVisionOS : public DisplayServerAppleEmbedded {

	_THREAD_SAFE_CLASS_

	DisplayServerVisionOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerVisionOS();

public:
	static void register_visionos_driver();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);

	virtual String get_name() const override;

	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

protected:
	virtual bool _screen_hdr_is_supported() const override;
	virtual float _screen_potential_edr_headroom() const override;
	virtual float _screen_current_edr_headroom() const override;
};

// --- Folded from platform/visionos/display_layer_visionos.h ---
#include "servers/display/display_server.h"

@interface GDTMetalLayer : CAMetalLayer <GDTDisplayLayer>
@end
// --- End platform/visionos/os_visionos.h ---

@implementation GDTMetalLayer

- (void)initializeDisplayLayer {
}

- (void)layoutDisplayLayer {
}

- (void)startRenderDisplayLayer {
}

- (void)stopRenderDisplayLayer {
}

@end

// === Merged from platform/visionos/display_server_visionos.mm ===
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

// --- Inlined from platform/visionos/os_visionos.h ---
#ifdef VISIONOS_ENABLED

#import "servers/display/display_server.h"

class OS_VisionOS final
{
public:
	OS_VisionOS() = delete;
	~OS_VisionOS() = delete;
	OS_VisionOS(const OS_VisionOS&) = delete;
	OS_VisionOS& operator=(const OS_VisionOS&) = delete;

	static void setup();
	static String get_name();
};

#endif // VISIONOS_ENABLED

// --- Folded from platform/visionos/display_server_visionos.h ---
#include "servers/display/display_server.h"

class DisplayServerVisionOS : public DisplayServerAppleEmbedded {

	_THREAD_SAFE_CLASS_

	DisplayServerVisionOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerVisionOS();

public:
	static void register_visionos_driver();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);

	virtual String get_name() const override;

	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

protected:
	virtual bool _screen_hdr_is_supported() const override;
	virtual float _screen_potential_edr_headroom() const override;
	virtual float _screen_current_edr_headroom() const override;
};

// --- Folded from platform/visionos/display_layer_visionos.h ---
#include "servers/display/display_server.h"

@interface GDTMetalLayer : CAMetalLayer <GDTDisplayLayer>
@end
// --- End platform/visionos/os_visionos.h ---

DisplayServerVisionOS::DisplayServerVisionOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) :
		DisplayServerAppleEmbedded(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error) {
}

DisplayServerVisionOS::~DisplayServerVisionOS() {
}

DisplayServerDriver *DisplayServerVisionOS::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	return memnew(DisplayServerVisionOS(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error));
}

void DisplayServerVisionOS::register_visionos_driver() {
	DisplayServer::register_create_function("visionOS", create_func, get_rendering_drivers_func);
}

String DisplayServerVisionOS::get_name() const {
	return "visionOS";
}

int DisplayServerVisionOS::screen_get_dpi(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 72);

	// TODO(Apple): Compute this properly from SwiftUI Metric APIs
	return 72;
}

float DisplayServerVisionOS::screen_get_refresh_rate(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, SCREEN_REFRESH_RATE_FALLBACK);

	return 90;
}

float DisplayServerVisionOS::screen_get_scale(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 1.0f);

	return 1;
}

bool DisplayServerVisionOS::_screen_hdr_is_supported() const {
	return true;
}

float DisplayServerVisionOS::_screen_potential_edr_headroom() const {
	// Current Apple Vision Pro hardware supports an EDR headroom of 2.0 (two times the SDR range).
	// See https://developer.apple.com/videos/play/wwdc2023/10089?time=603
	return 2.0f;
}

float DisplayServerVisionOS::_screen_current_edr_headroom() const {
	// Current Apple Vision Pro hardware  supports an EDR headroom of 2.0 (two times the SDR range).
	// See https://developer.apple.com/videos/play/wwdc2023/10089?time=603
	return 2.0f;
}

// === Merged from platform/visionos/godot_view_visionos.mm ===
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

// --- Inlined/Folded from platform/visionos/godot_view_visionos.h ---
#include "servers/display/display_server.h"

@interface GDTViewVisionOS : GDTView

@end

// --- Inlined from platform/visionos/os_visionos.h ---
#ifdef VISIONOS_ENABLED

#import "servers/display/display_server.h"

class OS_VisionOS final
{
public:
	OS_VisionOS() = delete;
	~OS_VisionOS() = delete;
	OS_VisionOS(const OS_VisionOS&) = delete;
	OS_VisionOS& operator=(const OS_VisionOS&) = delete;

	static void setup();
	static String get_name();
};

#endif // VISIONOS_ENABLED

// --- Folded from platform/visionos/display_server_visionos.h ---
#include "servers/display/display_server.h"

class DisplayServerVisionOS : public DisplayServerAppleEmbedded {

	_THREAD_SAFE_CLASS_

	DisplayServerVisionOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerVisionOS();

public:
	static void register_visionos_driver();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);

	virtual String get_name() const override;

	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

protected:
	virtual bool _screen_hdr_is_supported() const override;
	virtual float _screen_potential_edr_headroom() const override;
	virtual float _screen_current_edr_headroom() const override;
};

// --- Folded from platform/visionos/display_layer_visionos.h ---
#include "servers/display/display_server.h"

@interface GDTMetalLayer : CAMetalLayer <GDTDisplayLayer>
@end
// --- End platform/visionos/os_visionos.h ---

#include "core/typedefs.h"

#import <GameController/GameController.h>

@interface GDTViewVisionOS ()

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wobjc-property-synthesis")
@property(strong, nonatomic) CALayer<GDTDisplayLayer> *renderingLayer;
GODOT_CLANG_WARNING_POP

@end

@implementation GDTViewVisionOS

- (void)godot_commonInit {
	[super godot_commonInit];

	// Enable GamePad handler
	GCEventInteraction *gamepadInteraction = [[GCEventInteraction alloc] init];
	gamepadInteraction.handledEventTypes = GCUIEventTypeGamepad;
	[self addInteraction:gamepadInteraction];
}

- (CALayer<GDTDisplayLayer> *)initializeRenderingForDriver:(NSString *)driverName {
	if (self.renderingLayer) {
		return self.renderingLayer;
	}

	CALayer<GDTDisplayLayer> *layer = [GDTMetalLayer layer];

	layer.frame = self.bounds;
	layer.contentsScale = self.contentScaleFactor;

	[self.layer addSublayer:layer];
	self.renderingLayer = layer;

	[layer initializeDisplayLayer];

	return self.renderingLayer;
}

@end

GDTView *GDTViewCreate() {
	GDTViewVisionOS *view = [GDTViewVisionOS new];
	view.preferredFrameRate = 90;
	return view;
}

// === Merged from platform/visionos/main_visionos.mm ===
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

// --- Inlined from platform/visionos/os_visionos.h ---
#ifdef VISIONOS_ENABLED

#import "servers/display/display_server.h"

class OS_VisionOS final
{
public:
	OS_VisionOS() = delete;
	~OS_VisionOS() = delete;
	OS_VisionOS(const OS_VisionOS&) = delete;
	OS_VisionOS& operator=(const OS_VisionOS&) = delete;

	static void setup();
	static String get_name();
};

#endif // VISIONOS_ENABLED

// --- Folded from platform/visionos/display_server_visionos.h ---
#include "servers/display/display_server.h"

class DisplayServerVisionOS : public DisplayServerAppleEmbedded {

	_THREAD_SAFE_CLASS_

	DisplayServerVisionOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerVisionOS();

public:
	static void register_visionos_driver();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);

	virtual String get_name() const override;

	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;
	virtual float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override;

protected:
	virtual bool _screen_hdr_is_supported() const override;
	virtual float _screen_potential_edr_headroom() const override;
	virtual float _screen_current_edr_headroom() const override;
};

// --- Folded from platform/visionos/display_layer_visionos.h ---
#include "servers/display/display_server.h"

@interface GDTMetalLayer : CAMetalLayer <GDTDisplayLayer>
@end
// --- End platform/visionos/os_visionos.h ---

#include "core/os/os.h"
#include "core/profiling/profiling.h"
#import "servers/display/display_server.h"
#include "servers/display/display_server.h"

#import <UIKit/UIKit.h>

#include <cstdio>

int apple_embedded_main(int argc, char **argv) {
	change_to_launch_dir(argv);

	OS_VisionOS::setup();
	OS::initialize_core();
	OS::initialize();

	// We must override main when testing is enabled
	TEST_MAIN_OVERRIDE

	char *fargv[64];
	argc = process_args(argc, argv, fargv);

	godot_init_profiler();

	Error err = Main::setup(fargv[0], argc - 1, &fargv[1], false);

	if (err != OK) {
		if (err == ERR_HELP) { // Returned by --help and --version, so success.
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}

	OS_AppleEmbedded::initialize_modules();

	return OS::get_exit_code();
}

void apple_embedded_finish() {
	Main::cleanup();
	godot_cleanup_profiler();
	OS::finalize();
	OS::finalize_core();
}
