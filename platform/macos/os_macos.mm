/**************************************************************************/
/*  os_macos.mm                                                           */
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

#import "os_macos.h"

#import "platform/macos/os_macos.h"

#ifdef TOOLS_ENABLED
#import "platform/macos/os_macos.h"
#endif

#include "core/config/engine.h"
#include "core/crypto/crypto_core.h"
#include "core/input/input.h"
#include "core/io/file_access.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/profiling/profiling.h"
#include "core/version_generated.gen.h"
#include "drivers/apple/os_log_logger.h"
#include "main/main.h"

#ifdef SDL_ENABLED
#include "core/input/input.h"
#endif

#include <dlfcn.h>
#include <libproc.h>
#import <mach-o/dyld.h>
#include <os/log.h>
#include <sys/sysctl.h>

void OS_MacOS::add_frame_delay(bool p_can_draw, bool p_wake_for_events) {
	bool wake_for_events = p_wake_for_events;
	if (!wake_for_events) {
		NSArray<NSRunningApplication *> *proc_array = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.crowdcafe.windowmagnet"];
		if (proc_array && proc_array.count > 0) {
			wake_for_events = true;
		}
	}
	if (wake_for_events) {
		uint64_t delay = get_frame_delay(p_can_draw);
		if (delay == 0) {
			return;
		}
		if (wait_timer) {
			CFRunLoopTimerInvalidate(wait_timer);
			CFRelease(wait_timer);
		}
		wait_timer = CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + (double(delay) / 1000000.0), 0, 0, 0,
				^(CFRunLoopTimerRef timer) {
					CFRunLoopTimerInvalidate(wait_timer);
					CFRelease(wait_timer);
					wait_timer = nil;
				});
		CFRunLoopAddTimer(CFRunLoopGetCurrent(), wait_timer, kCFRunLoopCommonModes);
		return;
	}
	OS_Unix::add_frame_delay(p_can_draw, wake_for_events);
}

void OS_MacOS::initialize() {
	crash_handler.initialize();

	initialize_core();

#ifdef COREMIDI_ENABLED
	MIDIDriver::bind<MIDIDriverCoreMidi>();
	MIDIDriver::initialize();
#endif
}

String OS_MacOS::get_model_name() const {
	char buffer[256];
	size_t buffer_len = 256;
	if (sysctlbyname("hw.model", &buffer, &buffer_len, nullptr, 0) == 0 && buffer_len != 0) {
		return String::utf8(buffer, buffer_len);
	}
	return OS_Unix::get_model_name();
}

String OS_MacOS::get_processor_name() const {
	char buffer[256];
	size_t buffer_len = 256;
	if (sysctlbyname("machdep.cpu.brand_string", &buffer, &buffer_len, nullptr, 0) == 0) {
		return String::utf8(buffer, buffer_len);
	}
	ERR_FAIL_V_MSG("", String("Couldn't get the CPU model name. Returning an empty string."));
}

bool OS_MacOS::is_sandboxed() const {
	return has_environment("APP_SANDBOX_CONTAINER_ID");
}

bool OS_MacOS::request_permission(const String &p_name) {
	if (p_name == "macos.permission.RECORD_SCREEN") {
		if (CGPreflightScreenCaptureAccess()) {
			return true;
		} else {
			CGRequestScreenCaptureAccess();
			return false;
		}
	}
	return false;
}

Vector<String> OS_MacOS::get_granted_permissions() const {
	Vector<String> ret;

	if (CGPreflightScreenCaptureAccess()) {
		ret.push_back("macos.permission.RECORD_SCREEN");
	}

	if (is_sandboxed()) {
		NSArray *bookmarks = [[NSUserDefaults standardUserDefaults] arrayForKey:@"sec_bookmarks"];
		for (id bookmark in bookmarks) {
			NSError *error = nil;
			BOOL isStale = NO;
			NSURL *url = [NSURL URLByResolvingBookmarkData:bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&error];
			if (!error && !isStale) {
				String url_string = String::utf8([[url path] UTF8String]);
				ret.push_back(url_string);
			}
		}
	}

	return ret;
}

void OS_MacOS::revoke_granted_permissions() {
	if (is_sandboxed()) {
		[[NSUserDefaults standardUserDefaults] setObject:nil forKey:@"sec_bookmarks"];
	}
}

#if TOOLS_ENABLED

// Function to check if a debugger is attached to the current process
bool OS_MacOS::is_debugger_attached() {
	int mib[4];
	struct kinfo_proc info{};
	size_t size = sizeof(info);

	// Initialize the flags so that, if sysctl fails, info.kp_proc.p_flag will be 0.
	info.kp_proc.p_flag = 0;

	// Initialize mib, which tells sysctl the info we want, in this case we're looking for information
	// about a specific process ID.
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	mib[3] = getpid();

	if (sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, nullptr, 0) != 0) {
		perror("sysctl");
		return false;
	}

	return (info.kp_proc.p_flag & P_TRACED) != 0;
}

void OS_MacOS::wait_for_debugger(uint32_t p_msec) {
	if (p_msec == 0) {
		return;
	}

	CFAbsoluteTime start = CFAbsoluteTimeGetCurrent();
	CFTimeInterval wait_time = p_msec / 1000.0;

	NSTimer *timer = [NSTimer timerWithTimeInterval:0.100
											repeats:YES
											  block:^(NSTimer *t) {
												  if (is_debugger_attached() || CFAbsoluteTimeGetCurrent() > start + wait_time) {
													  [NSApp stopModalWithCode:NSModalResponseContinue];
													  [t invalidate];
												  }
											  }];

	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSModalPanelRunLoopMode];

	pid_t pid = getpid();
	alert(vformat("Attach debugger to pid: %d", pid));

	print("continue...");
}

#endif

void OS_MacOS::initialize_core() {
	OS_Unix::initialize_core();

	DirAccess::make_default<DirAccessMacOS>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessMacOS>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessMacOS>(DirAccess::ACCESS_FILESYSTEM);
}

void OS_MacOS::finalize() {
	if (is_sandboxed()) {
		NSArray *bookmarks = [[NSUserDefaults standardUserDefaults] arrayForKey:@"sec_bookmarks"];
		for (id bookmark in bookmarks) {
			NSError *error = nil;
			BOOL isStale = NO;
			NSURL *url = [NSURL URLByResolvingBookmarkData:bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&error];
			if (!error && !isStale) {
				[url stopAccessingSecurityScopedResource];
			}
		}
	}

#ifdef COREMIDI_ENABLED
	MIDIDriver::finalize();
	MIDIDriver::reset_backend();
#endif

	delete_main_loop();

#ifdef SDL_ENABLED
	memdelete(joypad_sdl);
#endif
}

void OS_MacOS::initialize_joypads() {
#ifdef SDL_ENABLED
	joypad_sdl = memnew(JoypadSDL());
	if (joypad_sdl->initialize() != OK) {
		ERR_PRINT("Couldn't initialize SDL joypad input driver.");
		memdelete(joypad_sdl);
		joypad_sdl = nullptr;
	}
#endif
}

void OS_MacOS::delete_main_loop() {
	MainLoop::clear();
}

void OS_MacOS::set_cmdline_platform_args(const List<String> &p_args) {
	launch_service_args = p_args;
}

List<String> OS_MacOS::get_cmdline_platform_args() const {
	return List<String>(launch_service_args);
}

void OS_MacOS::load_shell_environment() const {
	static bool shell_env_loaded = false;
	if (unlikely(!shell_env_loaded)) {
		shell_env_loaded = true;
		if (OS::has_environment("TERM") || OS::has_environment("__GODOT_SHELL_ENV_SET")) {
			return; // Already started from terminal, or other the instance with the shell environment, do nothing.
		}
		String pipe;
		List<String> args;
		args.push_back("-c");
		args.push_back(". /etc/zshrc;. /etc/zprofile;. ~/.zshenv;. ~/.zshrc;. ~/.zprofile;env");
		Error err = OS::execute("zsh", args, &pipe);
		if (err == OK) {
			Vector<String> env_vars = pipe.split("\n");
			for (const String &E : env_vars) {
				Vector<String> tags = E.split("=", 2);
				if (tags.size() != 2 || tags[0] == "SHELL" || tags[0] == "USER" || tags[0] == "COMMAND_MODE" || tags[0] == "TMPDIR" || tags[0] == "TERM_SESSION_ID" || tags[0] == "PWD" || tags[0] == "OLDPWD" || tags[0] == "SHLVL" || tags[0] == "HOME" || tags[0] == "DISPLAY" || tags[0] == "LOGNAME" || tags[0] == "TERM" || tags[0] == "COLORTERM" || tags[0] == "_" || tags[0].begins_with("__CF") || tags[0].begins_with("XPC_") || tags[0].begins_with("__GODOT")) {
					continue;
				}
				OS::set_environment(tags[0], tags[1]);
			}
		}
		OS::set_environment("__GODOT_SHELL_ENV_SET", "1");
	}
}

