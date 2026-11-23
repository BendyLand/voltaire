/**************************************************************************/
/*  os_apple_embedded.mm                                                  */
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

#import "os_apple_embedded.h"

#ifdef APPLE_EMBEDDED_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/profiling/profiling.h"
#import "drivers/coreaudio/audio_driver_coreaudio.h"
#import "drivers/apple_embedded/os_apple_embedded.h"
#ifdef SDL_ENABLED
#include "core/input/input.h"
#endif
#include "main/main.h"
#include "servers/xr/xr_server.h"

#import <AVFoundation/AVFAudio.h>
#import <AudioToolbox/AudioServices.h>
#import <CoreText/CoreText.h>
#import <UIKit/UIKit.h>
#import <dlfcn.h>
#include <sys/sysctl.h>

#include <iterator>

#if defined(RD_ENABLED)
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"

#import <QuartzCore/CAMetalLayer.h>

#if defined(VULKAN_ENABLED)
#include <drivers/vulkan/godot_vulkan.h>
#endif // VULKAN_ENABLED
#endif

// Initialization order between compilation units is not guaranteed,
// so we use this as a hack to ensure certain code is called before
// everything else, but after all units are initialized.
typedef void (*init_callback)();
static init_callback *apple_init_callbacks = nullptr;
static int apple_embedded_platform_init_callbacks_count = 0;
static int apple_embedded_platform_init_callbacks_capacity = 0;
HashMap<String, void *> OS_AppleEmbedded::dynamic_symbol_lookup_table;

void add_apple_embedded_platform_init_callback(init_callback cb) {
	if (apple_embedded_platform_init_callbacks_count == apple_embedded_platform_init_callbacks_capacity) {
		void *new_ptr = realloc(apple_init_callbacks, sizeof(cb) * (apple_embedded_platform_init_callbacks_capacity + 32));
		if (new_ptr) {
			apple_init_callbacks = (init_callback *)(new_ptr);
			apple_embedded_platform_init_callbacks_capacity += 32;
		} else {
			ERR_FAIL_MSG("Unable to allocate memory for extension callbacks.");
		}
	}
	apple_init_callbacks[apple_embedded_platform_init_callbacks_count++] = cb;
}

void register_dynamic_symbol(char *name, void *address) {
	OS_AppleEmbedded::dynamic_symbol_lookup_table[String(name)] = address;
}

Rect2 fit_keep_aspect_centered(const Vector2 &p_container, const Vector2 &p_rect) {
	real_t available_ratio = p_container.width / p_container.height;
	real_t fit_ratio = p_rect.width / p_rect.height;
	Rect2 result;
	if (fit_ratio < available_ratio) {
		// Fit height - we'll have horizontal gaps
		result.size.height = p_container.height;
		result.size.width = p_container.height * fit_ratio;
		result.position.y = 0;
		result.position.x = (p_container.width - result.size.width) * 0.5f;
	} else {
		// Fit width - we'll have vertical gaps
		result.size.width = p_container.width;
		result.size.height = p_container.width / fit_ratio;
		result.position.x = 0;
		result.position.y = (p_container.height - result.size.height) * 0.5f;
	}
	return result;
}

Rect2 fit_keep_aspect_covered(const Vector2 &p_container, const Vector2 &p_rect) {
	real_t available_ratio = p_container.width / p_container.height;
	real_t fit_ratio = p_rect.width / p_rect.height;
	Rect2 result;
	if (fit_ratio < available_ratio) {
		// Need to scale up to fit width, and crop height
		result.size.width = p_container.width;
		result.size.height = p_container.width / fit_ratio;
		result.position.x = 0;
		result.position.y = (p_container.height - result.size.height) * 0.5f;
	} else {
		// Need to scale up to fit height, and crop width
		result.size.width = p_container.height * fit_ratio;
		result.size.height = p_container.height;
		result.position.x = (p_container.width - result.size.width) * 0.5f;
		result.position.y = 0;
	}
	return result;
}

void OS_AppleEmbedded::setup() {
	if (!data) {
		data = memnew(Data);
	}
	for (int i = 0; i < apple_embedded_platform_init_callbacks_count; ++i) {
		apple_init_callbacks[i]();
	}
	free(apple_init_callbacks);
	apple_init_callbacks = nullptr;
	apple_embedded_platform_init_callbacks_count = 0;
	apple_embedded_platform_init_callbacks_capacity = 0;

	Vector<Logger *> loggers;
	loggers.push_back(memnew(OsLogLogger(NSBundle.mainBundle.bundleIdentifier.UTF8String)));
	OS::_set_logger(memnew(CompositeLogger(loggers)));

	AudioDriverManager::add_driver(&data->audio_driver);
}

void OS_AppleEmbedded::alert(const String &p_alert, const String &p_title) {
	const CharString utf8_alert = p_alert.utf8();
	const CharString utf8_title = p_title.utf8();
	AppleEmbedded::alert(utf8_alert.get_data(), utf8_title.get_data());
}

void OS_AppleEmbedded::initialize_core() {
	if (!data) {
		data = memnew(Data);
	}
	OS_Unix::initialize_core();
}

void OS_AppleEmbedded::initialize() {
	initialize_core();
}

void OS_AppleEmbedded::initialize_joypads() {
#ifdef SDL_ENABLED
	if (!data) {
		return;
	}
	data->joypad_sdl = memnew(JoypadSDL());
	if (data->joypad_sdl->initialize() != OK) {
		ERR_PRINT("Couldn't initialize SDL joypad input driver.");
		memdelete(data->joypad_sdl);
		data->joypad_sdl = nullptr;
	}
#endif
}

void OS_AppleEmbedded::initialize_modules() {
	if (!data) {
		data = memnew(Data);
	}
	data->apple_embedded = memnew(AppleEmbedded);
}

void OS_AppleEmbedded::deinitialize_modules() {
	if (!data) {
		return;
	}
#ifdef SDL_ENABLED
	if (data->joypad_sdl) {
		memdelete(data->joypad_sdl);
		data->joypad_sdl = nullptr;
	}
#endif
	if (data->apple_embedded) {
		memdelete(data->apple_embedded);
		data->apple_embedded = nullptr;
	}
}

void OS_AppleEmbedded::delete_main_loop() {
	MainLoop::clear();
}

bool OS_AppleEmbedded::iterate() {
	if (!MainLoop::is_active()) {
		return true;
	}

	GodotProfileFrameMark;
	GodotProfileZone("OS_AppleEmbedded::iterate");

	if (DisplayServer::is_initialized()) {
		DisplayServer::process_events();
	}

#ifdef SDL_ENABLED
	if (data && data->joypad_sdl) {
		data->joypad_sdl->process_events();
	}
#endif

	return Main::iteration();
}

void OS_AppleEmbedded::start() {
	if (Main::start() == EXIT_SUCCESS) {
		MainLoop::initialize();
	}
}

void OS_AppleEmbedded::finalize() {
	deinitialize_modules();
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

void OS_AppleEmbedded::finalize_core() {
	OS_Unix::finalize_core();
}

// MARK: Dynamic Libraries

_FORCE_INLINE_ String OS_AppleEmbedded::get_framework_executable(const String &p_path) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	// Read framework bundle to get executable name.
	NSURL *url = [NSURL fileURLWithPath:@(p_path.utf8().get_data())];
	NSBundle *bundle = [NSBundle bundleWithURL:url];
	if (bundle) {
		String exe_path = String::utf8([[bundle executablePath] UTF8String]);
		if (da->file_exists(exe_path)) {
			return exe_path;
		}
	}

	// Try default executable name (invalid framework).
	if (da->dir_exists(p_path) && da->file_exists(p_path.path_join(p_path.get_file().get_basename()))) {
		return p_path.path_join(p_path.get_file().get_basename());
	}

	// Not a framework, try loading as .dylib.
	return p_path;
}

