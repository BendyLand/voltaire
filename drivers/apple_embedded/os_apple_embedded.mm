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

#import "drivers/unix/os_unix.h"

#ifdef APPLE_EMBEDDED_ENABLED

#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/profiling/profiling.h"
#import "drivers/unix/os_unix.h"
#ifdef SDL_ENABLED
#include "core/os/os.h"
#endif
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

#import <AVFoundation/AVFAudio.h>
#import <AudioToolbox/AudioServices.h>
#import <CoreText/CoreText.h>
#import <UIKit/UIKit.h>
#import <dlfcn.h>
#include <sys/sysctl.h>

#include <iterator>

#if defined(RD_ENABLED)
#include "servers/rendering/renderer.h"

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

#import "drivers/unix/os_unix.h"

#include "core/os/os.h"
#import "drivers/unix/os_unix.h"
#include "servers/display/display_server.h"

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

#import "drivers/unix/os_unix.h"

#include "core/object/class_db.h"
#import "drivers/unix/os_unix.h"

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

#import "drivers/unix/os_unix.h"

#include "core/typedefs.h"
#import "drivers/unix/os_unix.h"

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

#import "drivers/unix/os_unix.h"

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

// === Merged from drivers/apple_embedded/display_server_apple_embedded.mm ===
/*  display_server_apple_embedded.mm                                      */
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

#import "drivers/unix/os_unix.h"

#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/os/os.h"
#import "drivers/unix/os_unix.h"
#include "servers/display/display_server.h"

#import <GameController/GameController.h>

static const float kDisplayServerIOSAcceleration = 1.f;

DisplayServerAppleEmbedded::DisplayServerAppleEmbedded(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	KeyMappingAppleEmbedded::initialize();

	rendering_driver = p_rendering_driver;

	// Init TTS
	bool tts_enabled = GLOBAL_GET("audio/general/text_to_speech");
	if (tts_enabled) {
		initialize_tts();
	}
	NativeMenu::initialize();

	bool has_made_render_compositor_current = false;

#if defined(RD_ENABLED)
	rendering_context = nullptr;
	rendering_device = nullptr;

	CALayer *layer = nullptr;

	union {
#ifdef VULKAN_ENABLED
		RenderingContextDriverVulkanAppleEmbedded::WindowPlatformData vulkan;
#endif
#ifdef METAL_ENABLED
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
		// Eliminate "RenderingContextDriverMetal is only available on iOS 14.0 or newer".
		RenderingContextDriverMetal::WindowPlatformData metal;
		GODOT_CLANG_WARNING_POP
#endif
	} wpd;

#if defined(VULKAN_ENABLED)
	if (rendering_driver == "vulkan") {
		layer = [GDTAppDelegateService.viewController.godotView initializeRenderingForDriver:@"vulkan"];
		if (!layer) {
			ERR_FAIL_MSG("Failed to create iOS Vulkan rendering layer.");
		}
		wpd.vulkan.layer_ptr = (CAMetalLayer *const *)&layer;
		rendering_context = memnew(RenderingContextDriverVulkanAppleEmbedded);
	}
#endif
#ifdef METAL_ENABLED
	if (rendering_driver == "metal") {
		if (@available(iOS 14.0, *)) {
			layer = [GDTAppDelegateService.viewController.godotView initializeRenderingForDriver:@"metal"];
			wpd.metal.layer = (__bridge CA::MetalLayer *)layer;
			rendering_context = memnew(RenderingContextDriverMetal);
		} else {
			OS::alert("Metal is only supported on iOS 14.0 and later.");
			r_error = ERR_UNAVAILABLE;
			return;
		}
	}
#endif
	if (rendering_context) {
		if (rendering_context->initialize() != OK) {
			memdelete(rendering_context);
			rendering_context = nullptr;
#if defined(GLES3_ENABLED)
			bool fallback_to_opengl3 = GLOBAL_GET("rendering/rendering_device/fallback_to_opengl3");
			if (fallback_to_opengl3 && rendering_driver != "opengl3") {
				WARN_PRINT("Your device does not seem to support MoltenVK or Metal, switching to OpenGL 3.");
				rendering_driver = "opengl3";
				OS::set_current_rendering_method("gl_compatibility", OS::RENDERING_SOURCE_FALLBACK);
				OS::set_current_rendering_driver_name(rendering_driver, OS::RENDERING_SOURCE_FALLBACK);
			} else
#endif
			{
				ERR_PRINT(vformat("Failed to initialize %s context", rendering_driver));
				r_error = ERR_UNAVAILABLE;
				return;
			}
		}
	}

	if (rendering_context) {
		if (rendering_context->window_create(DisplayServerEnums::MAIN_WINDOW_ID, &wpd) != OK) {
			ERR_PRINT(vformat("Failed to create %s window.", rendering_driver));
			memdelete(rendering_context);
			rendering_context = nullptr;
			r_error = ERR_UNAVAILABLE;
			return;
		}

		Size2i size = Size2i(layer.bounds.size.width, layer.bounds.size.height) * screen_get_max_scale();
		rendering_context->window_set_size(DisplayServerEnums::MAIN_WINDOW_ID, size.width, size.height);
		rendering_context->window_set_vsync_mode(DisplayServerEnums::MAIN_WINDOW_ID, p_vsync_mode);

		rendering_device = memnew(RenderingDevice);
		if (rendering_device->initialize(rendering_context, DisplayServerEnums::MAIN_WINDOW_ID) != OK) {
			rendering_device = nullptr;
			memdelete(rendering_context);
			rendering_context = nullptr;
			r_error = ERR_UNAVAILABLE;
			return;
		}
		rendering_device->screen_create(DisplayServerEnums::MAIN_WINDOW_ID);

		RendererCompositorRD::make_current();
		has_made_render_compositor_current = true;
	}
#endif

#if defined(GLES3_ENABLED)
	if (rendering_driver == "opengl3") {
		CALayer *layer = [GDTAppDelegateService.viewController.godotView initializeRenderingForDriver:@"opengl3"];

		if (!layer) {
			ERR_FAIL_MSG("Failed to create iOS OpenGLES rendering layer.");
		}

		RasterizerGLES3::make_current(false);
		has_made_render_compositor_current = true;
	}
#endif

	ERR_FAIL_COND_MSG(!has_made_render_compositor_current, vformat("Failed to make RendererCompositor current for rendering driver %s", rendering_driver));

	bool keep_screen_on = bool(GLOBAL_GET("display/window/energy_saving/keep_screen_on"));
	screen_set_keep_on(keep_screen_on);

	Input::set_event_dispatch_function(_dispatch_input_events);

	r_error = OK;
}

DisplayServerAppleEmbedded::~DisplayServerAppleEmbedded() {
	NativeMenu::finalize();

#if defined(RD_ENABLED)
	if (rendering_device) {
		rendering_device->screen_free(DisplayServerEnums::MAIN_WINDOW_ID);
		memdelete(rendering_device);
		rendering_device = nullptr;
	}

	if (rendering_context) {
		rendering_context->window_destroy(DisplayServerEnums::MAIN_WINDOW_ID);
		memdelete(rendering_context);
		rendering_context = nullptr;
	}
#endif
}

Vector<String> DisplayServerAppleEmbedded::get_rendering_drivers_func() {
	Vector<String> drivers;

#if defined(VULKAN_ENABLED)
	drivers.push_back("vulkan");
#endif
#if defined(METAL_ENABLED)
	if (@available(ios 14.0, *)) {
		drivers.push_back("metal");
	}
#endif
#if defined(GLES3_ENABLED)
	drivers.push_back("opengl3");
#endif

	return drivers;
}

// MARK: Events

void DisplayServerAppleEmbedded::window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	window_resize_callback = p_callable;
}

void DisplayServerAppleEmbedded::window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	window_event_callback = p_callable;
}
void DisplayServerAppleEmbedded::window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	input_event_callback = p_callable;
}

void DisplayServerAppleEmbedded::window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	input_text_callback = p_callable;
}

void DisplayServerAppleEmbedded::window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

void DisplayServerAppleEmbedded::process_events() {
	Input::flush_buffered_events();
}

void DisplayServerAppleEmbedded::_dispatch_input_events(const Ref<InputEvent> &p_event) {
	if (DisplayServerAppleEmbedded *ds = DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
		ds->send_input_event(p_event);
	}
}

void DisplayServerAppleEmbedded::send_input_event(const Ref<InputEvent> &p_event) const {
	_window_callback(input_event_callback, p_event);
}

void DisplayServerAppleEmbedded::send_input_text(const String &p_text) const {
	_window_callback(input_text_callback, p_text);
}

void DisplayServerAppleEmbedded::send_window_event(DisplayServerEnums::WindowEvent p_event) const {
	_window_callback(window_event_callback, int(p_event));
}

void DisplayServerAppleEmbedded::_window_callback(const Callable &p_callable, const Variant &p_arg) const {
	if (p_callable.is_valid()) {
		p_callable.call(p_arg);
	}
}

// MARK: - Input

// MARK: Touches

void DisplayServerAppleEmbedded::touch_press(int p_idx, int p_x, int p_y, bool p_pressed, bool p_double_click) {
	Ref<InputEventScreenTouch> ev;
	ev.instantiate();

	ev->set_index(p_idx);
	ev->set_pressed(p_pressed);
	ev->set_position(Vector2(p_x, p_y));
	ev->set_double_tap(p_double_click);
	perform_event(ev);
}

void DisplayServerAppleEmbedded::touch_drag(int p_idx, int p_prev_x, int p_prev_y, int p_x, int p_y, float p_pressure, Vector2 p_tilt) {
	Ref<InputEventScreenDrag> ev;
	ev.instantiate();
	ev->set_index(p_idx);
	ev->set_pressure(p_pressure);
	ev->set_tilt(p_tilt);
	ev->set_position(Vector2(p_x, p_y));
	ev->set_relative(Vector2(p_x - p_prev_x, p_y - p_prev_y));
	ev->set_relative_screen_position(ev->get_relative());
	perform_event(ev);
}