String OS_MacOS::get_name() const {
	return "macOS";
}

String OS_MacOS::get_distribution_name() const {
	return get_name();
}

String OS_MacOS::get_version() const {
	NSOperatingSystemVersion ver = [NSProcessInfo processInfo].operatingSystemVersion;
	return vformat("%d.%d.%d", (int64_t)ver.majorVersion, (int64_t)ver.minorVersion, (int64_t)ver.patchVersion);
}

String OS_MacOS::get_version_alias() const {
	NSOperatingSystemVersion ver = [NSProcessInfo processInfo].operatingSystemVersion;
	String macos_string;
	if (ver.majorVersion == 26) {
		macos_string += "Tahoe";
	} else if (ver.majorVersion == 15) {
		macos_string += "Sequoia";
	} else if (ver.majorVersion == 14) {
		macos_string += "Sonoma";
	} else if (ver.majorVersion == 13) {
		macos_string += "Ventura";
	} else if (ver.majorVersion == 12) {
		macos_string += "Monterey";
	} else if (ver.majorVersion == 11 || (ver.majorVersion == 10 && ver.minorVersion == 16)) {
		// Big Sur was 10.16 during beta, but it became 11 for the stable version.
		macos_string += "Big Sur";
	} else if (ver.majorVersion == 10 && ver.minorVersion == 15) {
		macos_string += "Catalina";
	} else if (ver.majorVersion == 10 && ver.minorVersion == 14) {
		macos_string += "Mojave";
	} else if (ver.majorVersion == 10 && ver.minorVersion == 13) {
		macos_string += "High Sierra";
	} else {
		macos_string += "Unknown";
	}
	// macOS versions older than 10.13 cannot run Godot.
	return vformat("%s (%s)", macos_string, get_version());
}

void OS_MacOS::alert(const String &p_alert, const String &p_title) {
	NSAlert *window = [[NSAlert alloc] init];
	NSString *ns_title = [NSString stringWithUTF8String:p_title.utf8().get_data()];
	NSString *ns_alert = [NSString stringWithUTF8String:p_alert.utf8().get_data()];

	NSTextField *text_field = [NSTextField labelWithString:ns_alert];
	[text_field setAlignment:NSTextAlignmentCenter];
	[window addButtonWithTitle:@"OK"];
	[window setMessageText:ns_title];
	[window setAccessoryView:text_field];
	[window setAlertStyle:NSAlertStyleWarning];

	id key_window = [[NSApplication sharedApplication] keyWindow];
	[window runModal];
	if (key_window) {
		[key_window makeKeyAndOrderFront:nil];
	}
}

_FORCE_INLINE_ String OS_MacOS::get_framework_executable(const String &p_path) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	// Read framework bundle to get executable name.
	NSURL *url = [NSURL fileURLWithPath:@(p_path.utf8().get_data())];
	if (url) {
		NSBundle *bundle = [NSBundle bundleWithURL:url];
		if (bundle) {
			String exe_path = String::utf8([[bundle executablePath] UTF8String]);
			if (da->file_exists(exe_path)) {
				return exe_path;
			}
		}
	}

	// Try default executable name (invalid framework).
	if (da->dir_exists(p_path) && da->file_exists(p_path.path_join(p_path.get_file().get_basename()))) {
		return p_path.path_join(p_path.get_file().get_basename());
	}

	// Not a framework, try loading as .dylib.
	return p_path;
}

Error OS_MacOS::open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data) {
	String path = get_framework_executable(p_path);

	if (!FileAccess::exists(path)) {
		// Load .dylib or framework from within the executable path.
		path = get_framework_executable(get_executable_path().get_base_dir().path_join(p_path.get_file()));
	}

	if (!FileAccess::exists(path)) {
		// Load .dylib or framework from a standard macOS location.
		path = get_framework_executable(get_executable_path().get_base_dir().path_join("../Frameworks").path_join(p_path.get_file()));
	}

	if (!FileAccess::exists(path)) {
		// Try using path as is. macOS system libraries with `/usr/lib/*` path do not exist as physical files and are loaded from shared cache.
		path = p_path;
	}

	p_library_handle = dlopen(path.utf8().get_data(), RTLD_NOW);
	ERR_FAIL_NULL_V_MSG(p_library_handle, ERR_CANT_OPEN, vformat("Can't open dynamic library: %s. Error: %s.", p_path, dlerror()));

	if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
		*p_data->r_resolved_path = path;
	}

	return OK;
}

String OS_MacOS::get_config_path() const {
	if (has_environment("HOME")) {
		return get_environment("HOME").path_join("Library/Application Support");
	}
	return ".";
}

String OS_MacOS::get_data_path() const {
	return get_config_path();
}

String OS_MacOS::get_cache_path() const {
	if (has_environment("HOME")) {
		return get_environment("HOME").path_join("Library/Caches");
	}
	return get_config_path();
}

String OS_MacOS::get_temp_path() const {
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

String OS_MacOS::get_bundle_resource_dir() const {
	String ret;

	NSBundle *main = [NSBundle mainBundle];
	if (main) {
		NSString *resource_path = [main resourcePath];
		ret.append_utf8([resource_path UTF8String]);
	}
	return ret;
}

String OS_MacOS::get_bundle_icon_path() const {
	String ret;

	NSBundle *main = [NSBundle mainBundle];
	if (main) {
		NSString *icon_path = [[main infoDictionary] objectForKey:@"CFBundleIconFile"];
		if (icon_path) {
			ret.append_utf8([icon_path UTF8String]);
		}
	}
	return ret;
}

String OS_MacOS::get_bundle_icon_name() const {
	String ret;

	NSBundle *main = [NSBundle mainBundle];
	if (main) {
		NSString *icon_name = [[main infoDictionary] objectForKey:@"CFBundleIconName"];
		if (icon_name) {
			ret.append_utf8([icon_name UTF8String]);
		}
	}
	return ret;
}

// Get properly capitalized engine name for system paths
String OS_MacOS::get_godot_dir_name() const {
	return String(VLTR_VERSION_SHORT_NAME).capitalize();
}

String OS_MacOS::get_system_dir(SystemDir p_dir, bool p_shared_storage) const {
	NSSearchPathDirectory id;
	bool found = true;

	switch (p_dir) {
		case SYSTEM_DIR_DESKTOP: {
			id = NSDesktopDirectory;
		} break;
		case SYSTEM_DIR_DOCUMENTS: {
			id = NSDocumentDirectory;
		} break;
		case SYSTEM_DIR_DOWNLOADS: {
			id = NSDownloadsDirectory;
		} break;
		case SYSTEM_DIR_MOVIES: {
			id = NSMoviesDirectory;
		} break;
		case SYSTEM_DIR_MUSIC: {
			id = NSMusicDirectory;
		} break;
		case SYSTEM_DIR_PICTURES: {
			id = NSPicturesDirectory;
		} break;
		default: {
			found = false;
		}
	}

	String ret;
	if (found) {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(id, NSUserDomainMask, YES);
		if (paths && [paths count] >= 1) {
			ret.append_utf8([[paths firstObject] UTF8String]);
		}
	}

	return ret;
}

Error OS_MacOS::shell_show_in_file_manager(String p_path, bool p_open_folder) {
	bool open_folder = false;
	if (p_open_folder) {
		Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (dir->dir_exists(p_path) && !dir->is_bundle(p_path)) {
			open_folder = true;
		}
	}

	if (!p_path.begins_with("file://")) {
		p_path = String("file://") + p_path;
	}

	NSString *string = [NSString stringWithUTF8String:p_path.utf8().get_data()];
	NSURL *uri = [[NSURL alloc] initWithString:[string stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]]];

	if (open_folder) {
		[[NSWorkspace sharedWorkspace] openURL:uri];
	} else {
		[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[ uri ]];
	}
	return OK;
}