Error OS_AppleEmbedded::open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data) {
	if (p_path.length() == 0) {
		// Static xcframework.
		p_library_handle = RTLD_SELF;

		if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
			*p_data->r_resolved_path = p_path;
		}

		return OK;
	}

	String path = get_framework_executable(p_path);

	if (!FileAccess::exists(path)) {
		// Load .dylib or framework from within the executable path.
		path = get_framework_executable(OS::get_executable_path().get_base_dir().path_join(p_path.get_file()));
	}

	if (!FileAccess::exists(path)) {
		// Load .dylib converted to framework from within the executable path.
		path = get_framework_executable(OS::get_executable_path().get_base_dir().path_join(p_path.get_file().get_basename() + ".framework"));
	}

	if (!FileAccess::exists(path)) {
		// Load .dylib from within the executable path.
		path = get_framework_executable(OS::get_executable_path().get_base_dir().path_join(p_path.get_file().get_basename() + ".dylib"));
	}

	if (!FileAccess::exists(path)) {
		// Load .dylib or framework from a standard iOS location.
		path = get_framework_executable(OS::get_executable_path().get_base_dir().path_join("Frameworks").path_join(p_path.get_file()));
	}

	if (!FileAccess::exists(path)) {
		// Load .dylib converted to framework from a standard iOS location.
		path = get_framework_executable(OS::get_executable_path().get_base_dir().path_join("Frameworks").path_join(p_path.get_file().get_basename() + ".framework"));
	}

	if (!FileAccess::exists(path)) {
		// Load .dylib from a standard iOS location.
		path = get_framework_executable(OS::get_executable_path().get_base_dir().path_join("Frameworks").path_join(p_path.get_file().get_basename() + ".dylib"));
	}

	if (!FileAccess::exists(path) && (p_path.ends_with(".a") || p_path.ends_with(".xcframework"))) {
		// Static library already linked into the binary — use RTLD_SELF.
		p_library_handle = RTLD_SELF;

		if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
			*p_data->r_resolved_path = p_path;
		}

		return OK;
	} else {
		ERR_FAIL_COND_V(!FileAccess::exists(path), ERR_FILE_NOT_FOUND);
	}
	p_library_handle = dlopen(path.utf8().get_data(), RTLD_NOW);
	ERR_FAIL_NULL_V_MSG(p_library_handle, ERR_CANT_OPEN, vformat("Can't open dynamic library: %s. Error: %s.", p_path, dlerror()));

	if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
		*p_data->r_resolved_path = path;
	}

	return OK;
}

Error OS_AppleEmbedded::close_dynamic_library(void *p_library_handle) {
	if (p_library_handle == RTLD_SELF) {
		return OK;
	}
	return OS_Unix::close_dynamic_library(p_library_handle);
}

Error OS_AppleEmbedded::get_dynamic_library_symbol_handle(void *p_library_handle, const String &p_name, void *&p_symbol_handle, bool p_optional) {
	if (p_library_handle == RTLD_SELF) {
		void **ptr = OS_AppleEmbedded::dynamic_symbol_lookup_table.getptr(p_name);
		if (ptr) {
			p_symbol_handle = *ptr;
			return OK;
		}
	}
	return OS_Unix::get_dynamic_library_symbol_handle(p_library_handle, p_name, p_symbol_handle, p_optional);
}

String OS_AppleEmbedded::get_distribution_name() {
	return OS::get_name();
}

String OS_AppleEmbedded::get_version() {
	NSOperatingSystemVersion ver = [NSProcessInfo processInfo].operatingSystemVersion;
	return vformat("%d.%d.%d", (int64_t)ver.majorVersion, (int64_t)ver.minorVersion, (int64_t)ver.patchVersion);
}

String OS_AppleEmbedded::get_model_name() {
	if (data && data->apple_embedded) {
		String model = data->apple_embedded->get_model();
		if (model != "") {
			return model;
		}
	}

	return OS_Unix::get_model_name();
}

Error OS_AppleEmbedded::shell_open(const String &p_uri) {
	NSString *urlPath = [[NSString alloc] initWithUTF8String:p_uri.utf8().get_data()];
	NSURL *url = [NSURL URLWithString:urlPath];

	if (![[UIApplication sharedApplication] canOpenURL:url]) {
		return ERR_CANT_OPEN;
	}

	print_verbose(vformat("Opening URL %s", p_uri));

	[[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];

	return OK;
}

String OS_AppleEmbedded::get_user_data_dir(const String &p_user_dir) {
	static String ret;
	if (ret.is_empty()) {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		if (paths && [paths count] >= 1) {
			ret.append_utf8([[paths firstObject] UTF8String]);
		}
	}
	return ret;
}

String OS_AppleEmbedded::get_cache_path() {
	static String ret;
	if (ret.is_empty()) {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
		if (paths && [paths count] >= 1) {
			ret.append_utf8([[paths firstObject] UTF8String]);
		}
	}
	return ret;
}

String OS_AppleEmbedded::get_temp_path() {
	static String ret;
	if (ret.is_empty()) {
		NSURL *url = [NSURL fileURLWithPath:NSTemporaryDirectory()
								isDirectory:YES];
		if (url) {
			ret = String::utf8([url.path UTF8String]);
			ret = ret.trim_prefix("file://");
		}
	}
	return ret;
}

String OS_AppleEmbedded::get_resource_dir() {
#ifdef TOOLS_ENABLED
	return OS_Unix::get_resource_dir();
#else
	if (!data || data->remote_fs_dir.is_empty()) {
		return OS_Unix::get_resource_dir();
	} else {
		return data->remote_fs_dir;
	}
#endif
}

String OS_AppleEmbedded::get_bundle_resource_dir() {
	NSString *str = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"godot_path"];
	if (!str) {
		return OS_Unix::get_bundle_resource_dir();
	} else {
		String res_path = String::utf8([str cStringUsingEncoding:NSUTF8StringEncoding]);
		if (res_path.is_relative_path()) {
			res_path = String::utf8([[[NSBundle mainBundle] bundlePath] cStringUsingEncoding:NSUTF8StringEncoding]).path_join(res_path);
		}
		return res_path;
	}
}

String OS_AppleEmbedded::get_locale() {
	NSString *preferredLanguage = [NSLocale preferredLanguages].firstObject;

	if (preferredLanguage) {
		return String::utf8([preferredLanguage UTF8String]).replace_char('-', '_');
	}

	NSString *localeIdentifier = [[NSLocale currentLocale] localeIdentifier];
	return String::utf8([localeIdentifier UTF8String]).replace_char('-', '_');
}

Vector<String> OS_AppleEmbedded::get_preferred_locales() {
	Vector<String> out;
	for (NSString *locale_code in [NSLocale preferredLanguages]) {
		out.push_back(String([locale_code UTF8String]).replace_char('-', '_'));
	}
	if (out.is_empty()) {
		NSString *localeIdentifier = [[NSLocale currentLocale] localeIdentifier];
		out.push_back(String::utf8([localeIdentifier UTF8String]).replace_char('-', '_'));
	}
	return out;
}

String OS_AppleEmbedded::get_unique_id() {
	NSString *uuid = [UIDevice currentDevice].identifierForVendor.UUIDString;
	return String::utf8([uuid UTF8String]);
}

struct _ModelInfo {
	Vector<String> model;
	String soc;
};

