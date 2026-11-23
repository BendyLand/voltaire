/**************************************************************************/
/*  os_ios.mm                                                             */
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

#import "os_ios.h"

#import "platform/ios/os_ios.h"

#ifdef IOS_ENABLED

void OS_IOS::setup() {
	OS_AppleEmbedded::setup();
	DisplayServerIOS::register_ios_driver();
}

String OS_IOS::get_name() {
	return "iOS";
}

String OS::get_name() {
	return OS_IOS::get_name();
}

#endif // IOS_ENABLED

// === Merged from platform/ios/device_metrics.mm ===
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

#import "platform/ios/os_ios.h"

@implementation GDTDeviceMetrics

+ (NSDictionary *)dpiList {
	return @{
		@[
			@"iPad1,1", // iPad 1th Gen
			@"iPad1,2", // iPad 1th Gen (3G)
			@"iPad2,1", // iPad 2nd Gen
			@"iPad2,2", // iPad 2nd Gen (GSM)
			@"iPad2,3", // iPad 2nd Gen (CDMA)
			@"iPad2,4", // iPad 2nd Gen
		] : @132,
		@[
			@"iPhone1,1", // iPhone 1st Gen
			@"iPhone1,2", // iPhone 3G
			@"iPhone2,1", // iPhone 3GS
			@"iPad2,5", // iPad mini
			@"iPad2,6", // iPad mini (GSM+LTE)
			@"iPad2,7", // iPad mini (CDMA+LTE)
			@"iPod1,1", // iPod 1st Gen
			@"iPod2,1", // iPod 2nd Gen
			@"iPod3,1", // iPod 3rd Gen
		] : @163,
		@[
			@"iPad3,1", // iPad 3rd Gen
			@"iPad3,2", // iPad 3rd Gen (CDMA)
			@"iPad3,3", // iPad 3rd Gen (GSM)
			@"iPad3,4", // iPad 4th Gen
			@"iPad3,5", // iPad 4th Gen (GSM+LTE)
			@"iPad3,6", // iPad 4th Gen (CDMA+LTE)
			@"iPad4,1", // iPad Air (WiFi)
			@"iPad4,2", // iPad Air (GSM+CDMA)
			@"iPad4,3", // iPad Air (China)
			@"iPad4,7", // iPad mini 3 (WiFi)
			@"iPad4,8", // iPad mini 3 (GSM+CDMA)
			@"iPad4,9", // iPad Mini 3 (China)
			@"iPad6,3", // iPad Pro (9.7 inch, WiFi)
			@"iPad6,4", // iPad Pro (9.7 inch, WiFi+LTE)
			@"iPad6,7", // iPad Pro (12.9 inch, WiFi)
			@"iPad6,8", // iPad Pro (12.9 inch, WiFi+LTE)
			@"iPad6,11", // iPad 5th Gen (2017)
			@"iPad6,12", // iPad 5th Gen (2017)
			@"iPad7,1", // iPad Pro 2nd Gen (WiFi)
			@"iPad7,2", // iPad Pro 2nd Gen (WiFi+Cellular)
			@"iPad7,3", // iPad Pro 10.5-inch 2nd Gen
			@"iPad7,4", // iPad Pro 10.5-inch 2nd Gen
			@"iPad7,5", // iPad 6th Gen (WiFi)
			@"iPad7,6", // iPad 6th Gen (WiFi+Cellular)
			@"iPad7,11", // iPad 7th Gen 10.2-inch (WiFi)
			@"iPad7,12", // iPad 7th Gen 10.2-inch (WiFi+Cellular)
			@"iPad8,1", // iPad Pro 11 inch 3rd Gen (WiFi)
			@"iPad8,2", // iPad Pro 11 inch 3rd Gen (1TB, WiFi)
			@"iPad8,3", // iPad Pro 11 inch 3rd Gen (WiFi+Cellular)
			@"iPad8,4", // iPad Pro 11 inch 3rd Gen (1TB, WiFi+Cellular)
			@"iPad8,5", // iPad Pro 12.9 inch 3rd Gen (WiFi)
			@"iPad8,6", // iPad Pro 12.9 inch 3rd Gen (1TB, WiFi)
			@"iPad8,7", // iPad Pro 12.9 inch 3rd Gen (WiFi+Cellular)
			@"iPad8,8", // iPad Pro 12.9 inch 3rd Gen (1TB, WiFi+Cellular)
			@"iPad8,9", // iPad Pro 11 inch 4th Gen (WiFi)
			@"iPad8,10", // iPad Pro 11 inch 4th Gen (WiFi+Cellular)
			@"iPad8,11", // iPad Pro 12.9 inch 4th Gen (WiFi)
			@"iPad8,12", // iPad Pro 12.9 inch 4th Gen (WiFi+Cellular)
			@"iPad11,3", // iPad Air 3rd Gen (WiFi)
			@"iPad11,4", // iPad Air 3rd Gen
			@"iPad11,6", // iPad 8th Gen (WiFi)
			@"iPad11,7", // iPad 8th Gen (WiFi+Cellular)
			@"iPad12,1", // iPad 9th Gen (WiFi)
			@"iPad12,2", // iPad 9th Gen (WiFi+Cellular)
			@"iPad13,1", // iPad Air 4th Gen (WiFi)
			@"iPad13,2", // iPad Air 4th Gen (WiFi+Cellular)
			@"iPad13,4", // iPad Pro 11 inch 5th Gen
			@"iPad13,5", // iPad Pro 11 inch 5th Gen
			@"iPad13,6", // iPad Pro 11 inch 5th Gen
			@"iPad13,7", // iPad Pro 11 inch 5th Gen
			@"iPad13,8", // iPad Pro 12.9 inch 5th Gen
			@"iPad13,9", // iPad Pro 12.9 inch 5th Gen
			@"iPad13,10", // iPad Pro 12.9 inch 5th Gen
			@"iPad13,11", // iPad Pro 12.9 inch 5th Gen
			@"iPad13,16", // iPad Air 5th Gen (WiFi)
			@"iPad13,17", // iPad Air 5th Gen (WiFi+Cellular)
			@"iPad13,18", // iPad 10th Gen
			@"iPad13,19", // iPad 10th Gen
			@"iPad14,3", // iPad Pro 11 inch 6th Gen
			@"iPad14,4", // iPad Pro 11 inch 6th Gen
			@"iPad14,5", // iPad Pro 12.9 inch 6th Gen
			@"iPad14,6", // iPad Pro 12.9 inch 6th Gen
			@"iPad16,3", // iPad Pro 11-inch M4
			@"iPad16,4", // iPad Pro 11-inch M4
			@"iPad16,5", // iPad Pro 13-inch M4
			@"iPad16,6", // iPad Pro 13-inch M4
			@"iPad15,3", // iPad Air M3
			@"iPad15,4", // iPad Air M3
			@"iPad15,5", // iPad Air M3
			@"iPad15,6", // iPad Air M3
			@"iPad15,7", // iPad 11th Gen
			@"iPad15,8", // iPad 11th Gen
			@"iPad17,1", // iPad Pro 11-inch M5
			@"iPad17,2", // iPad Pro 11-inch M5
			@"iPad17,3", // iPad Pro 13-inch M5
			@"iPad17,4", // iPad Pro 13-inch M5
		] : @264,
		@[
			@"iPhone3,1", // iPhone 4
			@"iPhone3,2", // iPhone 4 (GSM)
			@"iPhone3,3", // iPhone 4 (CDMA)
			@"iPhone4,1", // iPhone 4S
			@"iPhone5,1", // iPhone 5 (GSM)
			@"iPhone5,2", // iPhone 5 (GSM+CDMA)
			@"iPhone5,3", // iPhone 5C (GSM)
			@"iPhone5,4", // iPhone 5C (Global)
			@"iPhone6,1", // iPhone 5S (GSM)
			@"iPhone6,2", // iPhone 5S (Global)
			@"iPhone7,2", // iPhone 6
			@"iPhone8,1", // iPhone 6s
			@"iPhone8,4", // iPhone SE (GSM)
			@"iPhone9,1", // iPhone 7
			@"iPhone9,3", // iPhone 7
			@"iPhone10,1", // iPhone 8
			@"iPhone10,4", // iPhone 8
			@"iPhone11,8", // iPhone XR
			@"iPhone12,1", // iPhone 11
			@"iPhone12,8", // iPhone SE 2nd gen
			@"iPhone14,6", // iPhone SE 3rd gen
			@"iPad4,4", // iPad mini Retina (WiFi)
			@"iPad4,5", // iPad mini Retina (GSM+CDMA)
			@"iPad4,6", // iPad mini Retina (China)
			@"iPad5,1", // iPad mini 4th Gen (WiFi)
			@"iPad5,2", // iPad mini 4th Gen
			@"iPad5,3", // iPad Air 2 (WiFi)
			@"iPad5,4", // iPad Air 2
			@"iPad11,1", // iPad mini 5th Gen (WiFi)
			@"iPad11,2", // iPad mini 5th Gen
			@"iPad14,1", // iPad mini 6th Gen (WiFi)
			@"iPad14,2", // iPad mini 6th Gen
			@"iPod4,1", // iPod 4th Gen
			@"iPod5,1", // iPod 5th Gen
			@"iPod7,1", // iPod 6th Gen
			@"iPod9,1", // iPod 7th Gen
			@"iPad14,8", // iPad Air 11-inch M2
			@"iPad14,9", // iPad Air 11-inch M2
			@"iPad14,10", // iPad Air 13-inch M2
			@"iPad14,11", // iPad Air 13-inch M2
			@"iPad16,1", // iPad mini A17 Pro
			@"iPad16,2", // iPad mini A17 Pro
		] : @326,
		@[
			@"iPhone7,1", // iPhone 6 Plus
			@"iPhone8,2", // iPhone 6s Plus
			@"iPhone9,2", // iPhone 7 Plus
			@"iPhone9,4", // iPhone 7 Plus
			@"iPhone10,2", // iPhone 8 Plus
			@"iPhone10,5", // iPhone 8 Plus
		] : @401,
		@[
			@"iPhone10,3", // iPhone X Global
			@"iPhone10,6", // iPhone X GSM
			@"iPhone11,2", // iPhone XS
			@"iPhone11,4", // iPhone XS Max
			@"iPhone11,6", // iPhone XS Max Global
			@"iPhone12,3", // iPhone 11 Pro
			@"iPhone12,5", // iPhone 11 Pro Max
			@"iPhone13,4", // iPhone 12 Pro Max
			@"iPhone14,3", // iPhone 13 Pro Max
			@"iPhone14,8", // iPhone 14 Plus
		] : @458,
		@[
			@"iPhone13,2", // iPhone 12
			@"iPhone13,3", // iPhone 12 Pro
			@"iPhone14,2", // iPhone 13 Pro
			@"iPhone14,5", // iPhone 13
			@"iPhone14,7", // iPhone 14
			@"iPhone15,2", // iPhone 14 Pro
			@"iPhone15,3", // iPhone 14 Pro Max
			@"iPhone15,4", // iPhone 15
			@"iPhone15,5", // iPhone 15 Plus
			@"iPhone16,1", // iPhone 15 Pro
			@"iPhone16,2", // iPhone 15 Pro Max
			@"iPhone17,1", // iPhone 16 Pro
			@"iPhone17,2", // iPhone 16 Pro Max
			@"iPhone17,3", // iPhone 16
			@"iPhone17,4", // iPhone 16 Plus
			@"iPhone17,5", // iPhone 16e
			@"iPhone18,1", // iPhone 17 Pro
			@"iPhone18,2", // iPhone 17 Pro Max
			@"iPhone18,3", // iPhone 17
			@"iPhone18,4", // iPhone Air
			@"iPhone18,5", // iPhone 17e
		] : @460,
		@[
			@"iPhone13,1", // iPhone 12 Mini
			@"iPhone14,4", // iPhone 13 Mini
		] : @476
	};
}