Error OS_MacOS::shell_open(const String &p_uri) {
	NSString *string = [NSString stringWithUTF8String:p_uri.utf8().get_data()];
	NSURL *uri = [[NSURL alloc] initWithString:string];
	if (!uri || !uri.scheme || [uri.scheme isEqual:@"file"]) {
		// No scheme set, assume "file://" and escape special characters.
		if (!p_uri.begins_with("file://")) {
			string = [NSString stringWithUTF8String:("file://" + p_uri).utf8().get_data()];
		}
		uri = [[NSURL alloc] initWithString:[string stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLFragmentAllowedCharacterSet]]];
	}
	[[NSWorkspace sharedWorkspace] openURL:uri];
	return OK;
}

String OS_MacOS::get_locale() const {
	NSString *locale_code = [[NSLocale preferredLanguages] objectAtIndex:0];
	return String([locale_code UTF8String]).replace_char('-', '_');
}

Vector<String> OS_MacOS::get_preferred_locales() const {
	Vector<String> out;
	for (NSString *locale_code in [NSLocale preferredLanguages]) {
		out.push_back(String([locale_code UTF8String]).replace_char('-', '_'));
	}
	return out;
}

Vector<String> OS_MacOS::get_system_fonts() const {
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

String OS_MacOS::_get_default_fontname(const String &p_font_name) const {
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

CGFloat OS_MacOS::_weight_to_ct(int p_weight) const {
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

CGFloat OS_MacOS::_stretch_to_ct(int p_stretch) const {
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

Vector<String> OS_MacOS::get_system_font_path_for_text(const String &p_font_name, const String &p_text, const String &p_locale, const String &p_script, int p_weight, int p_stretch, bool p_italic) const {
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

String OS_MacOS::get_system_font_path(const String &p_font_name, int p_weight, int p_stretch, bool p_italic) const {
	String ret;
	String font_name = _get_default_fontname(p_font_name);

	CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, font_name.utf8().get_data(), kCFStringEncodingUTF8);

	CTFontSymbolicTraits traits = 0;
	if (p_weight > 700) {
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

String OS_MacOS::get_executable_path() const {
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
	int pid = getpid();
	pid_t ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0) {
		return OS::get_executable_path();
	} else {
		return String::utf8(pathbuf);
	}
}

Error OS_MacOS::create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id, bool p_open_console) {
	// Use NSWorkspace if path is an .app bundle.
	NSURL *url = [NSURL fileURLWithPath:@(p_path.utf8().get_data())];
	NSBundle *bundle = [NSBundle bundleWithURL:url];
	if (bundle) {
		NSMutableArray *arguments = [[NSMutableArray alloc] init];
		for (const String &arg : p_arguments) {
			[arguments addObject:[NSString stringWithUTF8String:arg.utf8().get_data()]];
		}
		NSWorkspaceOpenConfiguration *configuration = [[NSWorkspaceOpenConfiguration alloc] init];
		[configuration setArguments:arguments];
		[configuration setCreatesNewApplicationInstance:YES];
		[configuration setEnvironment:[[NSProcessInfo processInfo] environment]];
		__block dispatch_semaphore_t lock = dispatch_semaphore_create(0);
		__block Error err = ERR_TIMEOUT;
		__block pid_t pid = 0;

		[[NSWorkspace sharedWorkspace] openApplicationAtURL:url
											  configuration:configuration
										  completionHandler:^(NSRunningApplication *app, NSError *error) {
											  if (error) {
												  err = ERR_CANT_FORK;
												  NSLog(@"Failed to execute: %@", error.localizedDescription);
											  } else {
												  pid = [app processIdentifier];
												  err = OK;
											  }
											  dispatch_semaphore_signal(lock);
										  }];
		dispatch_semaphore_wait(lock, dispatch_time(DISPATCH_TIME_NOW, 20000000000)); // 20 sec timeout, wait for app to launch.

		if (err == OK) {
			if (r_child_id) {
				*r_child_id = (ProcessID)pid;
			}
		}

		return err;
	} else {
		return OS_Unix::create_process(p_path, p_arguments, r_child_id, p_open_console);
	}
}

Error OS_MacOS::create_instance(const List<String> &p_arguments, ProcessID *r_child_id) {
	// Do not run headless instance as app bundle, since it will never send `applicationDidFinishLaunching` and register as failed start after timeout.
	for (size_t i = 0; i < std::size(OS_MacOS::headless_args); i++) {
		if (p_arguments.find(String(OS_MacOS::headless_args[i]))) {
			return OS_Unix::create_process(get_executable_path(), p_arguments, r_child_id, false);
		}
	}

	// If executable is bundled, always execute editor instances as an app bundle to ensure app window is registered and activated correctly.
	NSString *nsappname = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
	if (nsappname != nil) {
		String path = String::utf8([[[NSBundle mainBundle] bundlePath] UTF8String]);
#ifdef TOOLS_ENABLED
		if (!Engine::is_project_manager_hint() && !Engine::is_editor_hint()) {
			// Project started from the editor, inject "path" argument to set instance working directory.
			char cwd[PATH_MAX];
			if (::getcwd(cwd, sizeof(cwd)) != nullptr) {
				List<String> arguments(p_arguments);
				arguments.push_back("--path");
				arguments.push_back(String::utf8(cwd));
				return create_process(path, arguments, r_child_id, false);
			}
		}
#endif
		return create_process(path, p_arguments, r_child_id, false);
	} else {
		return create_process(get_executable_path(), p_arguments, r_child_id, false);
	}
}

Error OS_MacOS::open_with_program(const String &p_program_path, const List<String> &p_paths) {
	NSURL *app_url = [NSURL fileURLWithPath:@(p_program_path.utf8().get_data())];
	if (!app_url) {
		return ERR_INVALID_PARAMETER;
	}

	NSBundle *bundle = [NSBundle bundleWithURL:app_url];
	if (!bundle) {
		return OS_Unix::create_process(p_program_path, p_paths);
	}

	NSMutableArray *urls_to_open = [[NSMutableArray alloc] init];
	for (const String &path : p_paths) {
		NSURL *file_url = [NSURL fileURLWithPath:@(path.utf8().get_data())];
		if (file_url) {
			[urls_to_open addObject:file_url];
		}
	}

	if ([urls_to_open count] == 0) {
		return ERR_INVALID_PARAMETER;
	}

	NSWorkspaceOpenConfiguration *configuration = [[NSWorkspaceOpenConfiguration alloc] init];
	[configuration setCreatesNewApplicationInstance:NO];
	__block dispatch_semaphore_t lock = dispatch_semaphore_create(0);
	__block Error err = ERR_TIMEOUT;

	[[NSWorkspace sharedWorkspace] openURLs:urls_to_open
					   withApplicationAtURL:app_url
							  configuration:configuration
						  completionHandler:^(NSRunningApplication *app, NSError *error) {
							  if (error) {
								  err = ERR_CANT_FORK;
								  NSLog(@"Failed to open paths: %@", error.localizedDescription);
							  } else {
								  err = OK;
							  }
							  dispatch_semaphore_signal(lock);
						  }];
	dispatch_semaphore_wait(lock, dispatch_time(DISPATCH_TIME_NOW, 20000000000)); // 20 sec timeout, wait for app to launch.

	return err;
}

bool OS_MacOS::is_process_running(const ProcessID &p_pid) const {
	NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:(pid_t)p_pid];
	if (!app) {
		return OS_Unix::is_process_running(p_pid);
	}

	return ![app isTerminated];
}

String OS_MacOS::get_unique_id() const {
	static String serial_number;

	if (serial_number.is_empty()) {
#if defined(__x86_64) || defined(__x86_64__)
		io_service_t platform_expert = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
#else
		io_service_t platform_expert = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
#endif
		CFStringRef serial_number_cf_string = nullptr;
		if (platform_expert) {
			serial_number_cf_string = (CFStringRef)IORegistryEntryCreateCFProperty(platform_expert, CFSTR(kIOPlatformSerialNumberKey), kCFAllocatorDefault, 0);
			IOObjectRelease(platform_expert);
		}

		NSString *serial_number_ns_string = nil;
		if (serial_number_cf_string) {
			serial_number_ns_string = [NSString stringWithString:(__bridge NSString *)serial_number_cf_string];
			CFRelease(serial_number_cf_string);
		}

		if (serial_number_ns_string) {
			serial_number.append_utf8([serial_number_ns_string UTF8String]);
		}
	}

	return serial_number;
}

bool OS_MacOS::_check_internal_feature_support(const String &p_feature) {
	if (p_feature == "system_fonts") {
		return true;
	}
	if (p_feature == "pc") {
		return true;
	}

	return false;
}

void OS_MacOS::disable_crash_handler() {
	crash_handler.disable();
}

bool OS_MacOS::is_disable_crash_handler() const {
	return crash_handler.is_disabled();
}

Error OS_MacOS::move_to_trash(const String &p_path) {
	NSFileManager *fm = [NSFileManager defaultManager];
	NSURL *url = [NSURL fileURLWithPath:@(p_path.utf8().get_data())];
	NSError *err;

	if (![fm trashItemAtURL:url resultingItemURL:nil error:&err]) {
		ERR_PRINT("trashItemAtURL error: " + String::utf8(err.localizedDescription.UTF8String));
		return FAILED;
	}

	return OK;
}

String OS_MacOS::get_system_ca_certificates() {
	CFArrayRef result;
	SecCertificateRef item;
	CFDataRef der;

	OSStatus ret = SecTrustCopyAnchorCertificates(&result);
	ERR_FAIL_COND_V(ret != noErr, "");

	CFIndex l = CFArrayGetCount(result);
	String certs;
	PackedByteArray pba;
	for (CFIndex i = 0; i < l; i++) {
		item = (SecCertificateRef)CFArrayGetValueAtIndex(result, i);
		der = SecCertificateCopyData(item);
		int derlen = CFDataGetLength(der);
		if (pba.size() < derlen * 3) {
			pba.resize(derlen * 3);
		}
		size_t b64len = 0;
		Error err = CryptoCore::b64_encode(pba.ptrw(), pba.size(), &b64len, (unsigned char *)CFDataGetBytePtr(der), derlen);
		CFRelease(der);
		ERR_CONTINUE(err != OK);
		// Certificate is bas64 encoded, aka ascii.
		certs += "-----BEGIN CERTIFICATE-----\n" + String::ascii(Span((char *)pba.ptr(), b64len)) + "\n-----END CERTIFICATE-----\n";
	}
	CFRelease(result);
	return certs;
}

OS::PreferredTextureFormat OS_MacOS::get_preferred_texture_format() const {
	// macOS supports both formats on ARM. Prefer S3TC/BPTC
	// for better compatibility with x86 platforms.
	return PREFERRED_TEXTURE_FORMAT_S3TC_BPTC;
}

OS_MacOS::OS_MacOS(const char *p_execpath, int p_argc, char **p_argv) {
	execpath = p_execpath;
	argc = p_argc;
	argv = p_argv;

	if (is_sandboxed()) {
		// Load security-scoped bookmarks, request access, remove stale or invalid bookmarks.
		NSArray *bookmarks = [[NSUserDefaults standardUserDefaults] arrayForKey:@"sec_bookmarks"];
		NSMutableArray *new_bookmarks = [[NSMutableArray alloc] init];
		for (id bookmark in bookmarks) {
			NSError *error = nil;
			BOOL isStale = NO;
			NSURL *url = [NSURL URLByResolvingBookmarkData:bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&error];
			if (!error && !isStale) {
				if ([url startAccessingSecurityScopedResource]) {
					[new_bookmarks addObject:bookmark];
				}
			}
		}
		[[NSUserDefaults standardUserDefaults] setObject:new_bookmarks forKey:@"sec_bookmarks"];
	}
	Vector<Logger *> loggers;
	loggers.push_back(memnew(OsLogLogger(NSBundle.mainBundle.bundleIdentifier.UTF8String)));
	loggers.push_back(memnew(UnixTerminalLogger));
	_set_logger(memnew(CompositeLogger(loggers)));

#ifdef COREAUDIO_ENABLED
	AudioDriverManager::add_driver(&audio_driver);
#endif

	DisplayServerMacOS::register_macos_driver();
}

// MARK: - OS_MacOS_NSApp

void OS_MacOS_NSApp::run() {
	[NSApp run]; // Note: this call will never return. Use `OS_MacOS_NSApp::cleanup()` for cleanup.
}

static bool sig_received = false;

static void handle_interrupt(int sig) {
	if (sig == SIGINT) {
		sig_received = true;
	}
}

void OS_MacOS_NSApp::start_main() {
	Error err;
	@autoreleasepool {
		err = Main::setup(execpath, argc, argv);
	}

	if (err == OK) {
		main_started = true;

		int ret;
		@autoreleasepool {
			ret = Main::start();
		}
		if (ret == EXIT_SUCCESS) {
			if (MainLoop::is_active()) {
				@autoreleasepool {
					MainLoop::initialize();
				}
				DisplayServerMacOS *ds_mac = DisplayServer::driver_as<DisplayServerMacOS>();

				pre_wait_observer = CFRunLoopObserverCreateWithHandler(kCFAllocatorDefault, kCFRunLoopBeforeWaiting, true, 0, ^(CFRunLoopObserverRef observer, CFRunLoopActivity activity) {
					@autoreleasepool {
						@try {
							GodotProfileFrameMark;
							GodotProfileZone("macOS main loop");

							if (ds_mac) {
								ds_mac->_process_events(false);
							} else if (DisplayServer::is_initialized()) {
								DisplayServer::process_events();
							}
#ifdef SDL_ENABLED
							if (joypad_sdl) {
								joypad_sdl->process_events();
							}
#endif

							if (Main::iteration() || sig_received) {
								terminate();
							}
						} @catch (NSException *exception) {
							ERR_PRINT("NSException: " + String::utf8([exception reason].UTF8String));
						}
					}
					if (wait_timer == nil) {
						CFRunLoopWakeUp(CFRunLoopGetCurrent()); // Prevent main loop from sleeping.
					}
				});
				CFRunLoopAddObserver(CFRunLoopGetCurrent(), pre_wait_observer, kCFRunLoopCommonModes);
				return;
			}
		} else {
			set_exit_code(EXIT_FAILURE);
		}
	} else if (err == ERR_HELP) { // Returned by --help and --version, so success.
		set_exit_code(EXIT_SUCCESS);
	} else {
		set_exit_code(EXIT_FAILURE);
	}

	terminate();
}

void OS_MacOS_NSApp::terminate() {
	// Note: This method only sends app termination request. Use `OS_MacOS_NSApp::cleanup()` for cleanup.
	if (pre_wait_observer) {
		CFRunLoopRemoveObserver(CFRunLoopGetCurrent(), pre_wait_observer, kCFRunLoopCommonModes);
		CFRelease(pre_wait_observer);
		pre_wait_observer = nil;
	}

	should_terminate = true;
	[NSApp terminate:nil];
}

void OS_MacOS_NSApp::cleanup() {
	if (MainLoop::is_active()) {
		MainLoop::finalize();
	}
	if (main_started) {
		@autoreleasepool {
			Main::cleanup();
		}
	}
	godot_cleanup_profiler();
}

OS_MacOS_NSApp::OS_MacOS_NSApp(const char *p_execpath, int p_argc, char **p_argv) :
		OS_MacOS(p_execpath, p_argc, p_argv) {
	// Implicitly create shared NSApplication instance.
	[GodotApplication sharedApplication];

	// In case we are unbundled, make us a proper UI application.
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

	// Menu bar setup must go between sharedApplication above and
	// finishLaunching below, in order to properly emulate the behavior
	// of NSApplicationMain.

	NSMenu *main_menu = [[NSMenu alloc] initWithTitle:@""];
	[NSApp setMainMenu:main_menu];

	delegate = [[GodotApplicationDelegate alloc] initWithOS:this];
	ERR_FAIL_NULL(delegate);
	[NSApp setDelegate:delegate];
	[NSApp registerUserInterfaceItemSearchHandler:delegate];

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = handle_interrupt;
	sigaction(SIGINT, &action, nullptr);
}

// MARK: - OS_MacOS_Headless

void OS_MacOS_Headless::run() {
	CFRunLoopGetCurrent();

	@autoreleasepool {
		Error err = Main::setup(execpath, argc, argv);
		if (err != OK) {
			if (err == ERR_HELP) {
				return set_exit_code(EXIT_SUCCESS);
			}
			return set_exit_code(EXIT_FAILURE);
		}
	}

	int ret;
	@autoreleasepool {
		ret = Main::start();
	}

	if (ret == EXIT_SUCCESS && MainLoop::is_active()) {
		@autoreleasepool {
			MainLoop::initialize();
		}

		while (true) {
			@autoreleasepool {
				@try {
					if (Input::is_initialized()) {
						Input::flush_buffered_events();
					}

					if (Main::iteration()) {
						break;
					}

					CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, 0);
				} @catch (NSException *exception) {
					ERR_PRINT("NSException: " + String::utf8([exception reason].UTF8String));
				}
			}
		}

		MainLoop::finalize();
	}

	Main::cleanup();
}

OS_MacOS_Headless::OS_MacOS_Headless(const char *p_execpath, int p_argc, char **p_argv) :
		OS_MacOS(p_execpath, p_argc, p_argv) {
}

// MARK: - OS_MacOS_Embedded

#ifdef TOOLS_ENABLED

void OS_MacOS_Embedded::run() {
	CFRunLoopGetCurrent();

	@autoreleasepool {
		Error err = Main::setup(execpath, argc, argv);
		if (err != OK) {
			if (err == ERR_HELP) {
				return set_exit_code(EXIT_SUCCESS);
			}
			return set_exit_code(EXIT_FAILURE);
		}
	}

	int ret;
	@autoreleasepool {
		ret = Main::start();
	}

	DisplayServerMacOSEmbedded *ds = DisplayServer::driver_as<DisplayServerMacOSEmbedded>();
	if (!ds) {
		ERR_FAIL_MSG("DisplayServerMacOSEmbedded is not initialized.");
	}

	if (ds && ret == EXIT_SUCCESS && MainLoop::is_active()) {
		@autoreleasepool {
			MainLoop::initialize();
		}

		while (true) {
			@autoreleasepool {
				@try {
					GodotProfileFrameMark;
					GodotProfileZone("macOS embedded main loop");

					ds->process_events();

#ifdef SDL_ENABLED
					if (joypad_sdl) {
						joypad_sdl->process_events();
					}
#endif
					if (Main::iteration()) {
						break;
					}

					CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, 0);
				} @catch (NSException *exception) {
					ERR_PRINT("NSException: " + String::utf8([exception reason].UTF8String));
				}
			}
		}

		MainLoop::finalize();
	}

	Main::cleanup();
}

OS_MacOS_Embedded::OS_MacOS_Embedded(const char *p_execpath, int p_argc, char **p_argv) :
		OS_MacOS(p_execpath, p_argc, p_argv) {
	DisplayServerMacOSEmbedded::register_embedded_driver();
}

#endif

// === Merged from drivers/coremidi/midi_driver_coremidi.mm ===
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

#import "platform/macos/os_macos.h"

#ifdef COREMIDI_ENABLED

#include "core/string/print_string.h"

#import <CoreServices/CoreServices.h>

MIDIDriverCoreMidi::InputConnection::InputConnection(int p_device_index, MIDIEndpointRef p_source) :
		parser(p_device_index), source(p_source) {}

void MIDIDriverCoreMidi::read(const MIDIPacketList *packet_list, void *read_proc_ref_con, void *src_conn_ref_con) {
	(void)read_proc_ref_con;
	if (!data) {
		return;
	}
	MutexLock lock(data->mutex);
	if (!data->core_midi_closed) {
		InputConnection *source = static_cast<InputConnection *>(src_conn_ref_con);
		const MIDIPacket *packet = packet_list->packet;
		for (UInt32 packet_index = 0; packet_index < packet_list->numPackets; packet_index++) {
			for (UInt16 data_index = 0; data_index < packet->length; data_index++) {
				source->parser.parse_fragment(packet->data[data_index]);
			}
			packet = MIDIPacketNext(packet);
		}
	}
}

void MIDIDriverCoreMidi::initialize() {
	if (data == nullptr) {
		data = memnew(Data);
	}
}

void MIDIDriverCoreMidi::finalize() {
	if (data == nullptr) {
		return;
	}
	close();
	memdelete(data);
	data = nullptr;
}

Error MIDIDriverCoreMidi::open() {
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(data->client || data->core_midi_closed, FAILED,
			"MIDIDriverCoreMidi cannot be reopened.");

	CFStringRef name = CFStringCreateWithCString(nullptr, "Godot", kCFStringEncodingASCII);
	OSStatus result = MIDIClientCreate(name, nullptr, nullptr, &data->client);
	CFRelease(name);
	if (result != noErr) {
		ERR_PRINT("MIDIClientCreate failed, code: " + itos(result));
		return ERR_CANT_OPEN;
	}

	result = MIDIInputPortCreate(data->client, CFSTR("Godot Input"), MIDIDriverCoreMidi::read, nullptr, &data->port_in);
	if (result != noErr) {
		ERR_PRINT("MIDIInputPortCreate failed, code: " + itos(result));
		return ERR_CANT_OPEN;
	}

	int source_count = MIDIGetNumberOfSources();
	int connection_index = 0;
	for (int i = 0; i < source_count; i++) {
		MIDIEndpointRef source = MIDIGetSource(i);
		if (source) {
			InputConnection *conn = memnew(InputConnection(connection_index, source));
			const OSStatus res = MIDIPortConnectSource(data->port_in, source, static_cast<void *>(conn));
			if (res != noErr) {
				memdelete(conn);
			} else {
				data->connected_sources.push_back(conn);

				CFStringRef nameRef = nullptr;
				char name[256];
				MIDIObjectGetStringProperty(source, kMIDIPropertyDisplayName, &nameRef);
				CFStringGetCString(nameRef, name, sizeof(name), kCFStringEncodingUTF8);
				CFRelease(nameRef);
				MIDIDriver::add_connected_input(name);

				connection_index++; // Contiguous index for successfully connected inputs.
			}
		}
	}

	return OK;
}

void MIDIDriverCoreMidi::close() {
	if (data == nullptr) {
		return;
	}

	data->mutex.lock();
	data->core_midi_closed = true;
	data->mutex.unlock();

	for (InputConnection *conn : data->connected_sources) {
		MIDIPortDisconnectSource(data->port_in, conn->source);
		memdelete(conn);
	}

	data->connected_sources.clear();
	MIDIDriver::clear_connected_inputs();

	if (data->port_in != 0) {
		MIDIPortDispose(data->port_in);
		data->port_in = 0;
	}

	if (data->client != 0) {
		MIDIClientDispose(data->client);
		data->client = 0;
	}
}

#endif // COREMIDI_ENABLED

// === Merged from platform/macos/crash_handler_macos.mm ===
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

#import "platform/macos/os_macos.h"

#include "core/config/project_settings.h"
#include "core/object/script_language.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "core/version.h"
#include "main/main.h"

#include <unistd.h>

#include <cstdio>

#if defined(DEBUG_ENABLED)
#define CRASH_HANDLER_ENABLED 1
#endif

#ifdef CRASH_HANDLER_ENABLED
// --- Inlined/Folded from platform/macos/stack_trace_macos.h ---
#include "core/os/os.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>

namespace StackTraceMacOS {

static void *find_executable_load_address() {
	char full_path[1024];
	uint32_t size = sizeof(full_path);
	if (!_NSGetExecutablePath(full_path, &size)) {
		void *handle = dlopen(full_path, RTLD_LAZY | RTLD_NOLOAD);
		void *addr = dlsym(handle, "main");
		Dl_info info;
		if (dladdr(addr, &info)) {
			return info.dli_fbase;
		}
	}
	return nullptr;
}

static Vector<String> symbolize_with_atos(const String &p_exec_path, const void *p_load_addr, const void *const *p_backtrace_addresses, size_t p_backtrace_size) {
	List<String> args;
	args.push_back("-o");
	args.push_back(p_exec_path);
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__)
	args.push_back("-arch");
	args.push_back("x86_64");
#elif defined(__aarch64__) || defined(__arm64__)
	args.push_back("-arch");
	args.push_back("arm64");
#endif
	args.push_back("--fullPath");
	args.push_back("-l");
	// Add load address first, then the backtrace addresses.
	char str[1024];
	snprintf(str, sizeof(str), "%p", p_load_addr);
	args.push_back(String(str));
	for (size_t i = 0; i < p_backtrace_size; i++) {
		snprintf(str, sizeof(str), "%p", p_backtrace_addresses[i]);
		args.push_back(String(str));
	}
	// Execute atos with the arguments and capture the output.
	String atos_output;
	int ret = 0;
	const Error err = OS::execute("atos", args, &atos_output, &ret);
	if (err == OK && ret == 0 && !atos_output.is_empty()) {
		return atos_output.split("\n", false);
	}
	return Vector<String>();
}

} // namespace StackTraceMacOS