static const _ModelInfo _models[] = {
	{ { "iPhone1,1", "iPhone1,2", "iPod1,1" }, "Samsung S5L8900" },
	{ { "iPod2,1" }, "Samsung S5L8720" },
	{ { "iPhone2,1" }, "Samsung S5L8920" },
	{ { "iPod3,1" }, "Samsung S5L8922" },
	{ { "iPhone3,1", "iPhone3,2", "iPhone3,3", "iPad1,1", "iPad1,2", "iPod4,1", "AppleTV2,1" }, "Apple A4" },
	{ { "iPhone4,1", "iPad2,1", "iPad2,2", "iPad2,3", "iPad2,4", "iPad2,5", "iPad2,6", "iPad2,7", "iPod5,1", "AppleTV3,1", "AppleTV3,2" }, "Apple A5" },
	{ { "iPad3,1", "iPad3,2", "iPad3,3" }, "Apple A5X" },
	{ { "iPhone5,1", "iPhone5,2", "iPhone5,3", "iPhone5,4" }, "Apple A6" },
	{ { "iPad3,4", "iPad3,5", "iPad3,6" }, "Apple A6X" },
	{ { "iPhone6,1", "iPhone6,2", "iPad4,1", "iPad4,2", "iPad4,3", "iPad4,4", "iPad4,5", "iPad4,6", "iPad4,7", "iPad4,8", "iPad4,9" }, "Apple A7" },
	{ { "iPhone7,1", "iPhone7,2", "iPad5,1", "iPad5,2", "iPod7,1", "AppleTV5,3" }, "Apple A8" },
	{ { "iPad5,3", "iPad5,4" }, "Apple A8X" },
	{ { "iPhone8,1", "iPhone8,2", "iPhone8,4", "iPad6,11", "iPad6,12" }, "Apple A9" },
	{ { "iPad6,3", "iPad6,4", "iPad6,7", "iPad6,8" }, "Apple A9X" },
	{ { "iPhone9,1", "iPhone9,2", "iPhone9,3", "iPhone9,4", "iPad7,5", "iPad7,6", "iPad7,11", "iPad7,12", "iPod9,1" }, "Apple A10 Fusion" },
	{ { "iPad7,1", "iPad7,2", "iPad7,3", "iPad7,4", "AppleTV6,2" }, "Apple A10X Fusion" },
	{ { "iPhone10,1", "iPhone10,2", "iPhone10,3", "iPhone10,4", "iPhone10,5", "iPhone10,6" }, "Apple A11 Bionic" },
	{ { "iPhone11,2", "iPhone11,4", "iPhone11,6", "iPhone11,8", "iPad11,1", "iPad11,2", "iPad11,3", "iPad11,4", "iPad11,6", "iPad11,7", "AppleTV11,1" }, "Apple A12 Bionic" },
	{ { "iPad8,1", "iPad8,2", "iPad8,3", "iPad8,4", "iPad8,5", "iPad8,6", "iPad8,7", "iPad8,8" }, "Apple A12X Bionic" },
	{ { "iPad8,9", "iPad8,10", "iPad8,11", "iPad8,12" }, "Apple A12Z Bionic" },
	{ { "iPhone12,1", "iPhone12,3", "iPhone12,5", "iPhone12,8", "iPad12,1", "iPad12,2" }, "Apple A13 Bionic" },
	{ { "iPhone13,1", "iPhone13,2", "iPhone13,3", "iPhone13,4", "iPad13,1", "iPad13,2", "iPad13,18", "iPad13,19" }, "Apple A14 Bionic" },
	{ { "iPad13,4", "iPad13,5", "iPad13,6", "iPad13,7", "iPad13,8", "iPad13,9", "iPad13,10", "iPad13,11", "iPad13,16", "iPad13,17" }, "Apple M1" },
	{ { "iPhone14,2", "iPhone14,3", "iPhone14,4", "iPhone14,5", "iPhone14,6", "iPhone14,7", "iPhone14,8", "iPad14,1", "iPad14,2", "AppleTV14,1" }, "Apple A15 Bionic" },
	{ { "iPhone15,2", "iPhone15,3", "iPhone15,4", "iPhone15,5", "iPad15,7", "iPad15,8" }, "Apple A16 Bionic" },
	{ { "iPad14,3", "iPad14,4", "iPad14,5", "iPad14,6", "iPad14,8", "iPad14,9", "iPad14,10", "iPad14,11", "RealityDevice14,1" }, "Apple M2" },
	{ { "iPhone16,1", "iPhone16,2", "iPad16,1", "iPad16,2" }, "Apple A17 Pro" },
	{ { "iPad15,3", "iPad15,4", "iPad15,5", "iPad15,6" }, "Apple M3" },
	{ { "iPad16,3", "iPad16,4", "iPad16,5", "iPad16,6" }, "Apple M4" },
	{ { "iPad17,1", "iPad17,2", "iPad17,3", "iPad17,4", "RealityDevice17,1" }, "Apple M5" },
	{ { "iPhone17,3", "iPhone17,4", "iPhone17,5" }, "Apple A18" },
	{ { "iPhone17,1", "iPhone17,2" }, "Apple A18 Pro" },
	{ { "iPhone18,3", "iPhone18,5" }, "Apple A19" },
	{ { "iPhone18,1", "iPhone18,2", "iPhone18,4" }, "Apple A19 Pro" },
};

String OS_AppleEmbedded::get_processor_name() {
#if defined(IOS_SIMULATOR) || defined(VISIONOS_SIMULATOR)
	return "Simulator";
#else
	if (data && data->apple_embedded) {
		String model = data->apple_embedded->get_model();
		for (unsigned int i = 0; i < std::size(_models); i++) {
			for (const String &m : _models[i].model) {
				if (model.contains(m)) {
					return _models[i].soc;
				}
			}
		}
	}
#endif
	return OS::get_processor_name();
}

Vector<String> OS_AppleEmbedded::get_system_fonts() {
	HashSet<String> font_names;
	CFArrayRef fonts = CTFontManagerCopyAvailableFontFamilyNames();
	if (fonts) {
		for (CFIndex i = 0; i < CFArrayGetCount(fonts); i++) {
			CFStringRef cf_name = (CFStringRef)CFArrayGetValueAtIndex(fonts, i);
			if (cf_name && (CFStringGetLength(cf_name) > 0) && (CFStringCompare(cf_name, CFSTR("LastResort"), kCFCompareCaseInsensitive) != kCFCompareEqualTo) && (CFStringGetCharacterAtIndex(cf_name, 0) != '.')) {
				NSString *ns_name = (__bridge NSString *)cf_name;
				font_names.insert(String::utf8([ns_name UTF8String]));
			}
		}
		CFRelease(fonts);
	}

	Vector<String> ret;
	for (const String &E : font_names) {
		ret.push_back(E);
	}
	return ret;
}

String OS_AppleEmbedded::_get_default_fontname(const String &p_font_name) {
	String font_name = p_font_name;
	if (font_name.to_lower() == "sans-serif") {
		font_name = "Helvetica";
	} else if (font_name.to_lower() == "serif") {
		font_name = "Times";
	} else if (font_name.to_lower() == "monospace") {
		font_name = "Courier";
	} else if (font_name.to_lower() == "fantasy") {
		font_name = "Papyrus";
	} else if (font_name.to_lower() == "cursive") {
		font_name = "Apple Chancery";
	};
	return font_name;
}

CGFloat OS_AppleEmbedded::_weight_to_ct(int p_weight) {
	if (p_weight < 150) {
		return -0.80;
	} else if (p_weight < 250) {
		return -0.60;
	} else if (p_weight < 350) {
		return -0.40;
	} else if (p_weight < 450) {
		return 0.0;
	} else if (p_weight < 550) {
		return 0.23;
	} else if (p_weight < 650) {
		return 0.30;
	} else if (p_weight < 750) {
		return 0.40;
	} else if (p_weight < 850) {
		return 0.56;
	} else if (p_weight < 925) {
		return 0.62;
	} else {
		return 1.00;
	}
}