@end

// === Merged from platform/ios/display_layer_ios.mm ===
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

#import "platform/ios/os_ios.h"


#include "core/config/project_settings.h"
#include "core/os/keyboard.h"
#include "main/main.h"
#include "servers/audio/audio_server.h"

#import <AudioToolbox/AudioServices.h>
#import <GameController/GameController.h>
#if defined(GLES3_ENABLED)
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#endif
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

@implementation GDTMetalLayer

- (void)initializeDisplayLayer {
#if defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR
	if (@available(iOS 13, *)) {
		// Simulator supports Metal since iOS 13
	} else {
		NSLog(@"iOS Simulator prior to iOS 13 does not support Metal rendering.");
	}
#endif
}

- (void)layoutDisplayLayer {
}

- (void)startRenderDisplayLayer {
}

- (void)stopRenderDisplayLayer {
}

@end

#if defined(GLES3_ENABLED)
@implementation GDTOpenGLLayer {
	// The pixel dimensions of the backbuffer
	GLint backingWidth;
	GLint backingHeight;

	EAGLContext *context;
	GLuint viewRenderbuffer, viewFramebuffer;
	GLuint depthRenderbuffer;
}

- (void)initializeDisplayLayer {
	// Configure it so that it is opaque, does not retain the contents of the backbuffer when displayed, and uses RGBA8888 color.
	self.opaque = YES;
	self.drawableProperties = [NSDictionary
			dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:FALSE],
			kEAGLDrawablePropertyRetainedBacking,
			kEAGLColorFormatRGBA8,
			kEAGLDrawablePropertyColorFormat,
			nil];

	// Create GL ES 3 context
	if (GLOBAL_GET("rendering/renderer/rendering_method") == "gl_compatibility") {
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
		NSLog(@"Setting up an OpenGL ES 3.0 context.");
		if (!context) {
			NSLog(@"Failed to create OpenGL ES 3.0 context!");
			return;
		}
	}

	if (![EAGLContext setCurrentContext:context]) {
		NSLog(@"Failed to set EAGLContext!");
		return;
	}
	if (![self createFramebuffer]) {
		NSLog(@"Failed to create frame buffer!");
		return;
	}
}