#include <cxxabi.h>
#include <execinfo.h>

static void handle_crash(int sig) {
	signal(SIGSEGV, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGILL, SIG_DFL);
	signal(SIGTRAP, SIG_DFL);

	if (!OS::is_initialized()) {
		abort();
	}

	if (OS::is_crash_handler_silent()) {
		std::_Exit(0);
	}

	void *bt_buffer[256];
	size_t size = backtrace(bt_buffer, 256);
	String exec_path = OS::get_executable_path();

	String msg;
	if (ProjectSettings::is_initialized()) {
		msg = GLOBAL_GET("debug/settings/crash_handler/message");
	}

	// Tell MainLoop about the crash. This can be handled by users too in Node.
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_CRASH);
	}

	// Dump the backtrace to stderr with a message to the user.
	print_error("\n================================================================");
	print_error(vformat("%s: Program crashed with signal %d", __FUNCTION__, sig));

	// Print the engine version just before, so that people are reminded to include the version in backtrace reports.
	if (String(VLTR_VERSION_HASH).is_empty()) {
		print_error(vformat("Engine version: %s", VLTR_VERSION_FULL_NAME));
	} else {
		print_error(vformat("Engine version: %s (%s)", VLTR_VERSION_FULL_NAME, VLTR_VERSION_HASH));
	}
	print_error(vformat("Dumping the backtrace. %s", msg));

	const void *load_addr = StackTraceMacOS::find_executable_load_address();
	print_error(vformat("Load address: %x\n", (uint64_t)load_addr));

	const Vector<String> lines = StackTraceMacOS::symbolize_with_atos(exec_path, load_addr, bt_buffer, size);

	if (!lines.is_empty()) {
		// Get demangled names from dladdr for fallback.
		char **strings = backtrace_symbols(bt_buffer, size);

		for (int i = 1; i < lines.size() && i < (int)size; i++) {
			String output = lines[i];
			String mod_name = "main";
			uint64_t mod_off = (uint64_t)load_addr;

			// If atos failed for this address, fall back to dladdr.
			if (strings) {
				bool atos_fail = output.substr(0, 2) == "0x";
				if (atos_fail) {
					Vector<String> fname_spl = String(strings[i]).split(" ", false, 3);
					output = fname_spl[fname_spl.size() - 1];
				}

				Dl_info info;
				if (dladdr(bt_buffer[i], &info)) {
					mod_off = (uint64_t)info.dli_fbase;
					if (mod_off != (uint64_t)load_addr) {
						mod_name = String(info.dli_fname).get_file();
					}
					if (atos_fail && info.dli_sname && info.dli_sname[0] == '_') {
						int status;
						char *demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status);

						if (status == 0 && demangled) {
							output = String(demangled);
						}

						if (demangled) {
							free(demangled);
						}
					}
				} else {
					mod_name = "<unknown module>";
				}
			}

			print_error(vformat("[%d] %x (%s+%x) - %s", (int64_t)i, (uint64_t)bt_buffer[i], mod_name, (uint64_t)bt_buffer[i] - mod_off, output));
		}

		if (strings) {
			free(strings);
		}
	} else {
		// Fallback if atos fails entirely
		char **strings = backtrace_symbols(bt_buffer, size);
		if (strings) {
			for (size_t i = 0; i < size; i++) {
				Vector<String> fname_spl = String(strings[i]).split(" ", false, 3);
				String output = fname_spl[fname_spl.size() - 1];

				String mod_name = "main";
				uint64_t mod_off = (uint64_t)load_addr;

				Dl_info info;
				if (dladdr(bt_buffer[i], &info)) {
					mod_off = (uint64_t)info.dli_fbase;
					if (mod_off != (uint64_t)load_addr) {
						mod_name = String(info.dli_fname).get_file();
					}
					if (info.dli_sname && info.dli_sname[0] == '_') {
						int status;
						char *demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status);

						if (status == 0 && demangled) {
							output = String(demangled);
						}

						if (demangled) {
							free(demangled);
						}
					}
				} else {
					mod_name = "<unknown module>";
				}

				print_error(vformat("[%d] %x (%s+%x) - %s", (int64_t)i, (uint64_t)bt_buffer[i], mod_name, (uint64_t)bt_buffer[i] - mod_off, output));
			}
			free(strings);
		}
	}

	print_error("-- END OF C++ BACKTRACE --");
	print_error("================================================================");

	for (const Ref<ScriptBacktrace> &backtrace : ScriptServer::capture_script_backtraces(false)) {
		if (!backtrace->is_empty()) {
			print_error(backtrace->format());
			print_error(vformat("-- END OF %s BACKTRACE --", backtrace->get_language_name().to_upper()));
			print_error("================================================================");
		}
	}

	// Abort to pass the error to the OS
	abort();
}
#endif