CGFloat OS_AppleEmbedded::_stretch_to_ct(int p_stretch) {
	if (p_stretch < 56) {
		return -0.5;
	} else if (p_stretch < 69) {
		return -0.37;
	} else if (p_stretch < 81) {
		return -0.25;
	} else if (p_stretch < 93) {
		return -0.13;
	} else if (p_stretch < 106) {
		return 0.0;
	} else if (p_stretch < 137) {
		return 0.13;
	} else if (p_stretch < 144) {
		return 0.25;
	} else if (p_stretch < 162) {
		return 0.37;
	} else {
		return 0.5;
	}
}

Vector<String> OS_AppleEmbedded::get_system_font_path_for_text(const String &p_font_name, const String &p_text, const String &p_locale, const String &p_script, int p_weight, int p_stretch, bool p_italic) {
	Vector<String> ret;
	String font_name = _get_default_fontname(p_font_name);

	CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, font_name.utf8().get_data(), kCFStringEncodingUTF8);
	CTFontSymbolicTraits traits = 0;
	if (p_weight >= 700) {
		traits |= kCTFontBoldTrait;
	}
	if (p_italic) {
		traits |= kCTFontItalicTrait;
	}
	if (p_stretch < 100) {
		traits |= kCTFontCondensedTrait;
	} else if (p_stretch > 100) {
		traits |= kCTFontExpandedTrait;
	}

	CFNumberRef sym_traits = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &traits);
	CFMutableDictionaryRef traits_dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, nullptr, nullptr);
	CFDictionaryAddValue(traits_dict, kCTFontSymbolicTrait, sym_traits);

	CGFloat weight = _weight_to_ct(p_weight);
	CFNumberRef font_weight = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &weight);
	CFDictionaryAddValue(traits_dict, kCTFontWeightTrait, font_weight);

	CGFloat stretch = _stretch_to_ct(p_stretch);
	CFNumberRef font_stretch = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &stretch);
	CFDictionaryAddValue(traits_dict, kCTFontWidthTrait, font_stretch);

	CFMutableDictionaryRef attributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, nullptr, nullptr);
	CFDictionaryAddValue(attributes, kCTFontFamilyNameAttribute, name);
	CFDictionaryAddValue(attributes, kCTFontTraitsAttribute, traits_dict);

	CTFontDescriptorRef font = CTFontDescriptorCreateWithAttributes(attributes);
	if (font) {
		CTFontRef family = CTFontCreateWithFontDescriptor(font, 0, nullptr);
		if (family) {
			CFStringRef string = CFStringCreateWithCString(kCFAllocatorDefault, p_text.utf8().get_data(), kCFStringEncodingUTF8);
			CFRange range = CFRangeMake(0, CFStringGetLength(string));
			CTFontRef fallback_family = CTFontCreateForString(family, string, range);
			if (fallback_family) {
				CTFontDescriptorRef fallback_font = CTFontCopyFontDescriptor(fallback_family);
				if (fallback_font) {
					CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(fallback_font, kCTFontURLAttribute);
					if (url) {
						NSString *font_path = [NSString stringWithString:[(__bridge NSURL *)url path]];
						ret.push_back(String::utf8([font_path UTF8String]));
						CFRelease(url);
					}
					CFRelease(fallback_font);
				}
				CFRelease(fallback_family);
			}
			CFRelease(string);
			CFRelease(family);
		}
		CFRelease(font);
	}

	CFRelease(attributes);
	CFRelease(traits_dict);
	CFRelease(sym_traits);
	CFRelease(font_stretch);
	CFRelease(font_weight);
	CFRelease(name);

	return ret;
}

String OS_AppleEmbedded::get_system_font_path(const String &p_font_name, int p_weight, int p_stretch, bool p_italic) {
	String ret;
	String font_name = _get_default_fontname(p_font_name);

	CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, font_name.utf8().get_data(), kCFStringEncodingUTF8);

	CTFontSymbolicTraits traits = 0;
	if (p_weight >= 700) {
		traits |= kCTFontBoldTrait;
	}
	if (p_italic) {
		traits |= kCTFontItalicTrait;
	}
	if (p_stretch < 100) {
		traits |= kCTFontCondensedTrait;
	} else if (p_stretch > 100) {
		traits |= kCTFontExpandedTrait;
	}

	CFNumberRef sym_traits = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &traits);
	CFMutableDictionaryRef traits_dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, nullptr, nullptr);
	CFDictionaryAddValue(traits_dict, kCTFontSymbolicTrait, sym_traits);

	CGFloat weight = _weight_to_ct(p_weight);
	CFNumberRef font_weight = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &weight);
	CFDictionaryAddValue(traits_dict, kCTFontWeightTrait, font_weight);

	CGFloat stretch = _stretch_to_ct(p_stretch);
	CFNumberRef font_stretch = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &stretch);
	CFDictionaryAddValue(traits_dict, kCTFontWidthTrait, font_stretch);

	CFMutableDictionaryRef attributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, nullptr, nullptr);
	CFDictionaryAddValue(attributes, kCTFontFamilyNameAttribute, name);
	CFDictionaryAddValue(attributes, kCTFontTraitsAttribute, traits_dict);

	CTFontDescriptorRef font = CTFontDescriptorCreateWithAttributes(attributes);
	if (font) {
		CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(font, kCTFontURLAttribute);
		if (url) {
			NSString *font_path = [NSString stringWithString:[(__bridge NSURL *)url path]];
			ret = String::utf8([font_path UTF8String]);
			CFRelease(url);
		}
		CFRelease(font);
	}

	CFRelease(attributes);
	CFRelease(traits_dict);
	CFRelease(sym_traits);
	CFRelease(font_stretch);
	CFRelease(font_weight);
	CFRelease(name);

	return ret;
}

void OS_AppleEmbedded::vibrate_handheld(int p_duration_ms, float p_amplitude) {
	if (data && data->apple_embedded && data->apple_embedded->supports_haptic_engine()) {
		if (p_amplitude > 0.0) {
			p_amplitude = CLAMP(p_amplitude, 0.0, 1.0);
		}

		data->apple_embedded->vibrate_haptic_engine((float)p_duration_ms / 1000.f, p_amplitude);
	} else {
		// iOS <13 does not support duration for vibration
		AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
	}
}

bool OS_AppleEmbedded::_check_internal_feature_support(const String &p_feature) {
	if (p_feature == "system_fonts") {
		return true;
	}
	if (p_feature == "mobile") {
		return true;
	}

	return false;
}

Error OS_AppleEmbedded::setup_remote_filesystem(const String &p_server_host, int p_port, const String &p_password, String &r_project_path) {
	r_project_path = OS::get_user_data_dir();
	Error err = OS_Unix::setup_remote_filesystem(p_server_host, p_port, p_password, r_project_path);
	if (err == OK && data) {
		data->remote_fs_dir = r_project_path;
	}
	return err;
}

void OS_AppleEmbedded::on_focus_out() {
	if (data && data->is_focused) {
		data->is_focused = false;

		if (DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->send_window_event(DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT);
		}

		if (MainLoop::is_active()) {
			MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_FOCUS_OUT);
		}

		[GDTAppDelegateService.viewController.godotView stopRendering];

		data->audio_driver.stop();
	}
}

void OS_AppleEmbedded::on_focus_in() {
	if (data && !data->is_focused) {
		data->is_focused = true;

		if (DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->send_window_event(DisplayServerEnums::WINDOW_EVENT_FOCUS_IN);
		}

		if (MainLoop::is_active()) {
			MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_FOCUS_IN);
		}

		[GDTAppDelegateService.viewController.godotView startRendering];

		data->audio_driver.start();
	}
}

void OS_AppleEmbedded::on_enter_background() {
	// Do not check for is_focused, because on_focus_out will always be fired first by applicationWillResignActive.

	if (CameraServer::is_initialized()) {
		CameraServer::handle_application_pause();
	}

	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_PAUSED);
	}

	on_focus_out();
}

void OS_AppleEmbedded::on_exit_background() {
	if (data && !data->is_focused) {
		on_focus_in();

		if (MainLoop::is_active()) {
			MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_RESUMED);
		}

		if (CameraServer::is_initialized()) {
			CameraServer::handle_application_resume();
		}
	}
}