- (void)layoutDisplayLayer {
	[EAGLContext setCurrentContext:context];
	[self destroyFramebuffer];
	[self createFramebuffer];
}

- (void)startRenderDisplayLayer {
	[EAGLContext setCurrentContext:context];

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
}

- (void)stopRenderDisplayLayer {
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];

#ifdef DEBUG_ENABLED
	GLenum err = glGetError();
	if (err) {
		NSLog(@"DrawView: %x error", err);
	}
#endif
}

- (void)dealloc {
	if ([EAGLContext currentContext] == context) {
		[EAGLContext setCurrentContext:nil];
	}

	if (context) {
		context = nil;
	}
}

- (BOOL)createFramebuffer {
	glGenFramebuffersOES(1, &viewFramebuffer);
	glGenRenderbuffersOES(1, &viewRenderbuffer);

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	// This call associates the storage for the current render buffer with the EAGLDrawable (our CAself)
	// allowing us to draw into a buffer that will later be rendered to screen wherever the layer is (which corresponds with our view).
	[context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(id<EAGLDrawable>)self];
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);

	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);

	// For this sample, we also need a depth buffer, so we'll create and attach one via another renderbuffer.
	glGenRenderbuffersOES(1, &depthRenderbuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
	glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);

	if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		return NO;
	}

	GLES3::TextureStorage::system_fbo = viewFramebuffer;

	return YES;
}