CrashHandler::CrashHandler() {
	disabled = false;
}

CrashHandler::~CrashHandler() {
	disable();
}

void CrashHandler::disable() {
	if (disabled) {
		return;
	}

#ifdef CRASH_HANDLER_ENABLED
	signal(SIGSEGV, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGILL, SIG_DFL);
	signal(SIGTRAP, SIG_DFL);
#endif

	disabled = true;
}

void CrashHandler::initialize() {
#ifdef CRASH_HANDLER_ENABLED
	signal(SIGSEGV, handle_crash);
	signal(SIGFPE, handle_crash);
	signal(SIGILL, handle_crash);
	signal(SIGTRAP, handle_crash);
#endif
}

// === Merged from platform/macos/dir_access_macos.mm ===
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

#import "platform/macos/os_macos.h"

#if defined(UNIX_ENABLED)

#include "core/config/project_settings.h"

#import <AppKit/NSWorkspace.h>
#import <Foundation/Foundation.h>
#include <sys/mount.h>

#include <cerrno>

String DirAccessMacOS::get_filesystem_type() const {
	struct statfs fs;
	if (statfs(current_dir.utf8().get_data(), &fs) != 0) {
		return "";
	}
	return String::utf8(fs.f_fstypename).to_upper();
}

String DirAccessMacOS::fix_unicode_name(const char *p_name) const {
	String fname;
	if (p_name != nullptr) {
		NSString *nsstr = [[NSString stringWithUTF8String:p_name] precomposedStringWithCanonicalMapping];
		fname.append_utf8([nsstr UTF8String]);
	}

	return fname;
}