Rect2 OS_AppleEmbedded::calculate_boot_screen_rect(const Size2 &p_window_size, const Size2 &p_imgrect_size) {
	String scalemodestr = GLOBAL_GET("ios/launch_screen_image_mode");

	if (scalemodestr == "scaleAspectFit") {
		return fit_keep_aspect_centered(p_window_size, p_imgrect_size);
	} else if (scalemodestr == "scaleAspectFill") {
		return fit_keep_aspect_covered(p_window_size, p_imgrect_size);
	} else if (scalemodestr == "scaleToFill") {
		return Rect2(Point2(), p_window_size);
	} else if (scalemodestr == "center") {
		return OS::calculate_boot_screen_rect(p_window_size, p_imgrect_size);
	} else {
		WARN_PRINT(vformat("Boot screen scale mode mismatch between iOS and Godot: %s not supported", scalemodestr));
		return OS::calculate_boot_screen_rect(p_window_size, p_imgrect_size);
	}
}

bool OS_AppleEmbedded::request_permission(const String &p_name) {
	if (p_name == "appleembedded.permission.AUDIO_RECORD") {
		if (@available(iOS 17.0, *)) {
			AVAudioApplicationRecordPermission permission = [AVAudioApplication sharedInstance].recordPermission;
			if (permission == AVAudioApplicationRecordPermissionGranted) {
				// Permission already granted, you can start recording.
				return true;
			} else if (permission == AVAudioApplicationRecordPermissionDenied) {
				// Permission denied, or not yet granted.
				return false;
			} else {
				// Request the permission, but for now return false as documented.
				[AVAudioApplication requestRecordPermissionWithCompletionHandler:^(BOOL granted) {
				}];
			}
		}
	}
	return false;
}

Vector<String> OS_AppleEmbedded::get_granted_permissions() {
	Vector<String> ret;

	if (@available(iOS 17.0, *)) {
		if ([AVAudioApplication sharedInstance].recordPermission == AVAudioApplicationRecordPermissionGranted) {
			ret.push_back("appleembedded.permission.AUDIO_RECORD");
		}
	}
	return ret;
}

/* -------------------------------------------------------------------------- */
/*                        OS Static Platform Dispatch                         */
/* -------------------------------------------------------------------------- */

void OS::initialize_core() {
	OS_AppleEmbedded::initialize_core();
	_initialized = true;
}

void OS::initialize() {
	OS_AppleEmbedded::initialize();
	_initialized = true;
}

void OS::finalize() {
	_initialized = false;
	OS_AppleEmbedded::finalize();
	if (_logger) {
		memdelete(_logger);
		_logger = nullptr;
	}
}

void OS::finalize_core() {
	OS_AppleEmbedded::finalize_core();
}

void OS::initialize_joypads() {
	OS_AppleEmbedded::initialize_joypads();
}

uint64_t OS::get_ticks_usec() {
	return OS_Unix::get_ticks_usec();
}

void OS::delay_usec(uint32_t p_usec) {
	OS_Unix::delay_usec(p_usec);
}

String OS::get_environment(const String &p_var) {
	return OS_Unix::get_environment(p_var);
}

bool OS::has_environment(const String &p_var) {
	return OS_Unix::has_environment(p_var);
}

void OS::set_environment(const String &p_var, const String &p_value) {
	OS_Unix::set_environment(p_var, p_value);
}

void OS::unset_environment(const String &p_var) {
	OS_Unix::unset_environment(p_var);
}

void OS::load_shell_environment() {
}

OS::DateTime OS::get_datetime(bool p_utc) {
	return OS_Unix::get_datetime(p_utc);
}

OS::TimeZoneInfo OS::get_time_zone_info() {
	return OS_Unix::get_time_zone_info();
}

Error OS::execute(const String &p_path, const List<String> &p_arguments, String *r_pipe, int *r_exitcode, bool p_read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	return OS_Unix::execute(p_path, p_arguments, r_pipe, r_exitcode, p_read_stderr, p_pipe_mutex, p_open_console);
}

Error OS::create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id, bool p_open_console) {
	return OS_Unix::create_process(p_path, p_arguments, r_child_id, p_open_console);
}

Error OS::kill(const ProcessID &p_pid) {
	return OS_Unix::kill(p_pid);
}

bool OS::is_process_running(const ProcessID &p_pid) {
	return OS_Unix::is_process_running(p_pid);
}

Error OS::get_entropy(uint8_t *r_buffer, int p_bytes) {
	return OS_Unix::get_entropy(r_buffer, p_bytes);
}

String OS::get_system_ca_certificates() {
	return String();
}

List<String> OS::get_cmdline_platform_args() {
	return List<String>();
}

bool OS::_check_internal_feature_support(const String &p_feature) {
	return OS_AppleEmbedded::_check_internal_feature_support(p_feature);
}

void OS::disable_crash_handler() {
}

void OS::vibrate_handheld(int p_duration_ms, float p_amplitude) {
	OS_AppleEmbedded::vibrate_handheld(p_duration_ms, p_amplitude);
}

Error OS::move_to_trash(const String &p_path) {
	return FAILED;
}

#ifdef TOOLS_ENABLED
bool OS::_test_create_rendering_device(const String &p_display_driver) {
	return false;
}

bool OS::_test_create_rendering_device_and_gl(const String &p_display_driver) {
	return false;
}
#endif

#endif // APPLE_EMBEDDED_ENABLED

// === Merged from drivers/apple_embedded/app_delegate_service.mm ===
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

#import "drivers/apple_embedded/os_apple_embedded.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"
#import "drivers/apple_embedded/os_apple_embedded.h"
#import "drivers/coreaudio/audio_driver_coreaudio.h"
#include "main/main.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioServices.h>

int gargc;
char **gargv;

extern int apple_embedded_main(int, char **);
extern void apple_embedded_finish();

@implementation GDTAppDelegateService

enum {
	SESSION_CATEGORY_AMBIENT,
	SESSION_CATEGORY_MULTI_ROUTE,
	SESSION_CATEGORY_PLAY_AND_RECORD,
	SESSION_CATEGORY_PLAYBACK,
	SESSION_CATEGORY_RECORD,
	SESSION_CATEGORY_SOLO_AMBIENT
};

static GDTViewController *mainViewController = nil;

+ (GDTViewController *)viewController {
	return mainViewController;
}

+ (void)setViewController:(GDTViewController *)viewController {
	mainViewController = viewController;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	// TODO: might be required to make an early return, so app wouldn't crash because of timeout.
	// TODO: logo screen is not displayed while shaders are compiling
	// DummyViewController(Splash/LoadingViewController) -> setup -> GodotViewController

	// Fetch the command-line arguments from NSProcessInfo
	NSArray *arguments = [[NSProcessInfo processInfo] arguments];
	gargc = (int)[arguments count];
	gargv = (char **)malloc(sizeof(char *) * gargc);
	for (int i = 0; i < gargc; i++) {
		NSString *arg = arguments[i];
		gargv[i] = strdup([arg UTF8String]);
	}

	int err = apple_embedded_main(gargc, gargv);

	if (err != 0) {
		// bail, things did not go very well for us, should probably output a message on screen with our error code...
		exit(0);
		return NO;
	}

	[[NSNotificationCenter defaultCenter]
			addObserver:self
			   selector:@selector(onAudioInterruption:)
				   name:AVAudioSessionInterruptionNotification
				 object:[AVAudioSession sharedInstance]];

	int sessionCategorySetting = GLOBAL_GET("audio/general/ios/session_category");

	// Initialize with default Ambient category.
	AVAudioSessionCategory category = AVAudioSessionCategoryAmbient;
	AVAudioSessionCategoryOptions options = 0;

	if (GLOBAL_GET("audio/general/ios/mix_with_others")) {
		options |= AVAudioSessionCategoryOptionMixWithOthers;
	}

	if (sessionCategorySetting == SESSION_CATEGORY_MULTI_ROUTE) {
		category = AVAudioSessionCategoryMultiRoute;
	} else if (sessionCategorySetting == SESSION_CATEGORY_PLAY_AND_RECORD) {
		category = AVAudioSessionCategoryPlayAndRecord;
		options |= AVAudioSessionCategoryOptionDefaultToSpeaker;
		options |= AVAudioSessionCategoryOptionAllowBluetoothA2DP;
		options |= AVAudioSessionCategoryOptionAllowAirPlay;
	} else if (sessionCategorySetting == SESSION_CATEGORY_PLAYBACK) {
		category = AVAudioSessionCategoryPlayback;
	} else if (sessionCategorySetting == SESSION_CATEGORY_RECORD) {
		category = AVAudioSessionCategoryRecord;
	} else if (sessionCategorySetting == SESSION_CATEGORY_SOLO_AMBIENT) {
		category = AVAudioSessionCategorySoloAmbient;
	}

	[[AVAudioSession sharedInstance] setCategory:category withOptions:options error:nil];

	return YES;
}