void DisplayServerAppleEmbedded::perform_event(const Ref<InputEvent> &p_event) {
	if (!Input::is_initialized()) {
		return;
	}

	Input::parse_input_event(p_event.ptr());
}

void DisplayServerAppleEmbedded::touches_canceled(int p_idx) {
	touch_press(p_idx, -1, -1, false, false);
}

// MARK: Keyboard

void DisplayServerAppleEmbedded::key(Key p_key, char32_t p_char, Key p_unshifted, Key p_physical, NSInteger p_modifier, bool p_pressed, KeyLocation p_location) {
	Ref<InputEventKey> ev;
	ev.instantiate();
	ev->set_echo(false);
	ev->set_pressed(p_pressed);
	ev->set_keycode(fix_keycode(p_char, p_key));
	if (@available(iOS 13.4, *)) {
		if (p_key != Key::SHIFT) {
			ev->set_shift_pressed(p_modifier & UIKeyModifierShift);
		}
		if (p_key != Key::CTRL) {
			ev->set_ctrl_pressed(p_modifier & UIKeyModifierControl);
		}
		if (p_key != Key::ALT) {
			ev->set_alt_pressed(p_modifier & UIKeyModifierAlternate);
		}
		if (p_key != Key::META) {
			ev->set_meta_pressed(p_modifier & UIKeyModifierCommand);
		}
	}
	ev->set_key_label(p_unshifted);
	ev->set_physical_keycode(p_physical);
	ev->set_unicode(fix_unicode(p_char));
	ev->set_location(p_location);
	perform_event(ev);
}

// MARK: Motion

void DisplayServerAppleEmbedded::update_gravity(const Vector3 &p_gravity) {
	Input::set_gravity(p_gravity);
}

void DisplayServerAppleEmbedded::update_accelerometer(const Vector3 &p_accelerometer) {
	Input::set_accelerometer(p_accelerometer / kDisplayServerIOSAcceleration);
}

void DisplayServerAppleEmbedded::update_magnetometer(const Vector3 &p_magnetometer) {
	Input::set_magnetometer(p_magnetometer);
}

void DisplayServerAppleEmbedded::update_gyroscope(const Vector3 &p_gyroscope) {
	Input::set_gyroscope(p_gyroscope);
}

// MARK: -

bool DisplayServerAppleEmbedded::has_feature(DisplayServerEnums::Feature p_feature) const {
	switch (p_feature) {
#ifndef DISABLE_DEPRECATED
		case DisplayServerEnums::FEATURE_GLOBAL_MENU: {
			return NativeMenu::is_initialized() && NativeMenu::has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
		} break;
#endif
		// case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
		// case DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE:
		// case DisplayServerEnums::FEATURE_HIDPI:
		// case DisplayServerEnums::FEATURE_ICON:
		// case DisplayServerEnums::FEATURE_IME:
		// case DisplayServerEnums::FEATURE_MOUSE:
		// case DisplayServerEnums::FEATURE_MOUSE_WARP:
		// case DisplayServerEnums::FEATURE_NATIVE_DIALOG:
		// case DisplayServerEnums::FEATURE_NATIVE_DIALOG_INPUT:
		// case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE:
		// case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA:
		// case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_MIME:
		// case DisplayServerEnums::FEATURE_NATIVE_ICON:
		// case DisplayServerEnums::FEATURE_WINDOW_TRANSPARENCY:
		case DisplayServerEnums::FEATURE_CLIPBOARD:
		case DisplayServerEnums::FEATURE_HDR_OUTPUT:
		case DisplayServerEnums::FEATURE_KEEP_SCREEN_ON:
		case DisplayServerEnums::FEATURE_ORIENTATION:
		case DisplayServerEnums::FEATURE_TOUCHSCREEN:
		case DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD:
		case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH:
			return true;
		default:
			return false;
	}
}

void DisplayServerAppleEmbedded::initialize_tts() const {
	const_cast<DisplayServerAppleEmbedded *>(this)->tts = [[GDTTTS alloc] init];
}

bool DisplayServerAppleEmbedded::tts_is_speaking() const {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL_V(tts, false);
	return [tts isSpeaking];
}

bool DisplayServerAppleEmbedded::tts_is_paused() const {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL_V(tts, false);
	return [tts isPaused];
}

TypedArray<Dictionary> DisplayServerAppleEmbedded::tts_get_voices() const {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL_V(tts, TypedArray<Dictionary>());
	return [tts getVoices];
}

void DisplayServerAppleEmbedded::tts_speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	[tts speak:p_text voice:p_voice volume:p_volume pitch:p_pitch rate:p_rate utterance_id:p_utterance_id interrupt:p_interrupt];
}

void DisplayServerAppleEmbedded::tts_pause() {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	[tts pauseSpeaking];
}

void DisplayServerAppleEmbedded::tts_resume() {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	[tts resumeSpeaking];
}

void DisplayServerAppleEmbedded::tts_stop() {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	[tts stopSpeaking];
}

bool DisplayServerAppleEmbedded::is_dark_mode_supported() const {
	if (@available(iOS 13.0, *)) {
		return true;
	} else {
		return false;
	}
}

bool DisplayServerAppleEmbedded::is_dark_mode() const {
	if (@available(iOS 13.0, *)) {
		return [UITraitCollection currentTraitCollection].userInterfaceStyle == UIUserInterfaceStyleDark;
	} else {
		return false;
	}
}

void DisplayServerAppleEmbedded::set_system_theme_change_callback(const Callable &p_callable) {
	system_theme_changed = p_callable;
}

void DisplayServerAppleEmbedded::emit_system_theme_changed() {
	if (system_theme_changed.is_valid()) {
		Variant ret;
		Callable::CallError ce;
		system_theme_changed.callp(nullptr, 0, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Failed to execute system theme changed callback: %s.", Variant::get_callable_error_text(system_theme_changed, nullptr, 0, ce)));
		}
	}
}

Rect2i DisplayServerAppleEmbedded::get_display_safe_area(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	UIEdgeInsets insets = UIEdgeInsetsZero;
	UIView *view = GDTAppDelegateService.viewController.godotView;
	if ([view respondsToSelector:@selector(safeAreaInsets)]) {
		insets = [view safeAreaInsets];
	}
	float scale = screen_get_scale();
	Size2i insets_position = Size2i(insets.left, insets.top) * scale;
	Size2i insets_size = Size2i(insets.left + insets.right, insets.top + insets.bottom) * scale;
	return Rect2i(screen_get_position() + insets_position, screen_get_size() - insets_size);
}

int DisplayServerAppleEmbedded::get_screen_count() const {
	return 1;
}

int DisplayServerAppleEmbedded::get_primary_screen() const {
	return 0;
}

Point2i DisplayServerAppleEmbedded::screen_get_position(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Point2i());

	return Point2i(0, 0);
}

Size2i DisplayServerAppleEmbedded::screen_get_size(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Size2i());

	CALayer *layer = GDTAppDelegateService.viewController.godotView.renderingLayer;

	if (!layer) {
		return Size2i();
	}

	return Size2i(layer.bounds.size.width, layer.bounds.size.height) * screen_get_scale(p_screen);
}

Rect2i DisplayServerAppleEmbedded::screen_get_usable_rect(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	return Rect2i(screen_get_position(p_screen), screen_get_size(p_screen));
}

Vector<DisplayServerEnums::WindowID> DisplayServerAppleEmbedded::get_window_list() const {
	Vector<DisplayServerEnums::WindowID> list;
	list.push_back(DisplayServerEnums::MAIN_WINDOW_ID);
	return list;
}

DisplayServerEnums::WindowID DisplayServerAppleEmbedded::get_window_at_screen_position(const Point2i &p_position) const {
	return DisplayServerEnums::MAIN_WINDOW_ID;
}

int64_t DisplayServerAppleEmbedded::window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, 0);
	switch (p_handle_type) {
		case DisplayServerEnums::DISPLAY_HANDLE: {
			return 0; // Not supported.
		}
		case DisplayServerEnums::WINDOW_HANDLE: {
			return (int64_t)GDTAppDelegateService.viewController;
		}
		case DisplayServerEnums::WINDOW_VIEW: {
			return (int64_t)GDTAppDelegateService.viewController.godotView;
		}
		default: {
			return 0;
		}
	}
}

void DisplayServerAppleEmbedded::window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window) {
	window_attached_instance_id = p_instance;
}

ObjectID DisplayServerAppleEmbedded::window_get_attached_instance_id(DisplayServerEnums::WindowID p_window) const {
	return window_attached_instance_id;
}

void DisplayServerAppleEmbedded::window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

int DisplayServerAppleEmbedded::window_get_current_screen(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, DisplayServerEnums::INVALID_SCREEN);
	return 0;
}

void DisplayServerAppleEmbedded::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

Point2i DisplayServerAppleEmbedded::window_get_position(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

Point2i DisplayServerAppleEmbedded::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

void DisplayServerAppleEmbedded::window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for single window iOS app
}

void DisplayServerAppleEmbedded::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) {
	// Probably not supported for iOS
}

void DisplayServerAppleEmbedded::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

Size2i DisplayServerAppleEmbedded::window_get_max_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerAppleEmbedded::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

Size2i DisplayServerAppleEmbedded::window_get_min_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerAppleEmbedded::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

Size2i DisplayServerAppleEmbedded::window_get_size(DisplayServerEnums::WindowID p_window) const {
	CGRect viewBounds = GDTAppDelegateService.viewController.view.bounds;
	return Size2i(viewBounds.size.width, viewBounds.size.height) * screen_get_max_scale();
}

Size2i DisplayServerAppleEmbedded::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return window_get_size(p_window);
}

void DisplayServerAppleEmbedded::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

DisplayServerEnums::WindowMode DisplayServerAppleEmbedded::window_get_mode(DisplayServerEnums::WindowID p_window) const {
	return DisplayServerEnums::WindowMode::WINDOW_MODE_FULLSCREEN;
}