int DirAccessMacOS::get_drive_count() {
	NSArray *res_keys = [NSArray arrayWithObjects:NSURLVolumeURLKey, NSURLIsSystemImmutableKey, nil];
	NSArray *vols = [[NSFileManager defaultManager] mountedVolumeURLsIncludingResourceValuesForKeys:res_keys options:NSVolumeEnumerationSkipHiddenVolumes];

	return [vols count];
}

String DirAccessMacOS::get_drive(int p_drive) {
	NSArray *res_keys = [NSArray arrayWithObjects:NSURLVolumeURLKey, NSURLIsSystemImmutableKey, nil];
	NSArray *vols = [[NSFileManager defaultManager] mountedVolumeURLsIncludingResourceValuesForKeys:res_keys options:NSVolumeEnumerationSkipHiddenVolumes];
	int count = [vols count];

	ERR_FAIL_INDEX_V(p_drive, count, "");

	String volname;
	NSString *path = [vols[p_drive] path];

	volname.append_utf8([path UTF8String]);

	return volname;
}

bool DirAccessMacOS::is_hidden(const String &p_name) {
	String f = get_current_dir().path_join(p_name);
	NSURL *url = [NSURL fileURLWithPath:@(f.utf8().get_data())];
	NSNumber *hidden = nil;
	if (![url getResourceValue:&hidden forKey:NSURLIsHiddenKey error:nil]) {
		return DirAccessUnix::is_hidden(p_name);
	}
	return [hidden boolValue];
}

bool DirAccessMacOS::is_case_sensitive(const String &p_path) const {
	String f = p_path;
	if (!f.is_absolute_path()) {
		f = get_current_dir().path_join(f);
	}
	f = fix_path(f);

	NSURL *url = [NSURL fileURLWithPath:@(f.utf8().get_data())];
	NSNumber *cs = nil;
	if (![url getResourceValue:&cs forKey:NSURLVolumeSupportsCaseSensitiveNamesKey error:nil]) {
		return false;
	}
	return [cs boolValue];
}