// Clean up any buffers we have allocated.
- (void)destroyFramebuffer {
	GLES3::TextureStorage::system_fbo = 0;

	glDeleteFramebuffersOES(1, &viewFramebuffer);
	viewFramebuffer = 0;
	glDeleteRenderbuffersOES(1, &viewRenderbuffer);
	viewRenderbuffer = 0;

	if (depthRenderbuffer) {
		glDeleteRenderbuffersOES(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}
}

@end
#endif

// === Merged from platform/ios/display_server_ios.mm ===
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

#import "platform/ios/os_ios.h"


#import <UIKit/UIKit.h>
#import <sys/utsname.h>

DisplayServerIOS::DisplayServerIOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) :
		DisplayServerAppleEmbedded(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error) {
	// See: https://github.com/godotengine/godot/pull/106814#discussion_r2854262156 for why this was set to 203 nits.
	hardware_reference_luminance_nits = 203.0f;
}

DisplayServerIOS::~DisplayServerIOS() {
}

DisplayServerDriver *DisplayServerIOS::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	return memnew(DisplayServerIOS(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error));
}

void DisplayServerIOS::register_ios_driver() {
	DisplayServer::register_create_function("iOS", create_func, get_rendering_drivers_func);
}

String DisplayServerIOS::get_name() const {
	return "iOS";
}