bool DisplayServerAppleEmbedded::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window) const {
	return false;
}

void DisplayServerAppleEmbedded::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

bool DisplayServerAppleEmbedded::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window) const {
	return false;
}

void DisplayServerAppleEmbedded::window_request_attention(DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

void DisplayServerAppleEmbedded::window_move_to_foreground(DisplayServerEnums::WindowID p_window) {
	// Probably not supported for iOS
}

bool DisplayServerAppleEmbedded::window_is_focused(DisplayServerEnums::WindowID p_window) const {
	return true;
}

float DisplayServerAppleEmbedded::screen_get_max_scale() const {
	return screen_get_scale(DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
}

void DisplayServerAppleEmbedded::screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen) {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX(p_screen, screen_count);

	screen_orientation = p_orientation;
#ifdef IOS_ENABLED
	// Under the SwiftUI app lifecycle, GDTViewController is wrapped by a UIHostingController
	// that is the window's root VC. iOS queries the root VC for orientation preferences, so we
	// must install the selectors on the hosting class before requesting an orientation update.
	GDTViewController *vc = GDTAppDelegateService.viewController;
	if (!vc) {
		return;
	}
	[vc propagateUIPreferencesToRootViewController];

	UIViewController *rootViewController = vc.view.window.rootViewController ?: vc;
	if (@available(iOS 16.0, *)) {
		[rootViewController setNeedsUpdateOfSupportedInterfaceOrientations];
		UIWindowScene *windowScene = rootViewController.view.window.windowScene;
		if (windowScene) {
			UIWindowSceneGeometryPreferencesIOS *preferences = [[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:[rootViewController supportedInterfaceOrientations]];
			[windowScene requestGeometryUpdateWithPreferences:preferences errorHandler:nil];
		}
	} else {
		[UIViewController attemptRotationToDeviceOrientation];
	}
#endif
}

DisplayServerEnums::ScreenOrientation DisplayServerAppleEmbedded::screen_get_orientation(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, DisplayServerEnums::SCREEN_LANDSCAPE);

	return screen_orientation;
}

bool DisplayServerAppleEmbedded::window_can_draw(DisplayServerEnums::WindowID p_window) const {
	return true;
}

bool DisplayServerAppleEmbedded::can_any_window_draw() const {
	return true;
}

bool DisplayServerAppleEmbedded::is_touchscreen_available() const {
	return true;
}

_FORCE_INLINE_ int _convert_utf32_offset_to_utf16(const String &p_existing_text, int p_pos) {
	int limit = p_pos;
	for (int i = 0; i < MIN(p_existing_text.length(), p_pos); i++) {
		if (p_existing_text[i] > 0xffff) {
			limit++;
		}
	}
	return limit;
}

void DisplayServerAppleEmbedded::virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect, DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start, int p_cursor_end) {
	NSString *existingString = [[NSString alloc] initWithUTF8String:p_existing_text.utf8().get_data()];

	GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypeDefault;
	GDTAppDelegateService.viewController.keyboardView.textContentType = nil;
	switch (p_type) {
		case DisplayServerEnums::KEYBOARD_TYPE_DEFAULT: {
			GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypeDefault;
		} break;
		case DisplayServerEnums::KEYBOARD_TYPE_MULTILINE: {
			GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypeDefault;
		} break;
		case DisplayServerEnums::KEYBOARD_TYPE_NUMBER: {
			GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypeNumberPad;
		} break;
		case DisplayServerEnums::KEYBOARD_TYPE_NUMBER_DECIMAL: {
			GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypeDecimalPad;
		} break;
		case DisplayServerEnums::KEYBOARD_TYPE_PHONE: {
			GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypePhonePad;
			GDTAppDelegateService.viewController.keyboardView.textContentType = UITextContentTypeTelephoneNumber;
		} break;
		case DisplayServerEnums::KEYBOARD_TYPE_EMAIL_ADDRESS: {
			GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypeEmailAddress;
			GDTAppDelegateService.viewController.keyboardView.textContentType = UITextContentTypeEmailAddress;
		} break;
		case DisplayServerEnums::KEYBOARD_TYPE_PASSWORD: {
			GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypeDefault;
			GDTAppDelegateService.viewController.keyboardView.textContentType = UITextContentTypePassword;
		} break;
		case DisplayServerEnums::KEYBOARD_TYPE_URL: {
			GDTAppDelegateService.viewController.keyboardView.keyboardType = UIKeyboardTypeWebSearch;
			GDTAppDelegateService.viewController.keyboardView.textContentType = UITextContentTypeURL;
		} break;
	}

	[GDTAppDelegateService.viewController.keyboardView
			becomeFirstResponderWithString:existingString
							   cursorStart:_convert_utf32_offset_to_utf16(p_existing_text, p_cursor_start)
								 cursorEnd:_convert_utf32_offset_to_utf16(p_existing_text, p_cursor_end)];
}

bool DisplayServerAppleEmbedded::is_keyboard_active() const {
	return [GDTAppDelegateService.viewController.keyboardView isFirstResponder];
}

void DisplayServerAppleEmbedded::virtual_keyboard_hide() {
	[GDTAppDelegateService.viewController.keyboardView resignFirstResponder];
}

void DisplayServerAppleEmbedded::virtual_keyboard_set_height(int height) {
	virtual_keyboard_height = height * screen_get_max_scale();
}

int DisplayServerAppleEmbedded::virtual_keyboard_get_height() const {
	return virtual_keyboard_height;
}

bool DisplayServerAppleEmbedded::has_hardware_keyboard() const {
	if (@available(iOS 14.0, *)) {
		return [GCKeyboard coalescedKeyboard];
	} else {
		return false;
	}
}

void DisplayServerAppleEmbedded::clipboard_set(const String &p_text) {
	[UIPasteboard generalPasteboard].string = [NSString stringWithUTF8String:p_text.utf8().get_data()];
}

String DisplayServerAppleEmbedded::clipboard_get() const {
	NSString *text = [UIPasteboard generalPasteboard].string;

	return String::utf8([text UTF8String]);
}

void DisplayServerAppleEmbedded::screen_set_keep_on(bool p_enable) {
	[UIApplication sharedApplication].idleTimerDisabled = p_enable;
}

bool DisplayServerAppleEmbedded::screen_is_kept_on() const {
	return [UIApplication sharedApplication].idleTimerDisabled;
}

void DisplayServerAppleEmbedded::resize_window(CGSize viewSize) {
	Size2i size = Size2i(viewSize.width, viewSize.height) * screen_get_max_scale();

#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_context->window_set_size(DisplayServerEnums::MAIN_WINDOW_ID, size.x, size.y);
	}
#endif

	Variant resize_rect = Rect2i(Point2i(), size);
	_window_callback(window_resize_callback, resize_rect);
}

void DisplayServerAppleEmbedded::window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_
#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_context->window_set_vsync_mode(p_window, p_vsync_mode);
	}
#endif
}

DisplayServerEnums::VSyncMode DisplayServerAppleEmbedded::window_get_vsync_mode(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_vsync_mode(p_window);
	}
#endif
	return DisplayServerEnums::VSYNC_ENABLED;
}

// MARK: - HDR / EDR

void DisplayServerAppleEmbedded::_update_hdr_output(bool edr_headroom_changed) {
#ifdef RD_ENABLED
	if (!rendering_context) {
		return;
	}

	bool desired = edr_requested && _screen_hdr_is_supported();
	bool hdr_state_changed = rendering_context->window_get_hdr_output_enabled(DisplayServerEnums::MAIN_WINDOW_ID) != desired;
	if (hdr_state_changed) {
		rendering_context->window_set_hdr_output_enabled(DisplayServerEnums::MAIN_WINDOW_ID, desired);
	}

	float reference_luminance = _calculate_current_reference_luminance();
	rendering_context->window_set_hdr_output_reference_luminance(DisplayServerEnums::MAIN_WINDOW_ID, reference_luminance);
	rendering_context->window_set_hdr_output_linear_luminance_scale(DisplayServerEnums::MAIN_WINDOW_ID, reference_luminance);

	float max_luminance = _screen_potential_edr_headroom() * hardware_reference_luminance_nits;
	rendering_context->window_set_hdr_output_max_luminance(DisplayServerEnums::MAIN_WINDOW_ID, max_luminance);

	if (hdr_state_changed || edr_headroom_changed) {
		send_window_event(DisplayServerEnums::WINDOW_EVENT_OUTPUT_MAX_LINEAR_VALUE_CHANGED);
	}
#endif
}

void DisplayServerAppleEmbedded::current_edr_headroom_changed() {
	_update_hdr_output(true);
}

bool DisplayServerAppleEmbedded::window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window) const {
	bool renderer_supports_hdr_output = false;
	bool surface_supports_hdr_output = false;
#if defined(RD_ENABLED)
	if (rendering_device && rendering_device->has_feature(RenderingDevice::Features::SUPPORTS_HDR_OUTPUT)) {
		renderer_supports_hdr_output = true;
		surface_supports_hdr_output = rendering_device->screen_get_hdr_output_supported(p_window);
	}
#endif
	if (!renderer_supports_hdr_output) {
		return false;
	}

	if (!surface_supports_hdr_output) {
		return false;
	}

	return _screen_hdr_is_supported();
}

void DisplayServerAppleEmbedded::window_request_hdr_output(const bool p_enabled, DisplayServerEnums::WindowID p_window) {
	if (p_enabled) {
		bool renderer_supports_hdr_output = false;
		bool surface_supports_hdr_output = false;
#if defined(RD_ENABLED)
		if (rendering_device && rendering_device->has_feature(RenderingDevice::Features::SUPPORTS_HDR_OUTPUT)) {
			renderer_supports_hdr_output = true;
			surface_supports_hdr_output = rendering_device->screen_get_hdr_output_supported(p_window);
		}
#endif
		if (!renderer_supports_hdr_output) {
			WARN_PRINT("HDR output requested, but is not supported by the renderer or rendering device driver.");
			return;
		}

		if (!surface_supports_hdr_output) {
			WARN_PRINT("HDR output requested, but the window does not support an HDR format.");
			return;
		}
	}

	edr_requested = p_enabled;
	_update_hdr_output(false);
}