bool DirAccessMacOS::is_bundle(const String &p_file) const {
	String f = p_file;
	if (!f.is_absolute_path()) {
		f = get_current_dir().path_join(f);
	}
	f = fix_path(f);

	return [[NSWorkspace sharedWorkspace] isFilePackageAtPath:[NSString stringWithUTF8String:f.utf8().get_data()]];
}

#endif // UNIX_ENABLED

// === Merged from platform/macos/godot_application.mm ===
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

#import "platform/macos/os_macos.h"


GodotApplication *GodotApp = nil;

@interface GodotApplication ()
- (void)forceUnbundledWindowActivationHackStep1;
- (void)forceUnbundledWindowActivationHackStep2;
- (void)forceUnbundledWindowActivationHackStep3;
@end

@implementation GodotApplication

- (GodotApplication *)init {
	self = [super init];

	GodotApp = self;

	return self;
}

- (GodotApplicationDelegate *)godotDelegate {
	return (GodotApplicationDelegate *)self.delegate;
}

- (void)activateApplication {
	[NSApp activateIgnoringOtherApps:YES];
	NSString *nsappname = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
	const char *bundled_id = getenv("__CFBundleIdentifier");
	NSString *nsbundleid_env = [NSString stringWithUTF8String:(bundled_id != nullptr) ? bundled_id : ""];
	NSString *nsbundleid = [[NSBundle mainBundle] bundleIdentifier];
	if (nsappname == nil || isatty(STDOUT_FILENO) || isatty(STDIN_FILENO) || isatty(STDERR_FILENO) || ![nsbundleid isEqualToString:nsbundleid_env]) {
#ifdef TOOLS_ENABLED
		if (!OS_MacOS::is_debugger_attached()) {
#else
		{
#endif
			// If the executable is started from terminal or is not bundled, macOS WindowServer won't register and activate app window correctly (menu and title bar are grayed out and input ignored).
			[self performSelector:@selector(forceUnbundledWindowActivationHackStep1) withObject:nil afterDelay:0.02];
		}
	}
}

- (void)mediaKeyEvent:(int)key state:(BOOL)state repeat:(BOOL)repeat {
	Key keycode = Key::NONE;
	switch (key) {
		case NX_KEYTYPE_SOUND_UP: {
			keycode = Key::VOLUMEUP;
		} break;
		case NX_KEYTYPE_SOUND_DOWN: {
			keycode = Key::VOLUMEDOWN;
		} break;
		//NX_KEYTYPE_BRIGHTNESS_UP
		//NX_KEYTYPE_BRIGHTNESS_DOWN
		case NX_KEYTYPE_CAPS_LOCK: {
			keycode = Key::CAPSLOCK;
		} break;
		case NX_KEYTYPE_HELP: {
			keycode = Key::HELP;
		} break;
		case NX_POWER_KEY: {
			keycode = Key::STANDBY;
		} break;
		case NX_KEYTYPE_MUTE: {
			keycode = Key::VOLUMEMUTE;
		} break;
		//NX_KEYTYPE_CONTRAST_UP
		//NX_KEYTYPE_CONTRAST_DOWN
		//NX_KEYTYPE_LAUNCH_PANEL
		//NX_KEYTYPE_EJECT
		//NX_KEYTYPE_VIDMIRROR
		//NX_KEYTYPE_FAST
		//NX_KEYTYPE_REWIND
		//NX_KEYTYPE_ILLUMINATION_UP
		//NX_KEYTYPE_ILLUMINATION_DOWN
		//NX_KEYTYPE_ILLUMINATION_TOGGLE
		case NX_KEYTYPE_PLAY: {
			keycode = Key::MEDIAPLAY;
		} break;
		case NX_KEYTYPE_NEXT: {
			keycode = Key::MEDIANEXT;
		} break;
		case NX_KEYTYPE_PREVIOUS: {
			keycode = Key::MEDIAPREVIOUS;
		} break;
		default: {
			keycode = Key::NONE;
		} break;
	}

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds && keycode != Key::NONE) {
		DisplayServerMacOS::KeyEvent ke;

		ke.window_id = ds->_get_focused_window_or_popup();
		ke.macos_state = 0;
		ke.pressed = state;
		ke.echo = repeat;
		ke.keycode = keycode;
		ke.physical_keycode = keycode;
		ke.key_label = keycode;
		ke.unicode = 0;
		ke.raw = true;

		ds->push_to_key_event_buffer(ke);
	}
}

- (void)sendEvent:(NSEvent *)event {
	if ([event type] == NSEventTypeSystemDefined && [event subtype] == 8) {
		int keyCode = (([event data1] & 0xFFFF0000) >> 16);
		int keyFlags = ([event data1] & 0x0000FFFF);
		int keyState = (((keyFlags & 0xFF00) >> 8)) == 0xA;
		int keyRepeat = (keyFlags & 0x1);

		[self mediaKeyEvent:keyCode state:keyState repeat:keyRepeat];
	}

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds) {
		if ([event type] == NSEventTypeLeftMouseDown || [event type] == NSEventTypeRightMouseDown || [event type] == NSEventTypeOtherMouseDown) {
			if (ds->mouse_process_popups()) {
				return;
			}
		}
		ds->send_event(event);
	}

	// From http://cocoadev.com/index.pl?GameKeyboardHandlingAlmost
	// This works around an AppKit bug, where key up events while holding
	// down the command key don't get sent to the key window.
	if ([event type] == NSEventTypeKeyUp && ([event modifierFlags] & NSEventModifierFlagCommand)) {
		[[self keyWindow] sendEvent:event];
	} else {
		[super sendEvent:event];
	}
}

- (void)forceUnbundledWindowActivationHackStep1 {
	// Step 1: Switch focus to macOS SystemUIServer process.
	// Required to perform step 2, TransformProcessType will fail if app is already the in focus.
	for (NSRunningApplication *app in [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.apple.systemuiserver"]) {
		[app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
		break;
	}
	[self performSelector:@selector(forceUnbundledWindowActivationHackStep2)
			   withObject:nil
			   afterDelay:0.02];
}

- (void)forceUnbundledWindowActivationHackStep2 {
	// Step 2: Register app as foreground process.
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	(void)TransformProcessType(&psn, kProcessTransformToForegroundApplication);
	[self performSelector:@selector(forceUnbundledWindowActivationHackStep3) withObject:nil afterDelay:0.02];
}

- (void)forceUnbundledWindowActivationHackStep3 {
	// Step 3: Switch focus back to app window.
	[[NSRunningApplication currentApplication] activateWithOptions:NSApplicationActivateIgnoringOtherApps];
}

@end

// === Merged from platform/macos/godot_application_delegate.mm ===
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

#import "platform/macos/os_macos.h"


#import "core/input/input.h"
#import "core/input/input_event.h"
#import "core/os/main_loop.h"
#import "main/main.h"

#import <Carbon/Carbon.h>

@interface GodotApplicationDelegate ()
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context;
- (void)accessibilityDisplayOptionsChange:(NSNotification *)notification;
@end

@implementation GodotApplicationDelegate {
	bool high_contrast;
	bool reduce_motion;
	bool reduce_transparency;
	bool voice_over;
	OS_MacOS_NSApp *os_mac;
}

- (GodotApplicationDelegate *)initWithOS:(OS_MacOS_NSApp *)os {
	self = [super init];
	if (self) {
		os_mac = os;
	}

	[[NSWorkspace sharedWorkspace] addObserver:self forKeyPath:@"voiceOverEnabled" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld) context:(void *)godot_ac_ctx];
	[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(accessibilityDisplayOptionsChange:) name:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification object:nil];
	high_contrast = [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldIncreaseContrast];
	reduce_motion = [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
	reduce_transparency = [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceTransparency];
	voice_over = [[NSWorkspace sharedWorkspace] isVoiceOverEnabled];

	[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(system_theme_changed:) name:@"AppleInterfaceThemeChangedNotification" object:nil];
	[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(system_theme_changed:) name:@"AppleColorPreferencesChangedNotification" object:nil];

	return self;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
	return YES;
}

- (NSArray<NSString *> *)localizedTitlesForItem:(id)item {
	NSArray *item_name = @[ item[1] ];
	return item_name;
}

- (void)searchForItemsWithSearchString:(NSString *)searchString resultLimit:(NSInteger)resultLimit matchedItemHandler:(void (^)(NSArray *items))handleMatchedItems {
	NSMutableArray *found_items = [[NSMutableArray alloc] init];

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds && ds->_help_get_search_callback().is_valid()) {
		Callable cb = ds->_help_get_search_callback();

		Variant ret;
		Variant search_string = String::utf8([searchString UTF8String]);
		Variant result_limit = (uint64_t)resultLimit;
		Callable::CallError ce;
		const Variant *args[2] = { &search_string, &result_limit };

		cb.callp(args, 2, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat(RTR("Failed to execute help search callback: %s."), Variant::get_callable_error_text(cb, args, 2, ce)));
		}
		Dictionary results = ret;
		for (const Variant *E = results.next(); E; E = results.next(E)) {
			const String &key = *E;
			const String &value = results[*E];
			if (key.length() > 0 && value.length() > 0) {
				NSArray *item = @[ [NSString stringWithUTF8String:key.utf8().get_data()], [NSString stringWithUTF8String:value.utf8().get_data()] ];
				[found_items addObject:item];
			}
		}
	}

	handleMatchedItems(found_items);
}