- (void)onAudioInterruption:(NSNotification *)notification {
	if ([notification.name isEqualToString:AVAudioSessionInterruptionNotification]) {
		if ([[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] isEqualToNumber:[NSNumber numberWithInt:AVAudioSessionInterruptionTypeBegan]]) {
			NSLog(@"Audio interruption began");
			OS_AppleEmbedded::on_focus_out();
		} else if ([[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] isEqualToNumber:[NSNumber numberWithInt:AVAudioSessionInterruptionTypeEnded]]) {
			NSLog(@"Audio interruption ended");
			OS_AppleEmbedded::on_focus_in();
		}
	}
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_OS_MEMORY_WARNING);
	}
}

- (void)applicationWillTerminate:(UIApplication *)application {
	apple_embedded_finish();
}

// When application goes to background (e.g. user switches to another app or presses Home),
// then applicationWillResignActive -> applicationDidEnterBackground are called.
// When user opens the inactive app again,
// applicationWillEnterForeground -> applicationDidBecomeActive are called.

// There are cases when applicationWillResignActive -> applicationDidBecomeActive
// sequence is called without the app going to background. For example, that happens
// if you open the app list without switching to another app or open/close the
// notification panel by swiping from the upper part of the screen.

- (void)sceneDidDisconnect:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::on_focus_out();
}

- (void)sceneWillResignActive:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::on_focus_out();
}

- (void)sceneDidBecomeActive:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::on_focus_in();
}

- (void)sceneDidEnterBackground:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::on_enter_background();
}

- (void)sceneWillEnterForeground:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	OS_AppleEmbedded::on_exit_background();
}

- (void)applicationWillResignActive:(UIApplication *)application {
	OS_AppleEmbedded::on_focus_out();
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	OS_AppleEmbedded::on_focus_in();
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
	OS_AppleEmbedded::on_enter_background();
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	OS_AppleEmbedded::on_exit_background();
}

- (void)dealloc {
	self.window = nil;
}

@end

// === Merged from drivers/apple_embedded/apple_embedded.mm ===
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

#import "drivers/apple_embedded/os_apple_embedded.h"

#include "core/object/class_db.h"
#import "drivers/apple_embedded/os_apple_embedded.h"

#import <CoreHaptics/CoreHaptics.h>
#import <UIKit/UIKit.h>
#include <sys/sysctl.h>

void AppleEmbedded::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_rate_url", "app_id"), &AppleEmbedded::get_rate_url);
	ClassDB::bind_method(D_METHOD("supports_haptic_engine"), &AppleEmbedded::supports_haptic_engine);
	ClassDB::bind_method(D_METHOD("start_haptic_engine"), &AppleEmbedded::start_haptic_engine);
	ClassDB::bind_method(D_METHOD("stop_haptic_engine"), &AppleEmbedded::stop_haptic_engine);
}

bool AppleEmbedded::supports_haptic_engine() {
	if (@available(iOS 13, *)) {
		id<CHHapticDeviceCapability> capabilities = [CHHapticEngine capabilitiesForHardware];
		return capabilities.supportsHaptics;
	}

	return false;
}

CHHapticEngine *AppleEmbedded::get_haptic_engine_instance() API_AVAILABLE(ios(13)) {
	if (haptic_engine == nullptr) {
		NSError *error = nullptr;
		haptic_engine = [[CHHapticEngine alloc] initAndReturnError:&error];

		if (!error) {
			[haptic_engine setAutoShutdownEnabled:true];
		} else {
			haptic_engine = nullptr;
			NSLog(@"Could not initialize haptic engine: %@", error);
		}
	}

	return haptic_engine;
}

void AppleEmbedded::vibrate_haptic_engine(float p_duration_seconds, float p_amplitude) API_AVAILABLE(ios(13)) {
	if (@available(iOS 13, *)) { // We need the @available check every time to make the compiler happy...
		if (supports_haptic_engine()) {
			CHHapticEngine *cur_haptic_engine = get_haptic_engine_instance();
			if (cur_haptic_engine) {
				NSDictionary *hapticDict;
				if (p_amplitude < 0) {
					hapticDict = @{
						CHHapticPatternKeyPattern : @[
							@{CHHapticPatternKeyEvent : @{
								CHHapticPatternKeyEventType : CHHapticEventTypeHapticContinuous,
								CHHapticPatternKeyTime : @(CHHapticTimeImmediate),
								CHHapticPatternKeyEventDuration : @(p_duration_seconds),
							},
							},
						],
					};
				} else {
					hapticDict = @{
						CHHapticPatternKeyPattern : @[
							@{CHHapticPatternKeyEvent : @{
								CHHapticPatternKeyEventType : CHHapticEventTypeHapticContinuous,
								CHHapticPatternKeyTime : @(CHHapticTimeImmediate),
								CHHapticPatternKeyEventDuration : @(p_duration_seconds),
								CHHapticPatternKeyEventParameters : @[
									@{
										CHHapticPatternKeyParameterID : @("HapticIntensity"),
										CHHapticPatternKeyParameterValue : @(p_amplitude)
									},
								],
							},
							},
						],
					};
				}

				NSError *error;
				CHHapticPattern *pattern = [[CHHapticPattern alloc] initWithDictionary:hapticDict error:&error];

				[[cur_haptic_engine createPlayerWithPattern:pattern error:&error] startAtTime:0 error:&error];

				NSLog(@"Could not vibrate using haptic engine: %@", error);
			}

			return;
		}
	}

	NSLog(@"Haptic engine is not supported");
}

void AppleEmbedded::start_haptic_engine() {
	if (@available(iOS 13, *)) {
		if (supports_haptic_engine()) {
			CHHapticEngine *cur_haptic_engine = get_haptic_engine_instance();
			if (cur_haptic_engine) {
				[cur_haptic_engine startWithCompletionHandler:^(NSError *returnedError) {
					if (returnedError) {
						NSLog(@"Could not start haptic engine: %@", returnedError);
					}
				}];
			}

			return;
		}
	}

	NSLog(@"Haptic engine is not supported");
}

void AppleEmbedded::stop_haptic_engine() {
	if (@available(iOS 13, *)) {
		if (supports_haptic_engine()) {
			CHHapticEngine *cur_haptic_engine = get_haptic_engine_instance();
			if (cur_haptic_engine) {
				[cur_haptic_engine stopWithCompletionHandler:^(NSError *returnedError) {
					if (returnedError) {
						NSLog(@"Could not stop haptic engine: %@", returnedError);
					}
				}];
			}

			return;
		}
	}

	NSLog(@"Haptic engine is not supported");
}

