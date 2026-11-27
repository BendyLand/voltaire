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

#import "servers/display/display_server.h"


#ifdef TOOLS_ENABLED
#import "servers/display/display_server.h"
#endif

#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/os/os.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/profiling/profiling.h"
#include "core/version_generated.gen.h"
#include "servers/display/display_server.h"

#ifdef SDL_ENABLED
#include "core/os/os.h"
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

	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_FILESYSTEM);
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
	loggers.push_back(memnew(Logger(NSBundle.mainBundle.bundleIdentifier.UTF8String)));
	loggers.push_back(memnew(Logger));
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

#import "servers/display/display_server.h"

#ifdef COREMIDI_ENABLED

#include "core/string/ustring.h"

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

#import "servers/display/display_server.h"

#include "core/os/os.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/typedefs.h"
#include "servers/display/display_server.h"

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

#import "servers/display/display_server.h"

#if defined(UNIX_ENABLED)

#include "core/os/os.h"

#import <AppKit/NSWorkspace.h>
#import <Foundation/Foundation.h>
#include <sys/mount.h>

#include <cerrno>

String DirAccessUnix::get_filesystem_type() const {
	struct statfs fs;
	if (statfs(current_dir.utf8().get_data(), &fs) != 0) {
		return "";
	}
	return String::utf8(fs.f_fstypename).to_upper();
}

String DirAccessUnix::fix_unicode_name(const char *p_name) const {
	String fname;
	if (p_name != nullptr) {
		NSString *nsstr = [[NSString stringWithUTF8String:p_name] precomposedStringWithCanonicalMapping];
		fname.append_utf8([nsstr UTF8String]);
	}

	return fname;
}

int DirAccessUnix::get_drive_count() {
	NSArray *res_keys = [NSArray arrayWithObjects:NSURLVolumeURLKey, NSURLIsSystemImmutableKey, nil];
	NSArray *vols = [[NSFileManager defaultManager] mountedVolumeURLsIncludingResourceValuesForKeys:res_keys options:NSVolumeEnumerationSkipHiddenVolumes];

	return [vols count];
}

String DirAccessUnix::get_drive(int p_drive) {
	NSArray *res_keys = [NSArray arrayWithObjects:NSURLVolumeURLKey, NSURLIsSystemImmutableKey, nil];
	NSArray *vols = [[NSFileManager defaultManager] mountedVolumeURLsIncludingResourceValuesForKeys:res_keys options:NSVolumeEnumerationSkipHiddenVolumes];
	int count = [vols count];

	ERR_FAIL_INDEX_V(p_drive, count, "");

	String volname;
	NSString *path = [vols[p_drive] path];

	volname.append_utf8([path UTF8String]);

	return volname;
}

bool DirAccessUnix::is_hidden(const String &p_name) {
	String f = get_current_dir().path_join(p_name);
	NSURL *url = [NSURL fileURLWithPath:@(f.utf8().get_data())];
	NSNumber *hidden = nil;
	if (![url getResourceValue:&hidden forKey:NSURLIsHiddenKey error:nil]) {
		return p_name != "." && p_name != ".." && p_name.begins_with(".");
	}
	return [hidden boolValue];
}

bool DirAccessUnix::is_case_sensitive(const String &p_path) const {
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

bool DirAccessUnix::is_bundle(const String &p_file) const {
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

#import "servers/display/display_server.h"


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

#import "servers/display/display_server.h"


#import "core/os/os.h"
#import "servers/display/display_server.h"

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

#import "servers/display/display_server.h"

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

// === Merged from platform/macos/display_server_macos.mm ===
/*  display_server_macos.mm                                               */
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

#import "servers/display/display_server.h"


#include "core/os/os.h"
#include "core/io/file_access.h"
#include "core/io/net_socket.h"
#include "servers/display/display_server.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "servers/display/display_server.h"

#ifdef TOOLS_ENABLED
#import "servers/display/display_server.h"

#import "editor/embedded_process_macos.h"
#endif

// Rendering drivers - include after general Godot deps as they imply system includes
// which may need precise ordering.

#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
#include "servers/display/display_server.h"
#endif
#include "servers/display/display_server.h"

#endif

#if defined(RD_ENABLED)
#include "servers/display/display_server.h"

#if defined(VULKAN_ENABLED)
#import "servers/display/display_server.h"
#endif
#if defined(METAL_ENABLED)
#import "servers/display/display_server.h"
#endif
#endif // RD_ENABLED

// Keep Quartz after rendering includes, as it includes system GL.h
// which clashes with GLAD.
#ifdef TOOLS_ENABLED
#import "servers/display/display_server.h"
#endif

#include <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <IOKit/hid/IOHIDLib.h>

DisplayServerEnums::WindowID DisplayServerMacOS::_create_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, const Rect2i &p_rect) {
	const float scale = screen_get_max_scale();

	DisplayServerEnums::WindowID id = window_id_counter;
	{
		WindowData &wd = windows[id];

		wd.window_delegate = [[GodotWindowDelegate alloc] initWithDisplayServer:this];
		ERR_FAIL_NULL_V_MSG(wd.window_delegate, DisplayServerEnums::INVALID_WINDOW_ID, "Can't create a window delegate");
		[wd.window_delegate setWindowID:id];

		int rq_screen = get_screen_from_rect(p_rect);
		if (rq_screen < 0) {
			rq_screen = get_primary_screen(); // Requested window rect is outside any screen bounds.
		}

		Rect2i srect = screen_get_usable_rect(rq_screen);
		Point2i wpos = p_rect.position;
		if (srect != Rect2i()) {
			wpos = wpos.clamp(srect.position, srect.position + srect.size - p_rect.size / 3);
		}
		// macOS native y-coordinate relative to _get_screens_origin() is negative,
		// Godot passes a positive value.
		wpos.y *= -1;
		wpos += _get_screens_origin();
		wpos /= scale;

		// initWithContentRect uses bottom-left corner of the window’s frame as origin.
		wd.window_object = [[GodotWindow alloc]
				initWithContentRect:NSMakeRect(100, 100, MAX(1, p_rect.size.width / scale), MAX(1, p_rect.size.height / scale))
						  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
							backing:NSBackingStoreBuffered
							  defer:NO];
		ERR_FAIL_NULL_V_MSG(wd.window_object, DisplayServerEnums::INVALID_WINDOW_ID, "Can't create a window");
		[wd.window_object setWindowID:id];
		[wd.window_object setReleasedWhenClosed:NO];

		wd.window_view = [[GodotContentView alloc] init];
		if (wd.window_view == nil) {
			windows.erase(id);
			ERR_FAIL_V_MSG(DisplayServerEnums::INVALID_WINDOW_ID, "Can't create a window view");
		}
		[wd.window_view setWindowID:id];
		[wd.window_view setWantsLayer:TRUE];

		[wd.window_object setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
		[wd.window_object setContentView:wd.window_view];
		[wd.window_object setDelegate:wd.window_delegate];
		[wd.window_object setAcceptsMouseMovedEvents:YES];
		[wd.window_object setRestorable:NO];
		[wd.window_object setColorSpace:[NSColorSpace sRGBColorSpace]];

		if (!AccessibilityServer::window_create(id, (__bridge void *)wd.window_object)) {
			if (OS::is_stdout_verbose()) {
				ERR_PRINT("Can't create an accessibility adapter for window, accessibility support disabled!");
			}
		}

		if ([wd.window_object respondsToSelector:@selector(setTabbingMode:)]) {
			[wd.window_object setTabbingMode:NSWindowTabbingModeDisallowed];
		}

		CALayer *layer = [wd.window_view layer];
		if (layer) {
			layer.contentsScale = scale;
		}

		NSColor *bg_color = [NSColor windowBackgroundColor];
		Color _bg_color;
		if (_get_window_early_clear_override(_bg_color)) {
			bg_color = [NSColor colorWithCalibratedRed:_bg_color.r green:_bg_color.g blue:_bg_color.b alpha:1.f];
		}

		[wd.window_object setBackgroundColor:bg_color];
		if (layer) {
			[layer setBackgroundColor:bg_color.CGColor];
		}

#if defined(RD_ENABLED)
		if (rendering_context) {
			union {
#ifdef VULKAN_ENABLED
				RenderingContextDriverVulkan::WindowPlatformData vulkan;
#endif
#ifdef METAL_ENABLED
				RenderingContextDriverMetal::WindowPlatformData metal;
#endif
			} wpd;
#ifdef VULKAN_ENABLED
			if (rendering_driver == "vulkan") {
				wpd.vulkan.layer_ptr = (void *const *)&layer;
			}
#endif
#ifdef METAL_ENABLED
			if (rendering_driver == "metal") {
				wpd.metal.layer = (__bridge CA::MetalLayer *)layer;
			}
#endif
			Error err = rendering_context->window_create(window_id_counter, &wpd);

			if (err != OK) {
				AccessibilityServer::window_destroy(id);
			}

			ERR_FAIL_COND_V_MSG(err != OK, DisplayServerEnums::INVALID_WINDOW_ID, vformat("Can't create a %s context", rendering_driver));

			rendering_context->window_set_size(window_id_counter, p_rect.size.width, p_rect.size.height);
			rendering_context->window_set_vsync_mode(window_id_counter, p_vsync_mode);
		}
#endif

#if defined(GLES3_ENABLED)
		bool gl_failed = false;
		if (gl_manager_legacy) {
			Error err = gl_manager_legacy->window_create(window_id_counter, wd.window_view, p_rect.size.width, p_rect.size.height);
			if (err != OK) {
				gl_failed = true;
			}
		}
#if defined(ANGLE_ENABLED)
		if (gl_manager_angle) {
			Error err = gl_manager_angle->window_create(window_id_counter, nullptr, (__bridge void *)layer, p_rect.size.width, p_rect.size.height);
			if (err != OK) {
				gl_failed = true;
			}
		}
#endif
		if (gl_failed) {
			AccessibilityServer::window_destroy(id);

			windows.erase(id);
			ERR_FAIL_V_MSG(DisplayServerEnums::INVALID_WINDOW_ID, "Can't create an OpenGL context.");
		}
		window_set_vsync_mode(p_vsync_mode, id);
#endif
		[wd.window_view updateLayerDelegate];

		const NSRect contentRect = [wd.window_view frame];
		const NSRect windowRect = [wd.window_object frame];
		const NSRect nsrect = [wd.window_object convertRectToScreen:contentRect];
		Point2i offset;
		offset.x = (nsrect.origin.x - windowRect.origin.x);
		offset.y = (nsrect.origin.y + nsrect.size.height);
		offset.y -= (windowRect.origin.y + windowRect.size.height);
		[wd.window_object setFrameTopLeftPoint:NSMakePoint(wpos.x - offset.x, wpos.y - offset.y)];
	}
	window_id_counter++;

	WindowData &wd = windows[id];
	window_set_mode(p_mode, id);

	const NSRect contentRect = [wd.window_view frame];
	wd.size.width = contentRect.size.width * scale;
	wd.size.height = contentRect.size.height * scale;

	CALayer *layer = [(NSView *)wd.window_view layer];
	if (layer) {
		layer.contentsScale = scale;
	}

#if defined(GLES3_ENABLED)
	if (gl_manager_legacy) {
		gl_manager_legacy->window_resize(id, wd.size.width, wd.size.height);
	}
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->window_resize(id, wd.size.width, wd.size.height);
	}
#endif
#endif

	return id;
}

void DisplayServerMacOS::_update_window_style(WindowData p_wd, DisplayServerEnums::WindowID p_window) {
	bool borderless_full = false;

	if (p_wd.borderless) {
		NSRect frameRect = [p_wd.window_object frame];
		NSRect screenRect = [[p_wd.window_object screen] frame];

		// Check if our window covers up the screen.
		if (frameRect.origin.x <= screenRect.origin.x && frameRect.origin.y <= frameRect.origin.y &&
				frameRect.size.width >= screenRect.size.width && frameRect.size.height >= screenRect.size.height) {
			borderless_full = true;
		}
	}

	if (borderless_full) {
		// If the window covers up the screen set the level to above the main menu and hide on deactivate.
		[(NSWindow *)p_wd.window_object setLevel:NSMainMenuWindowLevel + 1];
		[(NSWindow *)p_wd.window_object setHidesOnDeactivate:YES];
	} else {
		// Reset these when our window is not a borderless window that covers up the screen.
		if (is_always_on_top_recursive(p_window) && !p_wd.fullscreen) {
			[(NSWindow *)p_wd.window_object setLevel:NSFloatingWindowLevel];
		} else {
			[(NSWindow *)p_wd.window_object setLevel:NSNormalWindowLevel];
		}
		[(NSWindow *)p_wd.window_object setHidesOnDeactivate:NO];
	}
}

bool DisplayServerMacOS::is_always_on_top_recursive(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(!windows.has(p_window), false);

	const WindowData &wd = windows[p_window];
	if (wd.on_top) {
		return true;
	}

	if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
		return is_always_on_top_recursive(wd.transient_parent);
	}

	return false;
}

void DisplayServerMacOS::set_window_per_pixel_transparency_enabled(bool p_enabled, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (!OS::is_layered_allowed()) {
		return;
	}
	if (p_enabled) {
		[wd.window_object setOpaque:NO];
		if (wd.borderless) {
			[wd.window_object setBackgroundColor:[NSColor clearColor]];
			[wd.window_object setHasShadow:NO];
		} else {
			// Note: transparent and not borderless - set alpha to non-zero value to ensure window shadow is rendered correctly.
			[wd.window_object setBackgroundColor:[NSColor colorWithCalibratedRed:0 green:0 blue:0 alpha:0.004f]];
			[wd.window_object setHasShadow:YES];
		}
		CALayer *layer = [(NSView *)wd.window_view layer];
		if (layer) {
			[layer setBackgroundColor:[NSColor clearColor].CGColor];
			[layer setOpaque:NO];
		}
#if defined(GLES3_ENABLED)
		if (gl_manager_legacy) {
			gl_manager_legacy->window_set_per_pixel_transparency_enabled(p_window, true);
		}
#endif
	} else {
		NSColor *bg_color = [NSColor windowBackgroundColor];
		Color _bg_color;
		if (_get_window_early_clear_override(_bg_color)) {
			bg_color = [NSColor colorWithCalibratedRed:_bg_color.r green:_bg_color.g blue:_bg_color.b alpha:1.f];
		}
		[wd.window_object setBackgroundColor:bg_color];
		[wd.window_object setOpaque:YES];
		if (!wd.borderless) {
			[wd.window_object setHasShadow:YES];
		}
		CALayer *layer = [(NSView *)wd.window_view layer];
		if (layer) {
			[layer setBackgroundColor:bg_color.CGColor];
			[layer setOpaque:YES];
		}
#if defined(GLES3_ENABLED)
		if (gl_manager_legacy) {
			gl_manager_legacy->window_set_per_pixel_transparency_enabled(p_window, false);
		}
#endif
	}
}

void DisplayServerMacOS::_update_displays_arrangement() const {
	origin = Point2i();

	for (int i = 0; i < get_screen_count(); i++) {
		Point2i position = _get_native_screen_position(i);
		if (position.x < origin.x) {
			origin.x = position.x;
		}
		if (position.y > origin.y) {
			origin.y = position.y;
		}
	}
	displays_arrangement_dirty = false;
}

void DisplayServerMacOS::set_menu_delegate(NSMenu *p_menu) {
	[p_menu setDelegate:menu_delegate];
}

Point2i DisplayServerMacOS::_get_screens_origin() const {
	// Returns the native top-left screen coordinate of the smallest rectangle
	// that encompasses all screens. Needed in get_screen_position(),
	// window_get_position, and window_set_position()
	// to convert between macOS native screen coordinates and the ones expected by Godot.

	if (displays_arrangement_dirty) {
		_update_displays_arrangement();
	}

	return origin;
}

Point2i DisplayServerMacOS::_get_native_screen_position(int p_screen) const {
	NSArray *screenArray = [NSScreen screens];
	if ((NSUInteger)p_screen < [screenArray count]) {
		NSRect nsrect = [[screenArray objectAtIndex:p_screen] frame];
		// Return the top-left corner of the screen, for macOS the y starts at the bottom.
		return Point2i(nsrect.origin.x, nsrect.origin.y + nsrect.size.height) * screen_get_max_scale();
	}

	return Point2i();
}

void DisplayServerMacOS::_displays_arrangement_changed(CGDirectDisplayID display_id, CGDisplayChangeSummaryFlags flags, void *user_info) {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds) {
		ds->displays_arrangement_dirty = true;
	}
}

DisplayServerEnums::WindowID DisplayServerMacOS::_get_focused_window_or_popup() const {
	const List<DisplayServerEnums::WindowID>::Element *E = popup_list.back();
	if (E) {
		return E->get();
	}

	return last_focused_window;
}

void DisplayServerMacOS::mouse_enter_window(DisplayServerEnums::WindowID p_window) {
	if (window_mouseover_id != p_window) {
		if (window_mouseover_id != DisplayServerEnums::INVALID_WINDOW_ID) {
			send_window_event(windows[window_mouseover_id], DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT);
		}
		window_mouseover_id = p_window;
		if (p_window != DisplayServerEnums::INVALID_WINDOW_ID) {
			send_window_event(windows[p_window], DisplayServerEnums::WINDOW_EVENT_MOUSE_ENTER);
		}
	}
}

void DisplayServerMacOS::mouse_exit_window(DisplayServerEnums::WindowID p_window) {
	if (window_mouseover_id == p_window && p_window != DisplayServerEnums::INVALID_WINDOW_ID) {
		send_window_event(windows[p_window], DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT);
	}
	window_mouseover_id = DisplayServerEnums::INVALID_WINDOW_ID;
}

void DisplayServerMacOS::_dispatch_input_events(const Ref<InputEvent> &p_event) {
	if (DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>()) {
		ds->_dispatch_input_event(p_event);
	}
}

void DisplayServerMacOS::_dispatch_input_event(const Ref<InputEvent> &p_event) {
	if (!in_dispatch_input_event) {
		in_dispatch_input_event = true;

		{
			List<DisplayServerEnums::WindowID>::Element *E = popup_list.back();
			if (E && Object::cast_to<InputEventKey>(*p_event)) {
				// Redirect keyboard input to active popup.
				if (windows.has(E->get())) {
					Callable callable = windows[E->get()].input_event_callback;
					if (callable.is_valid()) {
						callable.call(p_event);
					}
				}
				in_dispatch_input_event = false;
				return;
			}
		}

		Ref<InputEventFromWindow> event_from_window = p_event;
		if (event_from_window.is_valid() && event_from_window->get_window_id() != DisplayServerEnums::INVALID_WINDOW_ID) {
			// Send to a window.
			if (windows.has(event_from_window->get_window_id())) {
				Callable callable = windows[event_from_window->get_window_id()].input_event_callback;
				if (callable.is_valid()) {
					callable.call(p_event);
				}
			}
		} else {
			// Send to all windows. Copy all pending callbacks, since callback can erase window.
			Vector<Callable> cbs;
			for (KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
				Callable callable = E.value.input_event_callback;
				if (callable.is_valid()) {
					cbs.push_back(callable);
				}
			}
			for (const Callable &cb : cbs) {
				cb.call(p_event);
			}
		}
		in_dispatch_input_event = false;
	}
}

void DisplayServerMacOS::_push_input(const Ref<InputEvent> &p_event) {
	Ref<InputEvent> ev = p_event;
	Input::parse_input_event(ev);
}

void DisplayServerMacOS::_process_key_events() {
	Ref<InputEventKey> k;
	for (int i = 0; i < key_event_pos; i++) {
		const KeyEvent &ke = key_event_buffer[i];
		if (ke.raw) {
			// Non IME input - no composite characters, pass events as is.
			k.instantiate();

			k->set_window_id(ke.window_id);
			get_key_modifier_state(ke.macos_state, k);
			k->set_pressed(ke.pressed);
			k->set_echo(ke.echo);
			k->set_keycode(ke.keycode);
			k->set_physical_keycode(ke.physical_keycode);
			k->set_key_label(ke.key_label);
			k->set_unicode(ke.unicode);
			k->set_location(ke.location);

			_push_input(k);
		} else {
			// IME input.
			if ((i == 0 && ke.keycode == Key::NONE) || (i > 0 && key_event_buffer[i - 1].keycode == Key::NONE)) {
				k.instantiate();

				k->set_window_id(ke.window_id);
				get_key_modifier_state(ke.macos_state, k);
				k->set_pressed(ke.pressed);
				k->set_echo(ke.echo);
				k->set_keycode(Key::NONE);
				k->set_physical_keycode(Key::NONE);
				k->set_key_label(Key::NONE);
				k->set_unicode(ke.unicode);

				_push_input(k);
			}
			if (ke.keycode != Key::NONE) {
				k.instantiate();

				k->set_window_id(ke.window_id);
				get_key_modifier_state(ke.macos_state, k);
				k->set_pressed(ke.pressed);
				k->set_echo(ke.echo);
				k->set_keycode(ke.keycode);
				k->set_physical_keycode(ke.physical_keycode);
				k->set_key_label(ke.key_label);
				k->set_location(ke.location);

				if (i + 1 < key_event_pos && key_event_buffer[i + 1].keycode == Key::NONE) {
					k->set_unicode(key_event_buffer[i + 1].unicode);
				}

				_push_input(k);
			}
		}
	}

	key_event_pos = 0;
}

NSImage *DisplayServerMacOS::_convert_to_nsimg(Ref<Image> &p_image) const {
	p_image->convert(Image::FORMAT_RGBA8);
	NSBitmapImageRep *imgrep = [[NSBitmapImageRep alloc]
			initWithBitmapDataPlanes:nullptr
						  pixelsWide:p_image->get_width()
						  pixelsHigh:p_image->get_height()
					   bitsPerSample:8
					 samplesPerPixel:4
							hasAlpha:YES
							isPlanar:NO
					  colorSpaceName:NSDeviceRGBColorSpace
						 bytesPerRow:int(p_image->get_width()) * 4
						bitsPerPixel:32];
	ERR_FAIL_NULL_V(imgrep, nil);
	uint8_t *pixels = [imgrep bitmapData];

	int len = p_image->get_width() * p_image->get_height();
	const uint8_t *r = p_image->get_data().ptr();

	/* Premultiply the alpha channel */
	for (int i = 0; i < len; i++) {
		uint8_t alpha = r[i * 4 + 3];
		pixels[i * 4 + 0] = (uint8_t)(((uint16_t)r[i * 4 + 0] * alpha) / 255);
		pixels[i * 4 + 1] = (uint8_t)(((uint16_t)r[i * 4 + 1] * alpha) / 255);
		pixels[i * 4 + 2] = (uint8_t)(((uint16_t)r[i * 4 + 2] * alpha) / 255);
		pixels[i * 4 + 3] = alpha;
	}

	NSImage *nsimg = [[NSImage alloc] initWithSize:NSMakeSize(p_image->get_width(), p_image->get_height())];
	ERR_FAIL_NULL_V(nsimg, nil);
	[nsimg addRepresentation:imgrep];
	return nsimg;
}

NSCursor *DisplayServerMacOS::_cursor_from_selector(SEL p_selector, SEL p_fallback) {
	if ([NSCursor respondsToSelector:p_selector]) {
		id object = [NSCursor performSelector:p_selector];
		if ([object isKindOfClass:[NSCursor class]]) {
			return object;
		}
	}
	if (p_fallback) {
		// Fallback should be a reasonable default, no need to check.
		return [NSCursor performSelector:p_fallback];
	}
	return [NSCursor arrowCursor];
}

void DisplayServerMacOS::menu_callback(id p_sender) {
	if (![p_sender representedObject]) {
		return;
	}

	GodotMenuItem *value = [p_sender representedObject];
	if (value) {
		if (value->callback.is_valid()) {
			MenuCall mc;
			mc.tag = value->meta;
			mc.callback = value->callback;
			deferred_menu_calls.push_back(mc);
			// Do not run callback from here! If it is opening a new window or calling process_events, it will corrupt OS event queue and crash.
		}
	}
}

bool DisplayServerMacOS::has_window(DisplayServerEnums::WindowID p_window) const {
	return windows.has(p_window);
}

DisplayServerMacOS::WindowData &DisplayServerMacOS::get_window(DisplayServerEnums::WindowID p_window) {
	return windows[p_window];
}

void DisplayServerMacOS::send_event(NSEvent *p_event) {
	// Special case handling of shortcuts that don't arrive at the regular keyDown handler
	if ([p_event type] == NSEventTypeKeyDown) {
		NSEventModifierFlags flags = [p_event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

		// Command-period
		if ((flags == NSEventModifierFlagCommand) && [p_event keyCode] == 0x2f) {
			Ref<InputEventKey> k;
			k.instantiate();

			get_key_modifier_state([p_event modifierFlags], k);
			k->set_window_id(DisplayServerEnums::INVALID_WINDOW_ID);
			k->set_pressed(true);
			k->set_keycode(Key::PERIOD);
			k->set_physical_keycode(Key::PERIOD);
			k->set_key_label(Key::PERIOD);
			k->set_echo([p_event isARepeat]);

			Input::parse_input_event(k);
			return;
		}

		// Ctrl+Tab and Ctrl+Shift+Tab
		if (((flags == NSEventModifierFlagControl) || (flags == (NSEventModifierFlagControl | NSEventModifierFlagShift))) && [p_event keyCode] == 0x30) {
			Ref<InputEventKey> k;
			k.instantiate();

			get_key_modifier_state([p_event modifierFlags], k);
			k->set_window_id(DisplayServerEnums::INVALID_WINDOW_ID);
			k->set_pressed(true);
			k->set_keycode(Key::TAB);
			k->set_physical_keycode(Key::TAB);
			k->set_key_label(Key::TAB);
			k->set_echo([p_event isARepeat]);

			Input::parse_input_event(k);
			return;
		}
	}
}

void DisplayServerMacOS::send_window_event(const WindowData &wd, DisplayServerEnums::WindowEvent p_event) const {
	_THREAD_SAFE_METHOD_

	if (wd.event_callback.is_valid()) {
		Variant event = int(p_event);
		wd.event_callback.call(event);
	}
}

void DisplayServerMacOS::send_window_event_by_id(DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_id) const {
	const WindowData &wd = windows[p_id];
	send_window_event(wd, p_event);
}

void DisplayServerMacOS::release_pressed_events() {
	_THREAD_SAFE_METHOD_
	if (Input::is_initialized()) {
		Input::release_pressed_events();
	}
}

void DisplayServerMacOS::sync_mouse_state() {
	_THREAD_SAFE_METHOD_
	if (Input::is_initialized()) {
		Vector2i pos = Input::get_mouse_position();
		BitField<MouseButtonMask> in_mask = Input::get_mouse_button_mask();
		BitField<MouseButtonMask> ds_mask = mouse_get_button_state();
		for (int btn = (int)MouseButton::LEFT; btn <= (int)MouseButton::MB_XBUTTON2; btn++) {
			MouseButtonMask mbm = mouse_button_to_mask(MouseButton(btn));
			if (in_mask.has_flag(mbm) && !ds_mask.has_flag(mbm)) {
				Ref<InputEventMouseButton> mb;
				mb.instantiate();
				mb->set_button_index(MouseButton(btn));
				mb->set_pressed(false);
				mb->set_position(pos);
				mb->set_global_position(pos);
				mb->set_button_mask(ds_mask);
				Input::parse_input_event(mb);
			}
			if (!in_mask.has_flag(mbm) && ds_mask.has_flag(mbm)) {
				Ref<InputEventMouseButton> mb;
				mb.instantiate();
				mb->set_button_index(MouseButton(btn));
				mb->set_pressed(true);
				mb->set_position(pos);
				mb->set_global_position(pos);
				mb->set_button_mask(ds_mask);
				Input::parse_input_event(mb);
			}
		}
	}
}

void DisplayServerMacOS::get_key_modifier_state(unsigned int p_macos_state, Ref<InputEventWithModifiers> r_state) const {
	r_state->set_shift_pressed((p_macos_state & NSEventModifierFlagShift));
	r_state->set_ctrl_pressed((p_macos_state & NSEventModifierFlagControl));
	r_state->set_alt_pressed((p_macos_state & NSEventModifierFlagOption));
	r_state->set_meta_pressed((p_macos_state & NSEventModifierFlagCommand));
}

void DisplayServerMacOS::update_mouse_pos(DisplayServerMacOS::WindowData &p_wd, NSPoint p_location_in_window) {
	const NSRect content_rect = [p_wd.window_view frame];
	const float scale = screen_get_max_scale();
	p_wd.mouse_pos.x = p_location_in_window.x * scale;
	p_wd.mouse_pos.y = (content_rect.size.height - p_location_in_window.y) * scale;
	Input::set_mouse_position(p_wd.mouse_pos);
}

void DisplayServerMacOS::pop_last_key_event() {
	// Does not pop last key event when it is an IME key event.
	if (key_event_pos > 0 && key_event_buffer[key_event_pos - 1].raw) {
		key_event_pos--;
	}
}

void DisplayServerMacOS::push_to_key_event_buffer(const DisplayServerMacOS::KeyEvent &p_event) {
	if (key_event_pos >= key_event_buffer.size()) {
		key_event_buffer.resize(1 + key_event_pos);
	}
	key_event_buffer.write[key_event_pos++] = p_event;
}

void DisplayServerMacOS::set_last_focused_window(DisplayServerEnums::WindowID p_window) {
	last_focused_window = p_window;
}

void DisplayServerMacOS::set_is_resizing(bool p_is_resizing) {
	is_resizing = p_is_resizing;
}

bool DisplayServerMacOS::get_is_resizing() const {
	return is_resizing;
}

void DisplayServerMacOS::window_destroy(DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(!windows.has(p_window));

#if defined(GLES3_ENABLED)
	if (gl_manager_legacy) {
		gl_manager_legacy->window_destroy(p_window);
	}
#endif
#ifdef RD_ENABLED
	if (rendering_device) {
		rendering_device->screen_free(p_window);
	}

	if (rendering_context) {
		rendering_context->window_destroy(p_window);
	}
#endif
	AccessibilityServer::window_destroy(p_window);

	windows.erase(p_window);

	if (last_focused_window == p_window) {
		last_focused_window = DisplayServerEnums::INVALID_WINDOW_ID;
	}
	update_presentation_mode();
}

void DisplayServerMacOS::window_resize(DisplayServerEnums::WindowID p_window, int p_width, int p_height) {
#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_context->window_set_size(p_window, p_width, p_height);
	}
#endif
#if defined(GLES3_ENABLED)
	if (gl_manager_legacy) {
		gl_manager_legacy->window_resize(p_window, p_width, p_height);
	}
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->window_resize(p_window, p_width, p_height);
	}
#endif
#endif
}

bool DisplayServerMacOS::has_feature(DisplayServerEnums::Feature p_feature) const {
	switch (p_feature) {
#ifndef DISABLE_DEPRECATED
		case DisplayServerEnums::FEATURE_GLOBAL_MENU: {
			return (native_menu && native_menu->has_feature(NativeMenu::FEATURE_GLOBAL_MENU));
		} break;
#endif
		case DisplayServerEnums::FEATURE_SUBWINDOWS:
		//case DisplayServerEnums::FEATURE_TOUCHSCREEN:
		case DisplayServerEnums::FEATURE_MOUSE:
		case DisplayServerEnums::FEATURE_MOUSE_WARP:
		case DisplayServerEnums::FEATURE_CLIPBOARD:
		case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
		case DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_INPUT:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_MIME:
		case DisplayServerEnums::FEATURE_IME:
		case DisplayServerEnums::FEATURE_WINDOW_TRANSPARENCY:
		case DisplayServerEnums::FEATURE_HIDPI:
		case DisplayServerEnums::FEATURE_ICON:
		case DisplayServerEnums::FEATURE_NATIVE_ICON:
		//case DisplayServerEnums::FEATURE_KEEP_SCREEN_ON:
		case DisplayServerEnums::FEATURE_SWAP_BUFFERS:
		case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH:
		case DisplayServerEnums::FEATURE_EXTEND_TO_TITLE:
		case DisplayServerEnums::FEATURE_SCREEN_CAPTURE:
		case DisplayServerEnums::FEATURE_STATUS_INDICATOR:
		case DisplayServerEnums::FEATURE_NATIVE_HELP:
		case DisplayServerEnums::FEATURE_WINDOW_DRAG:
		case DisplayServerEnums::FEATURE_SCREEN_EXCLUDE_FROM_CAPTURE:
		case DisplayServerEnums::FEATURE_EMOJI_AND_SYMBOL_PICKER:
		case DisplayServerEnums::FEATURE_WINDOW_EMBEDDING:
		case DisplayServerEnums::FEATURE_HDR_OUTPUT:
			return true;
		case DisplayServerEnums::FEATURE_ACCESSIBILITY_SCREEN_READER: {
			return AccessibilityServer::is_supported();
		} break;
		default: {
		}
	}
	return false;
}

String DisplayServerMacOS::get_name() const {
	return "macOS";
}

void DisplayServerMacOS::help_set_search_callbacks(const Callable &p_search_callback, const Callable &p_action_callback) {
	help_search_callback = p_search_callback;
	help_action_callback = p_action_callback;
}

Callable DisplayServerMacOS::_help_get_search_callback() const {
	return help_search_callback;
}

Callable DisplayServerMacOS::_help_get_action_callback() const {
	return help_action_callback;
}

Error DisplayServerMacOS::dialog_show(String p_title, String p_description, Vector<String> p_buttons, const Callable &p_callback) {
	_THREAD_SAFE_METHOD_

	NSAlert *window = [[NSAlert alloc] init];
	NSString *ns_title = [NSString stringWithUTF8String:p_title.utf8().get_data()];
	NSString *ns_description = [NSString stringWithUTF8String:p_description.utf8().get_data()];

	for (int i = 0; i < p_buttons.size(); i++) {
		NSString *ns_button = [NSString stringWithUTF8String:p_buttons[i].utf8().get_data()];
		[window addButtonWithTitle:ns_button];
	}
	[window setMessageText:ns_title];
	[window setInformativeText:ns_description];
	[window setAlertStyle:NSAlertStyleInformational];

	Variant button_pressed;
	NSInteger ret = [window runModal];
	if (ret == NSAlertFirstButtonReturn) {
		button_pressed = int64_t(0);
	} else if (ret == NSAlertSecondButtonReturn) {
		button_pressed = int64_t(1);
	} else if (ret == NSAlertThirdButtonReturn) {
		button_pressed = int64_t(2);
	} else {
		button_pressed = int64_t(2 + (ret - NSAlertThirdButtonReturn));
	}

	if (p_callback.is_valid()) {
		Variant ret;
		Callable::CallError ce;
		const Variant *args[1] = { &button_pressed };

		p_callback.callp(args, 1, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Failed to execute dialog callback: %s.", Variant::get_callable_error_text(p_callback, args, 1, ce)));
		}
	}

	return OK;
}

Error DisplayServerMacOS::file_dialog_show(const String &p_title, const String &p_current_directory, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) {
	return _file_dialog_with_options_show(p_title, p_current_directory, String(), p_filename, p_show_hidden, p_mode, p_filters, TypedArray<Dictionary>(), p_callback, false, p_window_id);
}

Error DisplayServerMacOS::file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) {
	return _file_dialog_with_options_show(p_title, p_current_directory, p_root, p_filename, p_show_hidden, p_mode, p_filters, p_options, p_callback, true, p_window_id);
}

Error DisplayServerMacOS::_file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, bool p_options_in_cb, DisplayServerEnums::WindowID p_window_id) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_INDEX_V(int(p_mode), DisplayServerEnums::FILE_DIALOG_MODE_SAVE_MAX, FAILED);

	NSString *url = [NSString stringWithUTF8String:p_current_directory.utf8().get_data()];

	NSWindow *nswindow = nullptr;
	if (windows.has(p_window_id) && !windows[p_window_id].is_popup) {
		nswindow = windows[p_window_id].window_object;
	}

	GodotOpenSaveDelegate *panel_delegate = [[GodotOpenSaveDelegate alloc] init];
	if (p_root.length() > 0) {
		[panel_delegate setRootPath:p_root];
	}
	Callable callback = p_callback; // Make a copy for async completion handler.
	if (p_mode == DisplayServerEnums::FILE_DIALOG_MODE_SAVE_FILE) {
		NSSavePanel *panel = [NSSavePanel savePanel];

		[panel setDirectoryURL:[NSURL fileURLWithPath:url]];
		[panel_delegate makeAccessoryView:panel filters:p_filters options:p_options];
		[panel setExtensionHidden:YES];
		[panel setCanSelectHiddenExtension:YES];
		[panel setCanCreateDirectories:YES];
		[panel setShowsHiddenFiles:p_show_hidden];
		[panel setDelegate:panel_delegate];
		if (p_filename != "") {
			NSString *fileurl = [NSString stringWithUTF8String:p_filename.utf8().get_data()];
			[panel setNameFieldStringValue:fileurl];
		}

		void (^completion_handler)(NSInteger ret) = ^(NSInteger ret) {
			if (ret == NSModalResponseOK) {
				// Save bookmark for folder.
				if (OS::is_sandboxed()) {
					NSArray *bookmarks = [[NSUserDefaults standardUserDefaults] arrayForKey:@"sec_bookmarks"];
					bool skip = false;
					for (id bookmark in bookmarks) {
						NSError *error = nil;
						BOOL isStale = NO;
						NSURL *exurl = [NSURL URLByResolvingBookmarkData:bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&error];
						if (!error && !isStale && ([[exurl path] compare:[[panel directoryURL] path]] == NSOrderedSame)) {
							skip = true;
							break;
						}
					}
					if (!skip) {
						NSError *error = nil;
						NSData *bookmark = [[panel directoryURL] bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
						if (!error) {
							NSArray *new_bookmarks = [bookmarks arrayByAddingObject:bookmark];
							[[NSUserDefaults standardUserDefaults] setObject:new_bookmarks forKey:@"sec_bookmarks"];
						}
					}
				}
				// Callback.
				Vector<String> files;
				String url;
				url.append_utf8([[[panel URL] path] UTF8String]);
				files.push_back([panel_delegate validateFilename:url]);
				if (callback.is_valid()) {
					if (p_options_in_cb) {
						Variant v_result = true;
						Variant v_files = files;
						Variant v_index = [panel_delegate getIndex];
						Variant v_opt = [panel_delegate getSelection];
						Variant ret;
						Callable::CallError ce;
						const Variant *args[4] = { &v_result, &v_files, &v_index, &v_opt };

						callback.callp(args, 4, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(callback, args, 4, ce)));
						}
					} else {
						Variant v_result = true;
						Variant v_files = files;
						Variant v_index = [panel_delegate getIndex];
						Variant ret;
						Callable::CallError ce;
						const Variant *args[3] = { &v_result, &v_files, &v_index };

						callback.callp(args, 3, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(callback, args, 3, ce)));
						}
					}
				}
			} else {
				if (callback.is_valid()) {
					if (p_options_in_cb) {
						Variant v_result = false;
						Variant v_files = Vector<String>();
						Variant v_index = [panel_delegate getIndex];
						Variant v_opt = [panel_delegate getSelection];
						Variant ret;
						Callable::CallError ce;
						const Variant *args[4] = { &v_result, &v_files, &v_index, &v_opt };

						callback.callp(args, 4, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(callback, args, 4, ce)));
						}
					} else {
						Variant v_result = false;
						Variant v_files = Vector<String>();
						Variant v_index = [panel_delegate getIndex];
						Variant ret;
						Callable::CallError ce;
						const Variant *args[3] = { &v_result, &v_files, &v_index };

						callback.callp(args, 3, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(callback, args, 3, ce)));
						}
					}
				}
			}
			if (p_window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
				callable_mp_static(&DisplayServer::window_move_to_foreground).call_deferred(p_window_id);
			}
		};
		if (nswindow) {
			[panel beginSheetModalForWindow:nswindow completionHandler:completion_handler];
		} else {
			[panel beginWithCompletionHandler:completion_handler];
		}
	} else {
		NSOpenPanel *panel = [NSOpenPanel openPanel];

		[panel setDirectoryURL:[NSURL fileURLWithPath:url]];
		[panel_delegate makeAccessoryView:panel filters:p_filters options:p_options];
		[panel setExtensionHidden:YES];
		[panel setCanSelectHiddenExtension:YES];
		[panel setCanCreateDirectories:YES];
		[panel setCanChooseFiles:(p_mode != DisplayServerEnums::FILE_DIALOG_MODE_OPEN_DIR)];
		[panel setCanChooseDirectories:(p_mode == DisplayServerEnums::FILE_DIALOG_MODE_OPEN_DIR || p_mode == DisplayServerEnums::FILE_DIALOG_MODE_OPEN_ANY)];
		[panel setTreatsFilePackagesAsDirectories:YES];
		[panel setShowsHiddenFiles:p_show_hidden];
		[panel setDelegate:panel_delegate];
		if (p_filename != "") {
			NSString *fileurl = [NSString stringWithUTF8String:p_filename.utf8().get_data()];
			[panel setNameFieldStringValue:fileurl];
		}
		[panel setAllowsMultipleSelection:(p_mode == DisplayServerEnums::FILE_DIALOG_MODE_OPEN_FILES)];

		void (^completion_handler)(NSInteger ret) = ^(NSInteger ret) {
			if (ret == NSModalResponseOK) {
				// Save bookmark for folder.
				NSArray *urls = [(NSOpenPanel *)panel URLs];
				if (OS::is_sandboxed()) {
					NSArray *bookmarks = [[NSUserDefaults standardUserDefaults] arrayForKey:@"sec_bookmarks"];
					NSMutableArray *new_bookmarks = [bookmarks mutableCopy];
					for (NSUInteger i = 0; i != [urls count]; ++i) {
						bool skip = false;
						for (id bookmark in bookmarks) {
							NSError *error = nil;
							BOOL isStale = NO;
							NSURL *exurl = [NSURL URLByResolvingBookmarkData:bookmark options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:&isStale error:&error];
							if (!error && !isStale && ([[exurl path] compare:[[urls objectAtIndex:i] path]] == NSOrderedSame)) {
								skip = true;
								break;
							}
						}
						if (!skip) {
							NSError *error = nil;
							NSData *bookmark = [[urls objectAtIndex:i] bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
							if (!error) {
								[new_bookmarks addObject:bookmark];
							}
						}
					}
					[[NSUserDefaults standardUserDefaults] setObject:new_bookmarks forKey:@"sec_bookmarks"];
				}
				// Callback.
				Vector<String> files;
				for (NSUInteger i = 0; i != [urls count]; ++i) {
					String url;
					url.append_utf8([[[urls objectAtIndex:i] path] UTF8String]);
					files.push_back([panel_delegate validateFilename:url]);
				}
				if (callback.is_valid()) {
					if (p_options_in_cb) {
						Variant v_result = true;
						Variant v_files = files;
						Variant v_index = [panel_delegate getIndex];
						Variant v_opt = [panel_delegate getSelection];
						Variant ret;
						Callable::CallError ce;
						const Variant *args[4] = { &v_result, &v_files, &v_index, &v_opt };

						callback.callp(args, 4, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(callback, args, 4, ce)));
						}
					} else {
						Variant v_result = true;
						Variant v_files = files;
						Variant v_index = [panel_delegate getIndex];
						Variant ret;
						Callable::CallError ce;
						const Variant *args[3] = { &v_result, &v_files, &v_index };

						callback.callp(args, 3, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(callback, args, 3, ce)));
						}
					}
				}
			} else {
				if (callback.is_valid()) {
					if (p_options_in_cb) {
						Variant v_result = false;
						Variant v_files = Vector<String>();
						Variant v_index = [panel_delegate getIndex];
						Variant v_opt = [panel_delegate getSelection];
						Variant ret;
						Callable::CallError ce;
						const Variant *args[4] = { &v_result, &v_files, &v_index, &v_opt };

						callback.callp(args, 4, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(callback, args, 4, ce)));
						}
					} else {
						Variant v_result = false;
						Variant v_files = Vector<String>();
						Variant v_index = [panel_delegate getIndex];
						Variant ret;
						Callable::CallError ce;
						const Variant *args[3] = { &v_result, &v_files, &v_index };

						callback.callp(args, 3, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(callback, args, 3, ce)));
						}
					}
				}
			}
			if (p_window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
				callable_mp_static(&DisplayServer::window_move_to_foreground).call_deferred(p_window_id);
			}
		};
		if (nswindow) {
			[panel beginSheetModalForWindow:nswindow completionHandler:completion_handler];
		} else {
			[panel beginWithCompletionHandler:completion_handler];
		}
	}

	return OK;
}

Error DisplayServerMacOS::dialog_input_text(String p_title, String p_description, String p_partial, const Callable &p_callback) {
	_THREAD_SAFE_METHOD_

	NSAlert *window = [[NSAlert alloc] init];
	NSString *ns_title = [NSString stringWithUTF8String:p_title.utf8().get_data()];
	NSString *ns_description = [NSString stringWithUTF8String:p_description.utf8().get_data()];
	NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 250, 30)];

	[window addButtonWithTitle:@"OK"];
	[window setMessageText:ns_title];
	[window setInformativeText:ns_description];
	[window setAlertStyle:NSAlertStyleInformational];

	[input setStringValue:[NSString stringWithUTF8String:p_partial.utf8().get_data()]];
	[window setAccessoryView:input];

	[window runModal];

	String ret;
	ret.append_utf8([[input stringValue] UTF8String]);

	if (p_callback.is_valid()) {
		Variant v_result = ret;
		Variant ret;
		Callable::CallError ce;
		const Variant *args[1] = { &v_result };

		p_callback.callp(args, 1, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Failed to execute input dialog callback: %s.", Variant::get_callable_error_text(p_callback, args, 1, ce)));
		}
	}

	return OK;
}

void DisplayServerMacOS::_mouse_apply_mode(DisplayServerEnums::MouseMode p_prev_mode, DisplayServerEnums::MouseMode p_new_mode) {
	_THREAD_SAFE_METHOD_

	DisplayServerEnums::WindowID window_id = _get_focused_window_or_popup();
	if (!windows.has(window_id)) {
		window_id = DisplayServerEnums::MAIN_WINDOW_ID;
	}
	WindowData &wd = windows[window_id];

	bool show_cursor = (p_new_mode == DisplayServerEnums::MOUSE_MODE_VISIBLE || p_new_mode == DisplayServerEnums::MOUSE_MODE_CONFINED);
	bool previously_shown = (p_prev_mode == DisplayServerEnums::MOUSE_MODE_VISIBLE || p_prev_mode == DisplayServerEnums::MOUSE_MODE_CONFINED);

	if (show_cursor && !previously_shown) {
		window_id = get_window_at_screen_position(mouse_get_position());
		mouse_enter_window(window_id);
	}

	if (p_new_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
		// Apple Docs state that the display parameter is not used.
		// "This parameter is not used. By default, you may pass kCGDirectMainDisplay."
		// https://developer.apple.com/library/mac/documentation/graphicsimaging/reference/Quartz_Services_Ref/Reference/reference.html
		if (previously_shown) {
			CGDisplayHideCursor(kCGDirectMainDisplay);
		}
		CGAssociateMouseAndMouseCursorPosition(false);
		[wd.window_object setMovable:NO];
		const NSRect contentRect = [wd.window_view frame];
		NSRect pointInWindowRect = NSMakeRect(contentRect.size.width / 2, contentRect.size.height / 2, 0, 0);
		NSPoint pointOnScreen = [[wd.window_view window] convertRectToScreen:pointInWindowRect].origin;
		CGPoint lMouseWarpPos = { pointOnScreen.x, CGDisplayBounds(CGMainDisplayID()).size.height - pointOnScreen.y };
		CGWarpMouseCursorPosition(lMouseWarpPos);
	} else if (p_new_mode == DisplayServerEnums::MOUSE_MODE_HIDDEN) {
		if (previously_shown) {
			CGDisplayHideCursor(kCGDirectMainDisplay);
		}
		[wd.window_object setMovable:YES];
		CGAssociateMouseAndMouseCursorPosition(true);
	} else if (p_new_mode == DisplayServerEnums::MOUSE_MODE_CONFINED) {
		CGDisplayShowCursor(kCGDirectMainDisplay);
		[wd.window_object setMovable:NO];
		CGAssociateMouseAndMouseCursorPosition(false);
	} else if (p_new_mode == DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN) {
		if (previously_shown) {
			CGDisplayHideCursor(kCGDirectMainDisplay);
		}
		[wd.window_object setMovable:NO];
		CGAssociateMouseAndMouseCursorPosition(false);
	} else { // DisplayServerEnums::MOUSE_MODE_VISIBLE
		CGDisplayShowCursor(kCGDirectMainDisplay);
		[wd.window_object setMovable:YES];
		CGAssociateMouseAndMouseCursorPosition(true);
	}

	last_warp = [[NSProcessInfo processInfo] systemUptime];
	ignore_warp = true;
	warp_events.clear();

	if (show_cursor) {
		cursor_update_shape();
	}
}

bool DisplayServerMacOS::update_mouse_wrap(WindowData &p_wd, NSPoint &r_delta, NSPoint &r_mpos, NSTimeInterval p_timestamp) {
	_THREAD_SAFE_METHOD_

	if (ignore_warp) {
		// Discard late events, before warp.
		if (p_timestamp < last_warp) {
			return true;
		}
		ignore_warp = false;
		return true;
	}

	if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED || mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN) {
		// Discard late events.
		if (p_timestamp < last_warp) {
			return true;
		}

		// Warp affects next event delta, subtract previous warp deltas.
		List<WarpEvent>::Element *F = warp_events.front();
		while (F) {
			if (F->get().timestamp < p_timestamp) {
				List<DisplayServerMacOS::WarpEvent>::Element *E = F;
				r_delta.x -= E->get().delta.x;
				r_delta.y -= E->get().delta.y;
				F = F->next();
				warp_events.erase(E);
			} else {
				F = F->next();
			}
		}

		// Confine mouse position to the window, and update delta.
		NSRect frame = [p_wd.window_view frame];
		NSRect frameOnScreen = [[p_wd.window_view window] convertRectToScreen:frame];
		frameOnScreen.origin.y = CGDisplayBounds(CGMainDisplayID()).size.height - frameOnScreen.origin.y;

		CGEventRef ourEvent = CGEventCreate(nullptr);
		NSPoint conf_pos = CGEventGetLocation(ourEvent);
		CFRelease(ourEvent);

		NSPoint prev_conf_pos = conf_pos;

		conf_pos.x = CLAMP(conf_pos.x + r_delta.x, frameOnScreen.origin.x, frameOnScreen.origin.x + frameOnScreen.size.width);
		conf_pos.y = CLAMP(conf_pos.y + r_delta.y, frameOnScreen.origin.y - frameOnScreen.size.height, frameOnScreen.origin.y);
		r_delta.x = conf_pos.x - prev_conf_pos.x;
		r_delta.y = conf_pos.y - prev_conf_pos.y;

		NSPoint wnd_point = NSMakePoint(conf_pos.x, CGDisplayBounds(CGMainDisplayID()).size.height - conf_pos.y);
		r_mpos = [[p_wd.window_view window] convertPointFromScreen:wnd_point];

		// Move mouse cursor.
		CGWarpMouseCursorPosition(conf_pos);

		// Save warp data.
		last_warp = [[NSProcessInfo processInfo] systemUptime];

		DisplayServerMacOS::WarpEvent ev;
		ev.timestamp = last_warp;
		ev.delta = r_delta;
		warp_events.push_back(ev);
	}

	return false;
}

void DisplayServerMacOS::warp_mouse(const Point2i &p_position) {
	_THREAD_SAFE_METHOD_

	if (mouse_mode != DisplayServerEnums::MOUSE_MODE_CAPTURED) {
		DisplayServerEnums::WindowID window_id = _get_focused_window_or_popup();
		if (!windows.has(window_id)) {
			window_id = DisplayServerEnums::MAIN_WINDOW_ID;
		}
		WindowData &wd = windows[window_id];

		// Local point in window coords.
		const NSRect contentRect = [wd.window_view frame];
		const float scale = screen_get_max_scale();
		NSRect pointInWindowRect = NSMakeRect(p_position.x / scale, contentRect.size.height - (p_position.y / scale), scale, scale);
		NSPoint pointOnScreen = [[wd.window_view window] convertRectToScreen:pointInWindowRect].origin;

		// Point in screen coords.
		CGPoint lMouseWarpPos = { pointOnScreen.x, CGDisplayBounds(CGMainDisplayID()).size.height - pointOnScreen.y };

		// Do the warping.
		CGEventSourceRef lEventRef = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
		CGEventSourceSetLocalEventsSuppressionInterval(lEventRef, 0.0);
		CGAssociateMouseAndMouseCursorPosition(false);
		CGWarpMouseCursorPosition(lMouseWarpPos);
		if (mouse_mode != DisplayServerEnums::MOUSE_MODE_CONFINED && mouse_mode != DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN) {
			CGAssociateMouseAndMouseCursorPosition(true);
		}
	}
}

Point2i DisplayServerMacOS::mouse_get_position() const {
	_THREAD_SAFE_METHOD_

	const NSPoint mouse_pos = [NSEvent mouseLocation];
	const float scale = screen_get_max_scale();

	for (NSScreen *screen in [NSScreen screens]) {
		NSRect frame = [screen frame];
		if (NSMouseInRect(mouse_pos, frame, NO)) {
			Vector2i pos = Vector2i((int)mouse_pos.x, (int)mouse_pos.y);
			pos *= scale;
			pos -= _get_screens_origin();
			pos.y *= -1;
			return pos;
		}
	}
	return Vector2i();
}

BitField<MouseButtonMask> DisplayServerMacOS::mouse_get_button_state() const {
	BitField<MouseButtonMask> last_button_state = MouseButtonMask::NONE;

	NSUInteger buttons = [NSEvent pressedMouseButtons];
	if (buttons & (1 << 0)) {
		last_button_state.set_flag(MouseButtonMask::LEFT);
	}
	if (buttons & (1 << 1)) {
		last_button_state.set_flag(MouseButtonMask::RIGHT);
	}
	if (buttons & (1 << 2)) {
		last_button_state.set_flag(MouseButtonMask::MIDDLE);
	}
	if (buttons & (1 << 3)) {
		last_button_state.set_flag(MouseButtonMask::MB_XBUTTON1);
	}
	if (buttons & (1 << 4)) {
		last_button_state.set_flag(MouseButtonMask::MB_XBUTTON2);
	}
	return last_button_state;
}

int DisplayServerMacOS::get_screen_count() const {
	_THREAD_SAFE_METHOD_

	NSArray *screenArray = [NSScreen screens];
	return [screenArray count];
}

int DisplayServerMacOS::get_keyboard_focus_screen() const {
	const NSUInteger index = [[NSScreen screens] indexOfObject:[NSScreen mainScreen]];
	return (index == NSNotFound) ? get_primary_screen() : index;
}

Point2i DisplayServerMacOS::screen_get_position(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Point2i());

	Point2i position = _get_native_screen_position(p_screen) - _get_screens_origin();
	// macOS native y-coordinate relative to _get_screens_origin() is negative,
	// Godot expects a positive value.
	position.y *= -1;
	return position;
}

Size2i DisplayServerMacOS::screen_get_size(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Size2i());

	NSArray *screenArray = [NSScreen screens];
	if ((NSUInteger)p_screen < [screenArray count]) {
		// Note: Use frame to get the whole screen size.
		NSRect nsrect = [[screenArray objectAtIndex:p_screen] frame];
		return Size2i(nsrect.size.width, nsrect.size.height) * screen_get_max_scale();
	}

	return Size2i();
}

int DisplayServerMacOS::screen_get_dpi(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 72);

	NSArray *screenArray = [NSScreen screens];
	if ((NSUInteger)p_screen < [screenArray count]) {
		NSScreen *screen = [screenArray objectAtIndex:p_screen];
		NSDictionary *description = [screen deviceDescription];

		const NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
		const CGSize displayPhysicalSize = CGDisplayScreenSize(_get_display_id_for_screen(screen));
		float scale = [screen backingScaleFactor];

		float den2 = (displayPhysicalSize.width / 25.4f) * (displayPhysicalSize.width / 25.4f) + (displayPhysicalSize.height / 25.4f) * (displayPhysicalSize.height / 25.4f);
		if (den2 > 0.0f) {
			return std::ceil(std::sqrt(displayPixelSize.width * displayPixelSize.width + displayPixelSize.height * displayPixelSize.height) / std::sqrt(den2) * scale);
		}
	}

	return 72;
}

float DisplayServerMacOS::screen_get_scale(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 1.0f);

	if (OS::is_hidpi_allowed()) {
		NSArray<NSScreen *> *screens = NSScreen.screens;
		NSUInteger index = (NSUInteger)p_screen;
		if (index < screens.count) {
			return std::fmax(1.0f, screens[index].backingScaleFactor);
		}
	}

	return 1.f;
}

float DisplayServerMacOS::screen_get_max_scale() const {
	_THREAD_SAFE_METHOD_

	// Note: Do not update max display scale on screen configuration change, existing editor windows can't be rescaled on the fly.
	return display_max_scale;
}

Rect2i DisplayServerMacOS::screen_get_usable_rect(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	NSArray *screenArray = [NSScreen screens];
	if ((NSUInteger)p_screen < [screenArray count]) {
		const float scale = screen_get_max_scale();
		NSRect nsrect = [[screenArray objectAtIndex:p_screen] visibleFrame];

		Point2i position = Point2i(nsrect.origin.x, nsrect.origin.y + nsrect.size.height) * scale - _get_screens_origin();
		position.y *= -1;
		Size2i size = Size2i(nsrect.size.width, nsrect.size.height) * scale;

		return Rect2i(position, size);
	}

	return Rect2i();
}

TypedArray<Rect2> DisplayServerMacOS::get_display_cutouts(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	TypedArray<Rect2> ret = TypedArray<Rect2>();

	ERR_FAIL_INDEX_V(p_screen, screen_count, ret);

	NSArray *screenArray = [NSScreen screens];
	if ((NSUInteger)p_screen < [screenArray count]) {
		const float scale = screen_get_max_scale();
		NSRect nsrect = [[screenArray objectAtIndex:p_screen] frame];
		NSEdgeInsets safeAreaInsets = [[screenArray objectAtIndex:p_screen] safeAreaInsets];

		float inset_left = safeAreaInsets.left * scale;
		float inset_top = safeAreaInsets.top * scale;
		float inset_right = safeAreaInsets.right * scale;
		float inset_bottom = safeAreaInsets.bottom * scale;

		float screen_width = nsrect.size.width * scale;
		float screen_height = nsrect.size.height * scale;

		if (inset_left > 0) {
			Rect2 rect = Rect2(0.0, 0.0, inset_left, screen_height);
			ret.push_back(rect);
		}

		if (inset_top > 0) {
			Rect2 rect = Rect2(0.0, 0.0, screen_width, inset_top);
			ret.push_back(rect);
		}

		if (inset_bottom > 0) {
			Rect2 rect = Rect2(0.0, screen_height - inset_bottom, screen_width, inset_bottom);
			ret.push_back(rect);
		}

		if (inset_right > 0) {
			Rect2 rect = Rect2(screen_width - inset_right, 0.0, inset_right, screen_width);
			ret.push_back(rect);
		}
	}

	return ret;
}

Rect2i DisplayServerMacOS::get_display_safe_area(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	NSArray *screenArray = [NSScreen screens];
	if ((NSUInteger)p_screen < [screenArray count]) {
		const float scale = screen_get_max_scale();
		NSRect nsrect = [[screenArray objectAtIndex:p_screen] frame];
		NSEdgeInsets safeAreaInsets = [[screenArray objectAtIndex:p_screen] safeAreaInsets];

		int inset_left = safeAreaInsets.left * scale;
		int inset_top = safeAreaInsets.top * scale;
		int inset_right = safeAreaInsets.right * scale;
		int inset_bottom = safeAreaInsets.bottom * scale;

		int screen_width = nsrect.size.width * scale;
		int screen_height = nsrect.size.height * scale;

		return Rect2i(
				inset_left,
				inset_top,
				screen_width - (inset_left + inset_right),
				screen_height - (inset_top + inset_bottom));
	}

	return Rect2i();
}

Color DisplayServerMacOS::screen_get_pixel(const Point2i &p_position) const {
	HashSet<CGWindowID> exclude_windows;
	for (HashMap<DisplayServerEnums::WindowID, WindowData>::ConstIterator E = windows.begin(); E; ++E) {
		if (E->value.hide_from_capture) {
			exclude_windows.insert([E->value.window_object windowNumber]);
		}
	}

	CFArrayRef on_screen_windows = CGWindowListCreate(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
	CFMutableArrayRef capture_windows = CFArrayCreateMutableCopy(nullptr, 0, on_screen_windows);
	for (long i = CFArrayGetCount(on_screen_windows) - 1; i >= 0; i--) {
		CGWindowID window = (CGWindowID)(uintptr_t)CFArrayGetValueAtIndex(capture_windows, i);
		if (exclude_windows.has(window)) {
			CFArrayRemoveValueAtIndex(capture_windows, i);
		}
	}

	Point2i position = p_position - Vector2i(1, 1);
	position -= screen_get_position(0); // Note: coordinates where the screen origin is in the upper-left corner of the main display and y-axis values increase downward.
	position /= screen_get_max_scale();

	Color color;
	CGImageRef image = CGWindowListCreateImageFromArray(CGRectMake(position.x, position.y, 1, 1), capture_windows, kCGWindowListOptionAll);
	if (image) {
		CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
		if (color_space) {
			uint8_t img_data[4];
			CGContextRef context = CGBitmapContextCreate(img_data, 1, 1, 8, 4, color_space, (uint32_t)kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
			if (context) {
				CGContextDrawImage(context, CGRectMake(0, 0, 1, 1), image);
				color = Color(img_data[0] / 255.0f, img_data[1] / 255.0f, img_data[2] / 255.0f, img_data[3] / 255.0f);
				CGContextRelease(context);
			}
			CGColorSpaceRelease(color_space);
		}
		CGImageRelease(image);
	}
	return color;
}

Ref<Image> DisplayServerMacOS::screen_get_image(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Ref<Image>());

	HashSet<CGWindowID> exclude_windows;
	for (HashMap<DisplayServerEnums::WindowID, WindowData>::ConstIterator E = windows.begin(); E; ++E) {
		if (E->value.hide_from_capture) {
			exclude_windows.insert([E->value.window_object windowNumber]);
		}
	}

	CFArrayRef on_screen_windows = CGWindowListCreate(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
	CFMutableArrayRef capture_windows = CFArrayCreateMutableCopy(nullptr, 0, on_screen_windows);
	for (long i = CFArrayGetCount(on_screen_windows) - 1; i >= 0; i--) {
		CGWindowID window = (CGWindowID)(uintptr_t)CFArrayGetValueAtIndex(capture_windows, i);
		if (exclude_windows.has(window)) {
			CFArrayRemoveValueAtIndex(capture_windows, i);
		}
	}

	Point2i position = screen_get_position(p_screen);
	position -= screen_get_position(0); // Note: coordinates where the screen origin is in the upper-left corner of the main display and y-axis values increase downward.
	position /= screen_get_max_scale();

	Size2i size = screen_get_size(p_screen);
	size /= screen_get_max_scale();

	Ref<Image> img;
	CGImageRef image = CGWindowListCreateImageFromArray(CGRectMake(position.x, position.y, size.width, size.height), capture_windows, kCGWindowListOptionAll);
	if (image) {
		CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
		if (color_space) {
			NSUInteger width = CGImageGetWidth(image);
			NSUInteger height = CGImageGetHeight(image);

			Vector<uint8_t> img_data;
			img_data.resize(height * width * 4);
			CGContextRef context = CGBitmapContextCreate(img_data.ptrw(), width, height, 8, 4 * width, color_space, (uint32_t)kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
			if (context) {
				CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
				img = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, img_data);
				CGContextRelease(context);
				img->resize(screen_get_size(p_screen).x, screen_get_size(p_screen).y, Image::INTERPOLATE_NEAREST);
			}
			CGColorSpaceRelease(color_space);
		}
		CGImageRelease(image);
	}
	return img;
}

Ref<Image> DisplayServerMacOS::screen_get_image_rect(const Rect2i &p_rect) const {
	HashSet<CGWindowID> exclude_windows;
	for (HashMap<DisplayServerEnums::WindowID, WindowData>::ConstIterator E = windows.begin(); E; ++E) {
		if (E->value.hide_from_capture) {
			exclude_windows.insert([E->value.window_object windowNumber]);
		}
	}

	CFArrayRef on_screen_windows = CGWindowListCreate(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
	CFMutableArrayRef capture_windows = CFArrayCreateMutableCopy(nullptr, 0, on_screen_windows);
	for (long i = CFArrayGetCount(on_screen_windows) - 1; i >= 0; i--) {
		CGWindowID window = (CGWindowID)(uintptr_t)CFArrayGetValueAtIndex(capture_windows, i);
		if (exclude_windows.has(window)) {
			CFArrayRemoveValueAtIndex(capture_windows, i);
		}
	}

	Point2i position = p_rect.position;
	position -= screen_get_position(0); // Note: coordinates where the screen origin is in the upper-left corner of the main display and y-axis values increase downward.
	position /= screen_get_max_scale();

	Size2i size = p_rect.size;
	size /= screen_get_max_scale();

	Ref<Image> img;
	CGImageRef image = CGWindowListCreateImageFromArray(CGRectMake(position.x, position.y, size.width, size.height), capture_windows, kCGWindowListOptionAll);
	if (image) {
		CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
		if (color_space) {
			NSUInteger width = CGImageGetWidth(image);
			NSUInteger height = CGImageGetHeight(image);

			Vector<uint8_t> img_data;
			img_data.resize_initialized(height * width * 4);
			CGContextRef context = CGBitmapContextCreate(img_data.ptrw(), width, height, 8, 4 * width, color_space, (uint32_t)kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
			if (context) {
				CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
				img = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, img_data);
				CGContextRelease(context);
				img->resize(p_rect.size.x, p_rect.size.y, Image::INTERPOLATE_NEAREST);
			}
			CGColorSpaceRelease(color_space);
		}
		CGImageRelease(image);
	}
	return img;
}

Vector<DisplayServerEnums::WindowID> DisplayServerMacOS::get_window_list() const {
	_THREAD_SAFE_METHOD_

	Vector<int> ret;
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		ret.push_back(E.key);
	}
	return ret;
}

DisplayServerEnums::WindowID DisplayServerMacOS::create_sub_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i &p_rect, bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent) {
	_THREAD_SAFE_METHOD_

	DisplayServerEnums::WindowID id = _create_window(p_mode, p_vsync_mode, p_rect);

	uint32_t set_flags = p_flags & ~(DisplayServerEnums::WINDOW_FLAG_MAX - 1); // Clear the flags that are not supported by the window.
	while (set_flags != 0) {
		// Find the index of the next set bit.
		uint32_t index = (uint32_t)__builtin_ctzll(set_flags);
		// Clear the set bit.
		set_flags &= (set_flags - 1);
		window_set_flag(DisplayServerEnums::WindowFlags(index), true, id);
	}

#ifdef RD_ENABLED
	if (rendering_device) {
		rendering_device->screen_create(id);
	}
#endif

	window_set_exclusive(id, p_exclusive);
	if (p_transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
		window_set_transient(id, p_transient_parent);
	}

	return id;
}

void DisplayServerMacOS::show_window(DisplayServerEnums::WindowID p_id) {
	WindowData &wd = windows[p_id];

	if (p_id == DisplayServerEnums::MAIN_WINDOW_ID) {
		[GodotApp activateApplication];
	}

	popup_open(p_id);
	if ([wd.window_object isMiniaturized]) {
		return;
	} else if (wd.no_focus) {
		if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
			WindowData &wd_parent = windows[wd.transient_parent];
			[wd.window_object orderWindow:NSWindowAbove relativeTo:[wd_parent.window_object windowNumber]];
		} else if (p_id != DisplayServerEnums::MAIN_WINDOW_ID) {
			[wd.window_object orderWindow:NSWindowAbove relativeTo:[windows[DisplayServerEnums::MAIN_WINDOW_ID].window_object windowNumber]];
		} else {
			[wd.window_object orderFront:nil];
		}
	} else {
		[wd.window_object makeKeyAndOrderFront:nil];
	}
}

void DisplayServerMacOS::delete_sub_window(DisplayServerEnums::WindowID p_id) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_id));
	ERR_FAIL_COND_MSG(p_id == DisplayServerEnums::MAIN_WINDOW_ID, "Main window can't be deleted");

	WindowData &wd = windows[p_id];

	[wd.window_object setContentView:nil];
	// This will cause the delegate to release the window.
	[wd.window_object close];

	mouse_enter_window(get_window_at_screen_position(mouse_get_position()));
}

void DisplayServerMacOS::window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.rect_changed_callback = p_callable;
}

void DisplayServerMacOS::window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.event_callback = p_callable;
}

void DisplayServerMacOS::window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.input_event_callback = p_callable;
}

void DisplayServerMacOS::window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.input_text_callback = p_callable;
}

void DisplayServerMacOS::window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.drop_files_callback = p_callable;
}

void DisplayServerMacOS::window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	[wd.window_object setTitle:[NSString stringWithUTF8String:p_title.utf8().get_data()]];
}

Size2i DisplayServerMacOS::window_get_title_size(const String &p_title, DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	Size2i size;
	ERR_FAIL_COND_V(!windows.has(p_window), size);

	const WindowData &wd = windows[p_window];
	if (wd.fullscreen || wd.borderless) {
		return size;
	}
	if ([wd.window_object respondsToSelector:@selector(isMiniaturized)]) {
		if ([wd.window_object isMiniaturized]) {
			return size;
		}
	}

	float scale = screen_get_max_scale();

	if (wd.window_button_view) {
		size.x = ([wd.window_button_view getOffset].x + [wd.window_button_view frame].size.width);
		size.y = ([wd.window_button_view getOffset].y + [wd.window_button_view frame].size.height);
	} else {
		NSButton *cb = [wd.window_object standardWindowButton:NSWindowCloseButton];
		NSButton *mb = [wd.window_object standardWindowButton:NSWindowMiniaturizeButton];
		float cb_frame = NSMinX([cb frame]);
		float mb_frame = NSMinX([mb frame]);
		bool is_rtl = ([wd.window_object windowTitlebarLayoutDirection] == NSUserInterfaceLayoutDirectionRightToLeft);

		float window_buttons_spacing = (is_rtl) ? (cb_frame - mb_frame) : (mb_frame - cb_frame);
		size.x = window_buttons_spacing * 4;
		size.y = [cb frame].origin.y + [cb frame].size.height;
	}

	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont titleBarFontOfSize:0], NSFontAttributeName, nil];
	NSSize text_size = [[[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:p_title.utf8().get_data()] attributes:attributes] size];
	size.x += text_size.width;
	size.y = MAX(size.y, text_size.height);

	return size * scale;
}

void DisplayServerMacOS::window_set_mouse_passthrough(const Vector<Vector2> &p_region, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	wd.mpath = p_region;
}

int DisplayServerMacOS::window_get_current_screen(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND_V(!windows.has(p_window), DisplayServerEnums::INVALID_SCREEN);
	const WindowData &wd = windows[p_window];

	const NSUInteger index = [[NSScreen screens] indexOfObject:[wd.window_object screen]];
	return (index == NSNotFound) ? 0 : index;
}

void DisplayServerMacOS::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX(p_screen, screen_count);

	if (window_get_current_screen(p_window) == p_screen) {
		return;
	}
	WindowData &wd = windows[p_window];

	bool was_fullscreen = false;
	if (wd.fullscreen) {
		// Temporary exit fullscreen mode to move window.
		[wd.window_object toggleFullScreen:nil];
		was_fullscreen = true;
	}

	bool was_maximized = false;
	if (!was_fullscreen && NSEqualRects([wd.window_object frame], [[wd.window_object screen] visibleFrame])) {
		[wd.window_object zoom:nil];
		was_maximized = true;
	}

	Rect2i srect = screen_get_usable_rect(p_screen);
	Point2i wpos = window_get_position(p_window) - screen_get_position(window_get_current_screen(p_window));
	Size2i wsize = window_get_size(p_window);
	wpos += srect.position;

	wpos = wpos.clamp(srect.position, srect.position + srect.size - wsize / 3);
	window_set_position(wpos, p_window);

	if (was_maximized) {
		[wd.window_object zoom:nil];
	}

	if (was_fullscreen) {
		// Re-enter fullscreen mode.
		[wd.window_object toggleFullScreen:nil];
	}
}

void DisplayServerMacOS::reparent_check(DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	NSScreen *screen = [wd.window_object screen];

	_update_hdr_output(p_window, wd.hdr_output);

	_window_update_display_id(&wd);

	if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
		WindowData &wd_parent = windows[wd.transient_parent];
		NSScreen *parent_screen = [wd_parent.window_object screen];

		if (parent_screen == screen) {
			if (![[wd_parent.window_object childWindows] containsObject:wd.window_object]) {
				[wd.window_object setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
				[wd_parent.window_object addChildWindow:wd.window_object ordered:NSWindowAbove];
			}
		} else {
			if ([[wd_parent.window_object childWindows] containsObject:wd.window_object]) {
				[wd_parent.window_object removeChildWindow:wd.window_object];
				[wd.window_object setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
				if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
					WindowData &wd_parent = windows[wd.transient_parent];
					[wd.window_object orderWindow:NSWindowAbove relativeTo:[wd_parent.window_object windowNumber]];
				} else if (p_window != DisplayServerEnums::MAIN_WINDOW_ID) {
					[wd.window_object orderWindow:NSWindowAbove relativeTo:[windows[DisplayServerEnums::MAIN_WINDOW_ID].window_object windowNumber]];
				} else {
					[wd.window_object orderFront:nil];
				}
			}
		}
	}

	for (const DisplayServerEnums::WindowID &child : wd.transient_children) {
		WindowData &wd_child = windows[child];
		NSScreen *child_screen = [wd_child.window_object screen];

		if (child_screen == screen) {
			if (![[wd.window_object childWindows] containsObject:wd_child.window_object]) {
				if (wd_child.fullscreen) {
					[wd_child.window_object toggleFullScreen:nil];
				}
				[wd_child.window_object setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
				[wd.window_object addChildWindow:wd_child.window_object ordered:NSWindowAbove];
			}
		} else {
			if ([[wd.window_object childWindows] containsObject:wd_child.window_object]) {
				[wd.window_object removeChildWindow:wd_child.window_object];
				[wd_child.window_object setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
			}
		}
	}
}

void DisplayServerMacOS::window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive) {
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	if (wd.exclusive != p_exclusive) {
		wd.exclusive = p_exclusive;
		reparent_check(p_window);
	}
}

Point2i DisplayServerMacOS::window_get_position(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Point2i());
	const WindowData &wd = windows[p_window];

	// Use content rect position (without titlebar / window border).
	const NSRect contentRect = [wd.window_view frame];
	const NSRect nsrect = [wd.window_object convertRectToScreen:contentRect];
	Point2i pos;

	// Return the position of the top-left corner, for macOS the y starts at the bottom.
	const float scale = screen_get_max_scale();
	pos.x = nsrect.origin.x;
	pos.y = (nsrect.origin.y + nsrect.size.height);
	pos *= scale;
	pos -= _get_screens_origin();
	// macOS native y-coordinate relative to _get_screens_origin() is negative,
	// Godot expects a positive value.
	pos.y *= -1;
	return pos;
}

Point2i DisplayServerMacOS::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Point2i());
	const WindowData &wd = windows[p_window];

	const NSRect nsrect = [wd.window_object frame];
	Point2i pos;

	// Return the position of the top-left corner, for macOS the y starts at the bottom.
	const float scale = screen_get_max_scale();
	pos.x = nsrect.origin.x;
	pos.y = (nsrect.origin.y + nsrect.size.height);
	pos *= scale;
	pos -= _get_screens_origin();
	// macOS native y-coordinate relative to _get_screens_origin() is negative,
	// Godot expects a positive value.
	pos.y *= -1;
	return pos;
}

void DisplayServerMacOS::window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.fullscreen) {
		return;
	}

	Point2i position = p_position;
	// macOS native y-coordinate relative to _get_screens_origin() is negative,
	// Godot passes a positive value.
	position.y *= -1;
	position += _get_screens_origin();
	position /= screen_get_max_scale();

	// Remove titlebar / window border size.
	const NSRect contentRect = [wd.window_view frame];
	const NSRect windowRect = [wd.window_object frame];
	const NSRect nsrect = [wd.window_object convertRectToScreen:contentRect];
	Point2i offset;
	offset.x = (nsrect.origin.x - windowRect.origin.x);
	offset.y = (nsrect.origin.y + nsrect.size.height);
	offset.y -= (windowRect.origin.y + windowRect.size.height);

	[wd.window_object setFrameTopLeftPoint:NSMakePoint(position.x - offset.x, position.y - offset.y)];

	_update_window_style(wd, p_window);
	update_mouse_pos(wd, [wd.window_object mouseLocationOutsideOfEventStream]);
}

void DisplayServerMacOS::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) {
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND(p_window == p_parent);

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd_window = windows[p_window];

	ERR_FAIL_COND(wd_window.transient_parent == p_parent);

	ERR_FAIL_COND_MSG(wd_window.on_top, "Windows with the 'on top' can't become transient.");
	if (p_parent == DisplayServerEnums::INVALID_WINDOW_ID) {
		// Remove transient.
		ERR_FAIL_COND(wd_window.transient_parent == DisplayServerEnums::INVALID_WINDOW_ID);
		ERR_FAIL_COND(!windows.has(wd_window.transient_parent));

		WindowData &wd_parent = windows[wd_window.transient_parent];

		wd_window.transient_parent = DisplayServerEnums::INVALID_WINDOW_ID;
		wd_parent.transient_children.erase(p_window);
		if ([[wd_parent.window_object childWindows] containsObject:wd_window.window_object]) {
			[wd_parent.window_object removeChildWindow:wd_window.window_object];
		}
		[wd_window.window_object setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
	} else {
		ERR_FAIL_COND(!windows.has(p_parent));
		ERR_FAIL_COND_MSG(wd_window.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID, "Window already has a transient parent");
		WindowData &wd_parent = windows[p_parent];

		wd_window.transient_parent = p_parent;
		wd_parent.transient_children.insert(p_window);
		reparent_check(p_window);
	}
}

void DisplayServerMacOS::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if ((p_size != Size2i()) && ((p_size.x < wd.min_size.x) || (p_size.y < wd.min_size.y))) {
		ERR_PRINT("Maximum window size can't be smaller than minimum window size!");
		return;
	}
	wd.max_size = p_size;

	if ((wd.max_size != Size2i()) && !wd.fullscreen) {
		Size2i size = wd.max_size / screen_get_max_scale();
		[wd.window_object setContentMaxSize:NSMakeSize(size.x, size.y)];
	} else {
		[wd.window_object setContentMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
	}
}

Size2i DisplayServerMacOS::window_get_max_size(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Size2i());
	const WindowData &wd = windows[p_window];
	return wd.max_size;
}

void DisplayServerMacOS::update_presentation_mode() {
	bool has_fs_windows = false;
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &wd : windows) {
		if (wd.value.fullscreen) {
			if (wd.value.exclusive_fullscreen) {
				return;
			} else {
				has_fs_windows = true;
			}
		}
	}
	if (has_fs_windows) {
		[NSApp setPresentationOptions:NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationAutoHideDock | NSApplicationPresentationFullScreen];
	} else {
		[NSApp setPresentationOptions:NSApplicationPresentationDefault];
	}
}

void DisplayServerMacOS::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if ((p_size != Size2i()) && (wd.max_size != Size2i()) && ((p_size.x > wd.max_size.x) || (p_size.y > wd.max_size.y))) {
		ERR_PRINT("Minimum window size can't be larger than maximum window size!");
		return;
	}
	wd.min_size = p_size;

	if ((wd.min_size != Size2i()) && !wd.fullscreen) {
		Size2i size = wd.min_size / screen_get_max_scale();
		[wd.window_object setContentMinSize:NSMakeSize(size.x, size.y)];
	} else {
		[wd.window_object setContentMinSize:NSMakeSize(0, 0)];
	}
}

Size2i DisplayServerMacOS::window_get_min_size(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Size2i());
	const WindowData &wd = windows[p_window];

	return wd.min_size;
}

void DisplayServerMacOS::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (NSEqualRects([wd.window_object frame], [[wd.window_object screen] visibleFrame])) {
		return;
	}

	Size2i size = p_size / screen_get_max_scale();

	NSPoint top_left;
	NSRect old_frame = [wd.window_object frame];
	top_left.x = old_frame.origin.x;
	top_left.y = NSMaxY(old_frame);

	NSRect new_frame = NSMakeRect(0, 0, MAX(1, size.x), MAX(1, size.y));
	new_frame = [wd.window_object frameRectForContentRect:new_frame];

	new_frame.origin.x = top_left.x;
	new_frame.origin.y = top_left.y - new_frame.size.height;

	[wd.window_object setFrame:new_frame display:YES];

	_update_window_style(wd, p_window);
}

Size2i DisplayServerMacOS::window_get_size(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Size2i());
	const WindowData &wd = windows[p_window];
	return wd.size;
}

Size2i DisplayServerMacOS::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Size2i());
	const WindowData &wd = windows[p_window];
	NSRect frame = [wd.window_object frame];
	return Size2i(frame.size.width, frame.size.height) * screen_get_max_scale();
}

void DisplayServerMacOS::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	DisplayServerEnums::WindowMode old_mode = window_get_mode(p_window);
	if (old_mode == p_mode) {
		return; // Do nothing.
	}

	switch (old_mode) {
		case DisplayServerEnums::WINDOW_MODE_WINDOWED: {
			// Do nothing.
		} break;
		case DisplayServerEnums::WINDOW_MODE_MINIMIZED: {
			[wd.window_object deminiaturize:nil];
		} break;
		case DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN:
		case DisplayServerEnums::WINDOW_MODE_FULLSCREEN: {
			if (p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN) {
				if (p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
					const NSUInteger presentationOptions = NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
					[NSApp setPresentationOptions:presentationOptions];
					wd.exclusive_fullscreen = true;
				} else {
					wd.exclusive_fullscreen = false;
					update_presentation_mode();
				}
				return;
			}

			[(NSWindow *)wd.window_object setLevel:NSNormalWindowLevel];
			if (wd.resize_disabled) { // Restore resize disabled.
				[wd.window_object setStyleMask:[wd.window_object styleMask] & ~NSWindowStyleMaskResizable];
			}
			[[wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setEnabled:!wd.no_min_btn];
			[[wd.window_object standardWindowButton:NSWindowZoomButton] setEnabled:!wd.no_max_btn];
			[[wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setHidden:(wd.no_min_btn && wd.no_max_btn)];
			[[wd.window_object standardWindowButton:NSWindowZoomButton] setHidden:(wd.no_min_btn && wd.no_max_btn)];
			if (wd.min_size != Size2i()) {
				Size2i size = wd.min_size / screen_get_max_scale();
				[wd.window_object setContentMinSize:NSMakeSize(size.x, size.y)];
			}
			if (wd.max_size != Size2i()) {
				Size2i size = wd.max_size / screen_get_max_scale();
				[wd.window_object setContentMaxSize:NSMakeSize(size.x, size.y)];
			}
			[wd.window_object toggleFullScreen:nil];

			if (old_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
				update_presentation_mode();
			}

			wd.fullscreen = false;
			wd.exclusive_fullscreen = false;
		} break;
		case DisplayServerEnums::WINDOW_MODE_MAXIMIZED: {
			if (NSEqualRects([wd.window_object frame], [[wd.window_object screen] visibleFrame])) {
				if (wd.borderless) {
					if (wd.pre_zoom_rect.size.width > 0 && wd.pre_zoom_rect.size.height > 0) {
						[wd.window_object setFrame:wd.pre_zoom_rect display:NO animate:YES];
					}
				} else {
					[wd.window_object zoom:nil];
				}
			}
		} break;
	}

	switch (p_mode) {
		case DisplayServerEnums::WINDOW_MODE_WINDOWED: {
			// Do nothing.
		} break;
		case DisplayServerEnums::WINDOW_MODE_MINIMIZED: {
			[wd.window_object performMiniaturize:nil];
		} break;
		case DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN:
		case DisplayServerEnums::WINDOW_MODE_FULLSCREEN: {
			if (wd.resize_disabled) { // Fullscreen window should be resizable to work.
				[wd.window_object setStyleMask:[wd.window_object styleMask] | NSWindowStyleMaskResizable];
			}
			[wd.window_object setContentMinSize:NSMakeSize(0, 0)];
			[wd.window_object setContentMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
			[wd.window_object toggleFullScreen:nil];

			wd.fullscreen = true;
			if (p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
				const NSUInteger presentationOptions = NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
				[NSApp setPresentationOptions:presentationOptions];
				wd.exclusive_fullscreen = true;
			} else {
				wd.exclusive_fullscreen = false;
				update_presentation_mode();
			}
		} break;
		case DisplayServerEnums::WINDOW_MODE_MAXIMIZED: {
			if (!NSEqualRects([wd.window_object frame], [[wd.window_object screen] visibleFrame])) {
				wd.pre_zoom_rect = [wd.window_object frame];
				if (wd.borderless) {
					[wd.window_object setFrame:[[wd.window_object screen] visibleFrame] display:NO animate:YES];
				} else {
					[wd.window_object zoom:nil];
				}
			}
		} break;
	}
}

DisplayServerEnums::WindowMode DisplayServerMacOS::window_get_mode(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), DisplayServerEnums::WINDOW_MODE_WINDOWED);
	const WindowData &wd = windows[p_window];

	if (wd.fullscreen) { // If fullscreen, it's not in another mode.
		if (wd.exclusive_fullscreen) {
			return DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
		} else {
			return DisplayServerEnums::WINDOW_MODE_FULLSCREEN;
		}
	}
	if (NSEqualRects([wd.window_object frame], [[wd.window_object screen] visibleFrame])) {
		return DisplayServerEnums::WINDOW_MODE_MAXIMIZED;
	}
	if ([wd.window_object respondsToSelector:@selector(isMiniaturized)]) {
		if ([wd.window_object isMiniaturized]) {
			return DisplayServerEnums::WINDOW_MODE_MINIMIZED;
		}
	}

	// All other discarded, return windowed.
	return DisplayServerEnums::WINDOW_MODE_WINDOWED;
}

bool DisplayServerMacOS::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(!windows.has(p_window), false);
	const WindowData &wd = windows[p_window];

	return [wd.window_object standardWindowButton:NSWindowZoomButton].enabled;
}

bool DisplayServerMacOS::window_maximize_on_title_dbl_click() const {
	NSString *value = [NSUserDefaults.standardUserDefaults stringForKey:@"AppleActionOnDoubleClick"];
	return [value isEqualToString:@"Maximize"];
}

bool DisplayServerMacOS::window_minimize_on_title_dbl_click() const {
	NSString *value = [NSUserDefaults.standardUserDefaults stringForKey:@"AppleActionOnDoubleClick"];
	return [value isEqualToString:@"Minimize"];
}

void DisplayServerMacOS::window_start_drag(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:((NSWindow *)wd.window_object).mouseLocationOutsideOfEventStream modifierFlags:0 timestamp:[[NSProcessInfo processInfo] systemUptime] windowNumber:((NSWindow *)wd.window_object).windowNumber context:nil eventNumber:0 clickCount:1 pressure:1.0f];
	[wd.window_object performWindowDragWithEvent:event];
}

void DisplayServerMacOS::window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_INDEX(int(p_edge), DisplayServerEnums::WINDOW_EDGE_MAX);
	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	wd.edge = p_edge;
}

void DisplayServerMacOS::window_set_window_buttons_offset(const Vector2i &p_offset, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	float scale = screen_get_max_scale();
	wd.wb_offset = p_offset / scale;
	wd.wb_offset = wd.wb_offset.maxi(12);
	if (wd.window_button_view) {
		[wd.window_button_view setOffset:NSMakePoint(wd.wb_offset.x, wd.wb_offset.y)];
	}
}

Vector3i DisplayServerMacOS::window_get_safe_title_margins(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Vector3i());
	const WindowData &wd = windows[p_window];

	if (!wd.window_button_view) {
		return Vector3i();
	}

	float scale = screen_get_max_scale();
	float max_x = [wd.window_button_view getOffset].x + [wd.window_button_view frame].size.width;
	float max_y = [wd.window_button_view getOffset].y + [wd.window_button_view frame].size.height;

	if ([wd.window_object windowTitlebarLayoutDirection] == NSUserInterfaceLayoutDirectionRightToLeft) {
		return Vector3i(0, max_x * scale, max_y * scale);
	} else {
		return Vector3i(max_x * scale, 0, max_y * scale);
	}
}

void DisplayServerMacOS::window_set_custom_window_buttons(WindowData &p_wd, bool p_enabled) {
	if (p_wd.window_button_view) {
		[p_wd.window_button_view removeFromSuperview];
		p_wd.window_button_view = nil;
	}
	if (p_enabled) {
		float cb_frame = NSMinX([[p_wd.window_object standardWindowButton:NSWindowCloseButton] frame]);
		float mb_frame = NSMinX([[p_wd.window_object standardWindowButton:NSWindowMiniaturizeButton] frame]);
		bool is_rtl = ([p_wd.window_object windowTitlebarLayoutDirection] == NSUserInterfaceLayoutDirectionRightToLeft);

		float window_buttons_spacing = (is_rtl) ? (cb_frame - mb_frame) : (mb_frame - cb_frame);

		[p_wd.window_object setTitlebarAppearsTransparent:YES];
		[p_wd.window_object setTitleVisibility:NSWindowTitleHidden];
		[[p_wd.window_object standardWindowButton:NSWindowZoomButton] setHidden:YES];
		[[p_wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
		[[p_wd.window_object standardWindowButton:NSWindowCloseButton] setHidden:YES];

		p_wd.window_button_view = [[GodotButtonView alloc] initWithFrame:NSZeroRect];
		[p_wd.window_button_view initButtons:window_buttons_spacing offset:NSMakePoint(p_wd.wb_offset.x, p_wd.wb_offset.y) rtl:is_rtl];
		[p_wd.window_view addSubview:p_wd.window_button_view];

		[[p_wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setHidden:(p_wd.no_min_btn && p_wd.no_max_btn)];
		[[p_wd.window_object standardWindowButton:NSWindowZoomButton] setHidden:(p_wd.no_min_btn && p_wd.no_max_btn)];
	} else {
		[p_wd.window_object setTitleVisibility:NSWindowTitleVisible];
		[p_wd.window_object setTitlebarAppearsTransparent:NO];
		[[p_wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setHidden:(p_wd.no_min_btn && p_wd.no_max_btn)];
		[[p_wd.window_object standardWindowButton:NSWindowZoomButton] setHidden:(p_wd.no_min_btn && p_wd.no_max_btn)];
		[[p_wd.window_object standardWindowButton:NSWindowCloseButton] setHidden:NO];
	}
}

void DisplayServerMacOS::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	switch (p_flag) {
		case DisplayServerEnums::WINDOW_FLAG_MINIMIZE_DISABLED: {
			wd.no_min_btn = p_enabled;
			[[wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setEnabled:!p_enabled];
			[[wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setHidden:(wd.no_min_btn && wd.no_max_btn)];
			[[wd.window_object standardWindowButton:NSWindowZoomButton] setHidden:(wd.no_min_btn && wd.no_max_btn)];
		} break;
		case DisplayServerEnums::WINDOW_FLAG_MAXIMIZE_DISABLED: {
			wd.no_max_btn = p_enabled;
			[[wd.window_object standardWindowButton:NSWindowZoomButton] setEnabled:!p_enabled];
			[[wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setHidden:(wd.no_min_btn && wd.no_max_btn)];
			[[wd.window_object standardWindowButton:NSWindowZoomButton] setHidden:(wd.no_min_btn && wd.no_max_btn)];
		} break;
		case DisplayServerEnums::WINDOW_FLAG_RESIZE_DISABLED: {
			wd.resize_disabled = p_enabled;
			if (wd.fullscreen) { // Fullscreen window should be resizable, style will be applied on exiting fullscreen.
				return;
			}
			if (p_enabled) {
				[wd.window_object setStyleMask:[wd.window_object styleMask] & ~NSWindowStyleMaskResizable];
			} else {
				[wd.window_object setStyleMask:[wd.window_object styleMask] | NSWindowStyleMaskResizable];
			}
			[[wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setEnabled:!wd.no_min_btn];
			[[wd.window_object standardWindowButton:NSWindowZoomButton] setEnabled:!wd.no_max_btn];
			[[wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setHidden:(wd.no_min_btn && wd.no_max_btn)];
			[[wd.window_object standardWindowButton:NSWindowZoomButton] setHidden:(wd.no_min_btn && wd.no_max_btn)];
		} break;
		case DisplayServerEnums::WINDOW_FLAG_EXTEND_TO_TITLE: {
			NSRect rect = [wd.window_object frame];
			wd.extend_to_title = p_enabled;
			if (p_enabled) {
				[wd.window_object setStyleMask:[wd.window_object styleMask] | NSWindowStyleMaskFullSizeContentView];

				if (!wd.fullscreen) {
					window_set_custom_window_buttons(wd, true);
				}
			} else {
				[wd.window_object setStyleMask:[wd.window_object styleMask] & ~NSWindowStyleMaskFullSizeContentView];

				if (!wd.fullscreen) {
					window_set_custom_window_buttons(wd, false);
				}
			}
			[wd.window_object setFrame:rect display:YES];
			send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_TITLEBAR_CHANGE);
		} break;
		case DisplayServerEnums::WINDOW_FLAG_BORDERLESS: {
			if (wd.fullscreen) {
				return;
			}
			// OrderOut prevents a lose focus bug with the window.
			bool was_visible = false;
			if ([wd.window_object isVisible]) {
				was_visible = true;
				[wd.window_object orderOut:nil];
			}
			wd.borderless = p_enabled;
			bool was_maximized = (NSEqualRects([wd.window_object frame], [[wd.window_object screen] visibleFrame]));
			if (p_enabled) {
				[wd.window_object setStyleMask:NSWindowStyleMaskBorderless];
				[wd.window_object setHasShadow:NO];
				if (wd.layered_window) {
					[wd.window_object setBackgroundColor:[NSColor clearColor]];
				}
			} else {
				[wd.window_object setStyleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | (wd.extend_to_title ? NSWindowStyleMaskFullSizeContentView : 0) | (wd.resize_disabled ? 0 : NSWindowStyleMaskResizable)];
				[[wd.window_object standardWindowButton:NSWindowMiniaturizeButton] setEnabled:!wd.no_min_btn];
				[[wd.window_object standardWindowButton:NSWindowZoomButton] setEnabled:!wd.no_max_btn];
				[wd.window_object setHasShadow:YES];
				if (wd.layered_window) {
					// Note: transparent and not borderless - set alpha to non-zero value to ensure window shadow is rendered correctly.
					[wd.window_object setBackgroundColor:[NSColor colorWithCalibratedRed:0 green:0 blue:0 alpha:0.004f]];
				}
				// Force update of the window styles.
				if ([wd.window_object isVisible]) {
					NSRect frameRect = [wd.window_object frame];
					[wd.window_object setFrame:NSMakeRect(frameRect.origin.x, frameRect.origin.y, frameRect.size.width + 1, frameRect.size.height) display:NO];
					[wd.window_object setFrame:frameRect display:NO];
				}
			}
			_update_window_style(wd, p_window);
			if (was_visible || [wd.window_object isVisible]) {
				if ([wd.window_object isMiniaturized]) {
					return;
				} else if (wd.no_focus) {
					if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
						WindowData &wd_parent = windows[wd.transient_parent];
						[wd.window_object orderWindow:NSWindowAbove relativeTo:[wd_parent.window_object windowNumber]];
					} else if (p_window != DisplayServerEnums::MAIN_WINDOW_ID) {
						[wd.window_object orderWindow:NSWindowAbove relativeTo:[windows[DisplayServerEnums::MAIN_WINDOW_ID].window_object windowNumber]];
					} else {
						[wd.window_object orderFront:nil];
					}
				} else {
					[wd.window_object makeKeyAndOrderFront:nil];
				}
			}
			if (was_maximized) {
				[wd.window_object setFrame:[[wd.window_object screen] visibleFrame] display:NO];
			}
		} break;
		case DisplayServerEnums::WINDOW_FLAG_ALWAYS_ON_TOP: {
			wd.on_top = p_enabled;
			if (wd.fullscreen) {
				return;
			}
			if (p_enabled) {
				[(NSWindow *)wd.window_object setLevel:NSFloatingWindowLevel];
			} else {
				[(NSWindow *)wd.window_object setLevel:NSNormalWindowLevel];
			}
		} break;
		case DisplayServerEnums::WINDOW_FLAG_TRANSPARENT: {
			if (wd.fullscreen) {
				return;
			}
			wd.layered_window = p_enabled;
			set_window_per_pixel_transparency_enabled(p_enabled, p_window);
			// Force update of the window styles.
			if ([wd.window_object isVisible]) {
				NSRect frameRect = [wd.window_object frame];
				[wd.window_object setFrame:NSMakeRect(frameRect.origin.x, frameRect.origin.y, frameRect.size.width + 1, frameRect.size.height) display:NO];
				[wd.window_object setFrame:frameRect display:NO];
			}
		} break;
		case DisplayServerEnums::WINDOW_FLAG_NO_FOCUS: {
			wd.no_focus = p_enabled;

			NSWindow *w = wd.window_object;
			w.excludedFromWindowsMenu = wd.is_popup || wd.no_focus;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_EXCLUDE_FROM_CAPTURE: {
			if (p_enabled) {
				[wd.window_object setSharingType:NSWindowSharingNone];
			} else {
				[wd.window_object setSharingType:NSWindowSharingReadWrite];
			}
			wd.hide_from_capture = p_enabled;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_MOUSE_PASSTHROUGH: {
			wd.mpass = p_enabled;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_POPUP: {
			ERR_FAIL_COND_MSG(p_window == DisplayServerEnums::MAIN_WINDOW_ID, "Main window can't be popup.");
			ERR_FAIL_COND_MSG([wd.window_object isVisible] && (wd.is_popup != p_enabled), "Popup flag can't changed while window is opened.");
			wd.is_popup = p_enabled;

			NSWindow *w = wd.window_object;
			w.excludedFromWindowsMenu = wd.is_popup || wd.no_focus;
		} break;
		default: {
		}
	}
}

bool DisplayServerMacOS::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), false);
	const WindowData &wd = windows[p_window];

	switch (p_flag) {
		case DisplayServerEnums::WINDOW_FLAG_MAXIMIZE_DISABLED: {
			return wd.no_max_btn;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_MINIMIZE_DISABLED: {
			return wd.no_min_btn;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_RESIZE_DISABLED: {
			return wd.resize_disabled;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_EXTEND_TO_TITLE: {
			return [wd.window_object styleMask] & NSWindowStyleMaskFullSizeContentView;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_BORDERLESS: {
			return [wd.window_object styleMask] == NSWindowStyleMaskBorderless;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_ALWAYS_ON_TOP: {
			return wd.on_top;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_TRANSPARENT: {
			return wd.layered_window;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_NO_FOCUS: {
			return wd.no_focus;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_EXCLUDE_FROM_CAPTURE: {
			return wd.hide_from_capture;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_MOUSE_PASSTHROUGH: {
			return wd.mpass;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_POPUP: {
			return wd.is_popup;
		} break;
		default: {
		}
	}

	return false;
}

void DisplayServerMacOS::window_request_attention(DisplayServerEnums::WindowID p_window) {
	// It's app global, ignore window id.
	[NSApp requestUserAttention:NSCriticalRequest];
}

void DisplayServerMacOS::window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);

	if (!dock_progress) {
		dock_progress = [[GodotProgressView alloc] init];
		[NSApp.dockTile setContentView:dock_progress];
	}

	[dock_progress setValue:p_value];
}

void DisplayServerMacOS::window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);

	if (!dock_progress) {
		dock_progress = [[GodotProgressView alloc] init];
		[NSApp.dockTile setContentView:dock_progress];
	}

	[dock_progress setState:p_state];
}

void DisplayServerMacOS::window_move_to_foreground(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	const WindowData &wd = windows[p_window];

	[[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
	if (wd.no_focus || wd.is_popup) {
		if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
			WindowData &wd_parent = windows[wd.transient_parent];
			[wd.window_object orderWindow:NSWindowAbove relativeTo:[wd_parent.window_object windowNumber]];
		} else if (p_window != DisplayServerEnums::MAIN_WINDOW_ID) {
			[wd.window_object orderWindow:NSWindowAbove relativeTo:[windows[DisplayServerEnums::MAIN_WINDOW_ID].window_object windowNumber]];
		} else {
			[wd.window_object orderFront:nil];
		}
	} else {
		[wd.window_object makeKeyAndOrderFront:nil];
	}
}

bool DisplayServerMacOS::window_is_focused(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), false);
	const WindowData &wd = windows[p_window];

	return wd.focused;
}

DisplayServerEnums::WindowID DisplayServerMacOS::get_focused_window() const {
	return last_focused_window;
}

bool DisplayServerMacOS::window_can_draw(DisplayServerEnums::WindowID p_window) const {
	return windows[p_window].is_visible;
}

bool DisplayServerMacOS::can_any_window_draw() const {
	_THREAD_SAFE_METHOD_

	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (E.value.is_visible) {
			return true;
		}
	}
	return false;
}

void DisplayServerMacOS::window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	wd.im_active = p_active;

	if (!p_active) {
		[wd.window_view cancelComposition];
	}
}

void DisplayServerMacOS::window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	wd.im_position = p_pos;
}

DisplayServerEnums::WindowID DisplayServerMacOS::get_window_at_screen_position(const Point2i &p_position) const {
	Point2i position = p_position;
	position.y *= -1;
	position += _get_screens_origin();
	position /= screen_get_max_scale();

	NSInteger wnum = [NSWindow windowNumberAtPoint:NSMakePoint(position.x, position.y) belowWindowWithWindowNumber:0 /*topmost*/];
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if ([E.value.window_object windowNumber] == wnum) {
			return E.key;
		}
	}
	return DisplayServerEnums::INVALID_WINDOW_ID;
}

int64_t DisplayServerMacOS::window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(!windows.has(p_window), 0);
	switch (p_handle_type) {
		case DisplayServerEnums::DISPLAY_HANDLE: {
			return 0; // Not supported.
		}
		case DisplayServerEnums::WINDOW_HANDLE: {
			return (int64_t)windows[p_window].window_object;
		}
		case DisplayServerEnums::WINDOW_VIEW: {
			return (int64_t)windows[p_window].window_view;
		}
#ifdef GLES3_ENABLED
		case DisplayServerEnums::OPENGL_CONTEXT: {
			if (gl_manager_legacy) {
				return (int64_t)gl_manager_legacy->get_context(p_window);
			}
#if defined(ANGLE_ENABLED)
			if (gl_manager_angle) {
				return (int64_t)gl_manager_angle->get_context(p_window);
			}
#endif
			return 0;
		}
		case DisplayServerEnums::EGL_DISPLAY: {
#if defined(ANGLE_ENABLED)
			if (gl_manager_angle) {
				return (int64_t)gl_manager_angle->get_display(p_window);
			}
#endif
			return 0;
		}
		case DisplayServerEnums::EGL_CONFIG: {
#if defined(ANGLE_ENABLED)
			if (gl_manager_angle) {
				return (int64_t)gl_manager_angle->get_config(p_window);
			}
#endif
			return 0;
		}
#endif
		default: {
			return 0;
		}
	}
}

void DisplayServerMacOS::window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	windows[p_window].instance_id = p_instance;
}

ObjectID DisplayServerMacOS::window_get_attached_instance_id(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), ObjectID());
	return windows[p_window].instance_id;
}

void DisplayServerMacOS::gl_window_make_current(DisplayServerEnums::WindowID p_window_id) {
#if defined(GLES3_ENABLED)
	if (gl_manager_legacy) {
		gl_manager_legacy->window_make_current(p_window_id);
	}
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->window_make_current(p_window_id);
	}
#endif
#endif
}

void DisplayServerMacOS::window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_
#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->set_use_vsync(p_vsync_mode != DisplayServerEnums::VSYNC_DISABLED);
	}
#endif
	if (gl_manager_legacy) {
		gl_manager_legacy->set_use_vsync(p_vsync_mode != DisplayServerEnums::VSYNC_DISABLED);
	}
#endif
#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_context->window_set_vsync_mode(p_window, p_vsync_mode);
	}
#endif
}

DisplayServerEnums::VSyncMode DisplayServerMacOS::window_get_vsync_mode(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_
#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		return (gl_manager_angle->is_using_vsync() ? DisplayServerEnums::VSyncMode::VSYNC_ENABLED : DisplayServerEnums::VSyncMode::VSYNC_DISABLED);
	}
#endif
	if (gl_manager_legacy) {
		return (gl_manager_legacy->is_using_vsync() ? DisplayServerEnums::VSyncMode::VSYNC_ENABLED : DisplayServerEnums::VSyncMode::VSYNC_DISABLED);
	}
#endif
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_vsync_mode(p_window);
	}
#endif
	return DisplayServerEnums::VSYNC_ENABLED;
}

DisplayServerMacOSBase::HDROutput &DisplayServerMacOS::_get_hdr_output(DisplayServerEnums::WindowID p_window) {
	return windows.getptr(p_window)->hdr_output;
}

const DisplayServerMacOSBase::HDROutput &DisplayServerMacOS::_get_hdr_output(DisplayServerEnums::WindowID p_window) const {
	return windows.getptr(p_window)->hdr_output;
}

void DisplayServerMacOS::window_get_edr_values(DisplayServerEnums::WindowID p_window, CGFloat *r_max_potential_edr_value, CGFloat *r_max_edr_value) const {
	_THREAD_SAFE_METHOD_

	const WindowData *wd = windows.getptr(p_window);
	ERR_FAIL_NULL(wd);

	NSScreen *screen = wd->window_object.screen;
#define SET_VAL(v, val) \
	if (v) { \
		*v = val; \
	}

	SET_VAL(r_max_potential_edr_value, screen.maximumPotentialExtendedDynamicRangeColorComponentValue);
	SET_VAL(r_max_edr_value, screen.maximumExtendedDynamicRangeColorComponentValue);

#undef SET_VAL
}

void DisplayServerMacOS::update_screen_parameters() {
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (E.value.hdr_output.requested) {
			_update_hdr_output(E.key, E.value.hdr_output);
		}
	}

#ifdef TOOLS_ENABLED
	for (KeyValue<ProcessID, EmbeddedProcessData> &E : embedded_processes) {
		E.value.process->display_state_changed();
	}
#endif
}

void DisplayServerMacOS::cursor_update_shape() {
	_THREAD_SAFE_METHOD_

	if (cursors[cursor_shape] != nullptr) {
		[cursors[cursor_shape] set];
	} else {
		switch (cursor_shape) {
			case DisplayServerEnums::CURSOR_ARROW:
				[[NSCursor arrowCursor] set];
				break;
			case DisplayServerEnums::CURSOR_IBEAM:
				[[NSCursor IBeamCursor] set];
				break;
			case DisplayServerEnums::CURSOR_POINTING_HAND:
				[[NSCursor pointingHandCursor] set];
				break;
			case DisplayServerEnums::CURSOR_CROSS:
				[[NSCursor crosshairCursor] set];
				break;
			case DisplayServerEnums::CURSOR_WAIT:
				[[NSCursor arrowCursor] set];
				break;
			case DisplayServerEnums::CURSOR_BUSY:
				[[NSCursor arrowCursor] set];
				break;
			case DisplayServerEnums::CURSOR_DRAG:
				[[NSCursor closedHandCursor] set];
				break;
			case DisplayServerEnums::CURSOR_CAN_DROP:
				[[NSCursor openHandCursor] set];
				break;
			case DisplayServerEnums::CURSOR_FORBIDDEN:
				[[NSCursor operationNotAllowedCursor] set];
				break;
			case DisplayServerEnums::CURSOR_VSIZE:
				[_cursor_from_selector(@selector(_windowResizeNorthSouthCursor), @selector(resizeUpDownCursor)) set];
				break;
			case DisplayServerEnums::CURSOR_HSIZE:
				[_cursor_from_selector(@selector(_windowResizeEastWestCursor), @selector(resizeLeftRightCursor)) set];
				break;
			case DisplayServerEnums::CURSOR_BDIAGSIZE:
				[_cursor_from_selector(@selector(_windowResizeNorthEastSouthWestCursor)) set];
				break;
			case DisplayServerEnums::CURSOR_FDIAGSIZE:
				[_cursor_from_selector(@selector(_windowResizeNorthWestSouthEastCursor)) set];
				break;
			case DisplayServerEnums::CURSOR_MOVE:
				[[[GodotCoreCursor alloc] initWithType:GDCoreCursorWindowMove] set];
				break;
			case DisplayServerEnums::CURSOR_VSPLIT:
				[[NSCursor resizeUpDownCursor] set];
				break;
			case DisplayServerEnums::CURSOR_HSPLIT:
				[[NSCursor resizeLeftRightCursor] set];
				break;
			case DisplayServerEnums::CURSOR_HELP:
				[_cursor_from_selector(@selector(_helpCursor)) set];
				break;
			default: {
			}
		}
	}
}

void DisplayServerMacOS::cursor_set_shape(DisplayServerEnums::CursorShape p_shape) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);

	if (cursor_shape == p_shape) {
		return;
	}

	cursor_shape = p_shape;

	if (mouse_mode != DisplayServerEnums::MOUSE_MODE_VISIBLE && mouse_mode != DisplayServerEnums::MOUSE_MODE_CONFINED) {
		return;
	}

	cursor_update_shape();
}

void DisplayServerMacOS::cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape, const Vector2 &p_hotspot) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);

	if (p_cursor.is_valid()) {
		HashMap<DisplayServerEnums::CursorShape, Vector<Variant>>::Iterator cursor_c = cursors_cache.find(p_shape);

		if (cursor_c) {
			if (cursor_c->value[0] == p_cursor && cursor_c->value[1] == p_hotspot) {
				cursor_set_shape(p_shape);
				return;
			}
			cursors_cache.erase(p_shape);
		}

		Ref<Image> image = _get_cursor_image_from_resource(p_cursor, p_hotspot);
		ERR_FAIL_COND(image.is_null());
		Vector2i texture_size = image->get_size();

		NSBitmapImageRep *imgrep = [[NSBitmapImageRep alloc]
				initWithBitmapDataPlanes:nullptr
							  pixelsWide:int(texture_size.width)
							  pixelsHigh:int(texture_size.height)
						   bitsPerSample:8
						 samplesPerPixel:4
								hasAlpha:YES
								isPlanar:NO
						  colorSpaceName:NSDeviceRGBColorSpace
							 bytesPerRow:int(texture_size.width) * 4
							bitsPerPixel:32];

		ERR_FAIL_NULL(imgrep);
		uint8_t *pixels = [imgrep bitmapData];

		int len = int(texture_size.width * texture_size.height);

		for (int i = 0; i < len; i++) {
			int row_index = std::floor(i / texture_size.width);
			int column_index = i % int(texture_size.width);

			uint32_t color = image->get_pixel(column_index, row_index).to_argb32();

			uint8_t alpha = (color >> 24) & 0xFF;
			pixels[i * 4 + 0] = ((color >> 16) & 0xFF) * alpha / 255;
			pixels[i * 4 + 1] = ((color >> 8) & 0xFF) * alpha / 255;
			pixels[i * 4 + 2] = ((color) & 0xFF) * alpha / 255;
			pixels[i * 4 + 3] = alpha;
		}

		NSImage *nsimage = [[NSImage alloc] initWithSize:NSMakeSize(texture_size.width, texture_size.height)];
		[nsimage addRepresentation:imgrep];

		NSCursor *cursor = [[NSCursor alloc] initWithImage:nsimage hotSpot:NSMakePoint(p_hotspot.x, p_hotspot.y)];

		cursors[p_shape] = cursor;

		Vector<Variant> params;
		params.push_back(p_cursor);
		params.push_back(p_hotspot);
		cursors_cache.insert(p_shape, params);

		if (p_shape == cursor_shape) {
			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_VISIBLE || mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED) {
				[cursor set];
			}
		}
	} else {
		// Reset to default system cursor.
		if (cursors[p_shape] != nullptr) {
			cursors[p_shape] = nullptr;
		}

		cursors_cache.erase(p_shape);

		cursor_update_shape();
	}
}

bool DisplayServerMacOS::get_swap_cancel_ok() {
	return false;
}

void DisplayServerMacOS::enable_for_stealing_focus(ProcessID pid) {
}

#define GET_OR_FAIL_V(m_val, m_map, m_key, m_retval) \
	m_val = m_map.getptr(m_key); \
	if (m_val == nullptr) { \
		ERR_FAIL_V(m_retval); \
	}

uint32_t DisplayServerMacOS::window_get_display_id(DisplayServerEnums::WindowID p_window) const {
	const WindowData *wd;
	GET_OR_FAIL_V(wd, windows, p_window, -1);
	return wd->display_id;
}

void DisplayServerMacOS::_window_update_display_id(WindowData *p_wd) {
	NSScreen *screen = [p_wd->window_object screen];
	CGDirectDisplayID display_id = _get_display_id_for_screen(screen);
	if (p_wd->display_id == display_id) {
		return;
	}

	p_wd->display_id = display_id;

#ifdef TOOLS_ENABLED
	// Notify any embedded processes of the new display ID, so that they can potentially update their vsync.
	for (KeyValue<ProcessID, EmbeddedProcessData> &E : embedded_processes) {
		if (E.value.wd == p_wd) {
			E.value.process->display_state_changed();
		}
	}
#endif
}

#ifdef TOOLS_ENABLED

Error DisplayServerMacOS::embed_process_update(DisplayServerEnums::WindowID p_window, EmbeddedProcessMacOS *p_process) {
	_THREAD_SAFE_METHOD_

	WindowData *wd;
	GET_OR_FAIL_V(wd, windows, p_window, FAILED);

	ProcessID p_pid = p_process->get_embedded_pid();

	[CATransaction begin];
	[CATransaction setDisableActions:YES];

	EmbeddedProcessData *ed = embedded_processes.getptr(p_pid);
	CGFloat scale = screen_get_max_scale();
	if (ed == nil) {
		ed = &embedded_processes.insert(p_pid, EmbeddedProcessData())->value;

		ed->process = p_process;
		ed->wd = wd;

		CALayerHost *host = [CALayerHost new];
		uint32_t p_context_id = p_process->get_context_id();
		host.contextId = static_cast<CAContextID>(p_context_id);
		host.contentsScale = scale;
		host.contentsGravity = kCAGravityCenter;
		ed->layer_host = host;
		[wd->window_view.layer addSublayer:host];
	}

	Rect2i p_rect = p_process->get_screen_embedded_window_rect();
	CGRect rect = CGRectMake(p_rect.position.x, p_rect.position.y, p_rect.size.x, p_rect.size.y);
	rect = CGRectApplyAffineTransform(rect, CGAffineTransformInvert(CGAffineTransformMakeScale(scale, scale)));

	CGFloat height = wd->window_view.frame.size.height;
	CGFloat x = rect.origin.x;
	CGFloat y = (height - rect.origin.y);
	ed->layer_host.position = CGPointMake(x, y);
	ed->layer_host.hidden = !p_process->is_visible_in_tree();

	[CATransaction commit];

	return OK;
}

Error DisplayServerMacOS::request_close_embedded_process(ProcessID p_pid) {
	return OK;
}

Error DisplayServerMacOS::remove_embedded_process(ProcessID p_pid) {
	_THREAD_SAFE_METHOD_

	EmbeddedProcessData *ed;
	GET_OR_FAIL_V(ed, embedded_processes, p_pid, ERR_DOES_NOT_EXIST);
	[ed->layer_host removeFromSuperlayer];
	embedded_processes.erase(p_pid);

	return OK;
}

#endif

void DisplayServerMacOS::process_events() {
	_process_events(true);
}

void DisplayServerMacOS::_process_events(bool p_pump) {
	ERR_FAIL_COND(!Thread::is_main_thread());

	if (p_pump) {
		while (true) {
			NSEvent *event = [NSApp
					nextEventMatchingMask:NSEventMaskAny
								untilDate:[NSDate distantPast]
								   inMode:NSDefaultRunLoopMode
								  dequeue:YES];

			if (event == nil) {
				break;
			}

			[NSApp sendEvent:event];
		}
	}

	// Process "menu_callback"s.
	while (List<MenuCall>::Element *call_p = deferred_menu_calls.front()) {
		MenuCall call = call_p->get();
		deferred_menu_calls.pop_front(); // Remove before call to avoid infinite loop in case callback is using `process_events` (e.g. EditorProgress).

		Variant ret;
		Callable::CallError ce;
		const Variant *args[1] = { &call.tag };

		call.callback.callp(args, 1, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Failed to execute menu callback: %s.", Variant::get_callable_error_text(call.callback, args, 1, ce)));
		}
	}

	if (!drop_events) {
		_process_key_events();
		Input::flush_buffered_events();
	}

	_THREAD_SAFE_LOCK_

	for (KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		WindowData &wd = E.value;
		if (wd.mpass) {
			if (![wd.window_object ignoresMouseEvents]) {
				[wd.window_object setIgnoresMouseEvents:YES];
			}
		} else if (wd.mpath.size() > 0) {
			update_mouse_pos(wd, [wd.window_object mouseLocationOutsideOfEventStream]);
			if (Geometry2D::is_point_in_polygon(wd.mouse_pos, wd.mpath)) {
				if ([wd.window_object ignoresMouseEvents]) {
					[wd.window_object setIgnoresMouseEvents:NO];
				}
			} else {
				if (![wd.window_object ignoresMouseEvents]) {
					[wd.window_object setIgnoresMouseEvents:YES];
				}
			}
		} else {
			if ([wd.window_object ignoresMouseEvents]) {
				[wd.window_object setIgnoresMouseEvents:NO];
			}
		}
	}

	if (dock_progress) {
		dock_progress.needsDisplay = true;
		[NSApp.dockTile display];
	}

	_THREAD_SAFE_UNLOCK_
}

void DisplayServerMacOS::force_process_and_drop_events() {
	ERR_FAIL_COND(!Thread::is_main_thread());

	drop_events = true;
	process_events();
	drop_events = false;
}

void DisplayServerMacOS::release_rendering_thread() {
#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->release_current();
	}
#endif
	if (gl_manager_legacy) {
		gl_manager_legacy->release_current();
	}
#endif
}

void DisplayServerMacOS::swap_buffers() {
#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->swap_buffers();
	}
#endif
	if (gl_manager_legacy) {
		gl_manager_legacy->swap_buffers();
	}
#endif
}

void DisplayServerMacOS::set_native_icon(const String &p_filename) {
	_THREAD_SAFE_METHOD_

	Ref<FileAccess> f = FileAccess::open(p_filename, FileAccess::READ);
	ERR_FAIL_COND(f.is_null());

	Vector<uint8_t> data;
	uint64_t len = f->get_length();
	ERR_FAIL_COND_MSG(len < 8, "Error reading icon data."); // "icns" + 32-bit length

	data.resize(len);
	f->get_buffer((uint8_t *)&data.write[0], len);

	@try {
		NSData *icon_data = [[NSData alloc] initWithBytes:&data.write[0] length:len];
		ERR_FAIL_NULL_MSG(icon_data, "Error reading icon data.");

		NSImage *icon = [[NSImage alloc] initWithData:icon_data];
		ERR_FAIL_NULL_MSG(icon, "Error loading icon.");

		[NSApp setApplicationIconImage:icon];
	} @catch (NSException *exception) {
		ERR_FAIL_MSG("NSException: " + String::utf8([exception reason].UTF8String));
	}
}

void DisplayServerMacOS::set_icon(const Ref<Image> &p_icon) {
	_THREAD_SAFE_METHOD_

	if (p_icon.is_valid()) {
		ERR_FAIL_COND(p_icon->get_width() <= 0 || p_icon->get_height() <= 0);

		Ref<Image> img = p_icon->duplicate();
		img->convert(Image::FORMAT_RGBA8);

		NSBitmapImageRep *imgrep = [[NSBitmapImageRep alloc]
				initWithBitmapDataPlanes:nullptr
							  pixelsWide:img->get_width()
							  pixelsHigh:img->get_height()
						   bitsPerSample:8
						 samplesPerPixel:4
								hasAlpha:YES
								isPlanar:NO
						  colorSpaceName:NSDeviceRGBColorSpace
							 bytesPerRow:img->get_width() * 4
							bitsPerPixel:32];
		ERR_FAIL_NULL(imgrep);
		uint8_t *pixels = [imgrep bitmapData];

		int len = img->get_width() * img->get_height();
		const uint8_t *r = img->get_data().ptr();

		/* Premultiply the alpha channel */
		for (int i = 0; i < len; i++) {
			uint8_t alpha = r[i * 4 + 3];
			pixels[i * 4 + 0] = (uint8_t)(((uint16_t)r[i * 4 + 0] * alpha) / 255);
			pixels[i * 4 + 1] = (uint8_t)(((uint16_t)r[i * 4 + 1] * alpha) / 255);
			pixels[i * 4 + 2] = (uint8_t)(((uint16_t)r[i * 4 + 2] * alpha) / 255);
			pixels[i * 4 + 3] = alpha;
		}

		NSImage *nsimg = [[NSImage alloc] initWithSize:NSMakeSize(img->get_width(), img->get_height())];
		ERR_FAIL_NULL(nsimg);

		[nsimg addRepresentation:imgrep];
		[NSApp setApplicationIconImage:nsimg];
	} else {
		[NSApp setApplicationIconImage:nil];
	}
}

DisplayServerEnums::IndicatorID DisplayServerMacOS::create_status_indicator(const Ref<Texture2D> &p_icon, const String &p_tooltip, const Callable &p_callback) {
	NSImage *nsimg = nullptr;
	if (p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
		Ref<Image> img = p_icon->get_image();
		img = img->duplicate();
		if (img->is_compressed()) {
			img->decompress();
		}
		nsimg = _convert_to_nsimg(img);
	}

	IndicatorData idat;

	NSStatusItem *item = [[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength];
	idat.item = item;
	idat.delegate = [[GodotStatusItemDelegate alloc] init];
	[idat.delegate setCallback:p_callback];

	item.button.image = nsimg;
	item.button.imagePosition = NSImageOnly;
	item.button.imageScaling = NSImageScaleProportionallyUpOrDown;
	item.button.target = idat.delegate;
	item.button.action = @selector(click:);
	[item.button sendActionOn:(NSEventMaskLeftMouseDown | NSEventMaskRightMouseDown | NSEventMaskOtherMouseDown)];
	item.button.toolTip = [NSString stringWithUTF8String:p_tooltip.utf8().get_data()];

	DisplayServerEnums::IndicatorID iid = indicator_id_counter++;
	indicators[iid] = idat;

	return iid;
}

void DisplayServerMacOS::status_indicator_set_icon(DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_COND(!indicators.has(p_id));

	NSImage *nsimg = nullptr;
	if (p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
		Ref<Image> img = p_icon->get_image();
		img = img->duplicate();
		if (img->is_compressed()) {
			img->decompress();
		}
		nsimg = _convert_to_nsimg(img);
	}

	NSStatusItem *item = indicators[p_id].item;
	item.button.image = nsimg;
}

void DisplayServerMacOS::status_indicator_set_tooltip(DisplayServerEnums::IndicatorID p_id, const String &p_tooltip) {
	ERR_FAIL_COND(!indicators.has(p_id));

	NSStatusItem *item = indicators[p_id].item;
	item.button.toolTip = [NSString stringWithUTF8String:p_tooltip.utf8().get_data()];
}

void DisplayServerMacOS::status_indicator_set_menu(DisplayServerEnums::IndicatorID p_id, const RID &p_menu_rid) {
	ERR_FAIL_COND(!indicators.has(p_id));

	NSStatusItem *item = indicators[p_id].item;
	if (p_menu_rid.is_valid() && native_menu->has_menu(p_menu_rid)) {
		NSMenu *menu = native_menu->get_native_menu_handle(p_menu_rid);
		item.menu = menu;
	} else {
		item.menu = nullptr;
	}
}

void DisplayServerMacOS::status_indicator_set_callback(DisplayServerEnums::IndicatorID p_id, const Callable &p_callback) {
	ERR_FAIL_COND(!indicators.has(p_id));

	[indicators[p_id].delegate setCallback:p_callback];
}

Rect2 DisplayServerMacOS::status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const {
	ERR_FAIL_COND_V(!indicators.has(p_id), Rect2());

	NSStatusItem *item = indicators[p_id].item;
	NSView *v = item.button;
	const NSRect contentRect = [v frame];
	const NSRect nsrect = [v.window convertRectToScreen:contentRect];
	Rect2 rect;

	// Return the position of the top-left corner, for macOS the y starts at the bottom.
	const float scale = screen_get_max_scale();
	rect.size.x = nsrect.size.width;
	rect.size.y = nsrect.size.height;
	rect.size *= scale;
	rect.position.x = nsrect.origin.x;
	rect.position.y = (nsrect.origin.y + nsrect.size.height);
	rect.position *= scale;
	rect.position -= _get_screens_origin();
	// macOS native y-coordinate relative to _get_screens_origin() is negative,
	// Godot expects a positive value.
	rect.position.y *= -1;
	return rect;
}

void DisplayServerMacOS::delete_status_indicator(DisplayServerEnums::IndicatorID p_id) {
	ERR_FAIL_COND(!indicators.has(p_id));

	[[NSStatusBar systemStatusBar] removeStatusItem:indicators[p_id].item];
	indicators.erase(p_id);
}

bool DisplayServerMacOS::is_window_transparency_available() const {
#if defined(RD_ENABLED)
	if (rendering_device && !rendering_device->is_composite_alpha_supported()) {
		return false;
	}
#endif
	return OS::is_layered_allowed();
}

DisplayServerDriver *DisplayServerMacOS::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	DisplayServerDriver *ds = memnew(DisplayServerMacOS(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error));
	if (r_error != OK) {
		if (p_rendering_driver == "vulkan") {
			String executable_command;
			if (OS::get_bundle_resource_dir() == OS::get_executable_path().get_base_dir()) {
				executable_command = vformat("\"%s\" --rendering-driver opengl3", OS::get_executable_path());
			} else {
				executable_command = vformat("open \"%s\" --args --rendering-driver opengl3", OS::get_bundle_resource_dir().path_join("../..").simplify_path());
			}
			OS::alert(
					vformat("Your video card drivers seem not to support the required Vulkan version.\n\n"
							"If possible, consider updating your macOS version or using the OpenGL 3 driver.\n\n"
							"You can enable the OpenGL 3 driver by starting the engine from the\n"
							"command line with the command:\n\n    %s",
							executable_command),
					"Unable to initialize Vulkan video driver");
		} else {
			OS::alert(
					"Your video card drivers seem not to support the required OpenGL 3.3 version.\n\n"
					"If possible, consider updating your macOS version.",
					"Unable to initialize OpenGL video driver");
		}
	}
	return ds;
}

Vector<String> DisplayServerMacOS::get_rendering_drivers_func() {
	Vector<String> drivers;

#if defined(VULKAN_ENABLED)
	drivers.push_back("vulkan");
#endif
#if defined(METAL_ENABLED)
	drivers.push_back("metal");
#endif
#if defined(GLES3_ENABLED)
	drivers.push_back("opengl3");
#if defined(ANGLE_ENABLED)
	drivers.push_back("opengl3_angle");
#endif
#endif
	drivers.push_back("dummy");

	return drivers;
}

void DisplayServerMacOS::register_macos_driver() {
	DisplayServer::register_create_function("macos", create_func, get_rendering_drivers_func);
}

DisplayServerEnums::WindowID DisplayServerMacOS::window_get_active_popup() const {
	const List<DisplayServerEnums::WindowID>::Element *E = popup_list.back();
	if (E) {
		return E->get();
	} else {
		return DisplayServerEnums::INVALID_WINDOW_ID;
	}
}

void DisplayServerMacOS::window_set_popup_safe_rect(DisplayServerEnums::WindowID p_window, const Rect2i &p_rect) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.parent_safe_rect = p_rect;
}

Rect2i DisplayServerMacOS::window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Rect2i());
	const WindowData &wd = windows[p_window];
	return wd.parent_safe_rect;
}

void DisplayServerMacOS::popup_open(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	bool has_popup_ancestor = false;
	DisplayServerEnums::WindowID transient_root = p_window;
	while (true) {
		DisplayServerEnums::WindowID parent = windows[transient_root].transient_parent;
		if (parent == DisplayServerEnums::INVALID_WINDOW_ID) {
			break;
		} else {
			transient_root = parent;
			if (windows[parent].is_popup) {
				has_popup_ancestor = true;
				break;
			}
		}
	}

	// Detect tooltips and other similar popups that shouldn't block input to their parent.
	bool ignores_input = window_get_flag(DisplayServerEnums::WINDOW_FLAG_NO_FOCUS, p_window) && window_get_flag(DisplayServerEnums::WINDOW_FLAG_MOUSE_PASSTHROUGH, p_window);

	WindowData &wd = windows[p_window];
	if (wd.is_popup || (has_popup_ancestor && !ignores_input)) {
		bool was_empty = popup_list.is_empty();
		// Find current popup parent, or root popup if new window is not transient.
		List<DisplayServerEnums::WindowID>::Element *C = nullptr;
		List<DisplayServerEnums::WindowID>::Element *E = popup_list.back();
		while (E) {
			if (wd.transient_parent != E->get() || wd.transient_parent == DisplayServerEnums::INVALID_WINDOW_ID) {
				C = E;
				E = E->prev();
			} else {
				break;
			}
		}
		if (C) {
			send_window_event(windows[C->get()], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
		}

		if (was_empty && popup_list.is_empty()) {
			// Inform OS that popup was opened, to close other native popups.
			[[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"com.apple.HIToolbox.beginMenuTrackingNotification" object:@"org.godotengine.godot.popup_window"];
		}
		time_since_popup = OS::get_ticks_msec();
		popup_list.push_back(p_window);
	}
}

void DisplayServerMacOS::popup_close(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	bool was_empty = popup_list.is_empty();
	List<DisplayServerEnums::WindowID>::Element *E = popup_list.find(p_window);
	while (E) {
		List<DisplayServerEnums::WindowID>::Element *F = E->next();
		DisplayServerEnums::WindowID win_id = E->get();
		popup_list.erase(E);

		if (win_id != p_window) {
			// Only request close on related windows, not this window.  We are already processing it.
			send_window_event(windows[win_id], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
		}
		E = F;
	}
	if (!was_empty && popup_list.is_empty()) {
		// Inform OS that all popups are closed.
		[[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"com.apple.HIToolbox.endMenuTrackingNotification" object:@"org.godotengine.godot.popup_window"];
	}
}

bool DisplayServerMacOS::mouse_process_popups(bool p_close) {
	_THREAD_SAFE_METHOD_

	bool was_empty = popup_list.is_empty();
	bool closed = false;
	if (p_close) {
		// Close all popups.
		List<DisplayServerEnums::WindowID>::Element *E = popup_list.front();
		if (E) {
			send_window_event(windows[E->get()], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
			closed = true;
		}
		if (!was_empty) {
			// Inform OS that all popups are closed.
			[[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"com.apple.HIToolbox.endMenuTrackingNotification" object:@"org.godotengine.godot.popup_window"];
		}
	} else {
		uint64_t delta = OS::get_ticks_msec() - time_since_popup;
		if (delta < 250) {
			return false;
		}

		Point2i pos = mouse_get_position();
		List<DisplayServerEnums::WindowID>::Element *C = nullptr;
		List<DisplayServerEnums::WindowID>::Element *E = popup_list.back();
		// Find top popup to close.
		while (E) {
			// Popup window area.
			Rect2i win_rect = Rect2i(window_get_position_with_decorations(E->get()), window_get_size_with_decorations(E->get()));
			// Area of the parent window, which responsible for opening sub-menu.
			Rect2i safe_rect = window_get_popup_safe_rect(E->get());
			if (win_rect.has_point(pos)) {
				break;
			} else if (safe_rect != Rect2i() && safe_rect.has_point(pos)) {
				break;
			} else {
				C = E;
				E = E->prev();
			}
		}
		if (C) {
			send_window_event(windows[C->get()], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
			closed = true;
		}
		if (!was_empty && popup_list.is_empty()) {
			// Inform OS that all popups are closed.
			[[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"com.apple.HIToolbox.endMenuTrackingNotification" object:@"org.godotengine.godot.popup_window"];
		}
	}
	return closed;
}

DisplayServerMacOS::DisplayServerMacOS(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	Input::set_event_dispatch_function(_dispatch_input_events);

	r_error = OK;

	memset(cursors, 0, sizeof(cursors));

	event_source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
	ERR_FAIL_COND(!event_source);

	CGEventSourceSetLocalEventsSuppressionInterval(event_source, 0.0);

	int screen_count = get_screen_count();
	for (int i = 0; i < screen_count; i++) {
		display_max_scale = std::fmax(display_max_scale, screen_get_scale(i));
	}

	// Register to be notified on displays arrangement changes.
	CGDisplayRegisterReconfigurationCallback(_displays_arrangement_changed, nullptr);

	native_menu = memnew(NativeMenuMacOS);

	NSMenuItem *menu_item;
	NSString *title;

	NSString *nsappname = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
	if (nsappname == nil) {
		nsappname = [[NSProcessInfo processInfo] processName];
	}

	menu_delegate = [[GodotMenuDelegate alloc] init];

	// Setup Dock menu.
	NSMenu *dock_menu = [[NSMenu alloc] initWithTitle:@"_dock"];
	[dock_menu setAutoenablesItems:NO];
	[dock_menu setDelegate:menu_delegate];

	// Setup Apple menu.
	NSMenu *application_menu = [[NSMenu alloc] initWithTitle:@""];
	title = [NSString stringWithFormat:NSLocalizedString(@"About %@", nil), nsappname];
	[application_menu addItemWithTitle:title action:@selector(showAbout:) keyEquivalent:@""];
	[application_menu setAutoenablesItems:NO];
	[application_menu setDelegate:menu_delegate];

	[application_menu addItem:[NSMenuItem separatorItem]];

	menu_item = [application_menu addItemWithTitle:@"_start_" action:nil keyEquivalent:@""];
	menu_item.hidden = YES;
	menu_item.tag = MENU_TAG_START;
	menu_item = [application_menu addItemWithTitle:@"_end_" action:nil keyEquivalent:@""];
	menu_item.hidden = YES;
	menu_item.tag = MENU_TAG_END;

	NSMenu *services = [[NSMenu alloc] initWithTitle:@""];
	menu_item = [application_menu addItemWithTitle:NSLocalizedString(@"Services", nil) action:nil keyEquivalent:@""];
	[application_menu setSubmenu:services forItem:menu_item];
	[NSApp setServicesMenu:services];

	[application_menu addItem:[NSMenuItem separatorItem]];

	title = [NSString stringWithFormat:NSLocalizedString(@"Hide %@", nil), nsappname];
	[application_menu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

	menu_item = [application_menu addItemWithTitle:NSLocalizedString(@"Hide Others", nil) action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
	[menu_item setKeyEquivalentModifierMask:(NSEventModifierFlagOption | NSEventModifierFlagCommand)];

	[application_menu addItemWithTitle:NSLocalizedString(@"Show all", nil) action:@selector(unhideAllApplications:) keyEquivalent:@""];

	[application_menu addItem:[NSMenuItem separatorItem]];

	title = [NSString stringWithFormat:NSLocalizedString(@"Quit %@", nil), nsappname];
	[application_menu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

	NSMenu *window_menu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"Window", nil)];
	[window_menu addItemWithTitle:NSLocalizedString(@"Minimize", nil) action:@selector(performMiniaturize:) keyEquivalent:@"m"];
	[window_menu addItemWithTitle:NSLocalizedString(@"Zoom", nil) action:@selector(performZoom:) keyEquivalent:@""];
	[window_menu addItem:[NSMenuItem separatorItem]];
	[window_menu addItemWithTitle:NSLocalizedString(@"Bring All to Front", nil) action:@selector(bringAllToFront:) keyEquivalent:@""];
	[window_menu addItem:[NSMenuItem separatorItem]];
	menu_item = [window_menu addItemWithTitle:@"_start_" action:nil keyEquivalent:@""];
	menu_item.hidden = YES;
	menu_item.tag = MENU_TAG_START;
	menu_item = [window_menu addItemWithTitle:@"_end_" action:nil keyEquivalent:@""];
	menu_item.hidden = YES;
	menu_item.tag = MENU_TAG_END;

	NSMenu *help_menu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"Help", nil)];
	menu_item = [help_menu addItemWithTitle:@"_start_" action:nil keyEquivalent:@""];
	menu_item.hidden = YES;
	menu_item.tag = MENU_TAG_START;
	menu_item = [help_menu addItemWithTitle:@"_end_" action:nil keyEquivalent:@""];
	menu_item.hidden = YES;
	menu_item.tag = MENU_TAG_END;

	[NSApp setWindowsMenu:window_menu];
	[NSApp setHelpMenu:help_menu];

	// Add items to the menu bar.
	NSMenu *main_menu = [NSApp mainMenu];
	menu_item = [main_menu addItemWithTitle:@"" action:nil keyEquivalent:@""];
	[main_menu setSubmenu:application_menu forItem:menu_item];

	menu_item = [main_menu addItemWithTitle:NSLocalizedString(@"Window", nil) action:nil keyEquivalent:@""];
	[main_menu setSubmenu:window_menu forItem:menu_item];

	menu_item = [main_menu addItemWithTitle:NSLocalizedString(@"Help", nil) action:nil keyEquivalent:@""];
	[main_menu setSubmenu:help_menu forItem:menu_item];

	[main_menu setAutoenablesItems:NO];

	native_menu->_register_system_menus(main_menu, application_menu, window_menu, help_menu, dock_menu);

	//!!!!!!!!!!!!!!!!!!!!!!!!!!
	//TODO - do Vulkan and OpenGL support checks, driver selection and fallback
	rendering_driver = p_rendering_driver;

#if defined(RD_ENABLED)
#if defined(VULKAN_ENABLED)
#if defined(__x86_64__)
	bool fallback_to_vulkan = GLOBAL_GET("rendering/rendering_device/fallback_to_vulkan");
	if (!fallback_to_vulkan) {
		WARN_PRINT("Metal is not supported on Intel Macs, switching to Vulkan.");
	}
	// Metal rendering driver not available on Intel.
	if (rendering_driver == "metal") {
		rendering_driver = "vulkan";
		OS::set_current_rendering_driver_name(rendering_driver, OS::RENDERING_SOURCE_FALLBACK);
	}
#endif
	if (rendering_driver == "vulkan") {
		rendering_context = memnew(RenderingContextDriverVulkan(RenderingContextDriverVulkan::SURFACE_PLATFORM));
	}
#endif
#if defined(METAL_ENABLED)
	if (rendering_driver == "metal") {
		rendering_context = memnew(RenderingContextDriverMetal);
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
				r_error = ERR_CANT_CREATE;
				ERR_FAIL_MSG("Could not initialize " + rendering_driver);
			}
		}
	}
#endif

#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	if (rendering_driver == "opengl3" && OS::get_processor_name().contains("Virtual")) {
		WARN_PRINT("Virtual Machine detected, switching to ANGLE.");
		rendering_driver = "opengl3_angle";
	}
	if (rendering_driver == "opengl3_angle") {
		gl_manager_angle = memnew(GLManagerANGLE_MacOS);
		if (gl_manager_angle->initialize() != OK || gl_manager_angle->open_display(nullptr) != OK) {
			memdelete(gl_manager_angle);
			gl_manager_angle = nullptr;
			if (OS::get_processor_name().contains("Virtual")) {
				r_error = ERR_UNAVAILABLE;
				ERR_FAIL_MSG("Could not initialize ANGLE OpenGL.");
			}
			bool fallback = GLOBAL_GET("rendering/gl_compatibility/fallback_to_native");
			if (fallback) {
				WARN_PRINT("Your video card drivers seem not to support GLES3 / ANGLE, switching to native OpenGL.");
				rendering_driver = "opengl3";
				OS::set_current_rendering_driver_name(rendering_driver, OS::RENDERING_SOURCE_FALLBACK);
			} else {
				r_error = ERR_UNAVAILABLE;
				ERR_FAIL_MSG("Could not initialize ANGLE OpenGL.");
			}
		}
	}
#else
	if (rendering_driver == "opengl3" && OS::get_processor_name().contains("Virtual")) {
		r_error = ERR_UNAVAILABLE;
		ERR_FAIL_MSG("Virtual Machine detected, could not initialize OpenGL.");
	}
#endif
	if (rendering_driver == "opengl3") {
		gl_manager_legacy = memnew(GLManagerLegacy_MacOS);
		if (gl_manager_legacy->initialize() != OK) {
			memdelete(gl_manager_legacy);
			gl_manager_legacy = nullptr;
			r_error = ERR_UNAVAILABLE;
			ERR_FAIL_MSG("Could not initialize native OpenGL.");
		}
	}
#endif

	Point2i window_position;
	if (p_position != nullptr) {
		window_position = *p_position;
	} else {
		if (p_screen == DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) {
			p_screen = DisplayServerEnums::SCREEN_PRIMARY;
		}
		Rect2i scr_rect = screen_get_usable_rect(p_screen);
		window_position = scr_rect.position + (scr_rect.size - p_resolution) / 2;
	}

	DisplayServerEnums::WindowID main_window = _create_window(p_mode, p_vsync_mode, Rect2i(window_position, p_resolution));
	ERR_FAIL_COND(main_window == DisplayServerEnums::INVALID_WINDOW_ID);
	for (int i = 0; i < DisplayServerEnums::WINDOW_FLAG_MAX; i++) {
		if (p_flags & (1 << i)) {
			window_set_flag(DisplayServerEnums::WindowFlags(i), true, main_window);
		}
	}
	force_process_and_drop_events();

	__block DisplayServerMacOS *self = this;
	screen_observer = [NSNotificationCenter.defaultCenter
			addObserverForName:NSApplicationDidChangeScreenParametersNotification
						object:nil
						 queue:nil
					usingBlock:^(NSNotification *_Nonnull note) {
						self->update_screen_parameters();
					}];

	if (rendering_driver == "dummy") {
		RasterizerDummy::make_current();
	}

#if defined(GLES3_ENABLED)
	if (rendering_driver == "opengl3") {
		RasterizerGLES3::make_current(true);
	}
#if defined(ANGLE_ENABLED)
	if (rendering_driver == "opengl3_angle") {
		RasterizerGLES3::make_current(false);
	}
#endif
#endif
#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_device = memnew(RenderingDevice);
		rendering_device->initialize(rendering_context, DisplayServerEnums::MAIN_WINDOW_ID);
		rendering_device->screen_create(DisplayServerEnums::MAIN_WINDOW_ID);

		RendererCompositorRD::make_current();
	}
#endif
}

DisplayServerMacOS::~DisplayServerMacOS() {
	// Destroy all status indicators.
	for (HashMap<DisplayServerEnums::IndicatorID, IndicatorData>::Iterator E = indicators.begin(); E; ++E) {
		[[NSStatusBar systemStatusBar] removeStatusItem:E->value.item];
	}

	// Destroy native menu.
	if (native_menu) {
		NativeMenu::finalize();
		native_menu = nullptr;
	}

	// Destroy all windows.
	for (HashMap<DisplayServerEnums::WindowID, WindowData>::Iterator E = windows.begin(); E;) {
		HashMap<DisplayServerEnums::WindowID, WindowData>::Iterator F = E;
		++E;
		[F->value.window_object setContentView:nil];
		[F->value.window_object close];
	}

	// Destroy drivers.
#if defined(GLES3_ENABLED)
	if (gl_manager_legacy) {
		memdelete(gl_manager_legacy);
		gl_manager_legacy = nullptr;
	}
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		memdelete(gl_manager_angle);
		gl_manager_angle = nullptr;
	}
#endif
#endif
#if defined(RD_ENABLED)
	if (rendering_device) {
		memdelete(rendering_device);
		rendering_device = nullptr;
	}

	if (rendering_context) {
		memdelete(rendering_context);
		rendering_context = nullptr;
	}
#endif

	[NSNotificationCenter.defaultCenter removeObserver:screen_observer];

	CGDisplayRemoveReconfigurationCallback(_displays_arrangement_changed, nullptr);

	cursors_cache.clear();
}

// === Merged from platform/macos/display_server_macos_base.mm ===
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

#import "servers/display/display_server.h"


#include "core/os/os.h"
#include "core/io/net_socket.h"

#if defined(RD_ENABLED)
#include "servers/display/display_server.h"
#endif

#import <Carbon/Carbon.h>

void DisplayServerMacOSBase::_mouse_update_mode() {
	DisplayServerEnums::MouseMode wanted_mouse_mode = mouse_mode_override_enabled
			? mouse_mode_override
			: mouse_mode_base;

	if (wanted_mouse_mode == mouse_mode) {
		return;
	}

	DisplayServerEnums::MouseMode prev_mode = mouse_mode;
	mouse_mode = wanted_mouse_mode;
	_mouse_apply_mode(prev_mode, wanted_mouse_mode);
}

void DisplayServerMacOSBase::mouse_set_mode(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_base) {
		return;
	}
	mouse_mode_base = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerMacOSBase::mouse_get_mode() const {
	return mouse_mode;
}

void DisplayServerMacOSBase::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_override) {
		return;
	}
	mouse_mode_override = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerMacOSBase::mouse_get_mode_override() const {
	return mouse_mode_override;
}

void DisplayServerMacOSBase::mouse_set_mode_override_enabled(bool p_override_enabled) {
	if (p_override_enabled == mouse_mode_override_enabled) {
		return;
	}
	mouse_mode_override_enabled = p_override_enabled;
	_mouse_update_mode();
}

bool DisplayServerMacOSBase::mouse_is_mode_override_enabled() const {
	return mouse_mode_override_enabled;
}

void DisplayServerMacOSBase::clipboard_set(const String &p_text) {
	_THREAD_SAFE_METHOD_

	NSString *copiedString = [NSString stringWithUTF8String:p_text.utf8().get_data()];
	NSArray *copiedStringArray = [NSArray arrayWithObject:copiedString];

	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	[pasteboard clearContents];
	[pasteboard writeObjects:copiedStringArray];
}

String DisplayServerMacOSBase::clipboard_get() const {
	_THREAD_SAFE_METHOD_

	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
	NSDictionary *options = [NSDictionary dictionary];

	BOOL ok = [pasteboard canReadObjectForClasses:classArray options:options];

	if (!ok) {
		return "";
	}

	NSArray *objectsToPaste = [pasteboard readObjectsForClasses:classArray options:options];
	if (!objectsToPaste || [objectsToPaste count] < 1) {
		return "";
	}
	NSString *string = [objectsToPaste objectAtIndex:0];

	String ret;
	ret.append_utf8([string UTF8String]);
	return ret;
}

Ref<Image> DisplayServerMacOSBase::clipboard_get_image() const {
	Ref<Image> image;
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSString *result = [pasteboard availableTypeFromArray:[NSArray arrayWithObjects:NSPasteboardTypeTIFF, NSPasteboardTypePNG, nil]];
	if (!result) {
		return image;
	}
	NSData *data = [pasteboard dataForType:result];
	if (!data) {
		return image;
	}
	NSBitmapImageRep *bitmap = [NSBitmapImageRep imageRepWithData:data];
	NSData *pngData = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
	image.instantiate();
	PNGDriverCommon::png_to_image((const uint8_t *)pngData.bytes, pngData.length, false, image);
	return image;
}

bool DisplayServerMacOSBase::clipboard_has() const {
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
	NSDictionary *options = [NSDictionary dictionary];
	return [pasteboard canReadObjectForClasses:classArray options:options];
}

bool DisplayServerMacOSBase::clipboard_has_image() const {
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSString *result = [pasteboard availableTypeFromArray:[NSArray arrayWithObjects:NSPasteboardTypeTIFF, NSPasteboardTypePNG, nil]];
	return result;
}

CGDirectDisplayID DisplayServerMacOSBase::_get_display_id_for_screen(NSScreen *p_screen) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
	if (@available(macOS 26.0, *)) {
		return [p_screen CGDirectDisplayID];
	} else {
		return [[p_screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];
	}
#else
	return [[p_screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];
#endif
}

void DisplayServerMacOSBase::initialize_tts() const {
	const_cast<DisplayServerMacOSBase *>(this)->tts = [[TTS_MacOS alloc] init];
}

bool DisplayServerMacOSBase::tts_is_speaking() const {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL_V(tts, false);
	return [tts isSpeaking];
}

bool DisplayServerMacOSBase::tts_is_paused() const {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL_V(tts, false);
	return [tts isPaused];
}

TypedArray<Dictionary> DisplayServerMacOSBase::tts_get_voices() const {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL_V(tts, TypedArray<Dictionary>());
	return [tts getVoices];
}

void DisplayServerMacOSBase::tts_speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	[tts speak:p_text voice:p_voice volume:p_volume pitch:p_pitch rate:p_rate utterance_id:p_utterance_id interrupt:p_interrupt];
}

void DisplayServerMacOSBase::tts_pause() {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	[tts pauseSpeaking];
}

void DisplayServerMacOSBase::tts_resume() {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	[tts resumeSpeaking];
}

void DisplayServerMacOSBase::tts_stop() {
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	[tts stopSpeaking];
}

void DisplayServerMacOSBase::_update_keyboard_layouts() const {
	kbd_layouts.clear();
	current_layout = 0;

	TISInputSourceRef cur_source = TISCopyCurrentKeyboardInputSource();
	NSString *cur_name = (__bridge NSString *)TISGetInputSourceProperty(cur_source, kTISPropertyLocalizedName);
	CFRelease(cur_source);

	// Enum IME layouts.
	NSDictionary *filter_ime = @{ (NSString *)kTISPropertyInputSourceType : (NSString *)kTISTypeKeyboardInputMode };
	NSArray *list_ime = (__bridge NSArray *)TISCreateInputSourceList((__bridge CFDictionaryRef)filter_ime, false);
	for (NSUInteger i = 0; i < [list_ime count]; i++) {
		LayoutInfo ly;
		NSString *name = (__bridge NSString *)TISGetInputSourceProperty((__bridge TISInputSourceRef)[list_ime objectAtIndex:i], kTISPropertyLocalizedName);
		ly.name.append_utf8([name UTF8String]);

		NSArray *langs = (__bridge NSArray *)TISGetInputSourceProperty((__bridge TISInputSourceRef)[list_ime objectAtIndex:i], kTISPropertyInputSourceLanguages);
		ly.code.append_utf8([(NSString *)[langs objectAtIndex:0] UTF8String]);
		kbd_layouts.push_back(ly);

		if ([name isEqualToString:cur_name]) {
			current_layout = kbd_layouts.size() - 1;
		}
	}

	// Enum plain keyboard layouts.
	NSDictionary *filter_kbd = @{ (NSString *)kTISPropertyInputSourceType : (NSString *)kTISTypeKeyboardLayout };
	NSArray *list_kbd = (__bridge NSArray *)TISCreateInputSourceList((__bridge CFDictionaryRef)filter_kbd, false);
	for (NSUInteger i = 0; i < [list_kbd count]; i++) {
		LayoutInfo ly;
		NSString *name = (__bridge NSString *)TISGetInputSourceProperty((__bridge TISInputSourceRef)[list_kbd objectAtIndex:i], kTISPropertyLocalizedName);
		ly.name.append_utf8([name UTF8String]);

		NSArray *langs = (__bridge NSArray *)TISGetInputSourceProperty((__bridge TISInputSourceRef)[list_kbd objectAtIndex:i], kTISPropertyInputSourceLanguages);
		ly.code.append_utf8([(NSString *)[langs objectAtIndex:0] UTF8String]);
		kbd_layouts.push_back(ly);

		if ([name isEqualToString:cur_name]) {
			current_layout = kbd_layouts.size() - 1;
		}
	}

	keyboard_layout_dirty = false;
}

void DisplayServerMacOSBase::_keyboard_layout_changed(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef user_info) {
	DisplayServerMacOSBase *ds = DisplayServer::driver_as<DisplayServerMacOSBase>();
	if (ds) {
		ds->keyboard_layout_dirty = true;
	}
}

int DisplayServerMacOSBase::keyboard_get_layout_count() const {
	if (keyboard_layout_dirty) {
		_update_keyboard_layouts();
	}
	return kbd_layouts.size();
}

void DisplayServerMacOSBase::keyboard_set_current_layout(int p_index) {
	if (keyboard_layout_dirty) {
		_update_keyboard_layouts();
	}

	ERR_FAIL_INDEX(p_index, kbd_layouts.size());

	NSString *cur_name = [NSString stringWithUTF8String:kbd_layouts[p_index].name.utf8().get_data()];

	NSDictionary *filter_kbd = @{ (NSString *)kTISPropertyInputSourceType : (NSString *)kTISTypeKeyboardLayout };
	NSArray *list_kbd = (__bridge NSArray *)TISCreateInputSourceList((__bridge CFDictionaryRef)filter_kbd, false);
	for (NSUInteger i = 0; i < [list_kbd count]; i++) {
		NSString *name = (__bridge NSString *)TISGetInputSourceProperty((__bridge TISInputSourceRef)[list_kbd objectAtIndex:i], kTISPropertyLocalizedName);
		if ([name isEqualToString:cur_name]) {
			TISSelectInputSource((__bridge TISInputSourceRef)[list_kbd objectAtIndex:i]);
			break;
		}
	}

	NSDictionary *filter_ime = @{ (NSString *)kTISPropertyInputSourceType : (NSString *)kTISTypeKeyboardInputMode };
	NSArray *list_ime = (__bridge NSArray *)TISCreateInputSourceList((__bridge CFDictionaryRef)filter_ime, false);
	for (NSUInteger i = 0; i < [list_ime count]; i++) {
		NSString *name = (__bridge NSString *)TISGetInputSourceProperty((__bridge TISInputSourceRef)[list_ime objectAtIndex:i], kTISPropertyLocalizedName);
		if ([name isEqualToString:cur_name]) {
			TISSelectInputSource((__bridge TISInputSourceRef)[list_ime objectAtIndex:i]);
			break;
		}
	}
}

int DisplayServerMacOSBase::keyboard_get_current_layout() const {
	if (keyboard_layout_dirty) {
		_update_keyboard_layouts();
	}

	return current_layout;
}

String DisplayServerMacOSBase::keyboard_get_layout_language(int p_index) const {
	if (keyboard_layout_dirty) {
		_update_keyboard_layouts();
	}

	ERR_FAIL_INDEX_V(p_index, kbd_layouts.size(), "");
	return kbd_layouts[p_index].code;
}

String DisplayServerMacOSBase::keyboard_get_layout_name(int p_index) const {
	if (keyboard_layout_dirty) {
		_update_keyboard_layouts();
	}

	ERR_FAIL_INDEX_V(p_index, kbd_layouts.size(), "");
	return kbd_layouts[p_index].name;
}

Key DisplayServerMacOSBase::keyboard_get_keycode_from_physical(Key p_keycode) const {
	if (p_keycode == Key::PAUSE || p_keycode == Key::NONE) {
		return p_keycode;
	}

	Key modifiers = p_keycode & KeyModifierMask::MODIFIER_MASK;
	Key keycode_no_mod = p_keycode & KeyModifierMask::CODE_MASK;
	unsigned int macos_keycode = KeyMappingMacOS::unmap_key(keycode_no_mod);
	return (Key)(KeyMappingMacOS::remap_key(macos_keycode, 0, false) | modifiers);
}

Key DisplayServerMacOSBase::keyboard_get_label_from_physical(Key p_keycode) const {
	if (p_keycode == Key::PAUSE || p_keycode == Key::NONE) {
		return p_keycode;
	}

	Key modifiers = p_keycode & KeyModifierMask::MODIFIER_MASK;
	Key keycode_no_mod = p_keycode & KeyModifierMask::CODE_MASK;
	unsigned int macos_keycode = KeyMappingMacOS::unmap_key(keycode_no_mod);
	return (Key)(KeyMappingMacOS::remap_key(macos_keycode, 0, true) | modifiers);
}

void DisplayServerMacOSBase::show_emoji_and_symbol_picker() const {
	[[NSApplication sharedApplication] orderFrontCharacterPalette:nil];
}

bool DisplayServerMacOSBase::is_dark_mode_supported() const {
	return true;
}

bool DisplayServerMacOSBase::is_dark_mode() const {
	if (![[NSUserDefaults standardUserDefaults] objectForKey:@"AppleInterfaceStyle"]) {
		return false;
	} else {
		return ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"] isEqual:@"Dark"]);
	}
}

Color DisplayServerMacOSBase::get_accent_color() const {
	__block NSColor *color = nullptr;
	[NSApp.effectiveAppearance performAsCurrentDrawingAppearance:^{
		color = [[NSColor controlAccentColor] colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
	}];
	if (!color) {
		color = [[NSColor controlAccentColor] colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
	}
	if (color) {
		CGFloat components[4];
		[color getRed:&components[0] green:&components[1] blue:&components[2] alpha:&components[3]];
		return Color(components[0], components[1], components[2], components[3]);
	} else {
		return Color(0, 0, 0, 0);
	}
}

Color DisplayServerMacOSBase::get_base_color() const {
	__block NSColor *color = nullptr;
	[NSApp.effectiveAppearance performAsCurrentDrawingAppearance:^{
		color = [[NSColor windowBackgroundColor] colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
	}];
	if (!color) {
		color = [[NSColor controlColor] colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
	}
	if (color) {
		CGFloat components[4];
		[color getRed:&components[0] green:&components[1] blue:&components[2] alpha:&components[3]];
		return Color(components[0], components[1], components[2], components[3]);
	} else {
		return Color(0, 0, 0, 0);
	}
}

void DisplayServerMacOSBase::set_system_theme_change_callback(const Callable &p_callable) {
	system_theme_changed = p_callable;
}

bool DisplayServerMacOSBase::screen_is_kept_on() const {
	return (screen_keep_on_assertion);
}

void DisplayServerMacOSBase::screen_set_keep_on(bool p_enable) {
	if (screen_keep_on_assertion) {
		IOPMAssertionRelease(screen_keep_on_assertion);
		screen_keep_on_assertion = kIOPMNullAssertionID;
	}

	if (p_enable) {
		String app_name_string = GLOBAL_GET("application/config/name");
		NSString *name = [NSString stringWithUTF8String:(app_name_string.is_empty() ? "Godot Engine" : app_name_string.utf8().get_data())];
		NSString *reason = @"Godot Engine running with display/window/energy_saving/keep_screen_on = true";
		IOPMAssertionCreateWithDescription(kIOPMAssertPreventUserIdleDisplaySleep, (__bridge CFStringRef)name, (__bridge CFStringRef)reason, (__bridge CFStringRef)reason, nullptr, 0, nullptr, &screen_keep_on_assertion);
	}
}

int DisplayServerMacOSBase::accessibility_should_increase_contrast() const {
	return [(GodotApplicationDelegate *)[[NSApplication sharedApplication] delegate] getHighContrast];
}

int DisplayServerMacOSBase::accessibility_should_reduce_animation() const {
	return [(GodotApplicationDelegate *)[[NSApplication sharedApplication] delegate] getReduceMotion];
}

int DisplayServerMacOSBase::accessibility_should_reduce_transparency() const {
	return [(GodotApplicationDelegate *)[[NSApplication sharedApplication] delegate] getReduceTransparency];
}

int DisplayServerMacOSBase::accessibility_screen_reader_active() const {
	return [(GodotApplicationDelegate *)[[NSApplication sharedApplication] delegate] getVoiceOver];
}

void DisplayServerMacOSBase::update_im_text(const Point2i &p_selection, const String &p_text) {
	im_selection = p_selection;
	im_text = p_text;

	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_OS_IME_UPDATE);
	}
}

Point2i DisplayServerMacOSBase::ime_get_selection() const {
	return im_selection;
}

String DisplayServerMacOSBase::ime_get_text() const {
	return im_text;
}

DisplayServerEnums::CursorShape DisplayServerMacOSBase::cursor_get_shape() const {
	return cursor_shape;
}

void DisplayServerMacOSBase::beep() const {
	NSBeep();
}

int DisplayServerMacOSBase::get_primary_screen() const {
	return 0;
}

float DisplayServerMacOSBase::screen_get_refresh_rate(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, SCREEN_REFRESH_RATE_FALLBACK);

	NSArray *screenArray = [NSScreen screens];
	if ((NSUInteger)p_screen < [screenArray count]) {
		NSScreen *screen = [screenArray objectAtIndex:p_screen];
		const CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(_get_display_id_for_screen(screen));
		const double displayRefreshRate = CGDisplayModeGetRefreshRate(displayMode);
		return (float)displayRefreshRate;
	}
	ERR_PRINT("An error occurred while trying to get the screen refresh rate.");
	return SCREEN_REFRESH_RATE_FALLBACK;
}

void DisplayServerMacOSBase::emit_system_theme_changed() {
	if (system_theme_changed.is_valid()) {
		Variant ret;
		Callable::CallError ce;
		system_theme_changed.callp(nullptr, 0, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Failed to execute system theme changed callback: %s.", Variant::get_callable_error_text(system_theme_changed, nullptr, 0, ce)));
		}
	}
}

// MARK: - HDR / EDR

void DisplayServerMacOSBase::_update_hdr_output(DisplayServerEnums::WindowID p_window, const HDROutput &p_hdr) {
#ifdef RD_ENABLED
	if (!rendering_context) {
		return;
	}

	CGFloat max_potential_edr, max_edr;
	window_get_edr_values(p_window, &max_potential_edr, &max_edr);
	bool desired_hdr_enabled = p_hdr.requested && max_potential_edr > 1.0f;
	bool current_hdr_enabled = rendering_context->window_get_hdr_output_enabled(p_window);
	bool hdr_state_changed = false;
	if (current_hdr_enabled != desired_hdr_enabled) {
		rendering_context->window_set_hdr_output_enabled(p_window, desired_hdr_enabled);
		hdr_state_changed = true;
	}

	float new_reference_luminance = _calculate_current_reference_luminance(max_potential_edr, max_edr);
	float current_ref_luminance = rendering_context->window_get_hdr_output_reference_luminance(p_window);
	if (!Math::is_equal_approx(current_ref_luminance, new_reference_luminance)) {
		rendering_context->window_set_hdr_output_reference_luminance(p_window, new_reference_luminance);
		rendering_context->window_set_hdr_output_linear_luminance_scale(p_window, new_reference_luminance);
		hdr_state_changed = true;
	}

	float new_max_luminance = p_hdr.is_auto_max_luminance() ? max_potential_edr * HARDWARE_REFERENCE_LUMINANCE_NITS : p_hdr.max_luminance;
	float current_max_luminance = rendering_context->window_get_hdr_output_max_luminance(p_window);
	if (!Math::is_equal_approx(current_max_luminance, new_max_luminance)) {
		rendering_context->window_set_hdr_output_max_luminance(p_window, new_max_luminance);
		hdr_state_changed = true;
	}

	if (hdr_state_changed) {
		send_window_event_by_id(DisplayServerEnums::WINDOW_EVENT_OUTPUT_MAX_LINEAR_VALUE_CHANGED, p_window);
	}
#endif
}

bool DisplayServerMacOSBase::window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!has_window(p_window), false);
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

	CGFloat max_potential_edr;
	window_get_edr_values(p_window, &max_potential_edr, nullptr);
	return max_potential_edr > 1.0f;
}

void DisplayServerMacOSBase::window_request_hdr_output(const bool p_enabled, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!has_window(p_window));
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

	HDROutput &hdr = _get_hdr_output(p_window);
	hdr.requested = p_enabled;
	_update_hdr_output(p_window, hdr);
}

bool DisplayServerMacOSBase::window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!has_window(p_window), false);
	return _get_hdr_output(p_window).requested;
}

bool DisplayServerMacOSBase::window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!has_window(p_window), false);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_enabled(p_window);
	}
#endif

	return false;
}

void DisplayServerMacOSBase::window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window) {
	ERR_PRINT_ONCE("Manually setting reference white luminance is not supported on Apple devices, as they provide a user-facing brightness setting that directly controls reference white luminance.");
}

float DisplayServerMacOSBase::window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window) const {
	return -1.0f; // Always auto-adjusted by the OS on Apple platforms.
}

constexpr float DisplayServerMacOSBase::_calculate_current_reference_luminance(CGFloat p_max_potential_edr_value, CGFloat p_max_edr_value) const {
	return (p_max_potential_edr_value * HARDWARE_REFERENCE_LUMINANCE_NITS) / p_max_edr_value;
}

float DisplayServerMacOSBase::window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!has_window(p_window), 0.0);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_reference_luminance(p_window);
	}
#endif
	return 200.0f;
}

void DisplayServerMacOSBase::window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!has_window(p_window));
	HDROutput &hdr = _get_hdr_output(p_window);

	if (hdr.max_luminance == p_max_luminance) {
		return;
	}
	hdr.max_luminance = p_max_luminance;
	_update_hdr_output(p_window, hdr);
}

float DisplayServerMacOSBase::window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!has_window(p_window), 0.0);
	return _get_hdr_output(p_window).max_luminance;
}

float DisplayServerMacOSBase::window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!has_window(p_window), 0.0);
	const HDROutput &hdr = _get_hdr_output(p_window);
	if (hdr.is_auto_max_luminance()) {
		CGFloat max_potential_edr;
		window_get_edr_values(p_window, &max_potential_edr, nullptr);
		return max_potential_edr * HARDWARE_REFERENCE_LUMINANCE_NITS;
	}
	return hdr.max_luminance;
}

float DisplayServerMacOSBase::window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!has_window(p_window), 1.0);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_output_max_linear_value(p_window);
	}
#endif

	return 1.0f; // SDR
}

DisplayServerMacOSBase::DisplayServerMacOSBase() {
	KeyMappingMacOS::initialize();

	// Init TTS
	bool tts_enabled = GLOBAL_GET("audio/general/text_to_speech");
	if (tts_enabled) {
		initialize_tts();
	}

	screen_set_keep_on(GLOBAL_GET("display/window/energy_saving/keep_screen_on"));

	// Register to be notified on keyboard layout changes.
	CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
			nullptr, _keyboard_layout_changed,
			kTISNotifySelectedKeyboardInputSourceChanged, nullptr,
			CFNotificationSuspensionBehaviorDeliverImmediately);
}

DisplayServerMacOSBase::~DisplayServerMacOSBase() {
	if (screen_keep_on_assertion) {
		IOPMAssertionRelease(screen_keep_on_assertion);
		screen_keep_on_assertion = kIOPMNullAssertionID;
	}

	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(), nullptr, kTISNotifySelectedKeyboardInputSourceChanged, nullptr);
}

// === Merged from platform/macos/godot_button_view.mm ===
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

#import "servers/display/display_server.h"

@implementation GodotButtonView

- (id)initWithFrame:(NSRect)frame {
	self = [super initWithFrame:frame];

	tracking_area = nil;
	offset = NSMakePoint(8, 8);
	spacing = 20;
	mouse_in_group = false;
	rtl = false;
	close_button = nullptr;
	miniaturize_button = nullptr;
	zoom_button = nullptr;

	return self;
}

- (void)initButtons:(CGFloat)button_spacing offset:(NSPoint)button_offset rtl:(bool)is_rtl {
	spacing = button_spacing;
	rtl = is_rtl;

	close_button = [NSWindow standardWindowButton:NSWindowCloseButton forStyleMask:NSWindowStyleMaskTitled];
	[close_button setFrameOrigin:NSMakePoint(rtl ? spacing * 2 : 0, 0)];
	[self addSubview:close_button];

	miniaturize_button = [NSWindow standardWindowButton:NSWindowMiniaturizeButton forStyleMask:NSWindowStyleMaskTitled];
	[miniaturize_button setFrameOrigin:NSMakePoint(spacing, 0)];
	[self addSubview:miniaturize_button];

	zoom_button = [NSWindow standardWindowButton:NSWindowZoomButton forStyleMask:NSWindowStyleMaskTitled];
	[zoom_button setFrameOrigin:NSMakePoint(rtl ? 0 : spacing * 2, 0)];
	[self addSubview:zoom_button];

	offset.y = button_offset.y - zoom_button.frame.size.height / 2;
	offset.x = button_offset.x - zoom_button.frame.size.width / 2;

	if (rtl) {
		[self setFrameSize:NSMakeSize(close_button.frame.origin.x + close_button.frame.size.width, close_button.frame.size.height)];
	} else {
		[self setFrameSize:NSMakeSize(zoom_button.frame.origin.x + zoom_button.frame.size.width, zoom_button.frame.size.height)];
	}
	[self displayButtons];
}

- (void)setOffset:(NSPoint)button_offset {
	if (zoom_button) {
		offset.y = button_offset.y - zoom_button.frame.size.height / 2;
		offset.x = button_offset.x - zoom_button.frame.size.width / 2;

		[self viewDidMoveToWindow];
	}
}

- (NSPoint)getOffset {
	return offset;
}

- (void)viewDidMoveToWindow {
	if (!self.window) {
		return;
	}

	if (rtl) {
		[self setAutoresizingMask:NSViewMinXMargin | NSViewMinYMargin];
		[self setFrameOrigin:NSMakePoint(self.window.frame.size.width - self.frame.size.width - offset.x, self.window.frame.size.height - self.frame.size.height - offset.y)];
	} else {
		[self setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
		[self setFrameOrigin:NSMakePoint(offset.x, self.window.frame.size.height - self.frame.size.height - offset.y)];
	}
}

- (BOOL)_mouseInGroup:(NSButton *)button {
	return mouse_in_group;
}

- (void)updateTrackingAreas {
	if (tracking_area != nil) {
		[self removeTrackingArea:tracking_area];
	}

	NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways | NSTrackingInVisibleRect;
	tracking_area = [[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil];

	[self addTrackingArea:tracking_area];
}

- (void)mouseEntered:(NSEvent *)event {
	[super mouseEntered:event];

	mouse_in_group = true;
	[self displayButtons];
}

- (void)mouseExited:(NSEvent *)event {
	[super mouseExited:event];

	mouse_in_group = false;
	[self displayButtons];
}

- (void)displayButtons {
	for (NSView *subview in self.subviews) {
		[subview setNeedsDisplay:YES];
	}
}

@end

// === Merged from platform/macos/godot_content_view.mm ===
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

#import "servers/display/display_server.h"


#include "core/os/os.h"
#include "core/profiling/profiling.h"
#include "servers/display/display_server.h"

@implementation GodotContentLayerDelegate

- (id)init {
	self = [super init];
	window_id = DisplayServerEnums::INVALID_WINDOW_ID;
	need_redraw = false;
	return self;
}

- (void)setWindowID:(DisplayServerEnums::WindowID)wid {
	window_id = wid;
}

- (void)setNeedRedraw:(bool)redraw {
	need_redraw = redraw;
}

- (void)displayLayer:(CALayer *)layer {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (MainLoop::is_active() && ds->get_is_resizing() && need_redraw) {
		GodotProfileFrameMark;
		GodotProfileZone("[GodotContentLayerDelegate displayLayer]");

		Main::force_redraw();
		if (!Main::is_iterating()) { // Avoid cyclic loop.
			Main::iteration();
		}
		need_redraw = false;
	}
}

@end

@implementation GodotContentView

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
	return YES;
}

- (void)setFrameSize:(NSSize)newSize {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds && ds->has_window(window_id)) {
		DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
		NSRect frameRect = [wd.window_object frame];
		if (wd.fs_transition || wd.initial_size) {
			self.layerContentsPlacement = NSViewLayerContentsPlacementScaleAxesIndependently;
			wd.initial_size = false;
		} else {
			bool left = (wd.last_frame_rect.origin.x != frameRect.origin.x);
			bool bottom = (wd.last_frame_rect.origin.y != frameRect.origin.y);
			bool right = (wd.last_frame_rect.origin.x + wd.last_frame_rect.size.width != frameRect.origin.x + frameRect.size.width);
			bool top = (wd.last_frame_rect.origin.y + wd.last_frame_rect.size.height != frameRect.origin.y + frameRect.size.height);

			if (left && top) {
				self.layerContentsPlacement = NSViewLayerContentsPlacementBottomRight;
			} else if (left && bottom) {
				self.layerContentsPlacement = NSViewLayerContentsPlacementTopRight;
			} else if (left) {
				self.layerContentsPlacement = NSViewLayerContentsPlacementRight;
			} else if (right && top) {
				self.layerContentsPlacement = NSViewLayerContentsPlacementBottomLeft;
			} else if (right && bottom) {
				self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
			} else if (right) {
				self.layerContentsPlacement = NSViewLayerContentsPlacementLeft;
			}
		}
		wd.last_frame_rect = frameRect;
	}

	[super setFrameSize:newSize];
	[layer_delegate setNeedRedraw:true];
	[self.layer setNeedsDisplay]; // Force "drawRect" call.
}

- (void)updateLayerDelegate {
	self.layer.delegate = layer_delegate;
	self.layer.autoresizingMask = kCALayerHeightSizable | kCALayerWidthSizable;
	self.layer.needsDisplayOnBoundsChange = YES;
}

- (void)addObserver:(NSObject *)targetObserver forKeyPath:(NSString *)keyPath options:(NSKeyValueObservingOptions)options context:(void *)context {
	[registered_observers addObject:[[targetObserver description] stringByAppendingString:keyPath]];
	[super addObserver:targetObserver forKeyPath:keyPath options:options context:context];
}

- (void)removeObserver:(NSObject *)targetObserver forKeyPath:(NSString *)keyPath {
	if ([registered_observers containsObject:[[targetObserver description] stringByAppendingString:keyPath]]) {
		@try {
			[super removeObserver:targetObserver forKeyPath:keyPath];
			[registered_observers removeObject:[[targetObserver description] stringByAppendingString:keyPath]];
		} @catch (NSException *exception) {
			ERR_PRINT("NSException: " + String::utf8([exception reason].UTF8String));
		}
	}
}

- (id)init {
	self = [super init];
	layer_delegate = [[GodotContentLayerDelegate alloc] init];
	window_id = DisplayServerEnums::INVALID_WINDOW_ID;
	tracking_area = nil;
	ime_input_event_in_progress = false;
	mouse_down_control = false;
	ignore_momentum_scroll = false;
	last_pen_inverted = false;
	registered_observers = [[NSMutableSet alloc] init];
	[self updateTrackingAreas];

	self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
	self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;

	[self registerForDraggedTypes:[NSArray arrayWithObject:NSPasteboardTypeFileURL]];
	marked_text = [[NSMutableAttributedString alloc] init];
	return self;
}

- (void)setWindowID:(DisplayServerEnums::WindowID)wid {
	window_id = wid;
	[layer_delegate setWindowID:window_id];
}

// MARK: Backing Layer

- (CALayer *)makeBackingLayer {
	CAMetalLayer *layer = [CAMetalLayer new];
	layer.edgeAntialiasingMask = 0;
	layer.masksToBounds = NO;
	layer.presentsWithTransaction = NO;
	[layer removeAllAnimations];
	return layer;
}

- (BOOL)wantsUpdateLayer {
	return YES;
}

- (BOOL)isOpaque {
	return YES;
}

// MARK: IME

- (BOOL)hasMarkedText {
	return (marked_text.length > 0);
}

- (NSRange)markedRange {
	return NSMakeRange(0, marked_text.length);
}

- (NSRange)selectedRange {
	static const NSRange kEmptyRange = { NSNotFound, 0 };
	return kEmptyRange;
}

- (void)setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
	if ([aString isKindOfClass:[NSAttributedString class]]) {
		marked_text = [[NSMutableAttributedString alloc] initWithAttributedString:aString];
	} else {
		marked_text = [[NSMutableAttributedString alloc] initWithString:aString];
	}
	if (marked_text.length == 0) {
		[self unmarkText];
		return;
	}

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	if (wd.im_active) {
		ime_input_event_in_progress = true;
		ds->pop_last_key_event();
		ds->update_im_text(Point2i(selectedRange.location, selectedRange.length), String::utf8([[marked_text mutableString] UTF8String]));
	}
}

- (void)doCommandBySelector:(SEL)aSelector {
	[self tryToPerform:aSelector with:self];
}

- (void)unmarkText {
	if (ime_input_event_in_progress) {
		ime_suppress_next_keyup = true;
	}
	ime_input_event_in_progress = false;
	[[marked_text mutableString] setString:@""];

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	if (wd.im_active) {
		ds->update_im_text(Point2i(), String());
	}
}

- (NSArray *)validAttributesForMarkedText {
	return [NSArray array];
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange {
	return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint {
	return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return NSMakeRect(0, 0, 0, 0);
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	if (!wd.im_active) {
		return NSMakeRect(0, 0, 0, 0);
	}

	const NSRect content_rect = [wd.window_view frame];
	const float scale = ds->screen_get_max_scale();
	NSRect point_in_window_rect = NSMakeRect(wd.im_position.x / scale, content_rect.size.height - (wd.im_position.y / scale) - 1, 0, 0);
	NSPoint point_on_screen = [wd.window_object convertRectToScreen:point_in_window_rect].origin;

	return NSMakeRect(point_on_screen.x, point_on_screen.y, 0, 0);
}

- (void)cancelComposition {
	[self unmarkText];
	[[NSTextInputContext currentInputContext] discardMarkedText];
}

- (void)insertText:(id)aString {
	[self insertText:aString replacementRange:NSMakeRange(0, 0)];
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange {
	NSString *characters;
	if ([aString isKindOfClass:[NSAttributedString class]]) {
		characters = [aString string];
	} else {
		characters = (NSString *)aString;
	}

	NSCharacterSet *ctrl_chars = [NSCharacterSet controlCharacterSet];
	NSCharacterSet *wsnl_chars = [NSCharacterSet whitespaceAndNewlineCharacterSet];
	if ([characters rangeOfCharacterFromSet:ctrl_chars].length && [characters rangeOfCharacterFromSet:wsnl_chars].length == 0) {
		[[NSTextInputContext currentInputContext] discardMarkedText];
		[self cancelComposition];
		return;
	}

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		[self cancelComposition];
		return;
	}

	Char16String text;
	text.resize_uninitialized([characters length] + 1);
	[characters getCharacters:(unichar *)text.ptrw() range:NSMakeRange(0, [characters length])];

	String u32text = String::utf16(text.ptr(), text.length());

	for (int i = 0; i < u32text.length(); i++) {
		const char32_t codepoint = u32text[i];
		if ((codepoint & 0xFF00) == 0xF700) {
			continue;
		}

		DisplayServerMacOS::KeyEvent ke;

		ke.window_id = window_id;
		ke.macos_state = 0;
		ke.pressed = true;
		ke.echo = false;
		ke.raw = false; // IME input event.
		ke.keycode = Key::NONE;
		ke.physical_keycode = Key::NONE;
		ke.key_label = Key::NONE;
		ke.unicode = fix_unicode(codepoint);

		ds->push_to_key_event_buffer(ke);
	}
	[self cancelComposition];
}

// MARK: Drag and drop

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
	return NSDragOperationCopy;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
	return NSDragOperationCopy;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return NO;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	if (wd.drop_files_callback.is_valid()) {
		Vector<String> files;
		NSPasteboard *pboard = [sender draggingPasteboard];

		NSArray *items = pboard.pasteboardItems;
		for (NSPasteboardItem *item in items) {
			NSString *url = [item stringForType:NSPasteboardTypeFileURL];
			NSString *file = [NSURL URLWithString:url].path;
			files.push_back(String::utf8([file UTF8String]));
		}
		Variant v_files = files;
		const Variant *v_args[1] = { &v_files };
		Variant ret;
		Callable::CallError ce;
		wd.drop_files_callback.callp((const Variant **)&v_args, 1, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_FAIL_V_MSG(NO, vformat("Failed to execute drop files callback: %s.", Variant::get_callable_error_text(wd.drop_files_callback, v_args, 1, ce)));
		}
		return YES;
	}

	return NO;
}

// MARK: Focus

- (BOOL)canBecomeKeyView {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return YES;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	return !wd.no_focus;
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

// MARK: Mouse

- (void)cursorUpdate:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds) {
		return;
	}

	ds->cursor_update_shape();
}

- (void)processMouseEvent:(NSEvent *)event index:(MouseButton)index pressed:(bool)pressed outofstream:(bool)outofstream {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	Ref<InputEventMouseButton> mb;
	mb.instantiate();
	mb->set_window_id(window_id);
	if (outofstream) {
		ds->update_mouse_pos(wd, [wd.window_object mouseLocationOutsideOfEventStream]);
	} else {
		ds->update_mouse_pos(wd, [event locationInWindow]);
	}
	ds->get_key_modifier_state([event modifierFlags], mb);
	mb->set_button_index(index);
	mb->set_pressed(pressed);
	mb->set_position(wd.mouse_pos);
	mb->set_global_position(wd.mouse_pos);
	mb->set_button_mask(ds->mouse_get_button_state());
	if (!outofstream && index == MouseButton::LEFT && pressed) {
		mb->set_double_click([event clickCount] == 2);
	}

	Input::parse_input_event(mb);
}

- (void)mouseDown:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds && ds->has_window(window_id)) {
		DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
		wd.edge = DisplayServerEnums::WINDOW_EDGE_MAX;
	}
	if (([event modifierFlags] & NSEventModifierFlagControl)) {
		mouse_down_control = true;
		[self processMouseEvent:event index:MouseButton::RIGHT pressed:true outofstream:false];
	} else {
		mouse_down_control = false;
		[self processMouseEvent:event index:MouseButton::LEFT pressed:true outofstream:false];
	}
}

- (void)mouseDragged:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds && ds->has_window(window_id)) {
		DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
		if (wd.edge != DisplayServerEnums::WINDOW_EDGE_MAX) {
			Size2i max_size = wd.max_size / ds->screen_get_max_scale();
			Size2i min_size = wd.min_size / ds->screen_get_max_scale();
			NSRect frame = [wd.window_object frame];
			switch (wd.edge) {
				case DisplayServerEnums::WINDOW_EDGE_TOP_LEFT: {
					int clamped_dx = CLAMP(frame.size.width - event.deltaX, min_size.x, max_size.x) - frame.size.width;
					int clamped_dy = CLAMP(frame.size.height - event.deltaY, min_size.y, max_size.y) - frame.size.height;
					[wd.window_object setFrame:NSMakeRect(frame.origin.x - clamped_dx, frame.origin.y, frame.size.width + clamped_dx, frame.size.height + clamped_dy) display:YES];
				} break;
				case DisplayServerEnums::WINDOW_EDGE_TOP: {
					int clamped_dy = CLAMP(frame.size.height - event.deltaY, min_size.y, max_size.y) - frame.size.height;
					[wd.window_object setFrame:NSMakeRect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height + clamped_dy) display:YES];
				} break;
				case DisplayServerEnums::WINDOW_EDGE_TOP_RIGHT: {
					int clamped_dx = CLAMP(frame.size.width + event.deltaX, min_size.x, max_size.x) - frame.size.width;
					int clamped_dy = CLAMP(frame.size.height - event.deltaY, min_size.y, max_size.y) - frame.size.height;
					[wd.window_object setFrame:NSMakeRect(frame.origin.x, frame.origin.y, frame.size.width + clamped_dx, frame.size.height + clamped_dy) display:YES];
				} break;
				case DisplayServerEnums::WINDOW_EDGE_LEFT: {
					int clamped_dx = CLAMP(frame.size.width - event.deltaX, min_size.x, max_size.x) - frame.size.width;
					[wd.window_object setFrame:NSMakeRect(frame.origin.x - clamped_dx, frame.origin.y, frame.size.width + clamped_dx, frame.size.height) display:YES];
				} break;
				case DisplayServerEnums::WINDOW_EDGE_RIGHT: {
					int clamped_dx = CLAMP(frame.size.width + event.deltaX, min_size.x, max_size.x) - frame.size.width;
					[wd.window_object setFrame:NSMakeRect(frame.origin.x, frame.origin.y, frame.size.width + clamped_dx, frame.size.height) display:YES];
				} break;
				case DisplayServerEnums::WINDOW_EDGE_BOTTOM_LEFT: {
					int clamped_dx = CLAMP(frame.size.width - event.deltaX, min_size.x, max_size.x) - frame.size.width;
					int clamped_dy = CLAMP(frame.size.height + event.deltaY, min_size.y, max_size.y) - frame.size.height;
					[wd.window_object setFrame:NSMakeRect(frame.origin.x - clamped_dx, frame.origin.y - clamped_dy, frame.size.width + clamped_dx, frame.size.height + clamped_dy) display:YES];
				} break;
				case DisplayServerEnums::WINDOW_EDGE_BOTTOM: {
					int clamped_dy = CLAMP(frame.size.height + event.deltaY, min_size.y, max_size.y) - frame.size.height;
					[wd.window_object setFrame:NSMakeRect(frame.origin.x, frame.origin.y - clamped_dy, frame.size.width, frame.size.height + clamped_dy) display:YES];
				} break;
				case DisplayServerEnums::WINDOW_EDGE_BOTTOM_RIGHT: {
					int clamped_dx = CLAMP(frame.size.width + event.deltaX, min_size.x, max_size.x) - frame.size.width;
					int clamped_dy = CLAMP(frame.size.height + event.deltaY, min_size.y, max_size.y) - frame.size.height;
					[wd.window_object setFrame:NSMakeRect(frame.origin.x, frame.origin.y - clamped_dy, frame.size.width + clamped_dx, frame.size.height + clamped_dy) display:YES];
				} break;
				default:
					break;
			}
			return;
		}
	}
	[self mouseMoved:event];
}

- (void)mouseUp:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds && ds->has_window(window_id)) {
		DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
		wd.edge = DisplayServerEnums::WINDOW_EDGE_MAX;
	}
	if (mouse_down_control) {
		[self processMouseEvent:event index:MouseButton::RIGHT pressed:false outofstream:false];
	} else {
		[self processMouseEvent:event index:MouseButton::LEFT pressed:false outofstream:false];
	}
}

- (void)mouseMoved:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	NSPoint delta = NSMakePoint([event deltaX], [event deltaY]);
	NSPoint mpos = [event locationInWindow];

	if (ds->update_mouse_wrap(wd, delta, mpos, [event timestamp])) {
		return;
	}

	Ref<InputEventMouseMotion> mm;
	mm.instantiate();

	mm->set_window_id(window_id);
	mm->set_button_mask(ds->mouse_get_button_state());
	ds->update_mouse_pos(wd, mpos);
	mm->set_position(wd.mouse_pos);
	mm->set_pressure([event pressure]);
	NSEventSubtype subtype = [event subtype];
	if (subtype == NSEventSubtypeTabletPoint) {
		const NSPoint p = [event tilt];
		mm->set_tilt(Vector2(p.x, -p.y));
		mm->set_pen_inverted(last_pen_inverted);
	} else if (subtype == NSEventSubtypeTabletProximity) {
		// Check if using the eraser end of pen only on proximity event.
		last_pen_inverted = [event pointingDeviceType] == NSPointingDeviceTypeEraser;
		mm->set_pen_inverted(last_pen_inverted);
	}
	mm->set_global_position(wd.mouse_pos);
	mm->set_velocity(Input::get_last_mouse_velocity());
	mm->set_screen_velocity(mm->get_velocity());
	const Vector2i relativeMotion = Vector2i(delta.x, delta.y) * ds->screen_get_max_scale();
	mm->set_relative(relativeMotion);
	mm->set_relative_screen_position(relativeMotion);
	ds->get_key_modifier_state([event modifierFlags], mm);

	const NSRect contentRect = [wd.window_view frame];
	if (NSPointInRect([event locationInWindow], contentRect) && [NSWindow windowNumberAtPoint:[NSEvent mouseLocation] belowWindowWithWindowNumber:0 /*topmost*/] == [wd.window_object windowNumber]) {
		ds->mouse_enter_window(window_id);
	}

	Input::parse_input_event(mm);
}

- (void)rightMouseDown:(NSEvent *)event {
	[self processMouseEvent:event index:MouseButton::RIGHT pressed:true outofstream:false];
}

- (void)rightMouseDragged:(NSEvent *)event {
	[self mouseMoved:event];
}

- (void)rightMouseUp:(NSEvent *)event {
	[self processMouseEvent:event index:MouseButton::RIGHT pressed:false outofstream:false];
}

- (void)otherMouseDown:(NSEvent *)event {
	if ((int)[event buttonNumber] == 2) {
		[self processMouseEvent:event index:MouseButton::MIDDLE pressed:true outofstream:false];
	} else if ((int)[event buttonNumber] == 3) {
		[self processMouseEvent:event index:MouseButton::MB_XBUTTON1 pressed:true outofstream:false];
	} else if ((int)[event buttonNumber] == 4) {
		[self processMouseEvent:event index:MouseButton::MB_XBUTTON2 pressed:true outofstream:false];
	} else {
		return;
	}
}

- (void)otherMouseDragged:(NSEvent *)event {
	[self mouseMoved:event];
}

- (void)otherMouseUp:(NSEvent *)event {
	if ((int)[event buttonNumber] == 2) {
		[self processMouseEvent:event index:MouseButton::MIDDLE pressed:false outofstream:false];
	} else if ((int)[event buttonNumber] == 3) {
		[self processMouseEvent:event index:MouseButton::MB_XBUTTON1 pressed:false outofstream:false];
	} else if ((int)[event buttonNumber] == 4) {
		[self processMouseEvent:event index:MouseButton::MB_XBUTTON2 pressed:false outofstream:false];
	} else {
		return;
	}
}

- (void)swipeWithEvent:(NSEvent *)event {
	// Swipe gesture on Trackpad/Magic Mouse, or physical back/forward mouse buttons.
	if ([event phase] == NSEventPhaseEnded || [event phase] == NSEventPhaseChanged) {
		if (Math::is_equal_approx([event deltaX], 1.0)) {
			// Swipe left (back).
			[self processMouseEvent:event index:MouseButton::MB_XBUTTON1 pressed:true outofstream:true];
			[self processMouseEvent:event index:MouseButton::MB_XBUTTON1 pressed:false outofstream:true];
		} else if (Math::is_equal_approx([event deltaX], -1.0)) {
			// Swipe right (forward).
			[self processMouseEvent:event index:MouseButton::MB_XBUTTON2 pressed:true outofstream:true];
			[self processMouseEvent:event index:MouseButton::MB_XBUTTON2 pressed:false outofstream:true];
		}
	}
}

- (void)mouseExited:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	if (ds->mouse_get_mode() != DisplayServerEnums::MOUSE_MODE_CAPTURED) {
		ds->mouse_exit_window(window_id);
	}
}

- (void)mouseEntered:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	if (ds->mouse_get_mode() != DisplayServerEnums::MOUSE_MODE_CAPTURED) {
		ds->mouse_enter_window(window_id);
	}

	ds->cursor_update_shape();
}

- (void)magnifyWithEvent:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	Ref<InputEventMagnifyGesture> ev;
	ev.instantiate();
	ev->set_window_id(window_id);
	ds->get_key_modifier_state([event modifierFlags], ev);
	ds->update_mouse_pos(wd, [event locationInWindow]);
	ev->set_position(wd.mouse_pos);
	ev->set_factor([event magnification] + 1.0);

	Input::parse_input_event(ev);
}

- (void)updateTrackingAreas {
	if (tracking_area != nil) {
		[self removeTrackingArea:tracking_area];
	}

	NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingActiveWhenFirstResponder | NSTrackingCursorUpdate | NSTrackingInVisibleRect;
	tracking_area = [[NSTrackingArea alloc] initWithRect:[self bounds] options:options owner:self userInfo:nil];

	[self addTrackingArea:tracking_area];
	[super updateTrackingAreas];
}

// MARK: Keyboard

- (void)keyDown:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	ignore_momentum_scroll = true;

	// Ignore all input if IME input is in progress.
	if (!ime_input_event_in_progress) {
		NSString *characters = [event characters];
		NSUInteger length = [characters length];

		if (!wd.im_active && length > 0 && keycode_has_unicode(KeyMappingMacOS::remap_key([event keyCode], [event modifierFlags], true))) {
			// Fallback unicode character handler used if IME is not active.
			Char16String text;
			text.resize_uninitialized([characters length] + 1);
			[characters getCharacters:(unichar *)text.ptrw() range:NSMakeRange(0, [characters length])];

			String u32text = String::utf16(text.ptr(), text.length());

			DisplayServerMacOS::KeyEvent ke;
			ke.window_id = window_id;
			ke.macos_state = [event modifierFlags];
			ke.pressed = true;
			ke.echo = [event isARepeat];
			ke.keycode = KeyMappingMacOS::remap_key([event keyCode], [event modifierFlags], false);
			ke.physical_keycode = KeyMappingMacOS::translate_key([event keyCode]);
			ke.key_label = KeyMappingMacOS::remap_key([event keyCode], [event modifierFlags], true);
			ke.raw = true;

			if (u32text.is_empty()) {
				ke.unicode = 0;
				ds->push_to_key_event_buffer(ke);
			}
			for (int i = 0; i < u32text.length(); i++) {
				const char32_t codepoint = u32text[i];
				ke.unicode = fix_unicode(codepoint);
				ds->push_to_key_event_buffer(ke);
			}
		} else {
			DisplayServerMacOS::KeyEvent ke;

			ke.window_id = window_id;
			ke.macos_state = [event modifierFlags];
			ke.pressed = true;
			ke.echo = [event isARepeat];
			ke.keycode = KeyMappingMacOS::remap_key([event keyCode], [event modifierFlags], false);
			ke.physical_keycode = KeyMappingMacOS::translate_key([event keyCode]);
			ke.key_label = KeyMappingMacOS::remap_key([event keyCode], [event modifierFlags], true);
			ke.unicode = 0;
			ke.location = KeyMappingMacOS::translate_location([event keyCode]);
			ke.raw = false;

			ds->push_to_key_event_buffer(ke);
		}
	}

	// Pass events to IME handler
	if (wd.im_active) {
		[self interpretKeyEvents:[NSArray arrayWithObject:event]];
	}
}

- (void)flagsChanged:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}
	ignore_momentum_scroll = true;

	DisplayServerMacOS::KeyEvent ke;

	ke.window_id = window_id;
	ke.echo = false;
	ke.raw = true;

	int key = [event keyCode];
	int mod = [event modifierFlags];

	if (key == 0x36 || key == 0x37) {
		if (mod & NSEventModifierFlagCommand) {
			mod &= ~NSEventModifierFlagCommand;
			ke.pressed = true;
		} else {
			ke.pressed = false;
		}
	} else if (key == 0x38 || key == 0x3c) {
		if (mod & NSEventModifierFlagShift) {
			mod &= ~NSEventModifierFlagShift;
			ke.pressed = true;
		} else {
			ke.pressed = false;
		}
	} else if (key == 0x3a || key == 0x3d) {
		if (mod & NSEventModifierFlagOption) {
			mod &= ~NSEventModifierFlagOption;
			ke.pressed = true;
		} else {
			ke.pressed = false;
		}
	} else if (key == 0x3b || key == 0x3e) {
		if (mod & NSEventModifierFlagControl) {
			mod &= ~NSEventModifierFlagControl;
			ke.pressed = true;
		} else {
			ke.pressed = false;
		}
	} else {
		return;
	}

	ke.macos_state = mod;
	ke.keycode = KeyMappingMacOS::remap_key(key, mod, false);
	ke.physical_keycode = KeyMappingMacOS::translate_key(key);
	ke.key_label = KeyMappingMacOS::remap_key(key, mod, true);
	ke.unicode = 0;
	ke.location = KeyMappingMacOS::translate_location(key);

	ds->push_to_key_event_buffer(ke);
}

- (void)keyUp:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	// Ignore all input if IME input is in progress.
	if (ime_suppress_next_keyup) {
		ime_suppress_next_keyup = false;
		return;
	}

	if (!ime_input_event_in_progress) {
		DisplayServerMacOS::KeyEvent ke;

		ke.window_id = window_id;
		ke.macos_state = [event modifierFlags];
		ke.pressed = false;
		ke.echo = [event isARepeat];
		ke.keycode = KeyMappingMacOS::remap_key([event keyCode], [event modifierFlags], false);
		ke.physical_keycode = KeyMappingMacOS::translate_key([event keyCode]);
		ke.key_label = KeyMappingMacOS::remap_key([event keyCode], [event modifierFlags], true);
		ke.unicode = 0;
		ke.location = KeyMappingMacOS::translate_location([event keyCode]);
		ke.raw = true;

		ds->push_to_key_event_buffer(ke);
	}
}

// MARK: Scroll and pan

- (void)processScrollEvent:(NSEvent *)event button:(MouseButton)button factor:(double)factor {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	MouseButtonMask mask = mouse_button_to_mask(button);

	Ref<InputEventMouseButton> sc;
	sc.instantiate();

	sc->set_window_id(window_id);
	ds->get_key_modifier_state([event modifierFlags], sc);
	sc->set_button_index(button);
	sc->set_factor(factor);
	sc->set_pressed(true);
	sc->set_position(wd.mouse_pos);
	sc->set_global_position(wd.mouse_pos);
	BitField<MouseButtonMask> scroll_mask = ds->mouse_get_button_state();
	scroll_mask.set_flag(mask);
	sc->set_button_mask(scroll_mask);

	Input::parse_input_event(sc);

	sc.instantiate();
	sc->set_window_id(window_id);
	sc->set_button_index(button);
	sc->set_factor(factor);
	sc->set_pressed(false);
	sc->set_position(wd.mouse_pos);
	sc->set_global_position(wd.mouse_pos);
	scroll_mask.clear_flag(mask);
	sc->set_button_mask(scroll_mask);

	Input::parse_input_event(sc);
}

- (void)processPanEvent:(NSEvent *)event dx:(double)dx dy:(double)dy {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	Ref<InputEventPanGesture> pg;
	pg.instantiate();

	pg->set_window_id(window_id);
	ds->get_key_modifier_state([event modifierFlags], pg);
	pg->set_position(wd.mouse_pos);
	pg->set_delta(Vector2(-dx, -dy));

	Input::parse_input_event(pg);
}

- (void)scrollWheel:(NSEvent *)event {
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	ds->update_mouse_pos(wd, [event locationInWindow]);

	double delta_x = [event scrollingDeltaX];
	double delta_y = [event scrollingDeltaY];

	if ([event hasPreciseScrollingDeltas]) {
		delta_x *= 0.03;
		delta_y *= 0.03;
	}

	if ([event momentumPhase] != NSEventPhaseNone) {
		if (ignore_momentum_scroll) {
			return;
		}
	} else {
		ignore_momentum_scroll = false;
	}

	if ([event phase] != NSEventPhaseNone || [event momentumPhase] != NSEventPhaseNone) {
		[self processPanEvent:event dx:delta_x dy:delta_y];
	} else {
		if (std::abs(delta_x)) {
			[self processScrollEvent:event button:(0 > delta_x ? MouseButton::WHEEL_RIGHT : MouseButton::WHEEL_LEFT) factor:std::abs(delta_x * 0.3)];
		}
		if (std::abs(delta_y)) {
			[self processScrollEvent:event button:(0 < delta_y ? MouseButton::WHEEL_UP : MouseButton::WHEEL_DOWN) factor:std::abs(delta_y * 0.3)];
		}
	}
}

@end

// === Merged from platform/macos/godot_core_cursor.mm ===
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

#import "servers/display/display_server.h"

@implementation GodotCoreCursor
@synthesize _coreCursorType = _type;

- (id)initWithType:(int32_t)type {
	if ((self = [super init])) {
		_type = type;
	}
	return self;
}

@end

// === Merged from platform/macos/godot_status_item.mm ===
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

#import "servers/display/display_server.h"


@implementation GodotStatusItemDelegate

- (id)init {
	self = [super init];
	return self;
}

- (IBAction)click:(id)sender {
	NSEvent *current_event = [NSApp currentEvent];
	MouseButton index = MouseButton::LEFT;
	if (current_event) {
		if (current_event.type == NSEventTypeLeftMouseDown) {
			index = MouseButton::LEFT;
		} else if (current_event.type == NSEventTypeRightMouseDown) {
			index = MouseButton::RIGHT;
		} else if (current_event.type == NSEventTypeOtherMouseDown) {
			if ((int)[current_event buttonNumber] == 2) {
				index = MouseButton::MIDDLE;
			} else if ((int)[current_event buttonNumber] == 3) {
				index = MouseButton::MB_XBUTTON1;
			} else if ((int)[current_event buttonNumber] == 4) {
				index = MouseButton::MB_XBUTTON2;
			}
		}
	}

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds) {
		return;
	}

	if (cb.is_valid()) {
		Variant v_button = index;
		Variant v_pos = ds->mouse_get_position();
		const Variant *v_args[2] = { &v_button, &v_pos };
		Variant ret;
		Callable::CallError ce;
		cb.callp((const Variant **)&v_args, 2, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Failed to execute status indicator callback: %s.", Variant::get_callable_error_text(cb, v_args, 2, ce)));
		}
	}
}

- (void)setCallback:(const Callable &)callback {
	cb = callback;
}

@end

// === Merged from platform/macos/godot_window_delegate.mm ===
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

#import "servers/display/display_server.h"


#include "servers/display/display_server.h"

@implementation GodotWindowDelegate {
	DisplayServerEnums::WindowID window_id;
	DisplayServerMacOS *ds;
}

- (instancetype)initWithDisplayServer:(DisplayServerMacOS *)p_ds {
	if (self = [super init]) {
		ds = p_ds;
		window_id = DisplayServerEnums::INVALID_WINDOW_ID;
	}
	return self;
}

- (void)setWindowID:(DisplayServerEnums::WindowID)wid {
	window_id = wid;
}

- (BOOL)windowShouldClose:(id)sender {
	if (!ds->has_window(window_id)) {
		return YES;
	}

	ds->send_window_event(ds->get_window(window_id), DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
	return NO;
}

- (void)windowWillClose:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	ds->popup_close(window_id);

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	while (wd.transient_children.size()) {
		ds->window_set_transient(*wd.transient_children.begin(), DisplayServerEnums::INVALID_WINDOW_ID);
	}

	if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
		ds->window_set_transient(window_id, DisplayServerEnums::INVALID_WINDOW_ID);
	}

	ds->mouse_exit_window(window_id);
	ds->window_destroy(window_id);
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	wd.fs_transition = true;

	// Temporary disable borderless and transparent state.
	if ([wd.window_object styleMask] == NSWindowStyleMaskBorderless) {
		[wd.window_object setStyleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable];
	}
	if (wd.layered_window) {
		ds->set_window_per_pixel_transparency_enabled(false, window_id);
	}
}

- (void)windowDidFailToEnterFullScreen:(NSWindow *)window {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	wd.fs_transition = false;
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	wd.fullscreen = true;
	wd.fs_transition = false;

	// Reset window size limits.
	[wd.window_object setContentMinSize:NSMakeSize(0, 0)];
	[wd.window_object setContentMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];

	// Reset custom window buttons.
	if ([wd.window_object styleMask] & NSWindowStyleMaskFullSizeContentView) {
		ds->window_set_custom_window_buttons(wd, false);
	}

	ds->send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_TITLEBAR_CHANGE);

	// Force window resize event and redraw.
	[self windowDidResize:notification];
}

- (void)windowWillExitFullScreen:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	wd.fs_transition = true;

	// Restore custom window buttons.
	if ([wd.window_object styleMask] & NSWindowStyleMaskFullSizeContentView) {
		ds->window_set_custom_window_buttons(wd, true);
	}

	ds->send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_TITLEBAR_CHANGE);
}

- (void)windowDidFailToExitFullScreen:(NSWindow *)window {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	wd.fs_transition = false;

	if ([wd.window_object styleMask] & NSWindowStyleMaskFullSizeContentView) {
		ds->window_set_custom_window_buttons(wd, false);
	}

	ds->send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_TITLEBAR_CHANGE);
}

- (void)windowDidExitFullScreen:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	if (wd.exclusive_fullscreen) {
		ds->update_presentation_mode();
	}

	wd.fullscreen = false;
	wd.exclusive_fullscreen = false;
	wd.fs_transition = false;

	// Set window size limits.
	const float scale = ds->screen_get_max_scale();
	if (wd.min_size != Size2i()) {
		Size2i size = wd.min_size / scale;
		[wd.window_object setContentMinSize:NSMakeSize(size.x, size.y)];
	}
	if (wd.max_size != Size2i()) {
		Size2i size = wd.max_size / scale;
		[wd.window_object setContentMaxSize:NSMakeSize(size.x, size.y)];
	}

	// Restore borderless, transparent and resizability state.
	if (wd.borderless) {
		[wd.window_object setStyleMask:NSWindowStyleMaskBorderless];
		[wd.window_object setHasShadow:NO];
	} else {
		[wd.window_object setStyleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | (wd.extend_to_title ? NSWindowStyleMaskFullSizeContentView : 0) | (wd.resize_disabled ? 0 : NSWindowStyleMaskResizable)];
		[wd.window_object setHasShadow:YES];
	}
	if (wd.layered_window) {
		ds->set_window_per_pixel_transparency_enabled(true, window_id);
	}

	// Restore on-top state.
	if (ds->is_always_on_top_recursive(window_id)) {
		[wd.window_object setLevel:NSFloatingWindowLevel];
	}

	// Force window resize event and redraw.
	[self windowDidResize:notification];
}

- (BOOL)windowShouldZoom:(NSWindow *)window toFrame:(NSRect)newFrame {
	if (ds->has_window(window_id)) {
		DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
		if (!wd.borderless && !(NSEqualRects([wd.window_object frame], [[wd.window_object screen] visibleFrame]))) {
			wd.pre_zoom_rect = [wd.window_object frame];
		}
	}
	return YES;
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	CGFloat new_scale_factor = [wd.window_object backingScaleFactor];
	CGFloat old_scale_factor = [[[notification userInfo] objectForKey:@"NSBackingPropertyOldScaleFactorKey"] doubleValue];

	if (new_scale_factor != old_scale_factor) {
		// Set new display scale and window size.
		const float scale = ds->screen_get_max_scale();
		const NSRect content_rect = [wd.window_view frame];

		wd.size.width = content_rect.size.width * scale;
		wd.size.height = content_rect.size.height * scale;

		ds->send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_DPI_CHANGE);

		CALayer *layer = [wd.window_view layer];
		if (layer) {
			layer.contentsScale = scale;
		}

		//Force window resize event
		[self windowDidResize:notification];
	}
}

- (void)windowWillStartLiveResize:(NSNotification *)notification {
	if (ds->has_window(window_id)) {
		DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
		wd.last_frame_rect = [wd.window_object frame];
		ds->set_is_resizing(true);
	}
}

- (void)windowDidEndLiveResize:(NSNotification *)notification {
	ds->set_is_resizing(false);
}

- (void)windowDidResize:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	const NSRect content_rect = [wd.window_view frame];
	const float scale = ds->screen_get_max_scale();
	wd.size.width = content_rect.size.width * scale;
	wd.size.height = content_rect.size.height * scale;

	CALayer *layer = [wd.window_view layer];
	if (layer) {
		layer.contentsScale = scale;
	}

	ds->window_resize(window_id, wd.size.width, wd.size.height);

	if (wd.rect_changed_callback.is_valid()) {
		wd.rect_changed_callback.call(Rect2i(ds->window_get_position(window_id), ds->window_get_size(window_id)));
	}
}

- (void)windowDidChangeScreen:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	ds->reparent_check(window_id);
}

- (void)windowDidMove:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	ds->release_pressed_events();

	if (wd.rect_changed_callback.is_valid()) {
		wd.rect_changed_callback.call(Rect2i(ds->window_get_position(window_id), ds->window_get_size(window_id)));
	}
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	if (wd.window_button_view) {
		[wd.window_button_view displayButtons];
	}

	if (ds->mouse_get_mode() == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
		const NSRect content_rect = [wd.window_view frame];
		NSRect point_in_window_rect = NSMakeRect(content_rect.size.width / 2, content_rect.size.height / 2, 0, 0);
		NSPoint point_on_screen = [[wd.window_view window] convertRectToScreen:point_in_window_rect].origin;
		CGPoint mouse_warp_pos = { point_on_screen.x, CGDisplayBounds(CGMainDisplayID()).size.height - point_on_screen.y };
		CGWarpMouseCursorPosition(mouse_warp_pos);
	} else {
		ds->update_mouse_pos(wd, [wd.window_object mouseLocationOutsideOfEventStream]);
	}

	[self windowDidResize:notification]; // Emit resize event, to ensure content is resized if the window was resized while it was hidden.

	wd.focused = true;
	ds->set_last_focused_window(window_id);
	AccessibilityServer::set_window_focused(window_id, true);

	ds->send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_FOCUS_IN);
}

- (void)windowDidResignKey:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	if (wd.window_button_view) {
		[wd.window_button_view displayButtons];
	}

	wd.focused = false;
	ds->release_pressed_events();
	AccessibilityServer::set_window_focused(window_id, false);

	ds->send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT);
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);

	wd.focused = false;
	ds->release_pressed_events();
	AccessibilityServer::set_window_focused(window_id, false);

	ds->send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT);
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	wd.is_visible = ([wd.window_object occlusionState] & NSWindowOcclusionStateVisible) && [wd.window_object isVisible];
	if ([wd.window_object isKeyWindow]) {
		wd.focused = true;
		ds->set_last_focused_window(window_id);
		AccessibilityServer::set_window_focused(window_id, true);

		ds->send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_FOCUS_IN);
	}
}

- (void)windowDidChangeOcclusionState:(NSNotification *)notification {
	if (!ds->has_window(window_id)) {
		return;
	}
	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	wd.is_visible = ([wd.window_object occlusionState] & NSWindowOcclusionStateVisible) && [wd.window_object isVisible];
}

@end

// === Merged from platform/macos/godot_window.mm ===
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

#import "servers/display/display_server.h"


@implementation GodotWindow

- (id)init {
	self = [super init];
	window_id = DisplayServerEnums::INVALID_WINDOW_ID;
	anim_duration = -1.0f;
	return self;
}

- (void)setAnimDuration:(NSTimeInterval)duration {
	anim_duration = duration;
}

- (NSTimeInterval)animationResizeTime:(NSRect)newFrame {
	if (anim_duration > 0) {
		return anim_duration;
	} else {
		return [super animationResizeTime:newFrame];
	}
}

- (void)setWindowID:(DisplayServerEnums::WindowID)wid {
	window_id = wid;
}

- (BOOL)canBecomeKeyWindow {
	// Required for NSWindowStyleMaskBorderless windows.
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return YES;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	return !wd.no_focus;
}

- (BOOL)canBecomeMainWindow {
	// Required for NSWindowStyleMaskBorderless windows.
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (!ds || !ds->has_window(window_id)) {
		return YES;
	}

	DisplayServerMacOS::WindowData &wd = ds->get_window(window_id);
	return !wd.no_focus && !wd.is_popup;
}

@end

// === Merged from platform/macos/key_mapping_macos.mm ===
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

#import "servers/display/display_server.h"

#include "core/string/ustring.h"
#include "core/io/ip.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

HashSet<unsigned int> numpad_keys;
HashMap<unsigned int, Key> keysym_map;
HashMap<Key, unsigned int> keysym_map_inv;
HashMap<Key, char32_t> keycode_map;
HashMap<unsigned int, KeyLocation> location_map;

void KeyMappingMacOS::initialize() {
	numpad_keys.insert(0x41); //kVK_ANSI_KeypadDecimal
	numpad_keys.insert(0x43); //kVK_ANSI_KeypadMultiply
	numpad_keys.insert(0x45); //kVK_ANSI_KeypadPlus
	numpad_keys.insert(0x47); //kVK_ANSI_KeypadClear
	numpad_keys.insert(0x4b); //kVK_ANSI_KeypadDivide
	numpad_keys.insert(0x4c); //kVK_ANSI_KeypadEnter
	numpad_keys.insert(0x4e); //kVK_ANSI_KeypadMinus
	numpad_keys.insert(0x51); //kVK_ANSI_KeypadEquals
	numpad_keys.insert(0x52); //kVK_ANSI_Keypad0
	numpad_keys.insert(0x53); //kVK_ANSI_Keypad1
	numpad_keys.insert(0x54); //kVK_ANSI_Keypad2
	numpad_keys.insert(0x55); //kVK_ANSI_Keypad3
	numpad_keys.insert(0x56); //kVK_ANSI_Keypad4
	numpad_keys.insert(0x57); //kVK_ANSI_Keypad5
	numpad_keys.insert(0x58); //kVK_ANSI_Keypad6
	numpad_keys.insert(0x59); //kVK_ANSI_Keypad7
	numpad_keys.insert(0x5b); //kVK_ANSI_Keypad8
	numpad_keys.insert(0x5c); //kVK_ANSI_Keypad9
	numpad_keys.insert(0x5f); //kVK_JIS_KeypadComma

	keysym_map[0x00] = Key::A;
	keysym_map[0x01] = Key::S;
	keysym_map[0x02] = Key::D;
	keysym_map[0x03] = Key::F;
	keysym_map[0x04] = Key::H;
	keysym_map[0x05] = Key::G;
	keysym_map[0x06] = Key::Z;
	keysym_map[0x07] = Key::X;
	keysym_map[0x08] = Key::C;
	keysym_map[0x09] = Key::V;
	keysym_map[0x0a] = Key::SECTION;
	keysym_map[0x0b] = Key::B;
	keysym_map[0x0c] = Key::Q;
	keysym_map[0x0d] = Key::W;
	keysym_map[0x0e] = Key::E;
	keysym_map[0x0f] = Key::R;
	keysym_map[0x10] = Key::Y;
	keysym_map[0x11] = Key::T;
	keysym_map[0x12] = Key::KEY_1;
	keysym_map[0x13] = Key::KEY_2;
	keysym_map[0x14] = Key::KEY_3;
	keysym_map[0x15] = Key::KEY_4;
	keysym_map[0x16] = Key::KEY_6;
	keysym_map[0x17] = Key::KEY_5;
	keysym_map[0x18] = Key::EQUAL;
	keysym_map[0x19] = Key::KEY_9;
	keysym_map[0x1a] = Key::KEY_7;
	keysym_map[0x1b] = Key::MINUS;
	keysym_map[0x1c] = Key::KEY_8;
	keysym_map[0x1d] = Key::KEY_0;
	keysym_map[0x1e] = Key::BRACKETRIGHT;
	keysym_map[0x1f] = Key::O;
	keysym_map[0x20] = Key::U;
	keysym_map[0x21] = Key::BRACKETLEFT;
	keysym_map[0x22] = Key::I;
	keysym_map[0x23] = Key::P;
	keysym_map[0x24] = Key::ENTER;
	keysym_map[0x25] = Key::L;
	keysym_map[0x26] = Key::J;
	keysym_map[0x27] = Key::APOSTROPHE;
	keysym_map[0x28] = Key::K;
	keysym_map[0x29] = Key::SEMICOLON;
	keysym_map[0x2a] = Key::BACKSLASH;
	keysym_map[0x2b] = Key::COMMA;
	keysym_map[0x2c] = Key::SLASH;
	keysym_map[0x2d] = Key::N;
	keysym_map[0x2e] = Key::M;
	keysym_map[0x2f] = Key::PERIOD;
	keysym_map[0x30] = Key::TAB;
	keysym_map[0x31] = Key::SPACE;
	keysym_map[0x32] = Key::QUOTELEFT;
	keysym_map[0x33] = Key::BACKSPACE;
	keysym_map[0x35] = Key::ESCAPE;
	keysym_map[0x36] = Key::META;
	keysym_map[0x37] = Key::META;
	keysym_map[0x38] = Key::SHIFT;
	keysym_map[0x39] = Key::CAPSLOCK;
	keysym_map[0x3a] = Key::ALT;
	keysym_map[0x3b] = Key::CTRL;
	keysym_map[0x3c] = Key::SHIFT;
	keysym_map[0x3d] = Key::ALT;
	keysym_map[0x3e] = Key::CTRL;
	keysym_map[0x40] = Key::F17;
	keysym_map[0x41] = Key::KP_PERIOD;
	keysym_map[0x43] = Key::KP_MULTIPLY;
	keysym_map[0x45] = Key::KP_ADD;
	keysym_map[0x47] = Key::NUMLOCK;
	keysym_map[0x48] = Key::VOLUMEUP;
	keysym_map[0x49] = Key::VOLUMEDOWN;
	keysym_map[0x4a] = Key::VOLUMEMUTE;
	keysym_map[0x4b] = Key::KP_DIVIDE;
	keysym_map[0x4c] = Key::KP_ENTER;
	keysym_map[0x4e] = Key::KP_SUBTRACT;
	keysym_map[0x4f] = Key::F18;
	keysym_map[0x50] = Key::F19;
	keysym_map[0x51] = Key::EQUAL;
	keysym_map[0x52] = Key::KP_0;
	keysym_map[0x53] = Key::KP_1;
	keysym_map[0x54] = Key::KP_2;
	keysym_map[0x55] = Key::KP_3;
	keysym_map[0x56] = Key::KP_4;
	keysym_map[0x57] = Key::KP_5;
	keysym_map[0x58] = Key::KP_6;
	keysym_map[0x59] = Key::KP_7;
	keysym_map[0x5a] = Key::F20;
	keysym_map[0x5b] = Key::KP_8;
	keysym_map[0x5c] = Key::KP_9;
	keysym_map[0x5d] = Key::YEN;
	keysym_map[0x5e] = Key::UNDERSCORE;
	keysym_map[0x5f] = Key::COMMA;
	keysym_map[0x60] = Key::F5;
	keysym_map[0x61] = Key::F6;
	keysym_map[0x62] = Key::F7;
	keysym_map[0x63] = Key::F3;
	keysym_map[0x64] = Key::F8;
	keysym_map[0x65] = Key::F9;
	keysym_map[0x66] = Key::JIS_EISU;
	keysym_map[0x67] = Key::F11;
	keysym_map[0x68] = Key::JIS_KANA;
	keysym_map[0x69] = Key::F13;
	keysym_map[0x6a] = Key::F16;
	keysym_map[0x6b] = Key::F14;
	keysym_map[0x6d] = Key::F10;
	keysym_map[0x6e] = Key::MENU;
	keysym_map[0x6f] = Key::F12;
	keysym_map[0x71] = Key::F15;
	keysym_map[0x72] = Key::INSERT;
	keysym_map[0x73] = Key::HOME;
	keysym_map[0x74] = Key::PAGEUP;
	keysym_map[0x75] = Key::KEY_DELETE;
	keysym_map[0x76] = Key::F4;
	keysym_map[0x77] = Key::END;
	keysym_map[0x78] = Key::F2;
	keysym_map[0x79] = Key::PAGEDOWN;
	keysym_map[0x7a] = Key::F1;
	keysym_map[0x7b] = Key::LEFT;
	keysym_map[0x7c] = Key::RIGHT;
	keysym_map[0x7d] = Key::DOWN;
	keysym_map[0x7e] = Key::UP;

	for (const KeyValue<unsigned int, Key> &E : keysym_map) {
		keysym_map_inv[E.value] = E.key;
	}

	keycode_map[Key::ESCAPE] = 0x001B;
	keycode_map[Key::TAB] = 0x0009;
	keycode_map[Key::BACKTAB] = 0x007F;
	keycode_map[Key::BACKSPACE] = 0x0008;
	keycode_map[Key::ENTER] = 0x000D;
	keycode_map[Key::INSERT] = NSInsertFunctionKey;
	keycode_map[Key::KEY_DELETE] = 0x007F;
	keycode_map[Key::PAUSE] = NSPauseFunctionKey;
	keycode_map[Key::PRINT] = NSPrintScreenFunctionKey;
	keycode_map[Key::SYSREQ] = NSSysReqFunctionKey;
	keycode_map[Key::CLEAR] = NSClearLineFunctionKey;
	keycode_map[Key::HOME] = 0x2196;
	keycode_map[Key::END] = 0x2198;
	keycode_map[Key::LEFT] = 0x001C;
	keycode_map[Key::UP] = 0x001E;
	keycode_map[Key::RIGHT] = 0x001D;
	keycode_map[Key::DOWN] = 0x001F;
	keycode_map[Key::PAGEUP] = 0x21DE;
	keycode_map[Key::PAGEDOWN] = 0x21DF;
	keycode_map[Key::NUMLOCK] = NSClearLineFunctionKey;
	keycode_map[Key::SCROLLLOCK] = NSScrollLockFunctionKey;
	keycode_map[Key::F1] = NSF1FunctionKey;
	keycode_map[Key::F2] = NSF2FunctionKey;
	keycode_map[Key::F3] = NSF3FunctionKey;
	keycode_map[Key::F4] = NSF4FunctionKey;
	keycode_map[Key::F5] = NSF5FunctionKey;
	keycode_map[Key::F6] = NSF6FunctionKey;
	keycode_map[Key::F7] = NSF7FunctionKey;
	keycode_map[Key::F8] = NSF8FunctionKey;
	keycode_map[Key::F9] = NSF9FunctionKey;
	keycode_map[Key::F10] = NSF10FunctionKey;
	keycode_map[Key::F11] = NSF11FunctionKey;
	keycode_map[Key::F12] = NSF12FunctionKey;
	keycode_map[Key::F13] = NSF13FunctionKey;
	keycode_map[Key::F14] = NSF14FunctionKey;
	keycode_map[Key::F15] = NSF15FunctionKey;
	keycode_map[Key::F16] = NSF16FunctionKey;
	keycode_map[Key::F17] = NSF17FunctionKey;
	keycode_map[Key::F18] = NSF18FunctionKey;
	keycode_map[Key::F19] = NSF19FunctionKey;
	keycode_map[Key::F20] = NSF20FunctionKey;
	keycode_map[Key::F21] = NSF21FunctionKey;
	keycode_map[Key::F22] = NSF22FunctionKey;
	keycode_map[Key::F23] = NSF23FunctionKey;
	keycode_map[Key::F24] = NSF24FunctionKey;
	keycode_map[Key::F25] = NSF25FunctionKey;
	keycode_map[Key::F26] = NSF26FunctionKey;
	keycode_map[Key::F27] = NSF27FunctionKey;
	keycode_map[Key::F28] = NSF28FunctionKey;
	keycode_map[Key::F29] = NSF29FunctionKey;
	keycode_map[Key::F30] = NSF30FunctionKey;
	keycode_map[Key::F31] = NSF31FunctionKey;
	keycode_map[Key::F32] = NSF32FunctionKey;
	keycode_map[Key::F33] = NSF33FunctionKey;
	keycode_map[Key::F34] = NSF34FunctionKey;
	keycode_map[Key::F35] = NSF35FunctionKey;
	keycode_map[Key::MENU] = NSMenuFunctionKey;
	keycode_map[Key::HELP] = NSHelpFunctionKey;
	keycode_map[Key::STOP] = NSStopFunctionKey;
	keycode_map[Key::LAUNCH0] = NSUserFunctionKey;
	keycode_map[Key::SPACE] = 0x0020;
	keycode_map[Key::EXCLAM] = '!';
	keycode_map[Key::QUOTEDBL] = '\"';
	keycode_map[Key::NUMBERSIGN] = '#';
	keycode_map[Key::DOLLAR] = '$';
	keycode_map[Key::PERCENT] = '\%';
	keycode_map[Key::AMPERSAND] = '&';
	keycode_map[Key::APOSTROPHE] = '\'';
	keycode_map[Key::PARENLEFT] = '(';
	keycode_map[Key::PARENRIGHT] = ')';
	keycode_map[Key::ASTERISK] = '*';
	keycode_map[Key::PLUS] = '+';
	keycode_map[Key::COMMA] = ',';
	keycode_map[Key::MINUS] = '-';
	keycode_map[Key::PERIOD] = '.';
	keycode_map[Key::SLASH] = '/';
	keycode_map[Key::KEY_0] = '0';
	keycode_map[Key::KEY_1] = '1';
	keycode_map[Key::KEY_2] = '2';
	keycode_map[Key::KEY_3] = '3';
	keycode_map[Key::KEY_4] = '4';
	keycode_map[Key::KEY_5] = '5';
	keycode_map[Key::KEY_6] = '6';
	keycode_map[Key::KEY_7] = '7';
	keycode_map[Key::KEY_8] = '8';
	keycode_map[Key::KEY_9] = '9';
	keycode_map[Key::COLON] = ':';
	keycode_map[Key::SEMICOLON] = ';';
	keycode_map[Key::LESS] = '<';
	keycode_map[Key::EQUAL] = '=';
	keycode_map[Key::GREATER] = '>';
	keycode_map[Key::QUESTION] = '?';
	keycode_map[Key::AT] = '@';
	keycode_map[Key::A] = 'a';
	keycode_map[Key::B] = 'b';
	keycode_map[Key::C] = 'c';
	keycode_map[Key::D] = 'd';
	keycode_map[Key::E] = 'e';
	keycode_map[Key::F] = 'f';
	keycode_map[Key::G] = 'g';
	keycode_map[Key::H] = 'h';
	keycode_map[Key::I] = 'i';
	keycode_map[Key::J] = 'j';
	keycode_map[Key::K] = 'k';
	keycode_map[Key::L] = 'l';
	keycode_map[Key::M] = 'm';
	keycode_map[Key::N] = 'n';
	keycode_map[Key::O] = 'o';
	keycode_map[Key::P] = 'p';
	keycode_map[Key::Q] = 'q';
	keycode_map[Key::R] = 'r';
	keycode_map[Key::S] = 's';
	keycode_map[Key::T] = 't';
	keycode_map[Key::U] = 'u';
	keycode_map[Key::V] = 'v';
	keycode_map[Key::W] = 'w';
	keycode_map[Key::X] = 'x';
	keycode_map[Key::Y] = 'y';
	keycode_map[Key::Z] = 'z';
	keycode_map[Key::BRACKETLEFT] = '[';
	keycode_map[Key::BACKSLASH] = '\\';
	keycode_map[Key::BRACKETRIGHT] = ']';
	keycode_map[Key::ASCIICIRCUM] = '^';
	keycode_map[Key::UNDERSCORE] = '_';
	keycode_map[Key::QUOTELEFT] = '`';
	keycode_map[Key::BRACELEFT] = '{';
	keycode_map[Key::BAR] = '|';
	keycode_map[Key::BRACERIGHT] = '}';
	keycode_map[Key::ASCIITILDE] = '~';

	// Keysym -> physical location.
	// Ctrl.
	location_map[0x3b] = KeyLocation::LEFT;
	location_map[0x3e] = KeyLocation::RIGHT;
	// Shift.
	location_map[0x38] = KeyLocation::LEFT;
	location_map[0x3c] = KeyLocation::RIGHT;
	// Alt/Option.
	location_map[0x3a] = KeyLocation::LEFT;
	location_map[0x3d] = KeyLocation::RIGHT;
	// Meta/Command (yes, right < left).
	location_map[0x36] = KeyLocation::RIGHT;
	location_map[0x37] = KeyLocation::LEFT;
}

bool KeyMappingMacOS::is_numpad_key(unsigned int p_key) {
	return numpad_keys.has(p_key);
}

// Translates a macOS keycode to a Godot keycode.
Key KeyMappingMacOS::translate_key(unsigned int p_key) {
	const Key *key = keysym_map.getptr(p_key);
	if (key) {
		return *key;
	}
	return Key::NONE;
}

// Translates a Godot keycode back to a macOS keycode.
unsigned int KeyMappingMacOS::unmap_key(Key p_key) {
	const unsigned int *key = keysym_map_inv.getptr(p_key);
	if (key) {
		return *key;
	}
	return 127;
}

// Remap key according to current keyboard layout.
Key KeyMappingMacOS::remap_key(unsigned int p_key, unsigned int p_state, bool p_unicode) {
	if (is_numpad_key(p_key)) {
		return translate_key(p_key);
	}

	TISInputSourceRef current_keyboard = TISCopyCurrentKeyboardInputSource();
	if (!current_keyboard) {
		return translate_key(p_key);
	}

	CFDataRef layout_data = (CFDataRef)TISGetInputSourceProperty(current_keyboard, kTISPropertyUnicodeKeyLayoutData);
	if (!layout_data) {
		return translate_key(p_key);
	}

	const UCKeyboardLayout *keyboard_layout = (const UCKeyboardLayout *)CFDataGetBytePtr(layout_data);

	String keysym;
	UInt32 keys_down = 0;
	UniChar chars[256] = {};
	UniCharCount real_length = 0;

	OSStatus err = UCKeyTranslate(keyboard_layout,
			p_key,
			kUCKeyActionDisplay,
			(p_unicode) ? 0 : (p_state >> 8) & 0xFF,
			LMGetKbdType(),
			kUCKeyTranslateNoDeadKeysBit,
			&keys_down,
			std_size(chars),
			&real_length,
			chars);

	if (err != noErr) {
		return translate_key(p_key);
	}

	keysym = String::utf16((char16_t *)chars, real_length);
	if (keysym.is_empty()) {
		return translate_key(p_key);
	}

	char32_t c = keysym[0];
	if (p_unicode) {
		return fix_key_label(c, translate_key(p_key));
	} else {
		return fix_keycode(c, translate_key(p_key));
	}
}

// Translates a macOS keycode to a Godot key location.
KeyLocation KeyMappingMacOS::translate_location(unsigned int p_key) {
	const KeyLocation *location = location_map.getptr(p_key);
	if (location) {
		return *location;
	}
	return KeyLocation::UNSPECIFIED;
}

String KeyMappingMacOS::keycode_get_native_string(Key p_keycode) {
	const char32_t *key = keycode_map.getptr(p_keycode);
	if (key) {
		return String::chr(*key);
	}
	return String();
}

unsigned int KeyMappingMacOS::keycode_get_native_mask(Key p_keycode) {
	unsigned int mask = 0;
	if ((p_keycode & KeyModifierMask::CTRL) != Key::NONE) {
		mask |= NSEventModifierFlagControl;
	}
	if ((p_keycode & KeyModifierMask::ALT) != Key::NONE) {
		mask |= NSEventModifierFlagOption;
	}
	if ((p_keycode & KeyModifierMask::SHIFT) != Key::NONE) {
		mask |= NSEventModifierFlagShift;
	}
	if ((p_keycode & KeyModifierMask::META) != Key::NONE) {
		mask |= NSEventModifierFlagCommand;
	}
	if ((p_keycode & KeyModifierMask::KPAD) != Key::NONE) {
		mask |= NSEventModifierFlagNumericPad;
	}
	return mask;
}

// === Merged from platform/macos/godot_menu_delegate.mm ===
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

#import "servers/display/display_server.h"


@implementation GodotMenuDelegate

- (void)doNothing:(id)sender {
}

- (void)menuWillOpen:(NSMenu *)menu {
	if (NativeMenu::is_initialized()) {
		NativeMenuMacOS *nmenu = NativeMenu::driver_as<NativeMenuMacOS>();
		nmenu->_menu_open(menu);
	}
}

- (void)menuNeedsUpdate:(NSMenu *)menu {
	if (NativeMenu::is_initialized()) {
		NativeMenuMacOS *nmenu = NativeMenu::driver_as<NativeMenuMacOS>();
		nmenu->_menu_need_update(menu);
	}
}

- (void)menuDidClose:(NSMenu *)menu {
	if (NativeMenu::is_initialized()) {
		NativeMenuMacOS *nmenu = NativeMenu::driver_as<NativeMenuMacOS>();
		nmenu->_menu_close(menu);
	}
}

- (void)menu:(NSMenu *)menu willHighlightItem:(NSMenuItem *)item {
	if (item) {
		GodotMenuItem *value = [item representedObject];
		if (value && value->hover_callback.is_valid()) {
			// If custom callback is set, use it.
			Variant ret;
			Callable::CallError ce;
			const Variant *args[1] = { &value->meta };

			value->hover_callback.callp(args, 1, ret, ce);
			if (ce.error != Callable::CallError::CALL_OK) {
				ERR_PRINT(vformat("Failed to execute menu hover callback: %s.", Variant::get_callable_error_text(value->hover_callback, args, 1, ce)));
			}
		}
	}
}

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action {
	NSString *ev_key = [[event charactersIgnoringModifiers] lowercaseString];
	NSUInteger ev_modifiers = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
	for (int i = 0; i < [menu numberOfItems]; i++) {
		const NSMenuItem *menu_item = [menu itemAtIndex:i];
		if ([menu_item isEnabled] && [[menu_item keyEquivalent] compare:ev_key] == NSOrderedSame) {
			NSUInteger item_modifiers = [menu_item keyEquivalentModifierMask];

			if (ev_modifiers == item_modifiers) {
				GodotMenuItem *value = [menu_item representedObject];
				if (value) {
					if (value->key_callback.is_valid()) {
						// If custom callback is set, use it.
						Variant ret;
						Callable::CallError ce;
						const Variant *args[1] = { &value->meta };

						value->key_callback.callp(args, 1, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute menu key callback: %s.", Variant::get_callable_error_text(value->key_callback, args, 1, ce)));
						}
					} else {
						// Otherwise redirect event to the engine.
						if (DisplayServer::is_initialized()) {
							if ([[NSApplication sharedApplication] keyWindow].sheet) {
								[[[[NSApplication sharedApplication] keyWindow] sheetParent] sendEvent:event];
							} else {
								[[[NSApplication sharedApplication] keyWindow] sendEvent:event];
							}
						}
					}

					// Suppress default menu action.
					*target = self;
					*action = @selector(doNothing:);
				}
				return YES;
			}
		}
	}
	return NO;
}

@end

// === Merged from platform/macos/godot_menu_item.mm ===
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

#import "servers/display/display_server.h"

@implementation GodotMenuItem

- (id)init {
	self = [super init];

	self->callback = Callable();
	self->key_callback = Callable();
	self->checkable_type = GlobalMenuCheckType::CHECKABLE_TYPE_NONE;
	self->enabled = true;
	self->checked = false;
	self->indeterminate = false;
	self->max_states = 0;
	self->state = 0;
	self->indent = 0;
	self->accel = Key::NONE;

	return self;
}

@end

// === Merged from platform/macos/godot_open_save_delegate.mm ===
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

#import "servers/display/display_server.h"

@implementation GodotOpenSaveDelegate

- (instancetype)init {
	self = [super init];
	if ((self = [super init])) {
		dialog = nullptr;
		cur_index = 0;
		ctr_id = 1;
		allowed_types = nullptr;
		root = String();
	}
	return self;
}

- (void)makeAccessoryView:(NSSavePanel *)p_panel filters:(const Vector<String> &)p_filters options:(const TypedArray<Dictionary> &)p_options {
	dialog = p_panel;

	NSMutableArray *constraints = [NSMutableArray array];

	NSView *base_view = [[NSView alloc] initWithFrame:NSZeroRect];
	base_view.translatesAutoresizingMaskIntoConstraints = NO;

	NSGridView *view = [NSGridView gridViewWithNumberOfColumns:2 rows:0];
	view.translatesAutoresizingMaskIntoConstraints = NO;
	view.columnSpacing = 10;
	view.rowSpacing = 10;
	view.rowAlignment = NSGridRowAlignmentLastBaseline;

	int option_count = 0;

	for (int i = 0; i < p_options.size(); i++) {
		const Dictionary &item = p_options[i];
		if (!item.has("name") || !item.has("values") || !item.has("default")) {
			continue;
		}
		const String &name = item["name"];
		const Vector<String> &values = item["values"];
		int default_idx = item["default"];

		NSTextField *label = [NSTextField labelWithString:[NSString stringWithUTF8String:name.utf8().get_data()]];
		label.textColor = NSColor.secondaryLabelColor;
		if (@available(macOS 11.10, *)) {
			label.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
		}

		NSView *popup = nullptr;
		if (values.is_empty()) {
			NSButton *popup_check = [NSButton checkboxWithTitle:@"" target:self action:@selector(popupCheckAction:)];
			int tag = [self setDefaultBool:name value:(bool)default_idx];
			popup_check.state = (default_idx) ? NSControlStateValueOn : NSControlStateValueOff;
			popup_check.tag = tag;
			popup = popup_check;
		} else {
			NSPopUpButton *popup_list = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
			for (int i = 0; i < values.size(); i++) {
				[popup_list addItemWithTitle:[NSString stringWithUTF8String:values[i].utf8().get_data()]];
			}
			int tag = [self setDefaultInt:name value:default_idx];
			[popup_list selectItemAtIndex:default_idx];
			popup_list.tag = tag;
			popup_list.target = self;
			popup_list.action = @selector(popupOptionAction:);
			popup = popup_list;
		}

		[view addRowWithViews:[NSArray arrayWithObjects:label, popup, nil]];

		option_count++;
	}

	NSMutableArray *new_allowed_types = [[NSMutableArray alloc] init];
	Vector<Vector<String>> pref_types;
	bool has_type_popup = false;
	{
		NSTextField *label = [NSTextField labelWithString:[NSString stringWithUTF8String:RTR("Format").utf8().get_data()]];
		label.textColor = NSColor.secondaryLabelColor;
		if (@available(macOS 11.10, *)) {
			label.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
		}

		if (p_filters.size() > 1) {
			NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
			for (int i = 0; i < p_filters.size(); i++) {
				Vector<String> tokens = p_filters[i].split(";");
				Vector<String> pref_type;
				if (tokens.size() >= 1) {
					String flt = tokens[0].strip_edges();
					String mime = (tokens.size() >= 3) ? tokens[2].strip_edges() : String();
					int filter_slice_count = flt.get_slice_count(",");

					NSMutableArray *type_filters = [[NSMutableArray alloc] init];
					for (int j = 0; j < filter_slice_count; j++) {
						String str = (flt.get_slicec(',', j).strip_edges());
						if (!str.is_empty()) {
							UTType *ut = nullptr;
							if (str == "*.*") {
								ut = UTTypeData;
							} else {
								ut = [UTType typeWithFilenameExtension:[NSString stringWithUTF8String:str.replace("*.", "").strip_edges().utf8().get_data()]];
							}
							if (ut) {
								[type_filters addObject:ut];
								pref_type.push_back(str.replace("*.", "").strip_edges());
							}
						}
					}

					filter_slice_count = mime.get_slice_count(",");
					for (int j = 0; j < filter_slice_count; j++) {
						String str = mime.get_slicec(',', j).strip_edges();
						if (!str.is_empty()) {
							UTType *ut = [UTType typeWithMIMEType:[NSString stringWithUTF8String:str.strip_edges().utf8().get_data()]];
							if (ut) {
								[type_filters addObject:ut];
							}
						}
					}

					if ([type_filters count] > 0) {
						NSString *name_str = [NSString stringWithUTF8String:((tokens.size() == 1) ? tokens[0] : tokens[1].strip_edges()).utf8().get_data()];
						[new_allowed_types addObject:type_filters];
						pref_types.push_back(pref_type);
						[popup addItemWithTitle:name_str];
					}
				}
			}
			if (popup.numberOfItems > 1) {
				has_type_popup = true;
				popup.target = self;
				popup.action = @selector(popupFileAction:);

				[view addRowWithViews:[NSArray arrayWithObjects:label, popup, nil]];
			}
		} else if (p_filters.size() == 1) {
			Vector<String> tokens = p_filters[0].split(";");
			Vector<String> pref_type;
			if (tokens.size() >= 1) {
				String flt = tokens[0].strip_edges();
				String mime = (tokens.size() >= 3) ? tokens[2] : String();
				int filter_slice_count = flt.get_slice_count(",");

				NSMutableArray *type_filters = [[NSMutableArray alloc] init];
				for (int j = 0; j < filter_slice_count; j++) {
					String str = (flt.get_slicec(',', j).strip_edges());
					if (!str.is_empty()) {
						UTType *ut = nullptr;
						if (str == "*.*") {
							ut = UTTypeData;
						} else {
							ut = [UTType typeWithFilenameExtension:[NSString stringWithUTF8String:str.replace("*.", "").strip_edges().utf8().get_data()]];
						}
						if (ut) {
							[type_filters addObject:ut];
							pref_type.push_back(str.replace("*.", "").strip_edges());
						}
					}
				}
				filter_slice_count = mime.get_slice_count(",");
				for (int j = 0; j < filter_slice_count; j++) {
					String str = mime.get_slicec(',', j).strip_edges();
					if (!str.is_empty()) {
						UTType *ut = [UTType typeWithMIMEType:[NSString stringWithUTF8String:str.strip_edges().utf8().get_data()]];
						if (ut) {
							[type_filters addObject:ut];
						}
					}
				}

				if ([type_filters count] > 0) {
					[new_allowed_types addObject:type_filters];
					pref_types.push_back(pref_type);
				}
			}
		}
		[self setFileTypes:new_allowed_types pref:pref_types];
	}

	[base_view addSubview:view];
	[constraints addObject:[view.topAnchor constraintEqualToAnchor:base_view.topAnchor constant:10]];
	[constraints addObject:[base_view.bottomAnchor constraintEqualToAnchor:view.bottomAnchor constant:10]];
	[constraints addObject:[base_view.centerXAnchor constraintEqualToAnchor:view.centerXAnchor constant:10]];
	[NSLayoutConstraint activateConstraints:constraints];

	if (option_count > 0 || has_type_popup) {
		[p_panel setAccessoryView:base_view];
	}
	if ([new_allowed_types count] > 0) {
		NSMutableArray *type_filters = [new_allowed_types objectAtIndex:0];
		if (type_filters && [type_filters count] == 1 && [type_filters objectAtIndex:0] == UTTypeData) {
			[p_panel setAllowedContentTypes:@[ UTTypeData ]];
			[p_panel setAllowsOtherFileTypes:true];
		} else {
			[p_panel setAllowsOtherFileTypes:false];
			[p_panel setAllowedContentTypes:type_filters];
		}
	} else {
		[p_panel setAllowedContentTypes:@[ UTTypeData ]];
		[p_panel setAllowsOtherFileTypes:true];
	}
}

- (int)getIndex {
	return cur_index;
}

- (Dictionary)getSelection {
	return options;
}

- (int)setDefaultInt:(const String &)p_name value:(int)p_value {
	int cid = ctr_id++;
	options[p_name] = p_value;
	ctr_ids[cid] = p_name;

	return cid;
}

- (int)setDefaultBool:(const String &)p_name value:(bool)p_value {
	int cid = ctr_id++;
	options[p_name] = p_value;
	ctr_ids[cid] = p_name;

	return cid;
}

- (void)setFileTypes:(NSMutableArray *)p_allowed_types pref:(const Vector<Vector<String>> &)p_preftypes {
	allowed_types = p_allowed_types;
	preferred_types = p_preftypes;
}

- (instancetype)initWithDialog:(NSSavePanel *)p_dialog {
	if ((self = [super init])) {
		dialog = p_dialog;
		cur_index = 0;
		ctr_id = 1;
		allowed_types = nullptr;
	}
	return self;
}

- (void)popupCheckAction:(id)p_sender {
	NSButton *btn = p_sender;
	if (btn && ctr_ids.has(btn.tag)) {
		options[ctr_ids[btn.tag]] = ([btn state] == NSControlStateValueOn);
	}
}

- (void)popupOptionAction:(id)p_sender {
	NSPopUpButton *btn = p_sender;
	if (btn && ctr_ids.has(btn.tag)) {
		options[ctr_ids[btn.tag]] = (int)[btn indexOfSelectedItem];
	}
}

- (void)popupFileAction:(id)p_sender {
	NSPopUpButton *btn = p_sender;
	if (btn) {
		NSUInteger index = [btn indexOfSelectedItem];
		if (allowed_types && index < [allowed_types count]) {
			NSMutableArray *type_filters = [allowed_types objectAtIndex:index];
			if (type_filters && [type_filters count] == 1 && [type_filters objectAtIndex:0] == UTTypeData) {
				[dialog setAllowedContentTypes:@[ UTTypeData ]];
				[dialog setAllowsOtherFileTypes:true];
			} else {
				[dialog setAllowsOtherFileTypes:false];
				[dialog setAllowedContentTypes:type_filters];
			}
			cur_index = index;
		} else {
			[dialog setAllowedContentTypes:@[ UTTypeData ]];
			[dialog setAllowsOtherFileTypes:true];
			cur_index = -1;
		}
	}
}

- (void)setRootPath:(const String &)p_root_path {
	root = p_root_path;
}

- (String)validateFilename:(const String &)p_path {
	if (allowed_types) {
		NSMutableArray *type_filters = [allowed_types objectAtIndex:cur_index];
		UTType *ut = [type_filters objectAtIndex:0];
		String ext = String::utf8([[ut preferredFilenameExtension] UTF8String]);
		Vector<String> pref_ext = preferred_types[cur_index];
		if (!pref_ext.is_empty() && !pref_ext.has(ext) && p_path.has_extension(ext) && pref_ext[0] != "*") {
			return p_path.get_basename() + "." + pref_ext[0];
		}
	}
	return p_path;
}

- (BOOL)panel:(id)sender validateURL:(NSURL *)url error:(NSError *_Nullable *)outError {
	if (root.is_empty()) {
		return YES;
	}

	NSString *ns_path = url.URLByStandardizingPath.URLByResolvingSymlinksInPath.path;
	String path = String::utf8([ns_path UTF8String]).simplify_path();
	if (!path.begins_with(root.simplify_path())) {
		return NO;
	}

	return YES;
}

@end

// === Merged from platform/macos/native_menu_macos.mm ===
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

#import "servers/display/display_server.h"


#include "core/object/callable_mp.h"
#include "servers/display/display_server.h"

void NativeMenuMacOS::_register_system_menus(NSMenu *p_main_menu, NSMenu *p_application_menu, NSMenu *p_window_menu, NSMenu *p_help_menu, NSMenu *p_dock_menu) {
	{
		MenuData *md = memnew(MenuData);
		md->menu = p_main_menu;
		md->is_system = true;
		main_menu = menus.make_rid(md);
		main_menu_ns = p_main_menu;
		menu_lookup[md->menu] = main_menu;
	}
	{
		MenuData *md = memnew(MenuData);
		md->menu = p_application_menu;
		md->is_system = true;
		application_menu = menus.make_rid(md);
		application_menu_ns = p_application_menu;
		menu_lookup[md->menu] = application_menu;
	}
	{
		MenuData *md = memnew(MenuData);
		md->menu = p_window_menu;
		md->is_system = true;
		window_menu = menus.make_rid(md);
		window_menu_ns = p_window_menu;
		menu_lookup[md->menu] = window_menu;
	}
	{
		MenuData *md = memnew(MenuData);
		md->menu = p_help_menu;
		md->is_system = true;
		help_menu = menus.make_rid(md);
		help_menu_ns = p_help_menu;
		menu_lookup[md->menu] = help_menu;
	}
	{
		MenuData *md = memnew(MenuData);
		md->menu = p_dock_menu;
		md->is_system = true;
		dock_menu = menus.make_rid(md);
		dock_menu_ns = p_dock_menu;
		menu_lookup[md->menu] = dock_menu;
	}
}

NSMenu *NativeMenuMacOS::_get_dock_menu() {
	MenuData *md = menus.get_or_null(dock_menu);
	if (md) {
		return md->menu;
	}
	return nullptr;
}

void NativeMenuMacOS::_menu_open(NSMenu *p_menu) {
	if (menu_lookup.has(p_menu)) {
		MenuData *md = menus.get_or_null(menu_lookup[p_menu]);
		if (md) {
			// Note: Set "is_open" flag, but do not call callback, menu items can't be modified during this call and "_menu_need_update" will be called right before it.
			md->is_open = true;
		}
	}
}

void NativeMenuMacOS::_menu_need_update(NSMenu *p_menu) {
	if (menu_lookup.has(p_menu)) {
		MenuData *md = menus.get_or_null(menu_lookup[p_menu]);
		if (md) {
			// Note: "is_open" flag is set by "_menu_open", this method is always called before menu is shown, but might be called for the other reasons as well.
			if (md->open_cb.is_valid()) {
				Variant ret;
				Callable::CallError ce;

				// Callback is called directly, since it's expected to modify menu items before it's shown.
				md->open_cb.callp(nullptr, 0, ret, ce);
				if (ce.error != Callable::CallError::CALL_OK) {
					ERR_PRINT(vformat("Failed to execute menu open callback: %s.", Variant::get_callable_error_text(md->open_cb, nullptr, 0, ce)));
				}
			}
		}
	}
}

void NativeMenuMacOS::_menu_close(NSMenu *p_menu) {
	if (menu_lookup.has(p_menu)) {
		MenuData *md = menus.get_or_null(menu_lookup[p_menu]);
		if (md) {
			md->is_open = false;

			// Callback called deferred, since it should not modify menu items during "_menu_close" call.
			callable_mp(this, &NativeMenuMacOS::_menu_close_cb).call_deferred(menu_lookup[p_menu]);
		}
	}
}

void NativeMenuMacOS::_menu_close_cb(const RID &p_rid) {
	MenuData *md = menus.get_or_null(p_rid);
	if (md->close_cb.is_valid()) {
		Variant ret;
		Callable::CallError ce;

		md->close_cb.callp(nullptr, 0, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Failed to execute menu close callback: %s.", Variant::get_callable_error_text(md->close_cb, nullptr, 0, ce)));
		}
	}
}

bool NativeMenuMacOS::_is_menu_opened(NSMenu *p_menu) const {
	if (menu_lookup.has(p_menu)) {
		const MenuData *md = menus.get_or_null(menu_lookup[p_menu]);
		if (md && md->is_open) {
			return true;
		}
	}
	for (NSInteger i = (p_menu == [NSApp mainMenu]) ? 1 : 0; i < [p_menu numberOfItems]; i++) {
		const NSMenuItem *menu_item = [p_menu itemAtIndex:i];
		if ([menu_item submenu]) {
			if (_is_menu_opened([menu_item submenu])) {
				return true;
			}
		}
	}
	return false;
}

int NativeMenuMacOS::_get_system_menu_start(const NSMenu *p_menu) const {
	if (p_menu == [NSApp mainMenu]) { // Skip Apple menu.
		return 1;
	}
	if (p_menu == application_menu_ns || p_menu == window_menu_ns || p_menu == help_menu_ns) {
		int count = [p_menu numberOfItems];
		for (int i = 0; i < count; i++) {
			NSMenuItem *menu_item = [p_menu itemAtIndex:i];
			if (menu_item.tag == MENU_TAG_START) {
				return i + 1;
			}
		}
	}
	return 0;
}

int NativeMenuMacOS::_get_system_menu_count(const NSMenu *p_menu) const {
	if (p_menu == [NSApp mainMenu]) { // Skip Apple, Window and Help menu.
		return [p_menu numberOfItems] - 3;
	}
	if (p_menu == application_menu_ns || p_menu == window_menu_ns || p_menu == help_menu_ns) {
		int start = 0;
		int count = [p_menu numberOfItems];
		for (int i = 0; i < count; i++) {
			NSMenuItem *menu_item = [p_menu itemAtIndex:i];
			if (menu_item.tag == MENU_TAG_START) {
				start = i + 1;
			}
			if (menu_item.tag == MENU_TAG_END) {
				return i - start;
			}
		}
	}
	return [p_menu numberOfItems];
}

void NativeMenuMacOS::_update_align(MenuData *p_md) {
	int item_start = _get_system_menu_start(p_md->menu);
	int item_count = _get_system_menu_count(p_md->menu);

	bool has_icon = false;
	for (int i = item_start; i < item_start + item_count; i++) {
		NSMenuItem *menu_item = [p_md->menu itemAtIndex:i];
		if (!menu_item) {
			continue;
		}
		const GodotMenuItem *obj = [menu_item representedObject];
		if (obj && obj->img.is_valid()) {
			has_icon = true;
			break;
		}
	}
	for (int i = item_start; i < item_start + item_count; i++) {
		NSMenuItem *menu_item = [p_md->menu itemAtIndex:i];
		if (!menu_item) {
			continue;
		}
		const GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			if (has_icon && !obj->img.is_valid()) {
				[menu_item setIndentationLevel:obj->indent + 2];
			} else {
				[menu_item setIndentationLevel:obj->indent];
			}
		}
	}
}

bool NativeMenuMacOS::has_feature(Feature p_feature) const {
	switch (p_feature) {
		case FEATURE_GLOBAL_MENU:
		case FEATURE_POPUP_MENU:
		case FEATURE_OPEN_CLOSE_CALLBACK:
		case FEATURE_HOVER_CALLBACK:
		case FEATURE_KEY_CALLBACK:
			return true;
		default:
			return false;
	}
}

bool NativeMenuMacOS::has_system_menu(SystemMenus p_menu_id) const {
	switch (p_menu_id) {
		case MAIN_MENU_ID:
		case APPLICATION_MENU_ID:
		case WINDOW_MENU_ID:
		case HELP_MENU_ID:
		case DOCK_MENU_ID:
			return true;
		default:
			return false;
	}
}

RID NativeMenuMacOS::get_system_menu(SystemMenus p_menu_id) const {
	switch (p_menu_id) {
		case MAIN_MENU_ID:
			return main_menu;
		case APPLICATION_MENU_ID:
			return application_menu;
		case WINDOW_MENU_ID:
			return window_menu;
		case HELP_MENU_ID:
			return help_menu;
		case DOCK_MENU_ID:
			return dock_menu;
		default:
			return RID();
	}
}

String NativeMenuMacOS::get_system_menu_text(SystemMenus p_menu_id) const {
	NSMenu *menu = nullptr;
	switch (p_menu_id) {
		case WINDOW_MENU_ID: {
			menu = window_menu_ns;
		} break;
		case HELP_MENU_ID: {
			menu = help_menu_ns;
		} break;
		default:
			return String();
	}
	if (!menu) {
		return String();
	}
	return String::utf8([[menu title] UTF8String]);
}

void NativeMenuMacOS::set_system_menu_text(SystemMenus p_menu_id, const String &p_name) {
	NSMenu *menu = nullptr;
	switch (p_menu_id) {
		case WINDOW_MENU_ID: {
			menu = window_menu_ns;
		} break;
		case HELP_MENU_ID: {
			menu = help_menu_ns;
		} break;
		default:
			return;
	}
	if (!menu || !main_menu_ns) {
		return;
	}
	[menu setTitle:[NSString stringWithUTF8String:p_name.utf8().get_data()]];
	int idx = [main_menu_ns indexOfItemWithSubmenu:(NSMenu *)menu];
	NSMenuItem *menu_item = [main_menu_ns itemAtIndex:idx];
	if (menu_item) {
		[menu_item setTitle:[NSString stringWithUTF8String:p_name.utf8().get_data()]];
	}
}

RID NativeMenuMacOS::create_menu() {
	MenuData *md = memnew(MenuData);
	md->menu = [[NSMenu alloc] initWithTitle:@""];
	[md->menu setAutoenablesItems:NO];
	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds) {
		ds->set_menu_delegate(md->menu);
	}
	RID rid = menus.make_rid(md);
	menu_lookup[md->menu] = rid;
	return rid;
}

bool NativeMenuMacOS::has_menu(const RID &p_rid) const {
	return menus.owns(p_rid);
}

void NativeMenuMacOS::free_menu(const RID &p_rid) {
	MenuData *md = menus.get_or_null(p_rid);
	if (md && !md->is_system) {
		clear(p_rid);
		menus.free(p_rid);
		menu_lookup.erase(md->menu);
		md->menu = nullptr;
		memdelete(md);
	}
}

NSMenu *NativeMenuMacOS::get_native_menu_handle(const RID &p_rid) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, nullptr);

	return md->menu;
}

Size2 NativeMenuMacOS::get_size(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Size2());

	return Size2(md->menu.size.width, md->menu.size.height) * DisplayServer::screen_get_max_scale();
}

void NativeMenuMacOS::popup(const RID &p_rid, const Vector2i &p_position) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
	if (ds) {
		Point2i position = p_position;
		// macOS native y-coordinate relative to _get_screens_origin() is negative,
		// Godot passes a positive value.
		position.y *= -1;
		position += ds->_get_screens_origin();
		position /= ds->screen_get_max_scale();

		[md->menu popUpMenuPositioningItem:nil atLocation:NSMakePoint(position.x, position.y - 5) inView:nil]; // Menu vertical position doesn't include rounded corners, add `5` display pixels to better align it with Godot buttons.

		ds->release_pressed_events(); // Note: context menu block main loop and consume events, pressed keys and mouse buttons should be released manually.
		ds->sync_mouse_state();
	}
}

void NativeMenuMacOS::set_interface_direction(const RID &p_rid, bool p_is_rtl) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	md->menu.userInterfaceLayoutDirection = p_is_rtl ? NSUserInterfaceLayoutDirectionRightToLeft : NSUserInterfaceLayoutDirectionLeftToRight;
}

void NativeMenuMacOS::set_popup_open_callback(const RID &p_rid, const Callable &p_callback) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	md->open_cb = p_callback;
}

Callable NativeMenuMacOS::get_popup_open_callback(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Callable());

	return md->open_cb;
}

void NativeMenuMacOS::set_popup_close_callback(const RID &p_rid, const Callable &p_callback) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	md->close_cb = p_callback;
}

Callable NativeMenuMacOS::get_popup_close_callback(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Callable());

	return md->close_cb;
}

void NativeMenuMacOS::set_minimum_width(const RID &p_rid, float p_width) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	md->menu.minimumWidth = p_width / DisplayServer::screen_get_max_scale();
}

float NativeMenuMacOS::get_minimum_width(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, 0.0);

	return md->menu.minimumWidth * DisplayServer::screen_get_max_scale();
}

bool NativeMenuMacOS::is_opened(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);

	return md->is_open;
}

int NativeMenuMacOS::add_submenu_item(const RID &p_rid, const String &p_label, const RID &p_submenu_rid, const Variant &p_tag, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	MenuData *md_sub = menus.get_or_null(p_submenu_rid);
	ERR_FAIL_NULL_V(md, -1);
	ERR_FAIL_NULL_V(md_sub, -1);
	ERR_FAIL_COND_V_MSG(md->menu == md_sub->menu, -1, "Can't set submenu to self!");
	ERR_FAIL_COND_V_MSG([md_sub->menu supermenu], -1, "Can't set submenu to menu that is already a submenu of some other menu!");

	NSMenuItem *menu_item;
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	if (p_index < 0) {
		p_index = item_start + item_count;
	} else {
		p_index += item_start;
		p_index = CLAMP(p_index, item_start, item_start + item_count);
	}
	menu_item = [md->menu insertItemWithTitle:[NSString stringWithUTF8String:p_label.utf8().get_data()] action:nil keyEquivalent:@"" atIndex:p_index];

	GodotMenuItem *obj = [[GodotMenuItem alloc] init];
	obj->meta = p_tag;
	[menu_item setRepresentedObject:obj];

	[md_sub->menu setTitle:[NSString stringWithUTF8String:p_label.utf8().get_data()]];
	[md->menu setSubmenu:md_sub->menu forItem:menu_item];
	_update_align(md);

	return p_index - item_start;
}

NSMenuItem *NativeMenuMacOS::_menu_add_item(NSMenu *p_menu, const String &p_label, Key p_accel, int p_index, int *r_out) {
	if (p_menu) {
		String keycode = KeyMappingMacOS::keycode_get_native_string(p_accel & KeyModifierMask::CODE_MASK);
		NSMenuItem *menu_item;
		int item_start = _get_system_menu_start(p_menu);
		int item_count = _get_system_menu_count(p_menu);
		if (p_index < 0) {
			p_index = item_start + item_count;
		} else {
			p_index += item_start;
			p_index = CLAMP(p_index, item_start, item_start + item_count);
		}
		menu_item = [p_menu insertItemWithTitle:[NSString stringWithUTF8String:p_label.utf8().get_data()] action:@selector(globalMenuCallback:) keyEquivalent:[NSString stringWithUTF8String:keycode.utf8().get_data()] atIndex:p_index];
		*r_out = p_index - item_start;
		return menu_item;
	}
	return nullptr;
}

int NativeMenuMacOS::add_item(const RID &p_rid, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int out = -1;
	NSMenuItem *menu_item = _menu_add_item(md->menu, p_label, p_accel, p_index, &out);
	if (menu_item) {
		GodotMenuItem *obj = [[GodotMenuItem alloc] init];
		obj->callback = p_callback;
		obj->key_callback = p_key_callback;
		obj->meta = p_tag;
		obj->accel = p_accel;
		[menu_item setKeyEquivalentModifierMask:KeyMappingMacOS::keycode_get_native_mask(p_accel)];
		[menu_item setRepresentedObject:obj];
		_update_align(md);
	}
	return out;
}

int NativeMenuMacOS::add_check_item(const RID &p_rid, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int out = -1;
	NSMenuItem *menu_item = _menu_add_item(md->menu, p_label, p_accel, p_index, &out);
	if (menu_item) {
		GodotMenuItem *obj = [[GodotMenuItem alloc] init];
		obj->callback = p_callback;
		obj->key_callback = p_key_callback;
		obj->meta = p_tag;
		obj->checkable_type = CHECKABLE_TYPE_CHECK_BOX;
		obj->accel = p_accel;
		[menu_item setKeyEquivalentModifierMask:KeyMappingMacOS::keycode_get_native_mask(p_accel)];
		[menu_item setRepresentedObject:obj];
		_update_align(md);
	}
	return out;
}

int NativeMenuMacOS::add_icon_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int out = -1;
	NSMenuItem *menu_item = _menu_add_item(md->menu, p_label, p_accel, p_index, &out);
	if (menu_item) {
		GodotMenuItem *obj = [[GodotMenuItem alloc] init];
		obj->callback = p_callback;
		obj->key_callback = p_key_callback;
		obj->meta = p_tag;
		obj->accel = p_accel;
		DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
		if (ds && p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
			obj->img = p_icon->get_image();
			obj->img = obj->img->duplicate();
			if (obj->img->is_compressed()) {
				obj->img->decompress();
			}
			NSImage *image = ds->_convert_to_nsimg(obj->img);
			[image setSize:NSMakeSize(16, 16)];
			[menu_item setImage:image];
		}
		[menu_item setKeyEquivalentModifierMask:KeyMappingMacOS::keycode_get_native_mask(p_accel)];
		[menu_item setRepresentedObject:obj];
		_update_align(md);
	}
	return out;
}

int NativeMenuMacOS::add_icon_check_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int out = -1;
	NSMenuItem *menu_item = _menu_add_item(md->menu, p_label, p_accel, p_index, &out);
	if (menu_item) {
		GodotMenuItem *obj = [[GodotMenuItem alloc] init];
		obj->callback = p_callback;
		obj->key_callback = p_key_callback;
		obj->meta = p_tag;
		obj->checkable_type = CHECKABLE_TYPE_CHECK_BOX;
		obj->accel = p_accel;
		DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
		if (ds && p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
			obj->img = p_icon->get_image();
			obj->img = obj->img->duplicate();
			if (obj->img->is_compressed()) {
				obj->img->decompress();
			}
			NSImage *image = ds->_convert_to_nsimg(obj->img);
			[image setSize:NSMakeSize(16, 16)];
			[menu_item setImage:image];
		}
		[menu_item setKeyEquivalentModifierMask:KeyMappingMacOS::keycode_get_native_mask(p_accel)];
		[menu_item setRepresentedObject:obj];
		_update_align(md);
	}
	return out;
}

int NativeMenuMacOS::add_radio_check_item(const RID &p_rid, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int out = -1;
	NSMenuItem *menu_item = _menu_add_item(md->menu, p_label, p_accel, p_index, &out);
	if (menu_item) {
		GodotMenuItem *obj = [[GodotMenuItem alloc] init];
		obj->callback = p_callback;
		obj->key_callback = p_key_callback;
		obj->meta = p_tag;
		obj->checkable_type = CHECKABLE_TYPE_RADIO_BUTTON;
		obj->accel = p_accel;
		[menu_item setKeyEquivalentModifierMask:KeyMappingMacOS::keycode_get_native_mask(p_accel)];
		[menu_item setRepresentedObject:obj];
		_update_align(md);
	}
	return out;
}

int NativeMenuMacOS::add_icon_radio_check_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int out = -1;
	NSMenuItem *menu_item = _menu_add_item(md->menu, p_label, p_accel, p_index, &out);
	if (menu_item) {
		GodotMenuItem *obj = [[GodotMenuItem alloc] init];
		obj->callback = p_callback;
		obj->key_callback = p_key_callback;
		obj->meta = p_tag;
		obj->checkable_type = CHECKABLE_TYPE_RADIO_BUTTON;
		obj->accel = p_accel;
		DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
		if (ds && p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
			obj->img = p_icon->get_image();
			obj->img = obj->img->duplicate();
			if (obj->img->is_compressed()) {
				obj->img->decompress();
			}
			NSImage *image = ds->_convert_to_nsimg(obj->img);
			[image setSize:NSMakeSize(16, 16)];
			[menu_item setImage:image];
		}
		[menu_item setKeyEquivalentModifierMask:KeyMappingMacOS::keycode_get_native_mask(p_accel)];
		[menu_item setRepresentedObject:obj];
		_update_align(md);
	}
	return out;
}

int NativeMenuMacOS::add_multistate_item(const RID &p_rid, const String &p_label, int p_max_states, int p_default_state, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int out = -1;
	NSMenuItem *menu_item = _menu_add_item(md->menu, p_label, p_accel, p_index, &out);
	if (menu_item) {
		GodotMenuItem *obj = [[GodotMenuItem alloc] init];
		obj->callback = p_callback;
		obj->key_callback = p_key_callback;
		obj->meta = p_tag;
		obj->max_states = p_max_states;
		obj->state = p_default_state;
		obj->accel = p_accel;
		[menu_item setKeyEquivalentModifierMask:KeyMappingMacOS::keycode_get_native_mask(p_accel)];
		[menu_item setRepresentedObject:obj];
		_update_align(md);
	}
	return out;
}

int NativeMenuMacOS::add_separator(const RID &p_rid, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (md->menu == [NSApp mainMenu]) { // Do not add separators into main menu.
		return -1;
	}
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	if (p_index < 0) {
		p_index = item_start + item_count;
	} else {
		p_index += item_start;
		p_index = CLAMP(p_index, item_start, item_start + item_count);
	}
	[md->menu insertItem:[NSMenuItem separatorItem] atIndex:p_index];
	return p_index - item_start;
}

int NativeMenuMacOS::find_item_index_with_text(const RID &p_rid, const String &p_text) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int item_start = _get_system_menu_start(md->menu);
	int index = [md->menu indexOfItemWithTitle:[NSString stringWithUTF8String:p_text.utf8().get_data()]];
	if (index >= 0) {
		return index - item_start;
	}
	return -1;
}

int NativeMenuMacOS::find_item_index_with_tag(const RID &p_rid, const Variant &p_tag) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	for (NSInteger i = item_start; i < item_start + item_count; i++) {
		const NSMenuItem *menu_item = [md->menu itemAtIndex:i];
		if (menu_item) {
			const GodotMenuItem *obj = [menu_item representedObject];
			if (obj && obj->meta == p_tag) {
				return i - item_start;
			}
		}
	}
	return -1;
}

bool NativeMenuMacOS::is_item_checked(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, false);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		const GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->checked;
		}
	}
	return false;
}

bool NativeMenuMacOS::is_item_indeterminate(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, false);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		const GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->indeterminate;
		}
	}
	return false;
}

bool NativeMenuMacOS::is_item_checkable(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, false);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->checkable_type == CHECKABLE_TYPE_CHECK_BOX;
		}
	}
	return false;
}

bool NativeMenuMacOS::is_item_radio_checkable(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, false);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->checkable_type == CHECKABLE_TYPE_RADIO_BUTTON;
		}
	}
	return false;
}

Callable NativeMenuMacOS::get_item_callback(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Callable());

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Callable());

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, Callable());
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->callback;
		}
	}
	return Callable();
}

Callable NativeMenuMacOS::get_item_key_callback(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Callable());

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Callable());

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, Callable());
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->key_callback;
		}
	}
	return Callable();
}

Variant NativeMenuMacOS::get_item_tag(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Variant());

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Variant());

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, Variant());
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->meta;
		}
	}
	return Variant();
}

String NativeMenuMacOS::get_item_text(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, String());

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, String());

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, String());
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		return String::utf8([[menu_item title] UTF8String]);
	}
	return String();
}

RID NativeMenuMacOS::get_item_submenu(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, RID());

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, RID());

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, RID());
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		NSMenu *sub_menu = [menu_item submenu];
		if (sub_menu && menu_lookup.has(sub_menu)) {
			return menu_lookup[sub_menu];
		}
	}
	return RID();
}

Key NativeMenuMacOS::get_item_accelerator(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Key::NONE);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Key::NONE);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, Key::NONE);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		String ret = String::utf8([[menu_item keyEquivalent] UTF8String]);
		Key keycode = find_keycode(ret);
		NSUInteger mask = [menu_item keyEquivalentModifierMask];
		if (mask & NSEventModifierFlagControl) {
			keycode |= KeyModifierMask::CTRL;
		}
		if (mask & NSEventModifierFlagOption) {
			keycode |= KeyModifierMask::ALT;
		}
		if (mask & NSEventModifierFlagShift) {
			keycode |= KeyModifierMask::SHIFT;
		}
		if (mask & NSEventModifierFlagCommand) {
			keycode |= KeyModifierMask::META;
		}
		if (mask & NSEventModifierFlagNumericPad) {
			keycode |= KeyModifierMask::KPAD;
		}
		return keycode;
	}
	return Key::NONE;
}

bool NativeMenuMacOS::is_item_disabled(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, false);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		return ![menu_item isEnabled];
	}
	return false;
}

bool NativeMenuMacOS::is_item_hidden(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, false);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		return [menu_item isHidden];
	}
	return false;
}

String NativeMenuMacOS::get_item_tooltip(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, String());

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, String());

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, String());
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		return String::utf8([[menu_item toolTip] UTF8String]);
	}
	return String();
}

int NativeMenuMacOS::get_item_state(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, 0);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, 0);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, 0);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->state;
		}
	}
	return 0;
}

int NativeMenuMacOS::get_item_max_states(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, 0);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, 0);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, 0);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			return obj->max_states;
		}
	}
	return 0;
}

Ref<Texture2D> NativeMenuMacOS::get_item_icon(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Ref<Texture2D>());

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Ref<Texture2D>());

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, Ref<Texture2D>());
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			if (obj->img.is_valid()) {
				return ImageTexture::create_from_image(obj->img);
			}
		}
	}
	return Ref<Texture2D>();
}

int NativeMenuMacOS::get_item_indentation_level(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, 0);

	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, 0);

	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, 0);
	const NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		return [menu_item indentationLevel];
	}
	return 0;
}

void NativeMenuMacOS::set_item_checked(const RID &p_rid, int p_idx, bool p_checked) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			obj->checked = p_checked;
			obj->indeterminate = false;
			if (p_checked) {
				[menu_item setState:NSControlStateValueOn];
			} else {
				[menu_item setState:NSControlStateValueOff];
			}
		}
	}
}

void NativeMenuMacOS::set_item_indeterminate(const RID &p_rid, int p_idx, bool p_indeterminate) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			obj->indeterminate = p_indeterminate;
			obj->checked = false;
			if (p_indeterminate) {
				[menu_item setState:NSControlStateValueMixed];
			} else {
				[menu_item setState:NSControlStateValueOff];
			}
		}
	}
}

void NativeMenuMacOS::set_item_checkable(const RID &p_rid, int p_idx, bool p_checkable) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		obj->checkable_type = (p_checkable) ? CHECKABLE_TYPE_CHECK_BOX : CHECKABLE_TYPE_NONE;
	}
}

void NativeMenuMacOS::set_item_radio_checkable(const RID &p_rid, int p_idx, bool p_checkable) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		obj->checkable_type = (p_checkable) ? CHECKABLE_TYPE_RADIO_BUTTON : CHECKABLE_TYPE_NONE;
	}
}

void NativeMenuMacOS::set_item_callback(const RID &p_rid, int p_idx, const Callable &p_callback) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		obj->callback = p_callback;
	}
}

void NativeMenuMacOS::set_item_key_callback(const RID &p_rid, int p_idx, const Callable &p_key_callback) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		obj->key_callback = p_key_callback;
	}
}

void NativeMenuMacOS::set_item_hover_callbacks(const RID &p_rid, int p_idx, const Callable &p_callback) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		obj->hover_callback = p_callback;
	}
}

void NativeMenuMacOS::set_item_tag(const RID &p_rid, int p_idx, const Variant &p_tag) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		obj->meta = p_tag;
	}
}

void NativeMenuMacOS::set_item_text(const RID &p_rid, int p_idx, const String &p_text) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		[menu_item setTitle:[NSString stringWithUTF8String:p_text.utf8().get_data()]];
		NSMenu *sub_menu = [menu_item submenu];
		if (sub_menu) {
			[sub_menu setTitle:[NSString stringWithUTF8String:p_text.utf8().get_data()]];
		}
	}
}

void NativeMenuMacOS::set_item_submenu(const RID &p_rid, int p_idx, const RID &p_submenu_rid) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	if (p_submenu_rid.is_valid()) {
		MenuData *md_sub = menus.get_or_null(p_submenu_rid);
		ERR_FAIL_NULL(md_sub);
		ERR_FAIL_COND_MSG(md->menu == md_sub->menu, "Can't set submenu to self!");
		ERR_FAIL_COND_MSG([md_sub->menu supermenu], "Can't set submenu to menu that is already a submenu of some other menu!");

		int item_start = _get_system_menu_start(md->menu);
		int item_count = _get_system_menu_count(md->menu);
		p_idx += item_start;
		ERR_FAIL_COND(p_idx >= item_start + item_count);
		NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
		if (menu_item) {
			[md->menu setSubmenu:md_sub->menu forItem:menu_item];
			[menu_item setAction:nil];
			[menu_item setKeyEquivalent:@""];
		}
	} else {
		int item_start = _get_system_menu_start(md->menu);
		int item_count = _get_system_menu_count(md->menu);
		p_idx += item_start;
		ERR_FAIL_COND(p_idx >= item_start + item_count);
		NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
		if (menu_item) {
			if ([menu_item submenu] && _is_menu_opened([menu_item submenu])) {
				ERR_PRINT("Can't remove open menu!");
				return;
			}
			GodotMenuItem *obj = [menu_item representedObject];
			String keycode = KeyMappingMacOS::keycode_get_native_string(obj->accel & KeyModifierMask::CODE_MASK);
			[md->menu setSubmenu:nil forItem:menu_item];
			[menu_item setAction:@selector(globalMenuCallback:)];
			[menu_item setKeyEquivalent:[NSString stringWithUTF8String:keycode.utf8().get_data()]];
		}
	}
}

void NativeMenuMacOS::set_item_accelerator(const RID &p_rid, int p_idx, Key p_keycode) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		obj->accel = p_keycode;
		NSMenu *sub_menu = [menu_item submenu];
		if (!sub_menu) {
			if (p_keycode == Key::NONE) {
				[menu_item setKeyEquivalent:@""];
			} else {
				[menu_item setKeyEquivalentModifierMask:KeyMappingMacOS::keycode_get_native_mask(p_keycode)];
				String keycode = KeyMappingMacOS::keycode_get_native_string(p_keycode & KeyModifierMask::CODE_MASK);
				[menu_item setKeyEquivalent:[NSString stringWithUTF8String:keycode.utf8().get_data()]];
			}
		}
	}
}

void NativeMenuMacOS::set_item_disabled(const RID &p_rid, int p_idx, bool p_disabled) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		obj->enabled = !p_disabled;
		[menu_item setEnabled:(!p_disabled)];
	}
}

void NativeMenuMacOS::set_item_hidden(const RID &p_rid, int p_idx, bool p_hidden) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		[menu_item setHidden:p_hidden];
	}
}

void NativeMenuMacOS::set_item_tooltip(const RID &p_rid, int p_idx, const String &p_tooltip) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		[menu_item setToolTip:[NSString stringWithUTF8String:p_tooltip.utf8().get_data()]];
	}
}

void NativeMenuMacOS::set_item_state(const RID &p_rid, int p_idx, int p_state) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		obj->state = p_state;
	}
}

void NativeMenuMacOS::set_item_max_states(const RID &p_rid, int p_idx, int p_max_states) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		obj->max_states = p_max_states;
	}
}

void NativeMenuMacOS::set_item_icon(const RID &p_rid, int p_idx, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		ERR_FAIL_NULL(obj);
		DisplayServerMacOS *ds = DisplayServer::driver_as<DisplayServerMacOS>();
		if (ds && p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
			obj->img = p_icon->get_image();
			obj->img = obj->img->duplicate();
			if (obj->img->is_compressed()) {
				obj->img->decompress();
			}
			NSImage *image = ds->_convert_to_nsimg(obj->img);
			[image setSize:NSMakeSize(16, 16)];
			[menu_item setImage:image];
		} else {
			obj->img = Ref<Image>();
			[menu_item setImage:nil];
		}
		_update_align(md);
	}
}

void NativeMenuMacOS::set_item_indentation_level(const RID &p_rid, int p_idx, int p_level) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if (menu_item) {
		GodotMenuItem *obj = [menu_item representedObject];
		if (obj) {
			obj->indent = p_level;
			_update_align(md);
		}
	}
}

int NativeMenuMacOS::set_item_index(const RID &p_rid, int p_idx, int p_target_idx) {
	ERR_FAIL_COND_V(p_idx < 0, -1);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND_V(p_idx >= item_start + item_count, -1);
	ERR_FAIL_INDEX_V(p_target_idx, item_count, -1);
	p_target_idx += item_start;

	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if ([menu_item submenu] && _is_menu_opened([menu_item submenu])) {
		ERR_FAIL_V_MSG(-1, "Can't move open menu!");
	}
	if (menu_item) {
		[md->menu removeItemAtIndex:p_idx];
		[md->menu insertItem:menu_item atIndex:p_target_idx];
	}
	return p_target_idx - item_start;
}

int NativeMenuMacOS::get_item_count(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, 0);

	return _get_system_menu_count(md->menu);
}

bool NativeMenuMacOS::is_system_menu(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);

	return md->is_system;
}

void NativeMenuMacOS::remove_item(const RID &p_rid, int p_idx) {
	ERR_FAIL_COND(p_idx < 0);

	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int item_start = _get_system_menu_start(md->menu);
	int item_count = _get_system_menu_count(md->menu);
	p_idx += item_start;
	ERR_FAIL_COND(p_idx >= item_start + item_count);
	NSMenuItem *menu_item = [md->menu itemAtIndex:p_idx];
	if ([menu_item submenu] && _is_menu_opened([menu_item submenu])) {
		ERR_PRINT("Can't remove open menu!");
		return;
	}
	[md->menu removeItemAtIndex:p_idx];
	_update_align(md);
}

void NativeMenuMacOS::clear(const RID &p_rid) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	ERR_FAIL_COND_MSG(_is_menu_opened(md->menu), "Can't remove open menu!");

	if (p_rid == application_menu || p_rid == window_menu || p_rid == help_menu) {
		int start = _get_system_menu_start(md->menu);
		int count = _get_system_menu_count(md->menu);
		for (int i = start + count - 1; i >= start; i--) {
			[md->menu removeItemAtIndex:i];
		}
	} else {
		[md->menu removeAllItems];
	}

	if (p_rid == main_menu) {
		// Restore Apple, Window and Help menu.
		MenuData *md_app = menus.get_or_null(application_menu);
		if (md_app) {
			NSMenuItem *menu_item = [md->menu addItemWithTitle:@"" action:nil keyEquivalent:@""];
			[md->menu setSubmenu:md_app->menu forItem:menu_item];
		}
		MenuData *md_win = menus.get_or_null(window_menu);
		if (md_win) {
			NSMenuItem *menu_item = [md->menu addItemWithTitle:@"Window" action:nil keyEquivalent:@""];
			[md->menu setSubmenu:md_win->menu forItem:menu_item];
		}
		MenuData *md_hlp = menus.get_or_null(help_menu);
		if (md_hlp) {
			NSMenuItem *menu_item = [md->menu addItemWithTitle:@"Help" action:nil keyEquivalent:@""];
			[md->menu setSubmenu:md_hlp->menu forItem:menu_item];
		}
	}
}

NativeMenuMacOS::NativeMenuMacOS() {}

NativeMenuMacOS::~NativeMenuMacOS() {
	if (main_menu.is_valid()) {
		MenuData *md = menus.get_or_null(main_menu);
		if (md) {
			clear(main_menu);
			menus.free(main_menu);
			menu_lookup.erase(md->menu);
			md->menu = nullptr;
			main_menu_ns = nullptr;
			memdelete(md);
		}
	}
	if (application_menu.is_valid()) {
		MenuData *md = menus.get_or_null(application_menu);
		if (md) {
			clear(application_menu);
			menus.free(application_menu);
			menu_lookup.erase(md->menu);
			md->menu = nullptr;
			application_menu_ns = nullptr;
			memdelete(md);
		}
	}
	if (window_menu.is_valid()) {
		MenuData *md = menus.get_or_null(window_menu);
		if (md) {
			clear(window_menu);
			menus.free(window_menu);
			menu_lookup.erase(md->menu);
			md->menu = nullptr;
			window_menu_ns = nullptr;
			memdelete(md);
		}
	}
	if (help_menu.is_valid()) {
		MenuData *md = menus.get_or_null(help_menu);
		if (md) {
			clear(help_menu);
			menus.free(help_menu);
			menu_lookup.erase(md->menu);
			md->menu = nullptr;
			help_menu_ns = nullptr;
			memdelete(md);
		}
	}
	if (dock_menu.is_valid()) {
		MenuData *md = menus.get_or_null(dock_menu);
		if (md) {
			clear(dock_menu);
			menus.free(dock_menu);
			menu_lookup.erase(md->menu);
			md->menu = nullptr;
			dock_menu_ns = nullptr;
			memdelete(md);
		}
	}
}

// === Merged from platform/macos/rendering_context_driver_vulkan_macos.mm ===
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

#import "servers/display/display_server.h"

#ifdef VULKAN_ENABLED

#ifdef USE_VOLK
#include <volk.h>
#else
#include <vulkan/vulkan_metal.h>
#endif

const char *RenderingContextDriverVulkan::_get_platform_surface_extension() const {
	return surface_mode == SURFACE_NONE ? nullptr : VK_EXT_METAL_SURFACE_EXTENSION_NAME;
}

RenderingContextDriver::SurfaceID RenderingContextDriverVulkan::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	VkMetalSurfaceCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
	create_info.pLayer = (CAMetalLayer *)*wpd->layer_ptr;

	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkResult err = vkCreateMetalSurfaceEXT(instance_get(), &create_info, get_allocation_callbacks(VK_OBJECT_TYPE_SURFACE_KHR), &vk_surface);
	ERR_FAIL_COND_V(err != VK_SUCCESS, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->vk_surface = vk_surface;
	return SurfaceID(surface);
}

#endif // VULKAN_ENABLED

// === Merged from platform/macos/gl_manager_macos_legacy.mm ===
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

#import "servers/display/display_server.h"

#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#include "core/os/os.h"

#include <dlfcn.h>

#include <cstdio>
#include <cstdlib>

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in macOS 10.14.

Error GLManagerLegacy_MacOS::create_context(GLWindow &win) {
	NSOpenGLPixelFormatAttribute attributes[] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAClosestPolicy,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFAColorSize, 32,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		0
	};

	NSOpenGLPixelFormat *pixel_format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	ERR_FAIL_NULL_V(pixel_format, ERR_CANT_CREATE);

	win.context = [[NSOpenGLContext alloc] initWithFormat:pixel_format shareContext:shared_context];
	ERR_FAIL_NULL_V(win.context, ERR_CANT_CREATE);
	if (shared_context == nullptr) {
		shared_context = win.context;
	}

	[win.context setView:win.window_view];
	[win.context makeCurrentContext];

	return OK;
}

Error GLManagerLegacy_MacOS::window_create(DisplayServerEnums::WindowID p_window_id, id p_view, int p_width, int p_height) {
	GLWindow win;
	win.window_view = p_view;

	if (create_context(win) != OK) {
		return FAILED;
	}

	windows[p_window_id] = win;
	window_make_current(p_window_id);

	return OK;
}

void GLManagerLegacy_MacOS::window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height) {
	if (!windows.has(p_window_id)) {
		return;
	}

	GLWindow &win = windows[p_window_id];

	GLint dim[2];
	dim[0] = p_width;
	dim[1] = p_height;
	CGLSetParameter((CGLContextObj)[win.context CGLContextObj], kCGLCPSurfaceBackingSize, &dim[0]);
	CGLEnable((CGLContextObj)[win.context CGLContextObj], kCGLCESurfaceBackingSize);
	if (OS::is_hidpi_allowed()) {
		[win.window_view setWantsBestResolutionOpenGLSurface:YES];
	} else {
		[win.window_view setWantsBestResolutionOpenGLSurface:NO];
	}

	[win.context update];
}

void GLManagerLegacy_MacOS::window_destroy(DisplayServerEnums::WindowID p_window_id) {
	if (!windows.has(p_window_id)) {
		return;
	}

	if (current_window == p_window_id) {
		current_window = DisplayServerEnums::INVALID_WINDOW_ID;
	}

	windows.erase(p_window_id);
}

void GLManagerLegacy_MacOS::release_current() {
	if (current_window == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	[NSOpenGLContext clearCurrentContext];
	current_window = DisplayServerEnums::INVALID_WINDOW_ID;
}

void GLManagerLegacy_MacOS::window_make_current(DisplayServerEnums::WindowID p_window_id) {
	if (current_window == p_window_id) {
		return;
	}
	if (!windows.has(p_window_id)) {
		return;
	}

	GLWindow &win = windows[p_window_id];
	[win.context makeCurrentContext];

	current_window = p_window_id;
}

void GLManagerLegacy_MacOS::swap_buffers() {
	GLWindow &win = windows[current_window];
	[win.context flushBuffer];
}

void GLManagerLegacy_MacOS::window_set_per_pixel_transparency_enabled(DisplayServerEnums::WindowID p_window_id, bool p_enabled) {
	if (!windows.has(p_window_id)) {
		return;
	}

	GLWindow &win = windows[p_window_id];
	if (p_enabled) {
		GLint opacity = 0;
		[win.context setValues:&opacity forParameter:NSOpenGLContextParameterSurfaceOpacity];
	} else {
		GLint opacity = 1;
		[win.context setValues:&opacity forParameter:NSOpenGLContextParameterSurfaceOpacity];
	}
	[win.context update];
}

Error GLManagerLegacy_MacOS::initialize() {
	return framework_loaded ? OK : ERR_CANT_CREATE;
}

void GLManagerLegacy_MacOS::set_use_vsync(bool p_use) {
	use_vsync = p_use;

	CGLContextObj ctx = CGLGetCurrentContext();
	if (ctx) {
		GLint swapInterval = p_use ? 1 : 0;
		if (CGLSetParameter(ctx, kCGLCPSwapInterval, &swapInterval) != kCGLNoError) {
			WARN_PRINT("Could not set V-Sync mode.");
		}
		use_vsync = p_use;
	}
}

bool GLManagerLegacy_MacOS::is_using_vsync() const {
	return use_vsync;
}

NSOpenGLContext *GLManagerLegacy_MacOS::get_context(DisplayServerEnums::WindowID p_window_id) {
	if (!windows.has(p_window_id)) {
		return nullptr;
	}

	GLWindow &win = windows[p_window_id];
	return win.context;
}

GLManagerLegacy_MacOS::GLManagerLegacy_MacOS() {
	NSBundle *framework = [NSBundle bundleWithPath:@"/System/Library/Frameworks/OpenGL.framework"];
	if (framework) {
		void *library_handle = dlopen([framework.executablePath UTF8String], RTLD_NOW);
		if (library_handle) {
			CGLEnable = (CGLEnablePtr)dlsym(library_handle, "CGLEnable");
			CGLSetParameter = (CGLSetParameterPtr)dlsym(library_handle, "CGLSetParameter");
			CGLGetCurrentContext = (CGLGetCurrentContextPtr)dlsym(library_handle, "CGLGetCurrentContext");

			framework_loaded = CGLEnable && CGLSetParameter && CGLGetCurrentContext;
		}
	}
}

GLManagerLegacy_MacOS::~GLManagerLegacy_MacOS() {
	release_current();
}

GODOT_CLANG_WARNING_POP

#endif // MACOS_ENABLED && GLES3_ENABLED

// === Merged from platform/macos/godot_progress_view.mm ===
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

#import "servers/display/display_server.h"

@implementation GodotProgressView

- (id)init {
	self = [super init];
	pr_state = DisplayServerEnums::PROGRESS_STATE_NOPROGRESS;
	pr_value = 0.f;
	pr_offset = 0.f;
	return self;
}

- (void)setValue:(float)value {
	pr_value = value;
}

- (void)setState:(DisplayServerEnums::ProgressState)state {
	pr_state = state;
}

- (void)drawRect:(NSRect)dirtyRect {
	// Icon draw.
	[[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationHigh];
	[[NSApp applicationIconImage] drawInRect:self.bounds];

	if (pr_state == DisplayServerEnums::PROGRESS_STATE_NOPROGRESS) {
		return;
	}

	// Border draw.
	NSRect rect = NSMakeRect(1.f, 1.f, self.bounds.size.width - 2.f, 16.f);
	NSBezierPath *bezier_path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:8.f yRadius:8.f];
	[bezier_path setLineWidth:2.0];
	[[NSColor grayColor] set];
	[bezier_path stroke];

	// Fill clip path.
	rect = NSMakeRect(2.f, 2.f, self.bounds.size.width - 4.f, 14.f);
	bezier_path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:7.f yRadius:7.f];
	[bezier_path setLineWidth:1.0];
	[bezier_path addClip];

	// Fill draw.
	if (pr_state == DisplayServerEnums::PROGRESS_STATE_INDETERMINATE) {
		rect.size.width /= 5.0;
		pr_offset += rect.size.width / 10.0;
		if (pr_offset > self.bounds.size.width - rect.size.width) {
			pr_offset = 0.f;
		}
		rect.origin.x += pr_offset;
	} else {
		rect.size.width = Math::floor(rect.size.width * pr_value);
	}
	if (pr_state == DisplayServerEnums::PROGRESS_STATE_ERROR) {
		[[NSColor colorWithSRGBRed:1.0 green:0.2 blue:0.2 alpha:1.0] set];
	} else if (pr_state == DisplayServerEnums::PROGRESS_STATE_PAUSED) {
		[[NSColor colorWithSRGBRed:1.0 green:1.0 blue:0.2 alpha:1.0] set];
	} else {
		[[NSColor colorWithSRGBRed:0.2 green:0.6 blue:1.0 alpha:1.0] set];
	}
	NSRectFill(rect);
}

@end

// === Merged from platform/macos/display_server_macos_embedded.mm ===
#ifdef TOOLS_ENABLED
/*  display_server_macos_embedded.mm                                      */
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

#import "servers/display/display_server.h"


#import "core/os/os.h"
#import "core/io/net_socket.h"
#import "core/os/os.h"
#import "servers/display/display_server.h"

#if defined(GLES3_ENABLED)
#import "servers/display/display_server.h"


#endif

#if defined(RD_ENABLED)
#import "servers/display/display_server.h"

#if defined(VULKAN_ENABLED)
#import "servers/display/display_server.h"
#endif // VULKAN_ENABLED
#if defined(METAL_ENABLED)
#import "servers/display/display_server.h"
#endif
#endif // RD_ENABLED

// Keep Quartz after rendering includes, as it includes system GL.h
// which clashes with GLAD.
#import "servers/display/display_server.h"

DisplayServerMacOSEmbedded::DisplayServerMacOSEmbedded(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, Error &r_error) {
	EmbeddedDebugger::initialize(this);

	r_error = OK; // default to OK

	NativeMenu::initialize();

	Input::set_event_dispatch_function(_dispatch_input_events);

	rendering_driver = p_rendering_driver;

#if defined(RD_ENABLED)
#if defined(VULKAN_ENABLED)
#if defined(__x86_64__)
	bool fallback_to_vulkan = GLOBAL_GET("rendering/rendering_device/fallback_to_vulkan");
	if (!fallback_to_vulkan) {
		WARN_PRINT("Metal is not supported on Intel Macs, switching to Vulkan.");
	}
	// Metal rendering driver not available on Intel.
	if (rendering_driver == "metal") {
		rendering_driver = "vulkan";
		OS::set_current_rendering_driver_name(rendering_driver, OS::RENDERING_SOURCE_FALLBACK);
	}
#endif
	if (rendering_driver == "vulkan") {
		rendering_context = memnew(RenderingContextDriverVulkan(RenderingContextDriverVulkan::SURFACE_PLATFORM));
	}
#endif
#if defined(METAL_ENABLED)
	if (rendering_driver == "metal") {
		rendering_context = memnew(RenderingContextDriverMetal);
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
				r_error = ERR_CANT_CREATE;
				ERR_FAIL_MSG("Could not initialize " + rendering_driver);
			}
		}
	}
#endif

#if defined(GLES3_ENABLED)
	if (rendering_driver == "opengl3_angle") {
		WARN_PRINT("ANGLE not supported for embedded display, switching to native OpenGL.");
		rendering_driver = "opengl3";
		OS::set_current_rendering_driver_name(rendering_driver, OS::RENDERING_SOURCE_FALLBACK);
	}

	if (rendering_driver == "opengl3") {
		gl_manager = memnew(GLManagerEmbedded);
		if (gl_manager->initialize() != OK) {
			memdelete(gl_manager);
			gl_manager = nullptr;
			r_error = ERR_UNAVAILABLE;
			ERR_FAIL_MSG("Could not initialize native OpenGL.");
		}
		layer = [CALayer new];
		// OpenGL content is flipped, so it must be transformed.
		layer.anchorPoint = CGPointMake(0, 0);
		layer.transform = CATransform3DMakeScale(1.0, -1.0, 1.0);

		Size2i render_size = _source_to_render_size(p_resolution);
		Error err = gl_manager->window_create(window_id_counter, layer, render_size.width, render_size.height);
		if (err != OK) {
			ERR_FAIL_MSG("Could not create OpenGL context.");
		}
		gl_manager->set_vsync_enabled(p_vsync_mode != DisplayServerEnums::VSYNC_DISABLED);
	}
#endif

#if defined(RD_ENABLED)
	if (rendering_context) {
		layer = [CAMetalLayer new];
		layer.anchorPoint = CGPointMake(0, 1);

		union {
#ifdef VULKAN_ENABLED
			RenderingContextDriverVulkan::WindowPlatformData vulkan;
#endif
#ifdef METAL_ENABLED
			RenderingContextDriverMetal::WindowPlatformData metal;
#endif
		} wpd;
#ifdef VULKAN_ENABLED
		if (rendering_driver == "vulkan") {
			wpd.vulkan.layer_ptr = (void *const *)&layer;
		}
#endif
#ifdef METAL_ENABLED
		if (rendering_driver == "metal") {
			wpd.metal.layer = (__bridge CA::MetalLayer *)layer;
		}
#endif
		Error err = rendering_context->window_create(window_id_counter, &wpd);
		ERR_FAIL_COND_MSG(err != OK, vformat("Can't create a %s context", rendering_driver));

		Size2i render_size = _source_to_render_size(p_resolution);
		rendering_context->window_set_size(window_id_counter, render_size.width, render_size.height);
		rendering_context->window_set_vsync_mode(window_id_counter, p_vsync_mode);
	}
#endif

#if defined(GLES3_ENABLED)
	if (rendering_driver == "opengl3") {
		RasterizerGLES3::make_current(true);
	}
	if (rendering_driver == "opengl3_angle") {
		RasterizerGLES3::make_current(false);
	}
#endif
#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_device = memnew(RenderingDevice);
		rendering_device->initialize(rendering_context, DisplayServerEnums::MAIN_WINDOW_ID);
		rendering_device->screen_create(DisplayServerEnums::MAIN_WINDOW_ID);

		RendererCompositorRD::make_current();
	}
#endif

	CGFloat display_scale = state.screen_max_scale;
	CGFloat render_scale = screen_get_max_scale();
	layer.contentsScale = render_scale;
	layer.magnificationFilter = kCAFilterNearest;
	layer.minificationFilter = kCAFilterNearest;
	transparent = ((p_flags & DisplayServerEnums::WINDOW_FLAG_TRANSPARENT_BIT) == DisplayServerEnums::WINDOW_FLAG_TRANSPARENT_BIT);
	layer.opaque = !(OS::is_layered_allowed() && transparent);
	layer.actions = @{ @"contents" : [NSNull null] }; // Disable implicit animations for contents.
	// AppKit frames, bounds and positions are always in points.
	CGRect bounds = CGRectMake(0, 0, p_resolution.width, p_resolution.height);
	bounds = CGRectApplyAffineTransform(bounds, CGAffineTransformInvert(CGAffineTransformMakeScale(display_scale, display_scale)));
	layer.bounds = bounds;

	CGSConnectionID connection_id = CGSMainConnectionID();
	ca_context = [CAContext contextWithCGSConnection:connection_id options:@{ kCAContextCIFilterBehavior : @"ignore" }];
	ca_context.layer = layer;

}

DisplayServerMacOSEmbedded::~DisplayServerMacOSEmbedded() {
	NativeMenu::finalize();

	EmbeddedDebugger::deinitialize();

#if defined(GLES3_ENABLED)
	if (gl_manager) {
		memdelete(gl_manager);
		gl_manager = nullptr;
	}
#endif

#if defined(RD_ENABLED)
	if (rendering_device) {
		memdelete(rendering_device);
		rendering_device = nullptr;
	}

	if (rendering_context) {
		memdelete(rendering_context);
		rendering_context = nullptr;
	}
#endif
}

DisplayServerDriver *DisplayServerMacOSEmbedded::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t /* p_parent_window */, Error &r_error) {
	return memnew(DisplayServerMacOSEmbedded(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, r_error));
}

Vector<String> DisplayServerMacOSEmbedded::get_rendering_drivers_func() {
	Vector<String> drivers;

#if defined(VULKAN_ENABLED)
	drivers.push_back("vulkan");
#endif
#if defined(METAL_ENABLED)
	drivers.push_back("metal");
#endif
#if defined(GLES3_ENABLED)
	drivers.push_back("opengl3");
#endif

	return drivers;
}

void DisplayServerMacOSEmbedded::register_embedded_driver() {
	DisplayServer::register_create_function("embedded", create_func, get_rendering_drivers_func);
}

// MARK: - Mouse

void DisplayServerMacOSEmbedded::_mouse_apply_mode(DisplayServerEnums::MouseMode p_prev_mode, DisplayServerEnums::MouseMode p_new_mode) {
}

void DisplayServerMacOSEmbedded::warp_mouse(const Point2i &p_position) {
	_THREAD_SAFE_METHOD_
	Input::set_mouse_position(p_position);
	// Convert from game pixels to points for the editor.
	float inv_scale = 1.0f / screen_get_max_scale();
}

Point2i DisplayServerMacOSEmbedded::mouse_get_position() const {
	_THREAD_SAFE_METHOD_

	const NSPoint mouse_pos = [NSEvent mouseLocation];
	const float scale = screen_get_max_scale();

	for (NSScreen *screen in [NSScreen screens]) {
		NSRect frame = [screen frame];
		if (NSMouseInRect(mouse_pos, frame, NO)) {
			Vector2i pos = Vector2i((int)mouse_pos.x, (int)mouse_pos.y);
			pos *= scale;
			// TODO(sgc): fix this
			// pos -= _get_screens_origin();
			pos.y *= -1;
			return pos;
		}
	}
	return Vector2i();
}

BitField<MouseButtonMask> DisplayServerMacOSEmbedded::mouse_get_button_state() const {
	BitField<MouseButtonMask> last_button_state = MouseButtonMask::NONE;

	NSUInteger buttons = [NSEvent pressedMouseButtons];
	if (buttons & (1 << 0)) {
		last_button_state.set_flag(MouseButtonMask::LEFT);
	}
	if (buttons & (1 << 1)) {
		last_button_state.set_flag(MouseButtonMask::RIGHT);
	}
	if (buttons & (1 << 2)) {
		last_button_state.set_flag(MouseButtonMask::MIDDLE);
	}
	if (buttons & (1 << 3)) {
		last_button_state.set_flag(MouseButtonMask::MB_XBUTTON1);
	}
	if (buttons & (1 << 4)) {
		last_button_state.set_flag(MouseButtonMask::MB_XBUTTON2);
	}
	return last_button_state;
}

// MARK: Events

void DisplayServerMacOSEmbedded::window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	window_resize_callbacks[p_window] = p_callable;
}

void DisplayServerMacOSEmbedded::window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	window_event_callbacks[p_window] = p_callable;
}
void DisplayServerMacOSEmbedded::window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	input_event_callbacks[p_window] = p_callable;
}

void DisplayServerMacOSEmbedded::window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	input_text_callbacks[p_window] = p_callable;
}

void DisplayServerMacOSEmbedded::window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	// Not supported
}

void DisplayServerMacOSEmbedded::process_events() {
	Input::flush_buffered_events();
}

void DisplayServerMacOSEmbedded::_dispatch_input_events(const Ref<InputEvent> &p_event) {
	Ref<InputEventFromWindow> event_from_window = p_event;
	DisplayServerEnums::WindowID window_id = DisplayServerEnums::INVALID_WINDOW_ID;
	if (event_from_window.is_valid()) {
		window_id = event_from_window->get_window_id();
	}
	if (DisplayServerMacOSEmbedded *ds = DisplayServer::driver_as<DisplayServerMacOSEmbedded>()) {
		ds->send_input_event(p_event, window_id);
	}
}

void DisplayServerMacOSEmbedded::send_input_event(const Ref<InputEvent> &p_event, DisplayServerEnums::WindowID p_id) const {
	if (p_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		const Callable *cb = input_event_callbacks.getptr(p_id);
		if (cb) {
			_window_callback(*cb, p_event);
		}
	} else {
		for (const KeyValue<DisplayServerEnums::WindowID, Callable> &E : input_event_callbacks) {
			_window_callback(E.value, p_event);
		}
	}
}

void DisplayServerMacOSEmbedded::send_input_text(const String &p_text, DisplayServerEnums::WindowID p_id) const {
	const Callable *cb = input_text_callbacks.getptr(p_id);
	if (cb) {
		_window_callback(*cb, p_text);
	}
}

void DisplayServerMacOSEmbedded::send_window_event_by_id(DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_id) const {
	const Callable *cb = window_event_callbacks.getptr(p_id);
	if (cb) {
		_window_callback(*cb, int(p_event));
	}
}

void DisplayServerMacOSEmbedded::_window_callback(const Callable &p_callable, const Variant &p_arg) const {
	if (p_callable.is_valid()) {
		p_callable.call(p_arg);
	}
}

// MARK: -

bool DisplayServerMacOSEmbedded::has_feature(DisplayServerEnums::Feature p_feature) const {
	switch (p_feature) {
#ifndef DISABLE_DEPRECATED
		case DisplayServerEnums::FEATURE_GLOBAL_MENU: {
			return NativeMenu::is_initialized() && NativeMenu::has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
		} break;
#endif
		case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
		case DisplayServerEnums::FEATURE_IME:
		case DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE:
			// case DisplayServerEnums::FEATURE_HIDPI:
			// case DisplayServerEnums::FEATURE_ICON:
			// case DisplayServerEnums::FEATURE_MOUSE:
		case DisplayServerEnums::FEATURE_HDR_OUTPUT:
		case DisplayServerEnums::FEATURE_MOUSE_WARP:
			// case DisplayServerEnums::FEATURE_NATIVE_DIALOG:
			// case DisplayServerEnums::FEATURE_NATIVE_ICON:
			// case DisplayServerEnums::FEATURE_WINDOW_TRANSPARENCY:
		case DisplayServerEnums::FEATURE_CLIPBOARD:
			// case DisplayServerEnums::FEATURE_KEEP_SCREEN_ON:
			// case DisplayServerEnums::FEATURE_ORIENTATION:
			// case DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD:
		case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH:
			// case DisplayServerEnums::FEATURE_TOUCHSCREEN:
			return true;
		default:
			return false;
	}
}

String DisplayServerMacOSEmbedded::get_name() const {
	return "embedded";
}

int DisplayServerMacOSEmbedded::get_screen_count() const {
	return 1;
}

Point2i DisplayServerMacOSEmbedded::screen_get_position(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Point2i());

	return Point2i(0, 0);
}

Size2i DisplayServerMacOSEmbedded::screen_get_size(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Size2i());

	return window_get_size(DisplayServerEnums::MAIN_WINDOW_ID);
}

Rect2i DisplayServerMacOSEmbedded::screen_get_usable_rect(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	return Rect2i(screen_get_position(p_screen), screen_get_size(p_screen));
}

int DisplayServerMacOSEmbedded::screen_get_dpi(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 72);

	return 96;
}

float DisplayServerMacOSEmbedded::screen_get_scale(int p_screen) const {
	_THREAD_SAFE_METHOD_

	if (!OS::is_hidpi_allowed()) {
		return 1.0f;
	}

	switch (p_screen) {
		case DisplayServerEnums::SCREEN_WITH_MOUSE_FOCUS:
		case DisplayServerEnums::SCREEN_WITH_KEYBOARD_FOCUS:
		case DisplayServerEnums::SCREEN_PRIMARY:
		case DisplayServerEnums::SCREEN_OF_MAIN_WINDOW:
		case 0:
			return state.screen_window_scale;
		default:
			return 1.0f;
	}
}

Vector<DisplayServerEnums::WindowID> DisplayServerMacOSEmbedded::get_window_list() const {
	Vector<DisplayServerEnums::WindowID> list;
	list.push_back(DisplayServerEnums::MAIN_WINDOW_ID);
	return list;
}

DisplayServerEnums::WindowID DisplayServerMacOSEmbedded::get_window_at_screen_position(const Point2i &p_position) const {
	return DisplayServerEnums::MAIN_WINDOW_ID;
}

void DisplayServerMacOSEmbedded::window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window) {
	window_attached_instance_id[p_window] = p_instance;
}

ObjectID DisplayServerMacOSEmbedded::window_get_attached_instance_id(DisplayServerEnums::WindowID p_window) const {
	return window_attached_instance_id[p_window];
}

void DisplayServerMacOSEmbedded::window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window) {
	// Not supported
}

int DisplayServerMacOSEmbedded::window_get_current_screen(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, DisplayServerEnums::INVALID_SCREEN);

	return 0;
}

void DisplayServerMacOSEmbedded::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window) {
	// Not supported
}

Point2i DisplayServerMacOSEmbedded::window_get_position(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

Point2i DisplayServerMacOSEmbedded::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

void DisplayServerMacOSEmbedded::window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window) {
	// Probably not supported for single window iOS app
}

void DisplayServerMacOSEmbedded::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) {
	// Not supported
}

void DisplayServerMacOSEmbedded::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported
}

Size2i DisplayServerMacOSEmbedded::window_get_max_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerMacOSEmbedded::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported
}

Size2i DisplayServerMacOSEmbedded::window_get_min_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerMacOSEmbedded::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	print_line("Embedded window can't be resized.");
}

void DisplayServerMacOSEmbedded::_window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	[CATransaction begin];
	[CATransaction setDisableActions:YES];

	CGFloat display_scale = state.screen_max_scale;
	CGFloat render_scale = screen_get_max_scale();
	CGRect bounds = CGRectMake(0, 0, p_size.width, p_size.height);
	bounds = CGRectApplyAffineTransform(bounds, CGAffineTransformInvert(CGAffineTransformMakeScale(display_scale, display_scale)));
	layer.bounds = bounds;
	layer.contentsScale = render_scale;

	Size2i render_size = _source_to_render_size(p_size);
#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_context->window_set_size(p_window, render_size.width, render_size.height);
	}
#endif
#if defined(GLES3_ENABLED)
	if (gl_manager) {
		gl_manager->window_resize(p_window, render_size.width, render_size.height);
	}
#endif
	[CATransaction commit];

	Callable *cb = window_resize_callbacks.getptr(p_window);
	if (cb) {
		Variant resize_rect = Rect2i(Point2i(), render_size);
		_window_callback(window_resize_callbacks[p_window], resize_rect);
	}
}

Size2i DisplayServerMacOSEmbedded::window_get_size(DisplayServerEnums::WindowID p_window) const {
#if defined(RD_ENABLED)
	if (rendering_context) {
		RenderingContextDriver::SurfaceID surface = rendering_context->surface_get_from_window(p_window);
		ERR_FAIL_COND_V_MSG(surface == 0, Size2i(), "Invalid window ID");
		uint32_t width = rendering_context->surface_get_width(surface);
		uint32_t height = rendering_context->surface_get_height(surface);
		return Size2i(width, height);
	}
#endif
#ifdef GLES3_ENABLED
	if (gl_manager) {
		return gl_manager->window_get_size(p_window);
	}
#endif
	return Size2i();
}

Size2i DisplayServerMacOSEmbedded::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return window_get_size(p_window);
}

void DisplayServerMacOSEmbedded::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window) {
	// Not supported
}

DisplayServerEnums::WindowMode DisplayServerMacOSEmbedded::window_get_mode(DisplayServerEnums::WindowID p_window) const {
	return DisplayServerEnums::WindowMode::WINDOW_MODE_WINDOWED;
}

bool DisplayServerMacOSEmbedded::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window) const {
	return false;
}

void DisplayServerMacOSEmbedded::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window) {
	if (p_flag == DisplayServerEnums::WINDOW_FLAG_TRANSPARENT && p_window == DisplayServerEnums::MAIN_WINDOW_ID) {
		transparent = p_enabled;
		layer.opaque = !(OS::is_layered_allowed() && transparent);
	}
}

bool DisplayServerMacOSEmbedded::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window) const {
	if (p_flag == DisplayServerEnums::WINDOW_FLAG_TRANSPARENT && p_window == DisplayServerEnums::MAIN_WINDOW_ID) {
		return transparent;
	}
	return false;
}

void DisplayServerMacOSEmbedded::window_request_attention(DisplayServerEnums::WindowID p_window) {
	// Not supported
}

void DisplayServerMacOSEmbedded::window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window) {
	// Not supported.
}

void DisplayServerMacOSEmbedded::window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window) {
	// Not supported.
}

void DisplayServerMacOSEmbedded::window_move_to_foreground(DisplayServerEnums::WindowID p_window) {
	// Not supported
}

bool DisplayServerMacOSEmbedded::window_is_focused(DisplayServerEnums::WindowID p_window) const {
	return true;
}

float DisplayServerMacOSEmbedded::screen_get_max_scale() const {
	if (!OS::is_hidpi_allowed()) {
		return 1.0f;
	}
	return state.screen_max_scale;
}

Size2i DisplayServerMacOSEmbedded::_source_to_render_size(const Size2i &p_source_size) const {
	CGFloat display_scale = state.screen_max_scale;
	CGFloat render_scale = screen_get_max_scale();
	return Size2i((int)(p_source_size.width / display_scale * render_scale),
			(int)(p_source_size.height / display_scale * render_scale));
}

bool DisplayServerMacOSEmbedded::window_can_draw(DisplayServerEnums::WindowID p_window) const {
	return true;
}

bool DisplayServerMacOSEmbedded::can_any_window_draw() const {
	return true;
}

void DisplayServerMacOSEmbedded::window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window) {
}

void DisplayServerMacOSEmbedded::window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window) {
	if (p_pos == ime_last_position) {
		return;
	}
	ime_last_position = p_pos;
}

DisplayServerMacOSBase::HDROutput &DisplayServerMacOSEmbedded::_get_hdr_output(DisplayServerEnums::WindowID p_window) {
	return hdr_output;
}

const DisplayServerMacOSBase::HDROutput &DisplayServerMacOSEmbedded::_get_hdr_output(DisplayServerEnums::WindowID p_window) const {
	return hdr_output;
}

void DisplayServerMacOSEmbedded::window_get_edr_values(DisplayServerEnums::WindowID p_window, CGFloat *r_max_potential_edr_value, CGFloat *r_max_edr_value) const {
	_THREAD_SAFE_METHOD_

#define SET_VAL(v, val) \
	if (v) { \
		*v = val; \
	}

	SET_VAL(r_max_potential_edr_value, state.screen_max_edr);
	SET_VAL(r_max_edr_value, state.screen_max_potential_edr);

#undef SET_VAL
}

void DisplayServerMacOSEmbedded::update_screen_parameters() {
	if (hdr_output.requested) {
		_update_hdr_output(DisplayServerEnums::MAIN_WINDOW_ID, hdr_output);
	}
}

void DisplayServerMacOSEmbedded::set_state(const DisplayServerMacOSEmbeddedState &p_state) {
	if (state == p_state) {
		return;
	}

	uint32_t old_display_id = state.display_id;
	float old_scale = state.screen_max_scale;

	state = p_state;

	if (state.display_id != old_display_id) {
#if defined(GLES3_ENABLED)
		if (gl_manager) {
			gl_manager->set_display_id(state.display_id);
		}
#endif
	}
	if (state.screen_max_scale != old_scale) {
		// Recover source pixel size from current bounds using the old display scale.
		CGSize bounds_size = layer.bounds.size;
		Size2i source_size((int)(bounds_size.width * old_scale), (int)(bounds_size.height * old_scale));
		_window_set_size(source_size, DisplayServerEnums::MAIN_WINDOW_ID);
	}
	if (hdr_output.requested) {
		_update_hdr_output(DisplayServerEnums::MAIN_WINDOW_ID, hdr_output);
	}
}

void DisplayServerMacOSEmbedded::window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window) {
#if defined(GLES3_ENABLED)
	if (gl_manager) {
		gl_manager->set_vsync_enabled(p_vsync_mode != DisplayServerEnums::VSYNC_DISABLED);
	}
#endif

#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_context->window_set_vsync_mode(p_window, p_vsync_mode);
	}
#endif
}

DisplayServerEnums::VSyncMode DisplayServerMacOSEmbedded::window_get_vsync_mode(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_
#if defined(GLES3_ENABLED)
	if (gl_manager) {
		return (gl_manager->is_vsync_enabled() ? DisplayServerEnums::VSyncMode::VSYNC_ENABLED : DisplayServerEnums::VSyncMode::VSYNC_DISABLED);
	}
#endif
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_vsync_mode(p_window);
	}
#endif
	return DisplayServerEnums::VSYNC_ENABLED;
}

void DisplayServerMacOSEmbedded::cursor_set_shape(DisplayServerEnums::CursorShape p_shape) {
	cursor_shape = p_shape;
}

void DisplayServerMacOSEmbedded::cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape, const Vector2 &p_hotspot) {
	PackedByteArray data;
	if (p_cursor.is_valid()) {
		Ref<Image> image = _get_cursor_image_from_resource(p_cursor, p_hotspot);
		if (image.is_valid()) {
			data = image->save_png_to_buffer();
		}
	}
}

void DisplayServerMacOSEmbedded::swap_buffers() {
#ifdef GLES3_ENABLED
	if (gl_manager) {
		gl_manager->swap_buffers();
	}
#endif
}

void DisplayServerMacOSEmbeddedState::serialize(PackedByteArray &r_data) {
	r_data.resize(32);

	uint8_t *data = r_data.ptrw();
	data += encode_float(screen_max_scale, data);
	data += encode_float(screen_dpi, data);
	data += encode_float(screen_window_scale, data);
	data += encode_uint32(display_id, data);
	data += encode_double(screen_max_edr, data);
	data += encode_double(screen_max_potential_edr, data);

	// Assert we had enough space.
	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

Error DisplayServerMacOSEmbeddedState::deserialize(const PackedByteArray &p_data) {
	const uint8_t *data = p_data.ptr();

	screen_max_scale = decode_float(data);
	data += sizeof(float);
	screen_dpi = decode_float(data);
	data += sizeof(float);
	screen_window_scale = decode_float(data);
	data += sizeof(float);
	display_id = decode_uint32(data);
	data += sizeof(uint32_t);
	screen_max_edr = decode_double(data);
	data += sizeof(double);
	screen_max_potential_edr = decode_double(data);

	return OK;
}

// === Merged from platform/macos/embedded_debugger.mm ===
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

#include "servers/display/display_server.h"


#include "core/debugger/engine_debugger.h"
#include "core/os/os.h"

void EmbeddedDebugger::initialize(DisplayServerMacOSEmbedded* p_ds)
{
#ifdef DEBUG_ENABLED
	if (EngineDebugger::is_active() && !data) {
		data = memnew(Data);
		data->ds = p_ds;
		_init_parse_message_handlers();
		EngineDebugger::register_message_capture(
			"embed", EngineDebugger::Capture(nullptr, EmbeddedDebugger::parse_message));
	}
#endif
}

void EmbeddedDebugger::deinitialize()
{
#ifdef DEBUG_ENABLED
	if (data) {
		memdelete(data);
		data = nullptr;
	}
#endif
}

#ifdef DEBUG_ENABLED
void EmbeddedDebugger::_init_parse_message_handlers()
{
	data->parse_message_handlers["window_size"] = &EmbeddedDebugger::_msg_window_size;
	data->parse_message_handlers["mouse_set_mode"] = &EmbeddedDebugger::_msg_mouse_set_mode;
	data->parse_message_handlers["event"] = &EmbeddedDebugger::_msg_event;
	data->parse_message_handlers["win_event"] = &EmbeddedDebugger::_msg_win_event;
	data->parse_message_handlers["notification"] = &EmbeddedDebugger::_msg_notification;
	data->parse_message_handlers["ime_update"] = &EmbeddedDebugger::_msg_ime_update;
	data->parse_message_handlers["ds_state"] = &EmbeddedDebugger::_msg_ds_state;
}

Error EmbeddedDebugger::_msg_window_size(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'window_size' message.");
	Size2i size = p_args[0];
	data->ds->_window_set_size(size);
	return OK;
}

Error EmbeddedDebugger::_msg_mouse_set_mode(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'mouse_set_mode' message.");
	DisplayServerEnums::MouseMode mode = p_args[0];
	data->ds->mouse_set_mode(mode);
	return OK;
}

Error EmbeddedDebugger::_msg_event(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'event' message.");
	if (!Input::is_initialized()) {
		// Ignore if we've received an event before the process has initialized.
		return OK;
	}

	PackedByteArray buf = p_args[0];
	Ref<InputEvent> event;
	decode_input_event(buf, event);

	// Events arrive in points (normalized by the editor). Scale to the
	// game's pixel coordinate space.
	float scale = data->ds->screen_get_max_scale();

	{
		Ref<InputEventMouse> e = event;
		if (e.is_valid()) {
			e->set_position(e->get_position() * scale);
			e->set_global_position(e->get_global_position() * scale);
			Ref<InputEventMouseMotion> mm = e;
			if (mm.is_valid()) {
				mm->set_relative(mm->get_relative() * scale);
			}
			Input::set_mouse_position(e->get_position());
		}
	}

	{
		Ref<InputEventMagnifyGesture> e = event;
		if (e.is_valid()) {
			e->set_position(e->get_position() * scale);
			Input::set_mouse_position(e->get_position());
		}
	}

	{
		Ref<InputEventPanGesture> e = event;
		if (e.is_valid()) {
			e->set_position(e->get_position() * scale);
			Input::set_mouse_position(e->get_position());
		}
	}

	if (event.is_valid()) {
		Input::parse_input_event(event.ptr());
	}

	return OK;
}

Error EmbeddedDebugger::_msg_win_event(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'win_event' message.");
	DisplayServerEnums::WindowEvent win_event = p_args[0];
	data->ds->send_window_event_by_id(win_event, DisplayServerEnums::MAIN_WINDOW_ID);
	if (win_event == DisplayServerEnums::WindowEvent::WINDOW_EVENT_MOUSE_EXIT) {
		Input::release_pressed_events();
	}
	return OK;
}

Error EmbeddedDebugger::_msg_ime_update(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 2, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'ime_update' message.");
	String ime_text = p_args[0];
	Vector2i ime_selection = p_args[1];
	data->ds->update_im_text(ime_selection, ime_text);
	return OK;
}

Error EmbeddedDebugger::_msg_notification(const Array& p_args)
{
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'notification' message.");
	int notification = p_args[0];
	if (MainLoop::is_active()) {
		MainLoop::notification(notification);
	}
	return OK;
}

Error EmbeddedDebugger::_msg_ds_state(const Array& p_args)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(p_args.size() != 1, ERR_INVALID_PARAMETER,
		"Invalid number of arguments for 'ds_state' message.");
	PackedByteArray buf = p_args[0];
	DisplayServerMacOSEmbeddedState state;
	state.deserialize(buf);
	data->ds->set_state(state);
	return OK;
}

Error EmbeddedDebugger::parse_message(
	void* p_user, const String& p_msg, const Array& p_args, bool& r_captured)
{
	r_captured = true;
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);

	ParseMessageFunc* fn_ptr = data->parse_message_handlers.getptr(p_msg);
	if (fn_ptr) {
		return (*fn_ptr)(p_args);
	}
	else {
		// Any other messages with this prefix should be ignored.
		WARN_PRINT("Unknown message: " + p_msg);
		return ERR_SKIP;
	}
}
#endif

// === Merged from platform/macos/embedded_gl_manager.mm ===
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

#import "servers/display/display_server.h"

#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#import "servers/display/display_server.h"


#import <QuartzCore/QuartzCore.h>
#include <dlfcn.h>

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations") // OpenGL is deprecated in macOS 10.14.

Error GLManagerEmbedded::create_context(GLWindow &p_win) {
	NSOpenGLPixelFormatAttribute attributes[] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAClosestPolicy,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFAColorSize, 32,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		0
	};

	NSOpenGLPixelFormat *pixel_format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	ERR_FAIL_NULL_V(pixel_format, ERR_CANT_CREATE);

	p_win.context = [[NSOpenGLContext alloc] initWithFormat:pixel_format shareContext:shared_context];
	ERR_FAIL_NULL_V(p_win.context, ERR_CANT_CREATE);
	if (shared_context == nullptr) {
		shared_context = p_win.context;
	}

	[p_win.context makeCurrentContext];

	return OK;
}

Error GLManagerEmbedded::window_create(DisplayServerEnums::WindowID p_window_id, CALayer *p_layer, int p_width, int p_height) {
	GLWindow win;
	win.layer = p_layer;
	win.width = 0;
	win.height = 0;

	if (create_context(win) != OK) {
		return FAILED;
	}

	windows[p_window_id] = win;
	window_make_current(p_window_id);

	return OK;
}

void GLManagerEmbedded::window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height) {
	GLWindowElement *el = windows.find(p_window_id);
	ERR_FAIL_NULL_MSG(el, "Window resize failed: window does not exist.");

	GLWindow &win = el->get();

	if (win.width == (uint32_t)p_width && win.height == (uint32_t)p_height) {
		return;
	}

	win.width = (uint32_t)p_width;
	win.height = (uint32_t)p_height;

	win.destroy_framebuffers();

	for (FrameBuffer &fb : win.framebuffers) {
		NSDictionary *surfaceProps = @{
			(NSString *)kIOSurfaceWidth : @(p_width),
			(NSString *)kIOSurfaceHeight : @(p_height),
			(NSString *)kIOSurfaceBytesPerElement : @(4),
			(NSString *)kIOSurfacePixelFormat : @(kCVPixelFormatType_32BGRA),
		};
		fb.surface = IOSurfaceCreate((__bridge CFDictionaryRef)surfaceProps);
		if (fb.surface == nullptr) {
			ERR_PRINT(vformat("Failed to create IOSurface: width=%d, height=%d", p_width, p_height));
			win.destroy_framebuffers();
			return;
		}

		glGenTextures(1, &fb.tex);
		glBindTexture(GL_TEXTURE_RECTANGLE, fb.tex);

		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		CGLError err = CGLTexImageIOSurface2D(CGLGetCurrentContext(),
				GL_TEXTURE_RECTANGLE,
				GL_RGBA,
				p_width,
				p_height,
				GL_BGRA,
				GL_UNSIGNED_INT_8_8_8_8_REV,
				fb.surface,
				0);
		if (err != kCGLNoError) {
			String err_string = String(CGLErrorString(err));
			ERR_PRINT(vformat("CGLTexImageIOSurface2D failed (%d): %s", err, err_string));
			win.destroy_framebuffers();
			return;
		}

		glGenFramebuffers(1, &fb.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, fb.tex, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			ERR_PRINT("Unable to create framebuffer from IOSurface texture.");
			win.destroy_framebuffers();
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	}
	win.current_fb = 0;
	GLES3::TextureStorage::system_fbo = win.framebuffers[win.current_fb].fbo;
	win.is_valid = true;
}

void GLManagerEmbedded::GLWindow::destroy_framebuffers() {
	is_valid = false;
	GLES3::TextureStorage::system_fbo = 0;

	for (FrameBuffer &fb : framebuffers) {
		if (fb.fbo) {
			glDeleteFramebuffers(1, &fb.fbo);
			fb.fbo = 0;
		}

		if (fb.tex) {
			glDeleteTextures(1, &fb.tex);
			fb.tex = 0;
		}

		if (fb.surface) {
			IOSurfaceRef old_surface = fb.surface;
			fb.surface = nullptr;
			CFRelease(old_surface);
		}
	}
}

Size2i GLManagerEmbedded::window_get_size(DisplayServerEnums::WindowID p_window_id) const {
	const GLWindowElement *el = windows.find(p_window_id);
	if (el == nullptr) {
		return Size2i();
	}

	const GLWindow &win = el->value();
	return Size2i(win.width, win.height);
}

void GLManagerEmbedded::window_destroy(DisplayServerEnums::WindowID p_window_id) {
	GLWindowElement *el = windows.find(p_window_id);
	if (el == nullptr) {
		return;
	}

	if (current_window == p_window_id) {
		current_window = DisplayServerEnums::INVALID_WINDOW_ID;
	}

	windows.erase(el);
}

void GLManagerEmbedded::release_current() {
	if (current_window == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	[NSOpenGLContext clearCurrentContext];
	current_window = DisplayServerEnums::INVALID_WINDOW_ID;
}

void GLManagerEmbedded::window_make_current(DisplayServerEnums::WindowID p_window_id) {
	if (current_window == p_window_id) {
		return;
	}

	const GLWindowElement *el = windows.find(p_window_id);
	if (el == nullptr) {
		return;
	}

	const GLWindow &win = el->value();
	[win.context makeCurrentContext];

	current_window = p_window_id;
}

void GLManagerEmbedded::swap_buffers() {
	GLWindow &win = windows[current_window];
	[win.context flushBuffer];

	static bool last_valid = false;
	if (!win.is_valid) {
		if (last_valid) {
			ERR_PRINT("GLWindow framebuffers are invalid.");
			last_valid = false;
		}
		return;
	}
	last_valid = true;

	if (display_link_running) {
		dispatch_semaphore_wait(display_semaphore, DISPATCH_TIME_FOREVER);
	}

	[CATransaction begin];
	[CATransaction setDisableActions:YES];
	win.layer.contents = (__bridge id)win.framebuffers[win.current_fb].surface;
	[CATransaction commit];
	win.current_fb = (win.current_fb + 1) % BUFFER_COUNT;
	GLES3::TextureStorage::system_fbo = win.framebuffers[win.current_fb].fbo;
}

Error GLManagerEmbedded::initialize() {
	return framework_loaded ? OK : ERR_CANT_CREATE;
}

void GLManagerEmbedded::create_display_link() {
	DEV_ASSERT(display_link == nullptr);

	CVReturn err = CVDisplayLinkCreateWithCGDisplay(CGMainDisplayID(), &display_link);
	ERR_FAIL_COND_MSG(err != kCVReturnSuccess, "Failed to create display link.");

	__block dispatch_semaphore_t local_semaphore = display_semaphore;

	CVDisplayLinkSetOutputHandler(display_link, ^CVReturn(CVDisplayLinkRef p_display_link, const CVTimeStamp *p_now, const CVTimeStamp *p_output_time, CVOptionFlags p_flags, CVOptionFlags *p_flags_out) {
		dispatch_semaphore_signal(local_semaphore);
		return kCVReturnSuccess;
	});
}

void GLManagerEmbedded::release_display_link() {
	DEV_ASSERT(display_link != nullptr);
	if (CVDisplayLinkIsRunning(display_link)) {
		CVDisplayLinkStop(display_link);
	}
	CVDisplayLinkRelease(display_link);
	display_link = nullptr;
}

void GLManagerEmbedded::set_display_id(uint32_t p_display_id) {
	if (display_id == p_display_id) {
		return;
	}

	CVReturn err = CVDisplayLinkSetCurrentCGDisplay(display_link, static_cast<CGDirectDisplayID>(p_display_id));
	ERR_FAIL_COND_MSG(err != kCVReturnSuccess, "Failed to set display ID for display link.");
}

void GLManagerEmbedded::set_vsync_enabled(bool p_enabled) {
	if (p_enabled == vsync_enabled) {
		return;
	}

	vsync_enabled = p_enabled;

	if (vsync_enabled) {
		if (!CVDisplayLinkIsRunning(display_link)) {
			CVReturn err = CVDisplayLinkStart(display_link);
			ERR_FAIL_COND_MSG(err != kCVReturnSuccess, "Failed to start display link.");
			display_link_running = true;
		}
	} else {
		if (CVDisplayLinkIsRunning(display_link)) {
			CVReturn err = CVDisplayLinkStop(display_link);
			ERR_FAIL_COND_MSG(err != kCVReturnSuccess, "Failed to stop display link.");
			display_link_running = false;
		}
	}
}

GLManagerEmbedded::GLManagerEmbedded() {
	display_semaphore = dispatch_semaphore_create(BUFFER_COUNT);

	create_display_link();

	NSBundle *framework = [NSBundle bundleWithIdentifier:@"com.apple.opengl"];
	if ([framework load]) {
		void *library_handle = dlopen([framework.executablePath UTF8String], RTLD_NOW);
		if (library_handle) {
			CGLGetCurrentContext = (CGLGetCurrentContextPtr)dlsym(library_handle, "CGLGetCurrentContext");
			CGLTexImageIOSurface2D = (CGLTexImageIOSurface2DPtr)dlsym(library_handle, "CGLTexImageIOSurface2D");
			CGLErrorString = (CGLErrorStringPtr)dlsym(library_handle, "CGLErrorString");
			framework_loaded = CGLGetCurrentContext && CGLTexImageIOSurface2D && CGLErrorString;
		}
	}
}

GLManagerEmbedded::~GLManagerEmbedded() {
	release_display_link();
	release_current();
}

GODOT_CLANG_WARNING_POP

#endif // MACOS_ENABLED && GLES3_ENABLED
#endif // TOOLS_ENABLED

// === Merged from platform/macos/gl_manager_macos_angle.mm ===
#ifdef ANGLE_ENABLED
/*  gl_manager_macos_angle.mm                                             */
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

#import "servers/display/display_server.h"

#if defined(MACOS_ENABLED) && defined(GLES3_ENABLED)

#import <EGL/eglext_angle.h>

#include <cstdio>
#include <cstdlib>

const char *GLManagerANGLE_MacOS::_get_platform_extension_name() const {
	return "EGL_ANGLE_platform_angle";
}

EGLenum GLManagerANGLE_MacOS::_get_platform_extension_enum() const {
	return EGL_PLATFORM_ANGLE_ANGLE;
}

Vector<EGLAttrib> GLManagerANGLE_MacOS::_get_platform_display_attributes() const {
	Vector<EGLAttrib> ret;
	ret.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
	ret.push_back(EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE);
	ret.push_back(EGL_NONE);

	return ret;
}

EGLenum GLManagerANGLE_MacOS::_get_platform_api_enum() const {
	return EGL_OPENGL_ES_API;
}

Vector<EGLint> GLManagerANGLE_MacOS::_get_platform_context_attribs() const {
	Vector<EGLint> ret;
	ret.push_back(EGL_CONTEXT_CLIENT_VERSION);
	ret.push_back(3);
	ret.push_back(EGL_NONE);

	return ret;
}

#endif // MACOS_ENABLED && GLES3_ENABLED
#endif // ANGLE_ENABLED

// === Merged from platform/macos/godot_main_macos.mm & libgodot_macos.mm ===
#if !defined(LIBGODOT_ENABLED)
#import "servers/display/display_server.h"

#include "servers/display/display_server.h"

#if defined(ASAN_ENABLED)
#include <sys/resource.h>
#endif

__attribute__((visibility("default"))) int main(int argc, char **argv) {
	godot_init_profiler();

#if defined(VULKAN_ENABLED)
	setenv("MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE", "1", 1); // MoltenVK - enable full component swizzling support.
	setenv("MVK_CONFIG_SWAPCHAIN_MIN_MAG_FILTER_USE_NEAREST", "0", 1); // MoltenVK - use linear surface scaling. TODO: remove when full DPI scaling is implemented.
#endif

#if defined(ASAN_ENABLED)
	// Note: Set stack size to be at least 30 MB (vs 8 MB default) to avoid overflow, address sanitizer can increase stack usage up to 3 times.
	struct rlimit stack_lim = { 0x1E00000, 0x1E00000 };
	setrlimit(RLIMIT_STACK, &stack_lim);
#endif

	LocalVector<char *> args;
	args.resize(argc);
	uint32_t argsc = 0;

	int wait_for_debugger = 0; // wait 5 second by default
	bool is_embedded = false;
	bool is_headless = false;

	for (int i = 0; i < argc; i++) {
		if (strcmp("-NSDocumentRevisionsDebugMode", argv[i]) == 0) {
			// remove "-NSDocumentRevisionsDebugMode" and the next argument
			continue;
		}

		if (strcmp("--os-debug", argv[i]) == 0) {
			i++;
			wait_for_debugger = 5000; // wait 5 seconds by default
			if (i < argc && strncmp(argv[i], "--", 2) != 0) {
				wait_for_debugger = atoi(argv[i]);
			}
			continue;
		}

		if (strcmp("--embedded", argv[i]) == 0) {
			is_embedded = true;
		}
		for (size_t j = 0; j < std::size(OS_MacOS::headless_args); j++) {
			if (strcmp(OS_MacOS::headless_args[j], argv[i]) == 0) {
				is_headless = true;
				break;
			}
		}

		if (i < argc - 1 && strcmp("--display-driver", argv[i]) == 0 && strcmp("headless", argv[i + 1]) == 0) {
			is_headless = true;
		}

		args.ptr()[argsc] = argv[i];
		argsc++;
	}

	uint32_t remaining_args = argsc - 1;

	OS_MacOS *os = nullptr;
	if (is_embedded) {
#ifdef TOOLS_ENABLED
		os = memnew(OS_MacOS_Embedded(args[0], remaining_args, remaining_args > 0 ? &args[1] : nullptr));
#else
		WARN_PRINT("Embedded mode is not supported in release builds.");
		return EXIT_FAILURE;
#endif
	} else if (is_headless) {
		os = memnew(OS_MacOS_Headless(args[0], remaining_args, remaining_args > 0 ? &args[1] : nullptr));
	} else {
		os = memnew(OS_MacOS_NSApp(args[0], remaining_args, remaining_args > 0 ? &args[1] : nullptr));
	}

#ifdef TOOLS_ENABLED
	if (wait_for_debugger > 0) {
		os->wait_for_debugger(wait_for_debugger);
		print_verbose("Continuing execution.");
	}
#else
	if (wait_for_debugger > 0) {
		WARN_PRINT("--os-debug is not supported in release builds.");
	}
#endif

	if (is_embedded) {
		// No dock icon for the embedded process, as it is hosted in the Godot editor.
		ProcessSerialNumber psn = { 0, kCurrentProcess };
		(void)TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
	}

	// We must override main when testing is enabled.
	TEST_MAIN_OVERRIDE

	os->run();

	// Note: `os->run()` will never return if `OS_MacOS_NSApp` is used. Use `OS_MacOS_NSApp::cleanup()` for cleanup.

	int exit_code = os->get_exit_code();

	memdelete(os);

	godot_cleanup_profiler();
	return exit_code;
}
#else
#include "servers/display/display_server.h"

#include "servers/display/display_server.h"

static OS_MacOS *os = nullptr;

static GodotInstance *instance = nullptr;

GDExtensionObjectPtr libgodot_create_godot_instance(int p_argc, char *p_argv[], GDExtensionInitializationFunction p_init_func) {
	ERR_FAIL_COND_V_MSG(instance != nullptr, nullptr, "Only one Godot Instance may be created.");

	uint32_t remaining_args = p_argc - 1;
	os = new OS_MacOS_NSApp(p_argv[0], remaining_args, remaining_args > 0 ? &p_argv[1] : nullptr);

	@autoreleasepool {
		Error err = Main::setup(p_argv[0], remaining_args, remaining_args > 0 ? &p_argv[1] : nullptr, false);
		if (err != OK) {
			return nullptr;
		}

		instance = memnew(GodotInstance);
		if (!instance->initialize(p_init_func)) {
			memdelete(instance);
			instance = nullptr;
			return nullptr;
		}

		return (GDExtensionObjectPtr)instance;
	}
}

void libgodot_destroy_godot_instance(GDExtensionObjectPtr p_godot_instance) {
	GodotInstance *godot_instance = (GodotInstance *)p_godot_instance;
	if (instance == godot_instance) {
		godot_instance->stop();
		memdelete(godot_instance);
		// Note: When Godot Engine supports reinitialization, clear the instance pointer here.
		//instance = nullptr;
		Main::cleanup();
	}
}
#endif // !defined(LIBGODOT_ENABLED)