bool DisplayServerAppleEmbedded::window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window) const {
	return edr_requested;
}

bool DisplayServerAppleEmbedded::window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window) const {
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_enabled(p_window);
	}
#endif
	return false;
}

void DisplayServerAppleEmbedded::window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window) {
	ERR_PRINT_ONCE("Manually setting reference white luminance is not supported on Apple devices, as they provide a user-facing brightness setting that directly controls reference white luminance.");
}

float DisplayServerAppleEmbedded::window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window) const {
	return -1.0f; // Always auto-adjusted by the OS on Apple platforms.
}

float DisplayServerAppleEmbedded::_calculate_current_reference_luminance() const {
	float potential = _screen_potential_edr_headroom();
	float current = _screen_current_edr_headroom();
	return potential * hardware_reference_luminance_nits / current;
}

float DisplayServerAppleEmbedded::window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window) const {
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_reference_luminance(p_window);
	}
#endif
	return 200.0f;
}

void DisplayServerAppleEmbedded::window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window) {
	ERR_PRINT_ONCE("Manually setting max luminance is not supported on Apple embedded devices as they provide accurate max luminance values for their built-in screens.");
}

float DisplayServerAppleEmbedded::window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window) const {
	return -1.0f;
}

float DisplayServerAppleEmbedded::window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window) const {
	return _screen_potential_edr_headroom() * hardware_reference_luminance_nits;
}

float DisplayServerAppleEmbedded::window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window) const {
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_output_max_linear_value(p_window);
	}
#endif
	return 1.0f;
}

void DisplayServerAppleEmbedded::set_native_icon(const String &p_filename) {
	// Not supported on Apple embedded platforms.
}

void DisplayServerAppleEmbedded::set_icon(const Ref<Image> &p_icon) {
	// Not supported on Apple embedded platforms.
}

// === Merged from drivers/apple_embedded/godot_keyboard_input_view.mm ===
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

#import "drivers/unix/os_unix.h"

#include "core/os/os.h"
#import "drivers/unix/os_unix.h"

@interface GDTKeyboardInputView () <UITextViewDelegate>

@property(nonatomic, copy) NSString *previousText;
@property(nonatomic, assign) NSRange previousSelectedRange;

@end

@implementation GDTKeyboardInputView

- (instancetype)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];

	if (self) {
		[self godot_commonInit];
	}

	return self;
}

- (instancetype)initWithFrame:(CGRect)frame textContainer:(NSTextContainer *)textContainer {
	self = [super initWithFrame:frame textContainer:textContainer];

	if (self) {
		[self godot_commonInit];
	}

	return self;
}

- (void)godot_commonInit {
	self.hidden = YES;
	self.delegate = self;

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(observeTextChange:)
												 name:UITextViewTextDidChangeNotification
											   object:self];
}

- (void)dealloc {
	self.delegate = nil;
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

// MARK: Keyboard

- (BOOL)canBecomeFirstResponder {
	return YES;
}

- (BOOL)becomeFirstResponderWithString:(NSString *)existingString cursorStart:(NSInteger)start cursorEnd:(NSInteger)end {
	self.text = existingString;
	self.previousText = existingString;

	NSInteger safeStartIndex = MAX(start, 0);

	NSRange textRange;

	// Either a simple cursor or a selection.
	if (end > 0) {
		textRange = NSMakeRange(safeStartIndex, end - start);
	} else {
		textRange = NSMakeRange(safeStartIndex, 0);
	}

	self.selectedRange = textRange;
	self.previousSelectedRange = textRange;

	return [self becomeFirstResponder];
}

- (BOOL)resignFirstResponder {
	self.text = nil;
	self.previousText = nil;
	return [super resignFirstResponder];
}

// MARK: OS Messages

- (void)deleteText:(NSInteger)charactersToDelete {
	for (int i = 0; i < charactersToDelete; i++) {
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->key(Key::BACKSPACE, 0, Key::BACKSPACE, Key::NONE, 0, true, KeyLocation::UNSPECIFIED);
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->key(Key::BACKSPACE, 0, Key::BACKSPACE, Key::NONE, 0, false, KeyLocation::UNSPECIFIED);
	}
}

- (void)enterText:(NSString *)substring {
	String characters = String::utf8([substring UTF8String]);

	for (int i = 0; i < characters.size(); i++) {
		int character = characters[i];
		Key key = Key::NONE;

		if (character == '\t') { // 0x09
			key = Key::TAB;
		} else if (character == '\n') { // 0x0A
			key = Key::ENTER;
		} else if (character == 0x2006) {
			key = Key::SPACE;
		}

		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->key(key, character, key, Key::NONE, 0, true, KeyLocation::UNSPECIFIED);
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->key(key, character, key, Key::NONE, 0, false, KeyLocation::UNSPECIFIED);
	}
}

// MARK: Observer

- (void)observeTextChange:(NSNotification *)notification {
	if (notification.object != self) {
		return;
	}

	NSString *substringToDelete = nil;
	if (self.previousSelectedRange.length == 0) {
		// Get previous text to delete.
		substringToDelete = [self.previousText substringToIndex:self.previousSelectedRange.location];
	} else {
		// If text was previously selected we are sending only one `backspace`. It will remove all text from text input.
		[self deleteText:1];
	}

	NSString *substringToEnter = nil;
	if (self.selectedRange.length == 0) {
		// If previous cursor had a selection we have to calculate an inserted text.
		if (self.previousSelectedRange.length != 0) {
			NSInteger rangeEnd = self.selectedRange.location + self.selectedRange.length;
			NSInteger rangeStart = MIN(self.previousSelectedRange.location, self.selectedRange.location);
			NSInteger rangeLength = MAX(0, rangeEnd - rangeStart);

			NSRange calculatedRange;

			if (rangeLength >= 0) {
				calculatedRange = NSMakeRange(rangeStart, rangeLength);
			} else {
				calculatedRange = NSMakeRange(rangeStart, 0);
			}

			substringToEnter = [self.text substringWithRange:calculatedRange];
		} else {
			substringToEnter = [self.text substringToIndex:self.selectedRange.location];
		}
	} else {
		substringToEnter = [self.text substringWithRange:self.selectedRange];
	}

	NSInteger skip = 0;
	if (substringToDelete != nil) {
		for (NSUInteger i = 0; i < MIN([substringToDelete length], [substringToEnter length]); i++) {
			if ([substringToDelete characterAtIndex:i] == [substringToEnter characterAtIndex:i]) {
				skip++;
			} else {
				break;
			}
		}
		[self deleteText:[substringToDelete length] - skip]; // Delete changed part of previous text.
	}
	[self enterText:[substringToEnter substringFromIndex:skip]]; // Enter changed part of new text.

	self.previousText = self.text;
	self.previousSelectedRange = self.selectedRange;
}

@end

// === Merged from drivers/apple_embedded/godot_view_apple_embedded.mm ===
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

#import "drivers/unix/os_unix.h"

#include "core/os/os.h"
#include "core/string/ustring.h"
#import "drivers/unix/os_unix.h"

#import <CoreMotion/CoreMotion.h>

static const int max_touches = 32;
static const float earth_gravity = 9.80665;

@interface GDTView () {
	UITouch *godot_touches[max_touches];
	CGFloat last_edr_headroom;
}

@property(assign, nonatomic) BOOL isActive;

// CADisplayLink available on 3.1+ synchronizes the animation timer & drawing with the refresh rate of the display, only supports animation intervals of 1/60 1/30 & 1/15
@property(strong, nonatomic) CADisplayLink *displayLink;

// An animation timer that, when animation is started, will periodically call -drawView at the given rate.
// Only used if CADisplayLink is not
@property(strong, nonatomic) NSTimer *animationTimer;

@property(strong, nonatomic) CALayer<GDTDisplayLayer> *renderingLayer;

@property(strong, nonatomic) CMMotionManager *motionManager;

@property(assign, nonatomic) BOOL delegateDidFinishSetUp;

@end

@implementation GDTView

// Implemented in subclasses
- (CALayer<GDTDisplayLayer> *)initializeRenderingForDriver:(NSString *)driverName {
	return nil;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];

	if (self) {
		[self godot_commonInit];
	}

	return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];

	if (self) {
		[self godot_commonInit];
	}

	return self;
}

- (void)dealloc {
	[self stopRendering];

	self.renderer = nil;
	self.delegate = nil;

	if (self.renderingLayer) {
		[self.renderingLayer removeFromSuperlayer];
		self.renderingLayer = nil;
	}

	if (self.motionManager) {
		[self.motionManager stopDeviceMotionUpdates];
		self.motionManager = nil;
	}

	if (self.displayLink) {
		[self.displayLink invalidate];
		self.displayLink = nil;
	}

	if (self.animationTimer) {
		[self.animationTimer invalidate];
		self.animationTimer = nil;
	}
}

- (void)godot_commonInit {
	self.preferredFrameRate = 60;
	self.useCADisplayLink = bool(GLOBAL_DEF("display.AppleEmbedded/use_cadisplaylink", true)) ? YES : NO;
	last_edr_headroom = 0.0;

#if !defined(VISIONOS_ENABLED)
	self.contentScaleFactor = [UIScreen mainScreen].scale;
#endif

	if (@available(iOS 17.0, *)) {
		[self registerForTraitChanges:@[ [UITraitUserInterfaceStyle class] ] withTarget:self action:@selector(traitCollectionDidChangeWithView:previousTraitCollection:)];
	}

	[self initTouches];

	self.multipleTouchEnabled = YES;

	// Configure and start accelerometer
	if (!self.motionManager) {
		self.motionManager = [[CMMotionManager alloc] init];
		if (self.motionManager.deviceMotionAvailable) {
			self.motionManager.deviceMotionUpdateInterval = 1.0 / 70.0;
			[self.motionManager startDeviceMotionUpdatesUsingReferenceFrame:CMAttitudeReferenceFrameXMagneticNorthZVertical];
		} else {
			self.motionManager = nil;
		}
	}
}