int DisplayServerIOS::screen_get_dpi(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 72);

	struct utsname systemInfo;
	uname(&systemInfo);

	NSString *string = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];

	NSDictionary *iOSModelToDPI = [GDTDeviceMetrics dpiList];

	for (NSArray *keyArray in iOSModelToDPI) {
		if ([keyArray containsObject:string]) {
			NSNumber *value = iOSModelToDPI[keyArray];
			return [value intValue];
		}
	}

	// If device wasn't found in dictionary
	// make a best guess from device metrics.
	CGFloat scale = [UIScreen mainScreen].scale;

	UIUserInterfaceIdiom idiom = [UIDevice currentDevice].userInterfaceIdiom;

	switch (idiom) {
		case UIUserInterfaceIdiomPad:
			return scale == 2 ? 264 : 132;
		case UIUserInterfaceIdiomPhone: {
			if (scale == 3) {
				CGFloat nativeScale = [UIScreen mainScreen].nativeScale;
				return nativeScale == 3 ? 458 : 401;
			}

			return 326;
		}
		default:
			return 72;
	}
}

float DisplayServerIOS::screen_get_refresh_rate(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, SCREEN_REFRESH_RATE_FALLBACK);

	float fps = [UIScreen mainScreen].maximumFramesPerSecond;
	if ([NSProcessInfo processInfo].lowPowerModeEnabled) {
		fps = 60;
	}
	return fps;
}

float DisplayServerIOS::screen_get_scale(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 1.0f);

	return [UIScreen mainScreen].scale;
}

bool DisplayServerIOS::_screen_hdr_is_supported() const {
	if (@available(iOS 16.0, *)) {
		return [UIScreen mainScreen].potentialEDRHeadroom > 1.0;
	}
	return false;
}

float DisplayServerIOS::_screen_potential_edr_headroom() const {
	if (@available(iOS 16.0, *)) {
		return [UIScreen mainScreen].potentialEDRHeadroom;
	}
	return 1.0f;
}

float DisplayServerIOS::_screen_current_edr_headroom() const {
	if (@available(iOS 16.0, *)) {
		return [UIScreen mainScreen].currentEDRHeadroom;
	}
	return 1.0f;
}

// === Merged from platform/ios/godot_view_ios.mm ===
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

// --- Inlined/Folded from platform/ios/godot_view_ios.h ---
#include "drivers/apple_embedded/os_apple_embedded.h"

@interface GDTViewIOS : GDTView

@end

#import "platform/ios/os_ios.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"

@interface GDTViewIOS ()

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wobjc-property-synthesis")
@property(strong, nonatomic) CALayer<GDTDisplayLayer> *renderingLayer;
GODOT_CLANG_WARNING_POP

@end

@implementation GDTViewIOS

- (CALayer<GDTDisplayLayer> *)initializeRenderingForDriver:(NSString *)driverName {
	if (self.renderingLayer) {
		return self.renderingLayer;
	}

	CALayer<GDTDisplayLayer> *layer;

	if ([driverName isEqualToString:@"vulkan"] || [driverName isEqualToString:@"metal"]) {
#if defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR
		if (@available(iOS 13, *)) {
			layer = [GDTMetalLayer layer];
		} else {
			return nil;
		}
#else
		layer = [GDTMetalLayer layer];
#endif
#if defined(GLES3_ENABLED)
	} else if ([driverName isEqualToString:@"opengl3"]) {
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in iOS 12.0.
		layer = [GDTOpenGLLayer layer];
		GODOT_CLANG_WARNING_POP
#endif
	} else {
		return nil;
	}

	layer.frame = self.bounds;
	layer.contentsScale = self.contentScaleFactor;

	[self.layer addSublayer:layer];
	self.renderingLayer = layer;

	[layer initializeDisplayLayer];

	return self.renderingLayer;
}

@end

GDTView *GDTViewCreate() {
	GDTViewIOS *view = [GDTViewIOS new];
	if (GLOBAL_GET("display/window/ios/allow_high_refresh_rate")) {
		view.preferredFrameRate = 120;
	} else {
		view.preferredFrameRate = 60;
	}
	return view;
}

// === Merged from platform/ios/main_ios.mm ===
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

#import "platform/ios/os_ios.h"

#include "core/os/os.h"
#include "core/profiling/profiling.h"
#import "drivers/apple_embedded/os_apple_embedded.h"
#include "main/main.h"

#import <UIKit/UIKit.h>

#include <cstdio>

int apple_embedded_main(int argc, char **argv) {
#if defined(VULKAN_ENABLED)
	//MoltenVK - enable full component swizzling support
	setenv("MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE", "1", 1);
#endif

	change_to_launch_dir(argv);

	OS_IOS::setup();
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