void AppleEmbedded::alert(const char *p_alert, const char *p_title) {
	NSString *title = [NSString stringWithUTF8String:p_title];
	NSString *message = [NSString stringWithUTF8String:p_alert];

	UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
	UIAlertAction *button = [UIAlertAction actionWithTitle:@"OK"
													 style:UIAlertActionStyleCancel
												   handler:^(id){
												   }];

	[alert addAction:button];

	[GDTAppDelegateService.viewController presentViewController:alert animated:YES completion:nil];
}

String AppleEmbedded::get_model() const {
	// [[UIDevice currentDevice] model] only returns "iPad" or "iPhone".
	size_t size;
	sysctlbyname("hw.machine", nullptr, &size, nullptr, 0);
	char *model = (char *)malloc(size);
	if (model == nullptr) {
		return "";
	}
	sysctlbyname("hw.machine", model, &size, nullptr, 0);
	NSString *platform = [NSString stringWithCString:model encoding:NSUTF8StringEncoding];
	free(model);
	const char *str = [platform UTF8String];
	return String::utf8(str != nullptr ? str : "");
}

String AppleEmbedded::get_rate_url(int p_app_id) const {
	String app_url_path = "itms-apps://itunes.apple.com/app/idAPP_ID";

	String ret = app_url_path.replace("APP_ID", String::num_int64(p_app_id));

	print_verbose(vformat("Returning rate url %s", ret));
	return ret;
}

AppleEmbedded::AppleEmbedded() {}

// === Merged from drivers/apple_embedded/godot_app_delegate.mm ===
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

#import "drivers/apple_embedded/os_apple_embedded.h"

#include "core/typedefs.h"
#import "drivers/apple_embedded/os_apple_embedded.h"

@implementation GDTApplicationDelegate

static NSMutableArray<GDTAppDelegateServiceProtocol *> *services = nil;

+ (NSArray<GDTAppDelegateServiceProtocol *> *)services {
	return services;
}

+ (void)load {
	services = [NSMutableArray new];
	[services addObject:[GDTAppDelegateService new]];
}

+ (void)addService:(GDTAppDelegateServiceProtocol *)service {
	if (!services || !service) {
		return;
	}
	[services addObject:service];
}

// UIApplicationDelegate documentation can be found here: https://developer.apple.com/documentation/uikit/uiapplicationdelegate

// MARK: Window

- (UIWindow *)window {
	UIWindow *result = nil;

	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		UIWindow *value = [service window];

		if (value) {
			result = value;
		}
	}

	return result;
}

// MARK: Initializing

- (BOOL)application:(UIApplication *)application willFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions {
	BOOL result = NO;

	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		if ([service application:application willFinishLaunchingWithOptions:launchOptions]) {
			result = YES;
		}
	}

	return result;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions {
	BOOL result = NO;

	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		if ([service application:application didFinishLaunchingWithOptions:launchOptions]) {
			result = YES;
		}
	}

	return result;
}

// MARK: Scene

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	UISceneConfiguration *config = [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
	config.delegateClass = [GDTApplicationDelegate class];
	return config;
}

- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
}

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service scene:scene willConnectToSession:session options:connectionOptions];
	}
}

- (void)scene:(UIScene *)scene openURLContexts:(NSSet<UIOpenURLContext *> *)URLContexts API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service scene:scene openURLContexts:URLContexts];
	}
}

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service scene:scene continueUserActivity:userActivity];
	}
}

// MARK: Life-Cycle

- (void)sceneDidDisconnect:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service sceneDidDisconnect:scene];
	}
}

- (void)sceneDidBecomeActive:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service sceneDidBecomeActive:scene];
	}
}

- (void)sceneWillResignActive:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service sceneWillResignActive:scene];
	}
}

- (void)sceneDidEnterBackground:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service sceneDidEnterBackground:scene];
	}
}

- (void)sceneWillEnterForeground:(UIScene *)scene API_AVAILABLE(ios(13.0), tvos(13.0), visionos(1.0)) {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service sceneWillEnterForeground:scene];
	}
}

// UIApplication lifecycle has become deprecated in favor of UIScene lifecycle
GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations")

- (void)applicationDidBecomeActive:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationDidBecomeActive:application];
	}
}

- (void)applicationWillResignActive:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationWillResignActive:application];
	}
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationDidEnterBackground:application];
	}
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationWillEnterForeground:application];
	}
}

- (void)applicationWillTerminate:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationWillTerminate:application];
	}
}

// MARK: Environment Changes

- (void)applicationProtectedDataDidBecomeAvailable:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationProtectedDataDidBecomeAvailable:application];
	}
}

- (void)applicationProtectedDataWillBecomeUnavailable:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationProtectedDataWillBecomeUnavailable:application];
	}
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationDidReceiveMemoryWarning:application];
	}
}

- (void)applicationSignificantTimeChange:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationSignificantTimeChange:application];
	}
}

// MARK: App State Restoration

- (BOOL)application:(UIApplication *)application shouldSaveSecureApplicationState:(NSCoder *)coder API_AVAILABLE(ios(13.2)) {
	BOOL result = NO;

	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		if ([service application:application shouldSaveSecureApplicationState:coder]) {
			result = YES;
		}
	}

	return result;
}

- (BOOL)application:(UIApplication *)application shouldRestoreSecureApplicationState:(NSCoder *)coder API_AVAILABLE(ios(13.2)) {
	BOOL result = NO;

	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		if ([service application:application shouldRestoreSecureApplicationState:coder]) {
			result = YES;
		}
	}

	return result;
}

- (UIViewController *)application:(UIApplication *)application viewControllerWithRestorationIdentifierPath:(NSArray<NSString *> *)identifierComponents coder:(NSCoder *)coder {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		UIViewController *controller = [service application:application viewControllerWithRestorationIdentifierPath:identifierComponents coder:coder];

		if (controller) {
			return controller;
		}
	}

	return nil;
}

- (void)application:(UIApplication *)application willEncodeRestorableStateWithCoder:(NSCoder *)coder {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service application:application willEncodeRestorableStateWithCoder:coder];
	}
}

- (void)application:(UIApplication *)application didDecodeRestorableStateWithCoder:(NSCoder *)coder {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service application:application didDecodeRestorableStateWithCoder:coder];
	}
}

// MARK: Download Data in Background

- (void)application:(UIApplication *)application handleEventsForBackgroundURLSession:(NSString *)identifier completionHandler:(void (^)(void))completionHandler {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service application:application handleEventsForBackgroundURLSession:identifier completionHandler:completionHandler];
	}

	completionHandler();
}

// MARK: Remote Notification

// Moved to the iOS Plugin

// MARK: User Activity and Handling Quick Actions

- (BOOL)application:(UIApplication *)application willContinueUserActivityWithType:(NSString *)userActivityType {
	BOOL result = NO;

	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		if ([service application:application willContinueUserActivityWithType:userActivityType]) {
			result = YES;
		}
	}

	return result;
}

- (BOOL)application:(UIApplication *)application continueUserActivity:(NSUserActivity *)userActivity restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> *restorableObjects))restorationHandler {
	BOOL result = NO;

	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		if ([service application:application continueUserActivity:userActivity restorationHandler:restorationHandler]) {
			result = YES;
		}
	}

	return result;
}

- (void)application:(UIApplication *)application didUpdateUserActivity:(NSUserActivity *)userActivity {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service application:application didUpdateUserActivity:userActivity];
	}
}

- (void)application:(UIApplication *)application didFailToContinueUserActivityWithType:(NSString *)userActivityType error:(NSError *)error {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service application:application didFailToContinueUserActivityWithType:userActivityType error:error];
	}
}

- (void)application:(UIApplication *)application performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem completionHandler:(void (^)(BOOL succeeded))completionHandler {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service application:application performActionForShortcutItem:shortcutItem completionHandler:completionHandler];
	}
}