- (void)system_theme_changed {
	DisplayServerAppleEmbedded *ds = DisplayServer::driver_as<DisplayServerAppleEmbedded>();
	if (ds) {
		ds->emit_system_theme_changed();
	}
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
	if (@available(iOS 13.0, *)) {
#if !defined(VISIONOS_ENABLED)
		[super traitCollectionDidChange:previousTraitCollection];
#endif
		[self traitCollectionDidChangeWithView:self
					   previousTraitCollection:previousTraitCollection];
	}
}

- (void)traitCollectionDidChangeWithView:(UIView *)view previousTraitCollection:(UITraitCollection *)previousTraitCollection {
	if (@available(iOS 13.0, *)) {
		if ([UITraitCollection currentTraitCollection].userInterfaceStyle != previousTraitCollection.userInterfaceStyle) {
			[self system_theme_changed];
		}
	}
}

- (void)stopRendering {
	if (!self.isActive) {
		return;
	}

	self.isActive = NO;

	print_verbose("Stop animation!");

	if (self.useCADisplayLink) {
		[self.displayLink invalidate];
		self.displayLink = nil;
	} else {
		[self.animationTimer invalidate];
		self.animationTimer = nil;
	}

	[self clearTouches];
}

- (void)startRendering {
	if (self.isActive) {
		return;
	}

	self.isActive = YES;

	print_verbose("Start animation!");

	if (self.useCADisplayLink) {
		self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView)];
		self.displayLink.preferredFramesPerSecond = self.preferredFrameRate;

		// Setup DisplayLink in main thread
		[self.displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
	} else {
		self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / self.preferredFrameRate) target:self selector:@selector(drawView) userInfo:nil repeats:YES];
	}
}

- (void)drawView {
	if (!self.isActive) {
		print_verbose("Draw view not active!");
		return;
	}

	if (self.useCADisplayLink) {
		// Pause the CADisplayLink to avoid recursion
		[self.displayLink setPaused:YES];

		// Process all input events
		while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE) == kCFRunLoopRunHandledSource) {
			// Continue.
		}

		// We are good to go, resume the CADisplayLink
		[self.displayLink setPaused:NO];
	}

	[self.renderingLayer startRenderDisplayLayer];

	if (!self.renderer) {
		return;
	}

	if ([self.renderer setupView:self]) {
		return;
	}

	if (self.delegate && !self.delegateDidFinishSetUp) {
		[self layoutRenderingLayer]; // Trigger DisplayServerVisionOS::resize_window after Main::start()
		self.delegateDidFinishSetUp = [self.delegate godotViewFinishedSetup:self];
		if (!_delegateDidFinishSetUp) {
			return;
		}
	}

	[self handleMotion];

#if !defined(VISIONOS_ENABLED)
	if (@available(iOS 16.0, *)) {
		CGFloat edr_headroom = UIScreen.mainScreen.currentEDRHeadroom;
		if (last_edr_headroom != edr_headroom) {
			last_edr_headroom = edr_headroom;
			if (DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
				DisplayServer::driver_as<DisplayServerAppleEmbedded>()->current_edr_headroom_changed();
			}
		}
	}

#endif

	[self.renderer renderOnView:self];

	[self.renderingLayer stopRenderDisplayLayer];
}

- (BOOL)canRender {
	if (self.useCADisplayLink) {
		return self.displayLink != nil;
	} else {
		return self.animationTimer != nil;
	}
}

- (void)setPreferredFrameRate:(float)preferredFrameRate {
	_preferredFrameRate = preferredFrameRate;

	if (self.canRender) {
		[self stopRendering];
		[self startRendering];
	}
}

- (void)layoutSubviews {
	[super layoutSubviews];
	[self layoutRenderingLayer];
}

- (void)layoutRenderingLayer {
	if (self.renderingLayer) {
		self.renderingLayer.frame = self.bounds;
		[self.renderingLayer layoutDisplayLayer];

		if (DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->resize_window(self.bounds.size);
		}
	}
}

// MARK: - Input

// MARK: Touches

- (void)initTouches {
	for (int i = 0; i < max_touches; i++) {
		godot_touches[i] = nullptr;
	}
}

- (int)getTouchIDForTouch:(UITouch *)p_touch {
	int first = -1;
	for (int i = 0; i < max_touches; i++) {
		if (first == -1 && godot_touches[i] == nullptr) {
			first = i;
			continue;
		}
		if (godot_touches[i] == p_touch) {
			return i;
		}
	}

	if (first != -1) {
		godot_touches[first] = p_touch;
		return first;
	}

	return -1;
}

- (int)removeTouch:(UITouch *)p_touch {
	int remaining = 0;
	for (int i = 0; i < max_touches; i++) {
		if (godot_touches[i] == nullptr) {
			continue;
		}
		if (godot_touches[i] == p_touch) {
			godot_touches[i] = nullptr;
		} else {
			++remaining;
		}
	}
	return remaining;
}

- (void)clearTouches {
	for (int i = 0; i < max_touches; i++) {
		godot_touches[i] = nullptr;
	}
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch *touch in touches) {
		int tid = [self getTouchIDForTouch:touch];
		ERR_FAIL_COND(tid == -1);
		CGPoint touchPoint = [touch locationInView:self];
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->touch_press(tid, touchPoint.x * self.contentScaleFactor, touchPoint.y * self.contentScaleFactor, true, touch.tapCount > 1);
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch *touch in touches) {
		int tid = [self getTouchIDForTouch:touch];
		ERR_FAIL_COND(tid == -1);
		CGPoint touchPoint = [touch locationInView:self];
		CGPoint prev_point = [touch previousLocationInView:self];
		CGFloat alt = [touch altitudeAngle];
		CGVector azim = [touch azimuthUnitVectorInView:self];
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->touch_drag(tid, prev_point.x * self.contentScaleFactor, prev_point.y * self.contentScaleFactor, touchPoint.x * self.contentScaleFactor, touchPoint.y * self.contentScaleFactor, [touch force] / [touch maximumPossibleForce], Vector2(azim.dx, azim.dy) * Math::cos(alt));
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch *touch in touches) {
		int tid = [self getTouchIDForTouch:touch];
		ERR_FAIL_COND(tid == -1);
		[self removeTouch:touch];
		CGPoint touchPoint = [touch locationInView:self];
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->touch_press(tid, touchPoint.x * self.contentScaleFactor, touchPoint.y * self.contentScaleFactor, false, false);
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch *touch in touches) {
		int tid = [self getTouchIDForTouch:touch];
		ERR_FAIL_COND(tid == -1);
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->touches_canceled(tid);
	}
	[self clearTouches];
}

// MARK: Motion

- (void)handleMotion {
	if (!self.motionManager) {
		return;
	}

	// Just using polling approach for now, we can set this up so it sends
	// data to us in intervals, might be better. See Apple reference pages
	// for more details:
	// https://developer.apple.com/reference/coremotion/cmmotionmanager?language=objc

	// Apple splits our accelerometer date into a gravity and user movement
	// component. We add them back together.
	CMAcceleration gravity = self.motionManager.deviceMotion.gravity;
	CMAcceleration acceleration = self.motionManager.deviceMotion.userAcceleration;

	// To be consistent with Android we convert the unit of measurement from g (Earth's gravity)
	// to m/s^2.
	gravity.x *= earth_gravity;
	gravity.y *= earth_gravity;
	gravity.z *= earth_gravity;
	acceleration.x *= earth_gravity;
	acceleration.y *= earth_gravity;
	acceleration.z *= earth_gravity;

	///@TODO We don't seem to be getting data here, is my device broken or
	/// is this code incorrect?
	CMMagneticField magnetic = self.motionManager.deviceMotion.magneticField.field;

	///@TODO we can access rotationRate as a CMRotationRate variable
	///(processed date) or CMGyroData (raw data), have to see what works
	/// best
	CMRotationRate rotation = self.motionManager.deviceMotion.rotationRate;

	// Adjust for screen orientation.
	// [[UIDevice currentDevice] orientation] changes even if we've fixed
	// our orientation which is not a good thing when you're trying to get
	// your user to move the screen in all directions and want consistent
	// output
#if defined(VISIONOS_ENABLED)
	UIInterfaceOrientation interfaceOrientation = [UIApplication sharedApplication].delegate.window.windowScene.effectiveGeometry.interfaceOrientation;
#else
	UIInterfaceOrientation interfaceOrientation = [UIApplication sharedApplication].delegate.window.windowScene.interfaceOrientation;
#endif

	switch (interfaceOrientation) {
		case UIInterfaceOrientationLandscapeLeft: {
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_gravity(Vector3(gravity.x, gravity.y, gravity.z).rotated(Vector3(0, 0, 1), -Math::PI * 0.5));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_accelerometer(Vector3(acceleration.x + gravity.x, acceleration.y + gravity.y, acceleration.z + gravity.z).rotated(Vector3(0, 0, 1), -Math::PI * 0.5));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_magnetometer(Vector3(magnetic.x, magnetic.y, magnetic.z).rotated(Vector3(0, 0, 1), -Math::PI * 0.5));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_gyroscope(Vector3(rotation.x, rotation.y, rotation.z).rotated(Vector3(0, 0, 1), -Math::PI * 0.5));
		} break;
		case UIInterfaceOrientationLandscapeRight: {
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_gravity(Vector3(gravity.x, gravity.y, gravity.z).rotated(Vector3(0, 0, 1), Math::PI * 0.5));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_accelerometer(Vector3(acceleration.x + gravity.x, acceleration.y + gravity.y, acceleration.z + gravity.z).rotated(Vector3(0, 0, 1), Math::PI * 0.5));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_magnetometer(Vector3(magnetic.x, magnetic.y, magnetic.z).rotated(Vector3(0, 0, 1), Math::PI * 0.5));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_gyroscope(Vector3(rotation.x, rotation.y, rotation.z).rotated(Vector3(0, 0, 1), Math::PI * 0.5));
		} break;
		case UIInterfaceOrientationPortraitUpsideDown: {
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_gravity(Vector3(gravity.x, gravity.y, gravity.z).rotated(Vector3(0, 0, 1), Math::PI));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_accelerometer(Vector3(acceleration.x + gravity.x, acceleration.y + gravity.y, acceleration.z + gravity.z).rotated(Vector3(0, 0, 1), Math::PI));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_magnetometer(Vector3(magnetic.x, magnetic.y, magnetic.z).rotated(Vector3(0, 0, 1), Math::PI));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_gyroscope(Vector3(rotation.x, rotation.y, rotation.z).rotated(Vector3(0, 0, 1), Math::PI));
		} break;
		default: { // assume portrait
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_gravity(Vector3(gravity.x, gravity.y, gravity.z));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_accelerometer(Vector3(acceleration.x + gravity.x, acceleration.y + gravity.y, acceleration.z + gravity.z));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_magnetometer(Vector3(magnetic.x, magnetic.y, magnetic.z));
			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->update_gyroscope(Vector3(rotation.x, rotation.y, rotation.z));
		} break;
	}
}