- (void)performActionForItem:(id)item {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds && ds->_help_get_action_callback().is_valid()) {
		Callable cb = ds->_help_get_action_callback();

		Variant ret;
		Variant item_string = String::utf8([item[0] UTF8String]);
		Callable::CallError ce;
		const Variant *args[1] = { &item_string };

		cb.callp(args, 1, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat(RTR("Failed to execute help action callback: %s."), Variant::get_callable_error_text(cb, args, 1, ce)));
		}
	}
}

- (void)system_theme_changed:(NSNotification *)notification {
	DisplayServerMacOSBase *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds) {
		ds->emit_system_theme_changed();
	}
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
	os_mac->start_main();
}

static const char *godot_ac_ctx = "gd_accessibility_observer_ctx";

- (void)dealloc {
	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self name:@"AppleInterfaceThemeChangedNotification" object:nil];
	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self name:@"AppleColorPreferencesChangedNotification" object:nil];
	[[NSWorkspace sharedWorkspace] removeObserver:self forKeyPath:@"voiceOverEnabled" context:(void *)godot_ac_ctx];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if (context == (void *)godot_ac_ctx) {
		voice_over = [[NSWorkspace sharedWorkspace] isVoiceOverEnabled];
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)accessibilityDisplayOptionsChange:(NSNotification *)notification {
	high_contrast = [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldIncreaseContrast];
	reduce_motion = [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
	reduce_transparency = [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceTransparency];
}

- (bool)getHighContrast {
	return high_contrast;
}

- (bool)getReduceMotion {
	return reduce_motion;
}

- (bool)getReduceTransparency {
	return reduce_transparency;
}

- (bool)getVoiceOver {
	return voice_over;
}

- (void)application:(NSApplication *)application openURLs:(NSArray<NSURL *> *)urls {
	List<String> args;
	for (NSURL *url in urls) {
		if ([url isFileURL]) {
			args.push_back(String::utf8([url.path UTF8String]));
		} else {
			args.push_back(vformat("--uri=\"%s\"", String::utf8([url.absoluteString UTF8String])));
		}
	}
	if (!args.is_empty()) {
		if (MainLoop::is_active()) {
			// Application is already running, open a new instance with the URL/files as command line arguments.
			os_mac->create_instance(args);
		} else if (os_mac->get_cmd_argc() == 0) {
			// Application is just started, add to the list of command line arguments and continue.
			os_mac->set_cmdline_platform_args(args);
		}
	}
}

- (void)application:(NSApplication *)sender openFiles:(NSArray<NSString *> *)filenames {
	List<String> args;
	for (NSString *filename in filenames) {
		NSURL *url = [NSURL URLWithString:filename];
		args.push_back(String::utf8([url.path UTF8String]));
	}
	if (!args.is_empty()) {
		if (MainLoop::is_active()) {
			// Application is already running, open a new instance with the URL/files as command line arguments.
			os_mac->create_instance(args);
		} else if (os_mac->get_cmd_argc() == 0) {
			// Application is just started, add to the list of command line arguments and continue.
			os_mac->set_cmdline_platform_args(args);
		}
	}
}

- (void)applicationDidResignActive:(NSNotification *)notification {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds) {
		ds->mouse_process_popups(true);
	}
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_FOCUS_OUT);
	}
}

static const CGKeyCode modifiers[8] = {
	kVK_Command,
	kVK_RightCommand,
	kVK_Shift,
	kVK_RightShift,
	kVK_Option,
	kVK_RightOption,
	kVK_Control,
	kVK_RightControl,
};

// The list of modifier flags we care about for raising pressed events when the application becomes active.
constexpr static NSEventModifierFlags FLAGS = NSEventModifierFlagCommand | NSEventModifierFlagShift | NSEventModifierFlagOption | NSEventModifierFlagControl;

- (void)applicationDidBecomeActive:(NSNotification *)notification {
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_FOCUS_IN);
	}
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds) {
		return;
	}
	if (!Input::is_initialized()) {
		return;
	}

	// Poll the modifier keys and submit pressed events if they are down when the application becomes active.
	int mod = NSEvent.modifierFlags;
	if ((mod & FLAGS) == 0) {
		// No flags we care about.
		return;
	}

	DisplayServerEnums::WindowID window_id = ds->get_focused_window();
	NSEventModifierFlags flags = static_cast<NSEventModifierFlags>(mod);

	for (const CGKeyCode key : modifiers) {
		bool is_down = CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, key);
		if (likely(!is_down)) {
			continue;
		}
		Ref<InputEventKey> ke;
		ke.instantiate();

		ke->set_window_id(window_id);
		ke->set_echo(false);
		ke->set_pressed(true);
		ds->get_key_modifier_state(flags, ke);
		ke->set_keycode(KeyMappingMacOS::remap_key(key, mod, false));
		ke->set_physical_keycode(KeyMappingMacOS::translate_key(key));
		ke->set_key_label(KeyMappingMacOS::remap_key(key, mod, true));
		ke->set_location(KeyMappingMacOS::translate_location(key));
		Input::parse_input_event(ke.ptr());
	}
}

- (BOOL)validateMenuItem:(NSMenuItem *)item {
	if (item) {
		GodotMenuItem *value = [item representedObject];
		if (value) {
			return value->enabled;
		}
	}
	return YES;
}

- (void)globalMenuCallback:(id)sender {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds) {
		return ds->menu_callback(sender);
	}
}

- (NSMenu *)applicationDockMenu:(NSApplication *)sender {
	if (NativeMenu::is_initialized()) {
		NativeMenuMacOS *nmenu = NativeMenu::driver_as<NativeMenuMacOS>();
		return nmenu->_get_dock_menu();
	} else {
		return nullptr;
	}
}

- (void)applicationWillTerminate:(NSNotification *)notification {
	os_mac->cleanup();
	exit(os_mac->get_exit_code());
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
	if (os_mac->os_should_terminate()) {
		return NSTerminateNow;
	}

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds && ds->has_window(DisplayServerEnums::MAIN_WINDOW_ID)) {
		ds->send_window_event(ds->get_window(DisplayServerEnums::MAIN_WINDOW_ID), DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
	}

	return NSTerminateCancel;
}

- (void)showAbout:(id)sender {
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_WM_ABOUT);
	}
}

@end

// === Merged from platform/macos/tts_macos.mm ===
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

#import "platform/macos/os_macos.h"

#include "servers/display/display_server.h"

@implementation TTS_MacOS

- (id)init {
	self = [super init];
	self->speaking = false;
	self->synth = [[AVSpeechSynthesizer alloc] init];
	[self->synth setDelegate:self];
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

		AVSpeechSynthesizer *av_synth = synth;
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
	AVSpeechSynthesizer *av_synth = synth;
	[av_synth pauseSpeakingAtBoundary:AVSpeechBoundaryImmediate];
}

- (void)resumeSpeaking {
	AVSpeechSynthesizer *av_synth = synth;
	[av_synth continueSpeaking];
}

- (void)stopSpeaking {
	for (TTSUtterance &message : queue) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, message.id);
	}
	queue.clear();
	AVSpeechSynthesizer *av_synth = synth;
	[av_synth stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
	speaking = false;
}

- (bool)isSpeaking {
	return speaking || (queue.size() > 0);
}

- (bool)isPaused {
	AVSpeechSynthesizer *av_synth = synth;
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