// MARK: WatchKit

- (void)application:(UIApplication *)application handleWatchKitExtensionRequest:(NSDictionary *)userInfo reply:(void (^)(NSDictionary *replyInfo))reply {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service application:application handleWatchKitExtensionRequest:userInfo reply:reply];
	}
}

// MARK: HealthKit

- (void)applicationShouldRequestHealthAuthorization:(UIApplication *)application {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service applicationShouldRequestHealthAuthorization:application];
	}
}

// MARK: Opening an URL

- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		if ([service application:app openURL:url options:options]) {
			return YES;
		}
	}

	return NO;
}

// MARK: Disallowing Specified App Extension Types

- (BOOL)application:(UIApplication *)application shouldAllowExtensionPointIdentifier:(UIApplicationExtensionPointIdentifier)extensionPointIdentifier {
	BOOL result = NO;

	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		if ([service application:application shouldAllowExtensionPointIdentifier:extensionPointIdentifier]) {
			result = YES;
		}
	}

	return result;
}

// MARK: SiriKit

- (id)application:(UIApplication *)application handlerForIntent:(INIntent *)intent {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		id result = [service application:application handlerForIntent:intent];

		if (result) {
			return result;
		}
	}

	return nil;
}

// MARK: CloudKit

- (void)application:(UIApplication *)application userDidAcceptCloudKitShareWithMetadata:(CKShareMetadata *)cloudKitShareMetadata {
	for (GDTAppDelegateServiceProtocol *service in services) {
		if (![service respondsToSelector:_cmd]) {
			continue;
		}

		[service application:application userDidAcceptCloudKitShareWithMetadata:cloudKitShareMetadata];
	}
}

/* Handled By Info.plist file for now

// MARK: Interface Geometry

- (UIInterfaceOrientationMask)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window {}

*/

@end

GODOT_CLANG_WARNING_POP

// === Merged from drivers/apple_embedded/main_utilities.mm ===
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

#include "core/string/ustring.h"

#import <UIKit/UIKit.h>
#include <unistd.h>

#include <cstdio>

void change_to_launch_dir(char **p_args) {
	size_t len = strlen(p_args[0]);

	while (len--) {
		if (p_args[0][len] == '/') {
			break;
		}
	}

	if (len >= 0) {
		char path[512];
		memcpy(path, p_args[0], len > sizeof(path) ? sizeof(path) : len);
		path[len] = 0;
		chdir(path);
	}
}

int add_cmdline(int p_argc, char **p_args) {
	NSArray *arr = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"godot_cmdline"];
	if (!arr) {
		return p_argc;
	}

	for (NSUInteger i = 0; i < [arr count]; i++) {
		NSString *str = [arr objectAtIndex:i];
		if (!str) {
			continue;
		}
		p_args[p_argc++] = (char *)[str cStringUsingEncoding:NSUTF8StringEncoding];
	}

	p_args[p_argc] = nullptr;

	return p_argc;
}

int process_args(int p_argc, char **p_args, char **r_args) {
	for (int i = 0; i < p_argc; i++) {
		r_args[i] = p_args[i];
	}
	r_args[p_argc] = nullptr;
	p_argc = add_cmdline(p_argc, r_args);
	return p_argc;
}

// === Merged from drivers/apple_embedded/tts_apple_embedded.mm ===
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

#import "drivers/apple_embedded/os_apple_embedded.h"

#include "servers/display/display_server.h"

@implementation GDTTTS

- (id)init {
	self = [super init];
	self->speaking = false;
	self->av_synth = [[AVSpeechSynthesizer alloc] init];
	[self->av_synth setDelegate:self];
	print_verbose("Text-to-Speech: AVSpeechSynthesizer initialized.");
	return self;
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)av_synth willSpeakRangeOfSpeechString:(NSRange)characterRange utterance:(AVSpeechUtterance *)utterance {
	NSString *string = [utterance speechString];

	// Convert from UTF-16 to UTF-32 position.
	int pos = 0;
	for (NSUInteger i = 0; i < MIN(characterRange.location, string.length); i++) {
		unichar c = [string characterAtIndex:i];
		if ((c & 0xfffffc00) == 0xd800) {
			i++;
		}
		pos++;
	}

	DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_BOUNDARY, ids[utterance], pos);
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)av_synth didCancelSpeechUtterance:(AVSpeechUtterance *)utterance {
	DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, ids[utterance]);
	ids.erase(utterance);
	speaking = false;
	[self update];
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)av_synth didFinishSpeechUtterance:(AVSpeechUtterance *)utterance {
	DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_ENDED, ids[utterance]);
	ids.erase(utterance);
	speaking = false;
	[self update];
}

- (void)update {
	if (!speaking && queue.size() > 0) {
		TTSUtterance &message = queue.front()->get();

		AVSpeechUtterance *new_utterance = [[AVSpeechUtterance alloc] initWithString:[NSString stringWithUTF8String:message.text.utf8().get_data()]];
		[new_utterance setVoice:[AVSpeechSynthesisVoice voiceWithIdentifier:[NSString stringWithUTF8String:message.voice.utf8().get_data()]]];
		if (message.rate > 1.f) {
			[new_utterance setRate:Math::remap(message.rate, 1.f, 10.f, AVSpeechUtteranceDefaultSpeechRate, AVSpeechUtteranceMaximumSpeechRate)];
		} else if (message.rate < 1.f) {
			[new_utterance setRate:Math::remap(message.rate, 0.1f, 1.f, AVSpeechUtteranceMinimumSpeechRate, AVSpeechUtteranceDefaultSpeechRate)];
		}
		[new_utterance setPitchMultiplier:message.pitch];
		[new_utterance setVolume:(Math::remap(message.volume, 0.f, 100.f, 0.f, 1.f))];

		ids[new_utterance] = message.id;
		[av_synth speakUtterance:new_utterance];
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_STARTED, message.id);

		queue.pop_front();
		speaking = true;
	}
}

- (void)pauseSpeaking {
	[av_synth pauseSpeakingAtBoundary:AVSpeechBoundaryImmediate];
}

- (void)resumeSpeaking {
	[av_synth continueSpeaking];
}

- (void)stopSpeaking {
	for (TTSUtterance &message : queue) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, message.id);
	}
	queue.clear();
	[av_synth stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
	speaking = false;
}

- (bool)isSpeaking {
	return speaking || (queue.size() > 0);
}

- (bool)isPaused {
	return [av_synth isPaused];
}

- (void)speak:(const String &)text voice:(const String &)voice volume:(int)volume pitch:(float)pitch rate:(float)rate utterance_id:(int64_t)utterance_id interrupt:(bool)interrupt {
	if (interrupt) {
		[self stopSpeaking];
	}

	if (text.is_empty()) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, utterance_id);
		return;
	}

	TTSUtterance message;
	message.text = text;
	message.voice = voice;
	message.volume = CLAMP(volume, 0, 100);
	message.pitch = CLAMP(pitch, 0.f, 2.f);
	message.rate = CLAMP(rate, 0.1f, 10.f);
	message.id = utterance_id;
	queue.push_back(message);

	if ([self isPaused]) {
		[self resumeSpeaking];
	} else {
		[self update];
	}
}

- (Array)getVoices {
	Array list;
	for (AVSpeechSynthesisVoice *voice in [AVSpeechSynthesisVoice speechVoices]) {
		NSString *voiceIdentifierString = [voice identifier];
		NSString *voiceLocaleIdentifier = [voice language];
		NSString *voiceName = [voice name];
		Dictionary voice_d;
		voice_d["name"] = String::utf8([voiceName UTF8String]);
		voice_d["id"] = String::utf8([voiceIdentifierString UTF8String]);
		voice_d["language"] = String::utf8([voiceLocaleIdentifier UTF8String]);
		list.push_back(voice_d);
	}
	return list;
}

@end