@end

// === Merged from drivers/apple_embedded/godot_view_controller.mm ===
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

#import "drivers/unix/os_unix.h"

#include "core/os/os.h"
#import "drivers/unix/os_unix.h"
#include "servers/rendering/renderer.h"

#import <AVFoundation/AVFoundation.h>
#import <GameController/GameController.h>
#import <objc/runtime.h>

@interface GDTViewController () <GDTViewDelegate>

@property(strong, nonatomic) GDTViewRenderer *renderer;
@property(strong, nonatomic) GDTKeyboardInputView *keyboardView;

@property(strong, nonatomic) UIView *godotLoadingOverlay;

@end

@implementation GDTViewController

- (GDTView *)godotView {
	return (GDTView *)self.view;
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
	[super pressesBegan:presses withEvent:event];

	if (!DisplayServer::driver_as<DisplayServerAppleEmbedded>() || DisplayServer::driver_as<DisplayServerAppleEmbedded>()->is_keyboard_active()) {
		return;
	}
	if (@available(iOS 13.4, *)) {
		for (UIPress *press in presses) {
			String u32lbl = String::utf8([press.key.charactersIgnoringModifiers UTF8String]);
			String u32text = String::utf8([press.key.characters UTF8String]);
			Key key = KeyMappingAppleEmbedded::remap_key(press.key.keyCode);

			if (press.key.keyCode == 0 && u32text.is_empty() && u32lbl.is_empty()) {
				continue;
			}

			char32_t us = 0;
			if (!u32lbl.is_empty() && !u32lbl.begins_with("UIKey")) {
				us = u32lbl[0];
			}

			KeyLocation location = KeyMappingAppleEmbedded::key_location(press.key.keyCode);

			if (!u32text.is_empty() && !u32text.begins_with("UIKey")) {
				for (int i = 0; i < u32text.length(); i++) {
					const char32_t c = u32text[i];
					DisplayServer::driver_as<DisplayServerAppleEmbedded>()->key(fix_keycode(us, key), c, fix_key_label(us, key), key, press.key.modifierFlags, true, location);
				}
			} else {
				DisplayServer::driver_as<DisplayServerAppleEmbedded>()->key(fix_keycode(us, key), 0, fix_key_label(us, key), key, press.key.modifierFlags, true, location);
			}
		}
	}
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
	[super pressesEnded:presses withEvent:event];

	if (!DisplayServer::driver_as<DisplayServerAppleEmbedded>() || DisplayServer::driver_as<DisplayServerAppleEmbedded>()->is_keyboard_active()) {
		return;
	}
	if (@available(iOS 13.4, *)) {
		for (UIPress *press in presses) {
			String u32lbl = String::utf8([press.key.charactersIgnoringModifiers UTF8String]);
			Key key = KeyMappingAppleEmbedded::remap_key(press.key.keyCode);

			if (press.key.keyCode == 0 && u32lbl.is_empty()) {
				continue;
			}

			char32_t us = 0;
			if (!u32lbl.is_empty() && !u32lbl.begins_with("UIKey")) {
				us = u32lbl[0];
			}

			KeyLocation location = KeyMappingAppleEmbedded::key_location(press.key.keyCode);

			DisplayServer::driver_as<DisplayServerAppleEmbedded>()->key(fix_keycode(us, key), 0, fix_key_label(us, key), key, press.key.modifierFlags, false, location);
		}
	}
}

- (void)loadView {
	GDTView *view = GDTViewCreate();
	GDTViewRenderer *renderer = [[GDTViewRenderer alloc] init];

	self.renderer = renderer;
	self.view = view;

	view.renderer = self.renderer;
	view.delegate = self;
}

- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];

	if (self) {
		[self godot_commonInit];
	}

	return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];

	if (self) {
		[self godot_commonInit];
	}

	return self;
}

- (void)godot_commonInit {
	// Initialize view controller values.
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	print_verbose("Did receive memory warning!");
}

- (void)viewDidLoad {
	[super viewDidLoad];

	[self observeKeyboard];
	[self displayLoadingOverlay];

	[self setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	[self.godotView startRendering];
#ifdef IOS_ENABLED
	[self propagateUIPreferencesToRootViewController];
#endif
}

- (void)viewDidDisappear:(BOOL)animated {
	[self.godotView stopRendering];
	[super viewDidDisappear:animated];
}

- (void)observeKeyboard {
	print_verbose("Setting up keyboard input view.");
	self.keyboardView = [GDTKeyboardInputView new];
	[self.view addSubview:self.keyboardView];

	print_verbose("Adding observer for keyboard show/hide.");
	[[NSNotificationCenter defaultCenter]
			addObserver:self
			   selector:@selector(keyboardOnScreen:)
				   name:UIKeyboardDidShowNotification
				 object:nil];
	[[NSNotificationCenter defaultCenter]
			addObserver:self
			   selector:@selector(keyboardHidden:)
				   name:UIKeyboardDidHideNotification
				 object:nil];
}

- (void)displayLoadingOverlay {
#if !defined(VISIONOS_ENABLED)
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *storyboardName = @"Launch Screen";

	if ([bundle pathForResource:storyboardName ofType:@"storyboardc"] == nil) {
		return;
	}

	UIStoryboard *launchStoryboard = [UIStoryboard storyboardWithName:storyboardName bundle:bundle];

	UIViewController *controller = [launchStoryboard instantiateInitialViewController];
	self.godotLoadingOverlay = controller.view;
	self.godotLoadingOverlay.frame = self.view.bounds;
	self.godotLoadingOverlay.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;

	[self.view addSubview:self.godotLoadingOverlay];
#endif
}

- (BOOL)godotViewFinishedSetup:(GDTView *)view {
	[self.godotLoadingOverlay removeFromSuperview];
	self.godotLoadingOverlay = nil;

	return YES;
}

- (void)dealloc {
	self.keyboardView = nil;

	self.renderer = nil;

	if (self.godotLoadingOverlay) {
		[self.godotLoadingOverlay removeFromSuperview];
		self.godotLoadingOverlay = nil;
	}

	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

// Propagate UI preferences to the root VC (the SwiftUI hosting controller doesn't delegate these).
#ifdef IOS_ENABLED
- (void)propagateUIPreferencesToRootViewController {
	UIViewController *rootVC = self.view.window.rootViewController;
	if (!rootVC || rootVC == self) {
		return;
	}

	Class rootClass = [rootVC class];

	SEL selectors[] = {
		@selector(preferredScreenEdgesDeferringSystemGestures),
		@selector(prefersHomeIndicatorAutoHidden),
		@selector(prefersStatusBarHidden),
		@selector(shouldAutorotate),
		@selector(supportedInterfaceOrientations),
	};

	for (SEL sel : selectors) {
		Method method = class_getInstanceMethod([GDTViewController class], sel);
		if (!class_addMethod(rootClass, sel, method_getImplementation(method), method_getTypeEncoding(method))) {
			method_setImplementation(class_getInstanceMethod(rootClass, sel), method_getImplementation(method));
		}
	}

	[rootVC setNeedsUpdateOfScreenEdgesDeferringSystemGestures];
	[rootVC setNeedsUpdateOfHomeIndicatorAutoHidden];
	[rootVC setNeedsStatusBarAppearanceUpdate];
}
#endif

// MARK: Orientation

#ifdef IOS_ENABLED
- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];

	[coordinator animateAlongsideTransition:nil
								 completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
									 // Get the new interface orientation after rotation completes (iOS only)
									 UIInterfaceOrientation orientation = self.view.window.windowScene.interfaceOrientation;

									 // Notify camera server of orientation change
									 if (CameraServer::is_initialized()) {
										 CameraServer::handle_display_rotation_change((int)orientation);
									 }
								 }];
}
#endif

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
	if (GLOBAL_GET("display/window/ios/suppress_ui_gesture")) {
		return UIRectEdgeAll;
	} else {
		return UIRectEdgeNone;
	}
}

