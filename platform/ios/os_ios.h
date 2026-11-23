/**************************************************************************/
/*  os_ios.h                                                              */
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

#ifdef IOS_ENABLED

#import "drivers/apple_embedded/os_apple_embedded.h"

class OS_IOS final
{
public:
	OS_IOS() = delete;
	~OS_IOS() = delete;
	OS_IOS(const OS_IOS&) = delete;
	OS_IOS& operator=(const OS_IOS&) = delete;

	static void setup();
	static String get_name();
};

#endif // IOS_ENABLED

// --- Folded from platform/ios/display_server_ios.h ---
#include "drivers/apple_embedded/display_server_apple_embedded.h"

class DisplayServerIOS final : public DisplayServerAppleEmbedded {

	_THREAD_SAFE_CLASS_

	DisplayServerIOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	~DisplayServerIOS();

public:
	static void register_ios_driver();
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

// --- Folded from platform/ios/device_metrics.h ---
#import <Foundation/Foundation.h>

@interface GDTDeviceMetrics : NSObject

@property(nonatomic, class, readonly, strong) NSDictionary<NSArray *, NSNumber *> *dpiList;

@end

// --- Folded from platform/ios/display_layer_ios.h ---
#include "drivers/apple_embedded/apple_embedded.h"

#if defined(GLES3_ENABLED)
#import <OpenGLES/EAGLDrawable.h>
#endif
#import <QuartzCore/QuartzCore.h>

// An ugly workaround for iOS simulator
#if defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR
#if defined(__IPHONE_13_0)
API_AVAILABLE(ios(13.0))
@interface GDTMetalLayer : CAMetalLayer <GDTDisplayLayer>
#else
@interface GDTMetalLayer : CALayer <GDTDisplayLayer>
#endif
#else
@interface GDTMetalLayer : CAMetalLayer <GDTDisplayLayer>
#endif
@end

#if defined(GLES3_ENABLED)
API_DEPRECATED("OpenGLES is deprecated", ios(2.0, 12.0))
@interface GDTOpenGLLayer : CAEAGLLayer <GDTDisplayLayer>

@end
#endif