- (BOOL)shouldAutorotate {
	if (!DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
		return NO;
	}

	switch (DisplayServer::driver_as<DisplayServerAppleEmbedded>()->screen_get_orientation(DisplayServerEnums::SCREEN_OF_MAIN_WINDOW)) {
		case DisplayServerEnums::SCREEN_SENSOR:
		case DisplayServerEnums::SCREEN_SENSOR_LANDSCAPE:
		case DisplayServerEnums::SCREEN_SENSOR_PORTRAIT:
			return YES;
		default:
			return NO;
	}
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
	if (!DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
		return UIInterfaceOrientationMaskAll;
	}

	switch (DisplayServer::driver_as<DisplayServerAppleEmbedded>()->screen_get_orientation(DisplayServerEnums::SCREEN_OF_MAIN_WINDOW)) {
		case DisplayServerEnums::SCREEN_PORTRAIT:
			return UIInterfaceOrientationMaskPortrait;
		case DisplayServerEnums::SCREEN_REVERSE_LANDSCAPE:
			if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
				return UIInterfaceOrientationMaskLandscapeLeft;
			} else {
				return UIInterfaceOrientationMaskLandscapeRight;
			}
		case DisplayServerEnums::SCREEN_REVERSE_PORTRAIT:
			return UIInterfaceOrientationMaskPortraitUpsideDown;
		case DisplayServerEnums::SCREEN_SENSOR_LANDSCAPE:
			return UIInterfaceOrientationMaskLandscape;
		case DisplayServerEnums::SCREEN_SENSOR_PORTRAIT:
			return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
		case DisplayServerEnums::SCREEN_SENSOR:
			return UIInterfaceOrientationMaskAll;
		case DisplayServerEnums::SCREEN_LANDSCAPE:
			if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
				return UIInterfaceOrientationMaskLandscapeRight;
			} else {
				return UIInterfaceOrientationMaskLandscapeLeft;
			}
	}
}

- (BOOL)prefersStatusBarHidden {
	if (GLOBAL_GET("display/window/ios/hide_status_bar")) {
		return YES;
	} else {
		return NO;
	}
}

- (BOOL)prefersHomeIndicatorAutoHidden {
	if (GLOBAL_GET("display/window/ios/hide_home_indicator")) {
		return YES;
	} else {
		return NO;
	}
}

// MARK: Keyboard

- (void)keyboardOnScreen:(NSNotification *)notification {
	NSDictionary *info = notification.userInfo;
	NSValue *value = info[UIKeyboardFrameEndUserInfoKey];

	CGRect rawFrame = [value CGRectValue];
	CGRect keyboardFrame = [self.view convertRect:rawFrame fromView:nil];

	if (DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->virtual_keyboard_set_height(keyboardFrame.size.height);
	}
}

- (void)keyboardHidden:(NSNotification *)notification {
	if (DisplayServer::driver_as<DisplayServerAppleEmbedded>()) {
		DisplayServer::driver_as<DisplayServerAppleEmbedded>()->virtual_keyboard_set_height(0);
	}
}

@end

// === Merged from drivers/apple_embedded/godot_view_renderer.mm ===
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

#import "drivers/unix/os_unix.h"

#include "core/os/os.h"
#import "drivers/unix/os_unix.h"
#include "servers/display/display_server.h"

#import <AudioToolbox/AudioServices.h>
#import <CoreMotion/CoreMotion.h>
#import <GameController/GameController.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

@interface GDTViewRenderer ()

@property(assign, nonatomic) BOOL hasFinishedProjectDataSetup;
@property(assign, nonatomic) BOOL hasStartedMain;
@property(assign, nonatomic) BOOL hasFinishedSetup;

@end

@implementation GDTViewRenderer

- (BOOL)setupView:(UIView *)view {
	if (self.hasFinishedSetup) {
		return NO;
	}

	if (!OS::is_initialized()) {
		exit(0);
	}

	if (!self.hasFinishedProjectDataSetup) {
		[self setupProjectData];
		return YES;
	}

	if (!self.hasStartedMain) {
		self.hasStartedMain = YES;
		OS_AppleEmbedded::start();
		return YES;
	}

	self.hasFinishedSetup = YES;

	return NO;
}

- (void)setupProjectData {
	self.hasFinishedProjectDataSetup = YES;

	Main::setup2();
}

- (void)renderOnView:(UIView *)view {
	if (!OS_AppleEmbedded::is_initialized()) {
		return;
	}

	OS_AppleEmbedded::iterate();
}

@end

// === Merged from drivers/apple_embedded/key_mapping_apple_embedded.mm ===
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

#import "drivers/unix/os_unix.h"

#include "core/types.h"

struct HashMapHasherKeys {
	static _FORCE_INLINE_ uint32_t hash(const Key p_key) { return hash_fmix32(static_cast<uint32_t>(p_key)); }
	static _FORCE_INLINE_ uint32_t hash(const CFIndex p_key) { return hash_fmix32(p_key); }
};

HashMap<CFIndex, Key, HashMapHasherKeys> keyusage_map;
HashMap<CFIndex, KeyLocation, HashMapHasherKeys> location_map;

void KeyMappingAppleEmbedded::initialize() {
	if (@available(iOS 13.4, *)) {
		keyusage_map[UIKeyboardHIDUsageKeyboardA] = Key::A;
		keyusage_map[UIKeyboardHIDUsageKeyboardB] = Key::B;
		keyusage_map[UIKeyboardHIDUsageKeyboardC] = Key::C;
		keyusage_map[UIKeyboardHIDUsageKeyboardD] = Key::D;
		keyusage_map[UIKeyboardHIDUsageKeyboardE] = Key::E;
		keyusage_map[UIKeyboardHIDUsageKeyboardF] = Key::F;
		keyusage_map[UIKeyboardHIDUsageKeyboardG] = Key::G;
		keyusage_map[UIKeyboardHIDUsageKeyboardH] = Key::H;
		keyusage_map[UIKeyboardHIDUsageKeyboardI] = Key::I;
		keyusage_map[UIKeyboardHIDUsageKeyboardJ] = Key::J;
		keyusage_map[UIKeyboardHIDUsageKeyboardK] = Key::K;
		keyusage_map[UIKeyboardHIDUsageKeyboardL] = Key::L;
		keyusage_map[UIKeyboardHIDUsageKeyboardM] = Key::M;
		keyusage_map[UIKeyboardHIDUsageKeyboardN] = Key::N;
		keyusage_map[UIKeyboardHIDUsageKeyboardO] = Key::O;
		keyusage_map[UIKeyboardHIDUsageKeyboardP] = Key::P;
		keyusage_map[UIKeyboardHIDUsageKeyboardQ] = Key::Q;
		keyusage_map[UIKeyboardHIDUsageKeyboardR] = Key::R;
		keyusage_map[UIKeyboardHIDUsageKeyboardS] = Key::S;
		keyusage_map[UIKeyboardHIDUsageKeyboardT] = Key::T;
		keyusage_map[UIKeyboardHIDUsageKeyboardU] = Key::U;
		keyusage_map[UIKeyboardHIDUsageKeyboardV] = Key::V;
		keyusage_map[UIKeyboardHIDUsageKeyboardW] = Key::W;
		keyusage_map[UIKeyboardHIDUsageKeyboardX] = Key::X;
		keyusage_map[UIKeyboardHIDUsageKeyboardY] = Key::Y;
		keyusage_map[UIKeyboardHIDUsageKeyboardZ] = Key::Z;
		keyusage_map[UIKeyboardHIDUsageKeyboard0] = Key::KEY_0;
		keyusage_map[UIKeyboardHIDUsageKeyboard1] = Key::KEY_1;
		keyusage_map[UIKeyboardHIDUsageKeyboard2] = Key::KEY_2;
		keyusage_map[UIKeyboardHIDUsageKeyboard3] = Key::KEY_3;
		keyusage_map[UIKeyboardHIDUsageKeyboard4] = Key::KEY_4;
		keyusage_map[UIKeyboardHIDUsageKeyboard5] = Key::KEY_5;
		keyusage_map[UIKeyboardHIDUsageKeyboard6] = Key::KEY_6;
		keyusage_map[UIKeyboardHIDUsageKeyboard7] = Key::KEY_7;
		keyusage_map[UIKeyboardHIDUsageKeyboard8] = Key::KEY_8;
		keyusage_map[UIKeyboardHIDUsageKeyboard9] = Key::KEY_9;
		keyusage_map[UIKeyboardHIDUsageKeyboardBackslash] = Key::BACKSLASH;
		keyusage_map[UIKeyboardHIDUsageKeyboardCloseBracket] = Key::BRACKETRIGHT;
		keyusage_map[UIKeyboardHIDUsageKeyboardComma] = Key::COMMA;
		keyusage_map[UIKeyboardHIDUsageKeyboardEqualSign] = Key::EQUAL;
		keyusage_map[UIKeyboardHIDUsageKeyboardHyphen] = Key::MINUS;
		keyusage_map[UIKeyboardHIDUsageKeyboardNonUSBackslash] = Key::SECTION;
		keyusage_map[UIKeyboardHIDUsageKeyboardNonUSPound] = Key::ASCIITILDE;
		keyusage_map[UIKeyboardHIDUsageKeyboardOpenBracket] = Key::BRACKETLEFT;
		keyusage_map[UIKeyboardHIDUsageKeyboardPeriod] = Key::PERIOD;
		keyusage_map[UIKeyboardHIDUsageKeyboardQuote] = Key::QUOTEDBL;
		keyusage_map[UIKeyboardHIDUsageKeyboardSemicolon] = Key::SEMICOLON;
		keyusage_map[UIKeyboardHIDUsageKeyboardSeparator] = Key::SECTION;
		keyusage_map[UIKeyboardHIDUsageKeyboardSlash] = Key::SLASH;
		keyusage_map[UIKeyboardHIDUsageKeyboardSpacebar] = Key::SPACE;
		keyusage_map[UIKeyboardHIDUsageKeyboardCapsLock] = Key::CAPSLOCK;
		keyusage_map[UIKeyboardHIDUsageKeyboardLeftAlt] = Key::ALT;
		keyusage_map[UIKeyboardHIDUsageKeyboardLeftControl] = Key::CTRL;
		keyusage_map[UIKeyboardHIDUsageKeyboardLeftShift] = Key::SHIFT;
		keyusage_map[UIKeyboardHIDUsageKeyboardRightAlt] = Key::ALT;
		keyusage_map[UIKeyboardHIDUsageKeyboardRightControl] = Key::CTRL;
		keyusage_map[UIKeyboardHIDUsageKeyboardRightShift] = Key::SHIFT;
		keyusage_map[UIKeyboardHIDUsageKeyboardScrollLock] = Key::SCROLLLOCK;
		keyusage_map[UIKeyboardHIDUsageKeyboardLeftArrow] = Key::LEFT;
		keyusage_map[UIKeyboardHIDUsageKeyboardRightArrow] = Key::RIGHT;
		keyusage_map[UIKeyboardHIDUsageKeyboardUpArrow] = Key::UP;
		keyusage_map[UIKeyboardHIDUsageKeyboardDownArrow] = Key::DOWN;
		keyusage_map[UIKeyboardHIDUsageKeyboardPageUp] = Key::PAGEUP;
		keyusage_map[UIKeyboardHIDUsageKeyboardPageDown] = Key::PAGEDOWN;
		keyusage_map[UIKeyboardHIDUsageKeyboardHome] = Key::HOME;
		keyusage_map[UIKeyboardHIDUsageKeyboardEnd] = Key::END;
		keyusage_map[UIKeyboardHIDUsageKeyboardDeleteForward] = Key::KEY_DELETE;
		keyusage_map[UIKeyboardHIDUsageKeyboardDeleteOrBackspace] = Key::BACKSPACE;
		keyusage_map[UIKeyboardHIDUsageKeyboardEscape] = Key::ESCAPE;
		keyusage_map[UIKeyboardHIDUsageKeyboardInsert] = Key::INSERT;
		keyusage_map[UIKeyboardHIDUsageKeyboardReturn] = Key::ENTER;
		keyusage_map[UIKeyboardHIDUsageKeyboardTab] = Key::TAB;
		keyusage_map[UIKeyboardHIDUsageKeyboardF1] = Key::F1;
		keyusage_map[UIKeyboardHIDUsageKeyboardF2] = Key::F2;
		keyusage_map[UIKeyboardHIDUsageKeyboardF3] = Key::F3;
		keyusage_map[UIKeyboardHIDUsageKeyboardF4] = Key::F4;
		keyusage_map[UIKeyboardHIDUsageKeyboardF5] = Key::F5;
		keyusage_map[UIKeyboardHIDUsageKeyboardF6] = Key::F6;
		keyusage_map[UIKeyboardHIDUsageKeyboardF7] = Key::F7;
		keyusage_map[UIKeyboardHIDUsageKeyboardF8] = Key::F8;
		keyusage_map[UIKeyboardHIDUsageKeyboardF9] = Key::F9;
		keyusage_map[UIKeyboardHIDUsageKeyboardF10] = Key::F10;
		keyusage_map[UIKeyboardHIDUsageKeyboardF11] = Key::F11;
		keyusage_map[UIKeyboardHIDUsageKeyboardF12] = Key::F12;
		keyusage_map[UIKeyboardHIDUsageKeyboardF13] = Key::F13;
		keyusage_map[UIKeyboardHIDUsageKeyboardF14] = Key::F14;
		keyusage_map[UIKeyboardHIDUsageKeyboardF15] = Key::F15;
		keyusage_map[UIKeyboardHIDUsageKeyboardF16] = Key::F16;
		keyusage_map[UIKeyboardHIDUsageKeyboardF17] = Key::F17;
		keyusage_map[UIKeyboardHIDUsageKeyboardF18] = Key::F18;
		keyusage_map[UIKeyboardHIDUsageKeyboardF19] = Key::F19;
		keyusage_map[UIKeyboardHIDUsageKeyboardF20] = Key::F20;
		keyusage_map[UIKeyboardHIDUsageKeyboardF21] = Key::F21;
		keyusage_map[UIKeyboardHIDUsageKeyboardF22] = Key::F22;
		keyusage_map[UIKeyboardHIDUsageKeyboardF23] = Key::F23;
		keyusage_map[UIKeyboardHIDUsageKeyboardF24] = Key::F24;
		keyusage_map[UIKeyboardHIDUsageKeypad0] = Key::KP_0;
		keyusage_map[UIKeyboardHIDUsageKeypad1] = Key::KP_1;
		keyusage_map[UIKeyboardHIDUsageKeypad2] = Key::KP_2;
		keyusage_map[UIKeyboardHIDUsageKeypad3] = Key::KP_3;
		keyusage_map[UIKeyboardHIDUsageKeypad4] = Key::KP_4;
		keyusage_map[UIKeyboardHIDUsageKeypad5] = Key::KP_5;
		keyusage_map[UIKeyboardHIDUsageKeypad6] = Key::KP_6;
		keyusage_map[UIKeyboardHIDUsageKeypad7] = Key::KP_7;
		keyusage_map[UIKeyboardHIDUsageKeypad8] = Key::KP_8;
		keyusage_map[UIKeyboardHIDUsageKeypad9] = Key::KP_9;
		keyusage_map[UIKeyboardHIDUsageKeypadAsterisk] = Key::KP_MULTIPLY;
		keyusage_map[UIKeyboardHIDUsageKeyboardGraveAccentAndTilde] = Key::BAR;
		keyusage_map[UIKeyboardHIDUsageKeypadEnter] = Key::KP_ENTER;
		keyusage_map[UIKeyboardHIDUsageKeypadHyphen] = Key::KP_SUBTRACT;
		keyusage_map[UIKeyboardHIDUsageKeypadNumLock] = Key::NUMLOCK;
		keyusage_map[UIKeyboardHIDUsageKeypadPeriod] = Key::KP_PERIOD;
		keyusage_map[UIKeyboardHIDUsageKeypadPlus] = Key::KP_ADD;
		keyusage_map[UIKeyboardHIDUsageKeypadSlash] = Key::KP_DIVIDE;
		keyusage_map[UIKeyboardHIDUsageKeyboardPause] = Key::PAUSE;
		keyusage_map[UIKeyboardHIDUsageKeyboardStop] = Key::STOP;
		keyusage_map[UIKeyboardHIDUsageKeyboardMute] = Key::VOLUMEMUTE;
		keyusage_map[UIKeyboardHIDUsageKeyboardVolumeUp] = Key::VOLUMEUP;
		keyusage_map[UIKeyboardHIDUsageKeyboardVolumeDown] = Key::VOLUMEDOWN;
		keyusage_map[UIKeyboardHIDUsageKeyboardFind] = Key::SEARCH;
		keyusage_map[UIKeyboardHIDUsageKeyboardHelp] = Key::HELP;
		keyusage_map[UIKeyboardHIDUsageKeyboardLeftGUI] = Key::META;
		keyusage_map[UIKeyboardHIDUsageKeyboardRightGUI] = Key::META;
		keyusage_map[UIKeyboardHIDUsageKeyboardMenu] = Key::MENU;
		keyusage_map[UIKeyboardHIDUsageKeyboardPrintScreen] = Key::PRINT;
		keyusage_map[UIKeyboardHIDUsageKeyboardReturnOrEnter] = Key::ENTER;
		keyusage_map[UIKeyboardHIDUsageKeyboardSysReqOrAttention] = Key::SYSREQ;
		keyusage_map[0x01AE] = Key::KEYBOARD; // On-screen keyboard key on smart connector keyboard.
		keyusage_map[0x029D] = Key::GLOBE; // "Globe" key on smart connector / Mac keyboard.
		keyusage_map[UIKeyboardHIDUsageKeyboardLANG1] = Key::JIS_EISU;
		keyusage_map[UIKeyboardHIDUsageKeyboardLANG2] = Key::JIS_KANA;

		location_map[UIKeyboardHIDUsageKeyboardLeftAlt] = KeyLocation::LEFT;
		location_map[UIKeyboardHIDUsageKeyboardRightAlt] = KeyLocation::RIGHT;
		location_map[UIKeyboardHIDUsageKeyboardLeftControl] = KeyLocation::LEFT;
		location_map[UIKeyboardHIDUsageKeyboardRightControl] = KeyLocation::RIGHT;
		location_map[UIKeyboardHIDUsageKeyboardLeftShift] = KeyLocation::LEFT;
		location_map[UIKeyboardHIDUsageKeyboardRightShift] = KeyLocation::RIGHT;
		location_map[UIKeyboardHIDUsageKeyboardLeftGUI] = KeyLocation::LEFT;
		location_map[UIKeyboardHIDUsageKeyboardRightGUI] = KeyLocation::RIGHT;
	}
}

Key KeyMappingAppleEmbedded::remap_key(CFIndex p_keycode) {
	if (@available(iOS 13.4, *)) {
		const Key *key = keyusage_map.getptr(p_keycode);
		if (key) {
			return *key;
		}
	}
	return Key::NONE;
}

KeyLocation KeyMappingAppleEmbedded::key_location(CFIndex p_keycode) {
	if (@available(iOS 13.4, *)) {
		const KeyLocation *location = location_map.getptr(p_keycode);
		if (location) {
			return *location;
		}
	}
	return KeyLocation::UNSPECIFIED;
}

// === Merged from drivers/apple_embedded/rendering_context_driver_vulkan_apple_embedded.mm ===
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

#import "drivers/unix/os_unix.h"

#ifdef VULKAN_ENABLED

#ifdef USE_VOLK
#include <volk.h>
#else
#include <vulkan/vulkan_metal.h>
#endif

const char *RenderingContextDriverVulkanAppleEmbedded::_get_platform_surface_extension() const {
	return VK_EXT_METAL_SURFACE_EXTENSION_NAME;
}

RenderingContextDriver::SurfaceID RenderingContextDriverVulkanAppleEmbedded::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	VkMetalSurfaceCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
	create_info.pLayer = *wpd->layer_ptr;

	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkResult err = vkCreateMetalSurfaceEXT(instance_get(), &create_info, get_allocation_callbacks(VK_OBJECT_TYPE_SURFACE_KHR), &vk_surface);
	ERR_FAIL_COND_V(err != VK_SUCCESS, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->vk_surface = vk_surface;
	return SurfaceID(surface);
}

RenderingContextDriverVulkanAppleEmbedded::RenderingContextDriverVulkanAppleEmbedded() {
	// Does nothing.
}

RenderingContextDriverVulkanAppleEmbedded::~RenderingContextDriverVulkanAppleEmbedded() {
	// Does nothing.
}

#endif // VULKAN_ENABLED
