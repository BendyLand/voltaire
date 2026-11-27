/**************************************************************************/
/*  os_windows.cpp                                                        */
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

#include "servers/display/display_server.h"
#include "core/io/packet_peer_udp.h"

#include "servers/display/display_server.h"
// --- Inlined from platform/windows/lang_table.h ---
#include <windef.h>

struct _WinLocale {
	const char *locale;
	int main_lang;
	int sublang;
};

static const _WinLocale _win_locales[] = {
	{ "ar", LANG_ARABIC, SUBLANG_NEUTRAL },
	{ "ar_AE", LANG_ARABIC, SUBLANG_ARABIC_UAE },
	{ "ar_BH", LANG_ARABIC, SUBLANG_ARABIC_BAHRAIN },
	{ "ar_DZ", LANG_ARABIC, SUBLANG_ARABIC_ALGERIA },
	{ "ar_EG", LANG_ARABIC, SUBLANG_ARABIC_EGYPT },
	{ "ar_IQ", LANG_ARABIC, SUBLANG_ARABIC_IRAQ },
	{ "ar_JO", LANG_ARABIC, SUBLANG_ARABIC_JORDAN },
	{ "ar_KW", LANG_ARABIC, SUBLANG_ARABIC_KUWAIT },
	{ "ar_LB", LANG_ARABIC, SUBLANG_ARABIC_LEBANON },
	{ "ar_LY", LANG_ARABIC, SUBLANG_ARABIC_LIBYA },
	{ "ar_MA", LANG_ARABIC, SUBLANG_ARABIC_MOROCCO },
	{ "ar_OM", LANG_ARABIC, SUBLANG_ARABIC_OMAN },
	{ "ar_QA", LANG_ARABIC, SUBLANG_ARABIC_QATAR },
	{ "ar_SA", LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA },
	//no sudan
	{ "ar_SY", LANG_ARABIC, SUBLANG_ARABIC_SYRIA },
	{ "ar_TN", LANG_ARABIC, SUBLANG_ARABIC_TUNISIA },
	{ "ar_YE", LANG_ARABIC, SUBLANG_ARABIC_YEMEN },
	{ "be", LANG_BELARUSIAN, SUBLANG_NEUTRAL },
	{ "be_BY", LANG_BELARUSIAN, SUBLANG_BELARUSIAN_BELARUS },
	{ "bg", LANG_BULGARIAN, SUBLANG_NEUTRAL },
	{ "bg_BG", LANG_BULGARIAN, SUBLANG_BULGARIAN_BULGARIA },
	{ "ca", LANG_CATALAN, SUBLANG_NEUTRAL },
	{ "ca_ES", LANG_CATALAN, SUBLANG_CATALAN_CATALAN },
	{ "cs", LANG_CZECH, SUBLANG_NEUTRAL },
	{ "cs_CZ", LANG_CZECH, SUBLANG_CZECH_CZECH_REPUBLIC },
	{ "da", LANG_DANISH, SUBLANG_NEUTRAL },
	{ "da_DK", LANG_DANISH, SUBLANG_DANISH_DENMARK },
	{ "de", LANG_GERMAN, SUBLANG_NEUTRAL },
	{ "de_AT", LANG_GERMAN, SUBLANG_GERMAN_AUSTRIAN },
	{ "de_CH", LANG_GERMAN, SUBLANG_GERMAN_SWISS },
	{ "de_DE", LANG_GERMAN, SUBLANG_GERMAN },
	{ "de_LU", LANG_GERMAN, SUBLANG_GERMAN_LUXEMBOURG },
	{ "el", LANG_GREEK, SUBLANG_NEUTRAL },
	{ "el_GR", LANG_GREEK, SUBLANG_GREEK_GREECE },
	//{"en_029", LANG_ENGLISH,SUBLANG_ENGLISH_CARIBBEAN},
	{ "en", LANG_ENGLISH, SUBLANG_NEUTRAL },
	{ "en_AU", LANG_ENGLISH, SUBLANG_ENGLISH_AUS },
	{ "en_CA", LANG_ENGLISH, SUBLANG_ENGLISH_CAN },
	{ "en_GB", LANG_ENGLISH, SUBLANG_ENGLISH_UK },
	//{"en_IE", LANG_ENGLISH,SUBLANG_ENGLISH_IRELAND},
	{ "en_IN", LANG_ENGLISH, SUBLANG_ENGLISH_INDIA },
	//MT
	{ "en_NZ", LANG_ENGLISH, SUBLANG_ENGLISH_NZ },
	{ "en_PH", LANG_ENGLISH, SUBLANG_ENGLISH_PHILIPPINES },
	{ "en_SG", LANG_ENGLISH, SUBLANG_ENGLISH_SINGAPORE },
	{ "en_US", LANG_ENGLISH, SUBLANG_ENGLISH_US },
	{ "en_ZA", LANG_ENGLISH, SUBLANG_ENGLISH_SOUTH_AFRICA },
	{ "es", LANG_SPANISH, SUBLANG_NEUTRAL },
	{ "es_AR", LANG_SPANISH, SUBLANG_SPANISH_ARGENTINA },
	{ "es_BO", LANG_SPANISH, SUBLANG_SPANISH_BOLIVIA },
	{ "es_CL", LANG_SPANISH, SUBLANG_SPANISH_CHILE },
	{ "es_CO", LANG_SPANISH, SUBLANG_SPANISH_COLOMBIA },
	{ "es_CR", LANG_SPANISH, SUBLANG_SPANISH_COSTA_RICA },
	{ "es_DO", LANG_SPANISH, SUBLANG_SPANISH_DOMINICAN_REPUBLIC },
	{ "es_EC", LANG_SPANISH, SUBLANG_SPANISH_ECUADOR },
	{ "es_ES", LANG_SPANISH, SUBLANG_SPANISH },
	{ "es_GT", LANG_SPANISH, SUBLANG_SPANISH_GUATEMALA },
	{ "es_HN", LANG_SPANISH, SUBLANG_SPANISH_HONDURAS },
	{ "es_MX", LANG_SPANISH, SUBLANG_SPANISH_MEXICAN },
	{ "es_NI", LANG_SPANISH, SUBLANG_SPANISH_NICARAGUA },
	{ "es_PA", LANG_SPANISH, SUBLANG_SPANISH_PANAMA },
	{ "es_PE", LANG_SPANISH, SUBLANG_SPANISH_PERU },
	{ "es_PR", LANG_SPANISH, SUBLANG_SPANISH_PUERTO_RICO },
	{ "es_PY", LANG_SPANISH, SUBLANG_SPANISH_PARAGUAY },
	{ "es_SV", LANG_SPANISH, SUBLANG_SPANISH_EL_SALVADOR },
	{ "es_US", LANG_SPANISH, SUBLANG_SPANISH_US },
	{ "es_UY", LANG_SPANISH, SUBLANG_SPANISH_URUGUAY },
	{ "es_VE", LANG_SPANISH, SUBLANG_SPANISH_VENEZUELA },
	{ "et", LANG_ESTONIAN, SUBLANG_NEUTRAL },
	{ "et_EE", LANG_ESTONIAN, SUBLANG_ESTONIAN_ESTONIA },
	{ "fi", LANG_FINNISH, SUBLANG_NEUTRAL },
	{ "fi_FI", LANG_FINNISH, SUBLANG_FINNISH_FINLAND },
	{ "fr", LANG_FRENCH, SUBLANG_NEUTRAL },
	{ "fr_BE", LANG_FRENCH, SUBLANG_FRENCH_BELGIAN },
	{ "fr_CA", LANG_FRENCH, SUBLANG_FRENCH_CANADIAN },
	{ "fr_CH", LANG_FRENCH, SUBLANG_FRENCH_SWISS },
	{ "fr_FR", LANG_FRENCH, SUBLANG_FRENCH },
	{ "fr_LU", LANG_FRENCH, SUBLANG_FRENCH_LUXEMBOURG },
	{ "ga", LANG_IRISH, SUBLANG_NEUTRAL },
	{ "ga_IE", LANG_IRISH, SUBLANG_IRISH_IRELAND },
	{ "hi", LANG_HINDI, SUBLANG_NEUTRAL },
	{ "hi_IN", LANG_HINDI, SUBLANG_HINDI_INDIA },
	{ "hr", LANG_CROATIAN, SUBLANG_NEUTRAL },
	{ "hr_HR", LANG_CROATIAN, SUBLANG_CROATIAN_CROATIA },
	{ "hu", LANG_HUNGARIAN, SUBLANG_NEUTRAL },
	{ "hu_HU", LANG_HUNGARIAN, SUBLANG_HUNGARIAN_HUNGARY },
	{ "in", LANG_ARMENIAN, SUBLANG_NEUTRAL },
	{ "in_ID", LANG_INDONESIAN, SUBLANG_INDONESIAN_INDONESIA },
	{ "is", LANG_ICELANDIC, SUBLANG_NEUTRAL },
	{ "is_IS", LANG_ICELANDIC, SUBLANG_ICELANDIC_ICELAND },
	{ "it", LANG_ITALIAN, SUBLANG_NEUTRAL },
	{ "it_CH", LANG_ITALIAN, SUBLANG_ITALIAN_SWISS },
	{ "it_IT", LANG_ITALIAN, SUBLANG_ITALIAN },
	{ "iw", LANG_HEBREW, SUBLANG_NEUTRAL },
	{ "iw_IL", LANG_HEBREW, SUBLANG_HEBREW_ISRAEL },
	{ "ja", LANG_JAPANESE, SUBLANG_NEUTRAL },
	{ "ja_JP", LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN },
	{ "ko", LANG_KOREAN, SUBLANG_NEUTRAL },
	{ "ko_KR", LANG_KOREAN, SUBLANG_KOREAN },
	{ "lt", LANG_LITHUANIAN, SUBLANG_NEUTRAL },
	//{"lt_LT", LANG_LITHUANIAN,SUBLANG_LITHUANIAN_LITHUANIA},
	{ "lv", LANG_LATVIAN, SUBLANG_NEUTRAL },
	{ "lv_LV", LANG_LATVIAN, SUBLANG_LATVIAN_LATVIA },
	{ "mk", LANG_MACEDONIAN, SUBLANG_NEUTRAL },
	{ "mk_MK", LANG_MACEDONIAN, SUBLANG_MACEDONIAN_MACEDONIA },
	{ "ms", LANG_MALAY, SUBLANG_NEUTRAL },
	{ "ms_MY", LANG_MALAY, SUBLANG_MALAY_MALAYSIA },
	{ "mt", LANG_MALTESE, SUBLANG_NEUTRAL },
	{ "mt_MT", LANG_MALTESE, SUBLANG_MALTESE_MALTA },
	{ "nl", LANG_DUTCH, SUBLANG_NEUTRAL },
	{ "nl_BE", LANG_DUTCH, SUBLANG_DUTCH_BELGIAN },
	{ "nl_NL", LANG_DUTCH, SUBLANG_DUTCH },
	{ "no", LANG_NORWEGIAN, SUBLANG_NEUTRAL },
	{ "no_NO", LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL },
	{ "no_NO_NY", LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK },
	{ "pl", LANG_POLISH, SUBLANG_NEUTRAL },
	{ "pl_PL", LANG_POLISH, SUBLANG_POLISH_POLAND },
	{ "pt", LANG_PORTUGUESE, SUBLANG_NEUTRAL },
	{ "pt_BR", LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN },
	{ "pt_PT", LANG_PORTUGUESE, SUBLANG_PORTUGUESE },
	{ "ro", LANG_ROMANIAN, SUBLANG_NEUTRAL },
	{ "ro_RO", LANG_ROMANIAN, SUBLANG_ROMANIAN_ROMANIA },
	{ "ru", LANG_RUSSIAN, SUBLANG_NEUTRAL },
	{ "ru_RU", LANG_RUSSIAN, SUBLANG_RUSSIAN_RUSSIA },
	{ "sk", LANG_SLOVAK, SUBLANG_NEUTRAL },
	{ "sk_SK", LANG_SLOVAK, SUBLANG_SLOVAK_SLOVAKIA },
	{ "sl", LANG_SLOVENIAN, SUBLANG_NEUTRAL },
	{ "sl_SI", LANG_SLOVENIAN, SUBLANG_SLOVENIAN_SLOVENIA },
	{ "sq", LANG_ALBANIAN, SUBLANG_NEUTRAL },
	{ "sq_AL", LANG_ALBANIAN, SUBLANG_ALBANIAN_ALBANIA },
	{ "sr", LANG_SERBIAN_NEUTRAL, SUBLANG_NEUTRAL },
	{ "sv", LANG_SWEDISH, SUBLANG_NEUTRAL },
	{ "sv_SE", LANG_SWEDISH, SUBLANG_SWEDISH },
	{ "th", LANG_THAI, SUBLANG_NEUTRAL },
	{ "th_TH", LANG_THAI, SUBLANG_THAI_THAILAND },
	{ "tr", LANG_TURKISH, SUBLANG_NEUTRAL },
	{ "tr_TR", LANG_TURKISH, SUBLANG_TURKISH_TURKEY },
	{ "uk", LANG_UKRAINIAN, SUBLANG_NEUTRAL },
	{ "uk_UA", LANG_UKRAINIAN, SUBLANG_UKRAINIAN_UKRAINE },
	{ "vi", LANG_VIETNAMESE, SUBLANG_NEUTRAL },
	{ "vi_VN", LANG_VIETNAMESE, SUBLANG_VIETNAMESE_VIETNAM },
	{ "zh", LANG_CHINESE, SUBLANG_NEUTRAL },
	{ "zh_CN", LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED },
	{ "zh_HK", LANG_CHINESE, SUBLANG_CHINESE_HONGKONG },
	{ "zh_SG", LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE },
	{ "zh_TW", LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL },
	{ nullptr, 0, 0 },
};

// --- End platform/windows/lang_table.h ---
#include "servers/display/display_server.h"

#include "core/os/os.h"
#include "core/debugger/script_debugger.h"
#include "core/os/os.h"
#include "core/version_generated.gen.h"
#include "servers/display/display_server.h"

#include <avrt.h>
#include <bcrypt.h>
#include <direct.h>
#include <knownfolders.h>
#include <process.h>
#include <psapi.h>
#include <regstr.h>
#include <shlobj.h>
#include <wbemcli.h>
#include <wincrypt.h>
#include <winternl.h>

// Workaround missing `extern "C"` in MinGW-w64 < 12.0.0.
#if defined(__MINGW32__) && (!defined(__MINGW64_VERSION_MAJOR) || __MINGW64_VERSION_MAJOR < 12)
extern "C" {
#include <hidsdi.h>
}
#else
#include <hidsdi.h>
#endif

#if defined(RD_ENABLED)
#include "servers/display/display_server.h"
#endif

#if defined(GLES3_ENABLED)
#include "servers/display/display_server.h"
#endif

#if defined(VULKAN_ENABLED)
#include "servers/display/display_server.h"
#endif
#if defined(D3D12_ENABLED)
#include "servers/display/display_server.h"
#endif
#if defined(GLES3_ENABLED)
#include "servers/display/display_server.h"
#endif

#ifdef DEBUG_ENABLED
#pragma pack(push, before_imagehlp, 8)
#include <imagehlp.h>
#pragma pack(pop, before_imagehlp)
#endif

extern "C" {
#ifdef ENABLE_PREFER_HIGH_PERFORMANCE_GPU
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif // ENABLE_PREFER_HIGH_PERFORMANCE_GPU
__declspec(dllexport) void NoHotPatch() {} // Disable Nahimic code injection.
}

// Workaround mingw-w64 < 4.0 bug
#ifndef WM_TOUCH
#define WM_TOUCH 576
#endif

#ifndef WM_POINTERUPDATE
#define WM_POINTERUPDATE 0x0245
#endif

// Missing in MinGW headers before 8.0.
#ifndef DWRITE_FONT_WEIGHT_SEMI_LIGHT
#define DWRITE_FONT_WEIGHT_SEMI_LIGHT (DWRITE_FONT_WEIGHT)350
#endif

static String fix_path(const String &p_path) {
	String path = p_path;
	if (p_path.is_relative_path()) {
		Char16String current_dir_name;
		size_t str_len = GetCurrentDirectoryW(0, nullptr);
		current_dir_name.resize_uninitialized(str_len + 1);
		GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
		path = String::utf16((const char16_t *)current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace_char('\\', '/').path_join(path);
	}
	path = path.simplify_path();
	path = path.replace_char('/', '\\');
	if (path.size() >= MAX_PATH && !path.is_network_share_path() && !path.begins_with(R"(\\?\)")) {
		path = R"(\\?\)" + path;
	}
	return path;
}

static String format_error_message(DWORD id) {
	LPWSTR messageBuffer = nullptr;
	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);

	String msg = "Error " + itos(id) + ": " + String::utf16((const char16_t *)messageBuffer, size);

	LocalFree(messageBuffer);

	return msg.remove_chars("\r\n");
}

void RedirectStream(const char *p_file_name, const char *p_mode, FILE *p_cpp_stream, const DWORD p_std_handle) {
	const HANDLE h_existing = GetStdHandle(p_std_handle);
	if (h_existing != INVALID_HANDLE_VALUE) { // Redirect only if attached console have a valid handle.
		const HANDLE h_cpp = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(p_cpp_stream)));
		if (h_cpp == INVALID_HANDLE_VALUE) { // Redirect only if it's not already redirected to the pipe or file.
			FILE *fp = p_cpp_stream;
			freopen_s(&fp, p_file_name, p_mode, p_cpp_stream); // Redirect stream.
			setvbuf(p_cpp_stream, nullptr, _IONBF, 0); // Disable stream buffering.
		}
	}
}

void RedirectIOToConsole() {
	// Save current handles.
	HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE h_stderr = GetStdHandle(STD_ERROR_HANDLE);

	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		// Restore redirection (Note: if not redirected it's NULL handles not INVALID_HANDLE_VALUE).
		if (h_stdin != nullptr) {
			SetStdHandle(STD_INPUT_HANDLE, h_stdin);
		}
		if (h_stdout != nullptr) {
			SetStdHandle(STD_OUTPUT_HANDLE, h_stdout);
		}
		if (h_stderr != nullptr) {
			SetStdHandle(STD_ERROR_HANDLE, h_stderr);
		}

		// Update file handles.
		RedirectStream("CONIN$", "r", stdin, STD_INPUT_HANDLE);
		RedirectStream("CONOUT$", "w", stdout, STD_OUTPUT_HANDLE);
		RedirectStream("CONOUT$", "w", stderr, STD_ERROR_HANDLE);
	}
}

bool OS_Windows::is_using_con_wrapper() const {
	static String exe_renames[] = {
		".console.exe",
		"_console.exe",
		" console.exe",
		"console.exe",
		String(),
	};

	bool found_exe = false;
	bool found_conwrap_exe = false;
	String exe_name = get_executable_path().to_lower();
	String exe_dir = exe_name.get_base_dir();
	String exe_fname = exe_name.get_file().get_basename();

	DWORD pids[256];
	DWORD count = GetConsoleProcessList(&pids[0], 256);
	for (DWORD i = 0; i < count; i++) {
		HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pids[i]);
		if (process != nullptr) {
			WCHAR proc_name[MAX_PATH];
			DWORD len = MAX_PATH;
			if (QueryFullProcessImageNameW(process, 0, &proc_name[0], &len)) {
				String name = String::utf16((const char16_t *)&proc_name[0], len).replace_char('\\', '/').to_lower();
				if (name == exe_name) {
					found_exe = true;
				}
				for (int j = 0; !exe_renames[j].is_empty(); j++) {
					if (name == exe_dir.path_join(exe_fname + exe_renames[j])) {
						found_conwrap_exe = true;
					}
				}
			}
			CloseHandle(process);
			if (found_conwrap_exe && found_exe) {
				break;
			}
		}
	}
	if (!found_exe) {
		return true; // Unable to read console info, assume true.
	}

	return found_conwrap_exe;
}

BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType) {
	if (!EngineDebugger::is_active()) {
		return FALSE;
	}

	switch (dwCtrlType) {
		case CTRL_C_EVENT:
			EngineDebugger::get_script_debugger()->set_depth(-1);
			EngineDebugger::get_script_debugger()->set_lines_left(1);
			return TRUE;
		default:
			return FALSE;
	}
}

void OS_Windows::alert(const String &p_alert, const String &p_title) {
	MessageBoxW(nullptr, (LPCWSTR)(p_alert.utf16().get_data()), (LPCWSTR)(p_title.utf16().get_data()), MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
}

void OS_Windows::initialize_debugging() {
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
}

#ifdef WINDOWS_DEBUG_OUTPUT_ENABLED
static void _error_handler(void *p_self, const char *p_func, const char *p_file, int p_line, const char *p_error, const char *p_errorexp, bool p_editor_notify, ErrorHandlerType p_type) {
	String err_str;
	if (p_errorexp && p_errorexp[0]) {
		err_str = String::utf8(p_errorexp) + "\n";
	} else {
		err_str = String::utf8(p_file) + ":" + itos(p_line) + " - " + String::utf8(p_error) + "\n";
	}

	OutputDebugStringW((LPCWSTR)err_str.utf16().ptr());
}
#endif

void OS_Windows::initialize() {
	crash_handler.initialize();

#ifdef WINDOWS_DEBUG_OUTPUT_ENABLED
	error_handlers.errfunc = _error_handler;
	error_handlers.userdata = this;
	add_error_handler(&error_handlers);
#endif

#ifdef THREADS_ENABLED
	init_thread_win();
#endif

	FileAccess::make_default<FileAccessWindows>(FileAccess::ACCESS_RESOURCES);
	FileAccess::make_default<FileAccessWindows>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessWindows>(FileAccess::ACCESS_FILESYSTEM);
	FileAccess::make_default<FileAccessWindowsPipe>(FileAccess::ACCESS_PIPE);
	DirAccess::make_default<DirAccessWindows>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessWindows>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessWindows>(DirAccess::ACCESS_FILESYSTEM);

	NetSocketWinSock::make_default();

	// We need to know how often the clock is updated
	QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second);
	QueryPerformanceCounter((LARGE_INTEGER *)&ticks_start);

#if WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
	// set minimum resolution for periodic timers, otherwise Sleep(n) may wait at least as
	//  long as the windows scheduler resolution (~16-30ms) even for calls like Sleep(1)
	TIMECAPS time_caps;
	if (timeGetDevCaps(&time_caps, sizeof(time_caps)) == MMSYSERR_NOERROR) {
		delay_resolution = time_caps.wPeriodMin * 1000;
		timeBeginPeriod(time_caps.wPeriodMin);
	} else {
		ERR_PRINT("Unable to detect sleep timer resolution.");
		delay_resolution = 1000;
		timeBeginPeriod(1);
	}
#else
	delay_resolution = 1000;
#endif

	process_map = memnew((HashMap<ProcessID, ProcessInfo>));

	// Add current Godot PID to the list of known PIDs
	ProcessInfo current_pi = {};
	PROCESS_INFORMATION current_pi_pi = {};
	current_pi.pi = current_pi_pi;
	current_pi.pi.hProcess = GetCurrentProcess();
	process_map->insert(GetCurrentProcessId(), current_pi);

	_ip_windows_make_default();

	HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown **>(&dwrite_factory));
	if (SUCCEEDED(hr)) {
		hr = dwrite_factory->GetSystemFontCollection(&font_collection, false);
		if (SUCCEEDED(hr)) {
			dwrite_init = true;
			hr = dwrite_factory->QueryInterface(&dwrite_factory2);
			if (SUCCEEDED(hr)) {
				hr = dwrite_factory2->GetSystemFontFallback(&system_font_fallback);
				if (SUCCEEDED(hr)) {
					dwrite2_init = true;
				}
			}
		}
	}
	if (!dwrite_init) {
		print_verbose("Unable to load IDWriteFactory, system font support is disabled.");
	} else if (!dwrite2_init) {
		print_verbose("Unable to load IDWriteFactory2, automatic system font fallback is disabled.");
	}

	FileAccessWindows::initialize();

#ifdef WINMIDI_ENABLED
	MIDIDriver::bind(&MIDIDriverWinMidi::initialize, &MIDIDriverWinMidi::finalize, &MIDIDriverWinMidi::open, &MIDIDriverWinMidi::close);
	MIDIDriver::initialize();
#endif
}

void OS_Windows::delete_main_loop() {
	MainLoop::clear();
}

void OS_Windows::finalize() {
	if (dwrite_factory2) {
		dwrite_factory2->Release();
		dwrite_factory2 = nullptr;
	}
	if (font_collection) {
		font_collection->Release();
		font_collection = nullptr;
	}
	if (system_font_fallback) {
		system_font_fallback->Release();
		system_font_fallback = nullptr;
	}
	if (dwrite_factory) {
		dwrite_factory->Release();
		dwrite_factory = nullptr;
	}
#ifdef WINMIDI_ENABLED
	MIDIDriver::finalize();
	MIDIDriver::reset_backend();
#endif

	MainLoop::clear();
}

void OS_Windows::finalize_core() {
	while (!temp_libraries.is_empty()) {
		_remove_temp_library(temp_libraries.last()->key);
	}

	FileAccessWindows::finalize();

#if WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
	timeEndPeriod(1);
#endif

	memdelete(process_map);
	IP::finalize();
	IP::reset_backend();
	NetSocketWinSock::cleanup();

#ifdef WINDOWS_DEBUG_OUTPUT_ENABLED
	remove_error_handler(&error_handlers);
#endif
}

Error OS_Windows::get_entropy(uint8_t *r_buffer, int p_bytes) {
	NTSTATUS status = BCryptGenRandom(nullptr, r_buffer, p_bytes, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	ERR_FAIL_COND_V(status, FAILED);
	return OK;
}

#ifdef DEBUG_ENABLED
void debug_dynamic_library_check_dependencies(const String &p_path, HashSet<String> &r_checked, HashSet<String> &r_missing) {
	if (r_checked.has(p_path)) {
		return;
	}
	r_checked.insert(p_path);

	LOADED_IMAGE loaded_image;
	HANDLE file = CreateFileW((LPCWSTR)p_path.utf16().get_data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (file != INVALID_HANDLE_VALUE) {
		HANDLE file_mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY | SEC_COMMIT, 0, 0, nullptr);
		if (file_mapping != INVALID_HANDLE_VALUE) {
			PVOID mapping = MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
			if (mapping) {
				PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)mapping;
				PIMAGE_NT_HEADERS nt_header = nullptr;
				if (dos_header->e_magic == IMAGE_DOS_SIGNATURE) {
					PCHAR nt_header_ptr;
					nt_header_ptr = ((PCHAR)mapping) + dos_header->e_lfanew;
					nt_header = (PIMAGE_NT_HEADERS)nt_header_ptr;
					if (nt_header->Signature != IMAGE_NT_SIGNATURE) {
						nt_header = nullptr;
					}
				}
				if (nt_header) {
					loaded_image.ModuleName = nullptr;
					loaded_image.hFile = file;
					loaded_image.MappedAddress = (PUCHAR)mapping;
					loaded_image.FileHeader = nt_header;
					loaded_image.Sections = (PIMAGE_SECTION_HEADER)((LPBYTE)&nt_header->OptionalHeader + nt_header->FileHeader.SizeOfOptionalHeader);
					loaded_image.NumberOfSections = nt_header->FileHeader.NumberOfSections;
					loaded_image.SizeOfImage = GetFileSize(file, nullptr);
					loaded_image.Characteristics = nt_header->FileHeader.Characteristics;
					loaded_image.LastRvaSection = loaded_image.Sections;
					loaded_image.fSystemImage = false;
					loaded_image.fDOSImage = false;
					loaded_image.Links.Flink = &loaded_image.Links;
					loaded_image.Links.Blink = &loaded_image.Links;

					ULONG size = 0;
					const IMAGE_IMPORT_DESCRIPTOR *import_desc = (const IMAGE_IMPORT_DESCRIPTOR *)ImageDirectoryEntryToData((HMODULE)loaded_image.MappedAddress, false, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);
					if (import_desc) {
						for (; import_desc->Name && import_desc->FirstThunk; import_desc++) {
							char16_t full_name_wc[32767];
							const char *name_cs = (const char *)ImageRvaToVa(loaded_image.FileHeader, loaded_image.MappedAddress, import_desc->Name, nullptr);
							String name = String(name_cs);
							if (name.begins_with("api-ms-win-")) {
								r_checked.insert(name);
							} else if (SearchPathW(nullptr, (LPCWSTR)name.utf16().get_data(), nullptr, 32767, (LPWSTR)full_name_wc, nullptr)) {
								debug_dynamic_library_check_dependencies(String::utf16(full_name_wc), r_checked, r_missing);
							} else if (SearchPathW((LPCWSTR)(p_path.get_base_dir().utf16().get_data()), (LPCWSTR)name.utf16().get_data(), nullptr, 32767, (LPWSTR)full_name_wc, nullptr)) {
								debug_dynamic_library_check_dependencies(String::utf16(full_name_wc), r_checked, r_missing);
							} else {
								r_missing.insert(name);
							}
						}
					}
				}
				UnmapViewOfFile(mapping);
			}
			CloseHandle(file_mapping);
		}
		CloseHandle(file);
	}
}
#endif

Error OS_Windows::open_dynamic_library(const String &p_path, void *&p_library_handle, GDExtensionData *p_data) {
	String path = p_path;

	if (!FileAccess::exists(path)) {
		//this code exists so gdextension can load .dll files from within the executable path
		path = get_executable_path().get_base_dir().path_join(p_path.get_file());
		ERR_FAIL_COND_V(!FileAccess::exists(path), ERR_FILE_NOT_FOUND);
	}
	// Path to load from may be different from original if we make copies.
	String load_path = path;

	// Here we want a copy to be loaded.
	// This is so the original file isn't locked and can be updated by a compiler.
	if (p_data != nullptr && p_data->generate_temp_files) {
		// Copy the file to the same directory as the original with a prefix in the name.
		// This is so relative path to dependencies are satisfied.
		load_path = path.get_base_dir().path_join("~" + path.get_file());

		// If there's a left-over copy (possibly from a crash) then delete it first.
		if (FileAccess::exists(load_path)) {
			DirAccess::remove_absolute(load_path);
		}

		Error copy_err = DirAccess::copy_absolute(path, load_path);
		if (copy_err) {
			ERR_PRINT("Error copying library: " + path);
			return ERR_CANT_CREATE;
		}

		FileAccess::set_hidden_attribute(load_path, true);

		Error pdb_err = WindowsUtils::copy_and_rename_pdb(load_path);
		if (pdb_err != OK && pdb_err != ERR_SKIP) {
			WARN_PRINT(vformat("Failed to rename the PDB file. The original PDB file for '%s' will be loaded.", path));
		}
	}

	DLL_DIRECTORY_COOKIE cookie = nullptr;

	String dll_path = fix_path(load_path);
	String dll_dir = fix_path(ProjectSettings::globalize_path(load_path.get_base_dir()));
	if (p_data != nullptr && p_data->also_set_library_path) {
		cookie = AddDllDirectory((LPCWSTR)(dll_dir.utf16().get_data()));
	}

	p_library_handle = (void *)LoadLibraryExW((LPCWSTR)(dll_path.utf16().get_data()), nullptr, (p_data != nullptr && p_data->also_set_library_path) ? LOAD_LIBRARY_SEARCH_DEFAULT_DIRS : 0);
	if (!p_library_handle) {
		if (p_data != nullptr && p_data->generate_temp_files) {
			DirAccess::remove_absolute(load_path);
		}

#ifdef DEBUG_ENABLED
		DWORD err_code = GetLastError();

		HashSet<String> checked_libs;
		HashSet<String> missing_libs;
		debug_dynamic_library_check_dependencies(dll_path, checked_libs, missing_libs);
		if (!missing_libs.is_empty()) {
			String missing;
			for (const String &E : missing_libs) {
				if (!missing.is_empty()) {
					missing += ", ";
				}
				missing += E;
			}
			ERR_FAIL_V_MSG(ERR_CANT_OPEN, vformat("Can't open dynamic library: %s. Missing dependencies: %s. Error: %s.", p_path, missing, format_error_message(err_code)));
		} else {
			ERR_FAIL_V_MSG(ERR_CANT_OPEN, vformat("Can't open dynamic library: %s. Error: %s.", p_path, format_error_message(err_code)));
		}
#endif
	}

#ifndef DEBUG_ENABLED
	ERR_FAIL_NULL_V_MSG(p_library_handle, ERR_CANT_OPEN, vformat("Can't open dynamic library: %s. Error: %s.", p_path, format_error_message(GetLastError())));
#endif

	if (cookie) {
		RemoveDllDirectory(cookie);
	}

	if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
		*p_data->r_resolved_path = path;
	}

	if (p_data != nullptr && p_data->generate_temp_files) {
		// Save the copied path so it can be deleted later.
		temp_libraries[p_library_handle] = load_path;
	}

	return OK;
}

Error OS_Windows::close_dynamic_library(void *p_library_handle) {
	if (!FreeLibrary((HMODULE)p_library_handle)) {
		return FAILED;
	}

	// Delete temporary copy of library if it exists.
	_remove_temp_library(p_library_handle);

	return OK;
}

void OS_Windows::_remove_temp_library(void *p_library_handle) {
	if (temp_libraries.has(p_library_handle)) {
		String path = temp_libraries[p_library_handle];
		DirAccess::remove_absolute(path);
		WindowsUtils::remove_temp_pdbs(path);
		temp_libraries.erase(p_library_handle);
	}
}

Error OS_Windows::get_dynamic_library_symbol_handle(void *p_library_handle, const String &p_name, void *&p_symbol_handle, bool p_optional) {
	p_symbol_handle = (void *)GetProcAddress((HMODULE)p_library_handle, p_name.utf8().get_data());
	if (!p_symbol_handle) {
		if (!p_optional) {
			ERR_FAIL_V_MSG(ERR_CANT_RESOLVE, vformat("Can't resolve symbol %s, error: \"%s\".", p_name, format_error_message(GetLastError())));
		} else {
			return ERR_CANT_RESOLVE;
		}
	}
	return OK;
}

String OS_Windows::get_name() const {
	return "Windows";
}

String OS_Windows::get_distribution_name() const {
	return get_name();
}

String OS_Windows::get_version() const {
	RtlGetVersionPtr version_ptr = (RtlGetVersionPtr)(void *)GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlGetVersion");
	if (version_ptr != nullptr) {
		RTL_OSVERSIONINFOEXW fow;
		ZeroMemory(&fow, sizeof(fow));
		fow.dwOSVersionInfoSize = sizeof(fow);
		if (version_ptr(&fow) == 0x00000000) {
			return vformat("%d.%d.%d", (int64_t)fow.dwMajorVersion, (int64_t)fow.dwMinorVersion, (int64_t)fow.dwBuildNumber);
		}
	}
	return "";
}

String OS_Windows::get_version_alias() const {
	RtlGetVersionPtr version_ptr = (RtlGetVersionPtr)(void *)GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlGetVersion");
	if (version_ptr != nullptr) {
		RTL_OSVERSIONINFOEXW fow;
		ZeroMemory(&fow, sizeof(fow));
		fow.dwOSVersionInfoSize = sizeof(fow);
		if (version_ptr(&fow) == 0x00000000) {
			String windows_string;
			if (fow.wProductType != VER_NT_WORKSTATION && fow.dwMajorVersion == 10 && fow.dwBuildNumber >= 26100) {
				windows_string = "Server 2025";
			} else if (fow.dwMajorVersion == 10 && fow.dwBuildNumber >= 20348) {
				// Builds above 20348 correspond to Windows 11 / Windows Server 2022.
				// Their major version numbers are still 10 though, not 11.
				if (fow.wProductType != VER_NT_WORKSTATION) {
					windows_string += "Server 2022";
				} else {
					windows_string += "11";
				}
			} else if (fow.dwMajorVersion == 10) {
				if (fow.wProductType != VER_NT_WORKSTATION && fow.dwBuildNumber >= 17763) {
					windows_string += "Server 2019";
				} else {
					if (fow.wProductType != VER_NT_WORKSTATION) {
						windows_string += "Server 2016";
					} else {
						windows_string += "10";
					}
				}
			} else {
				windows_string += "Unknown";
			}
			// Windows versions older than 10 cannot run Godot.

			return vformat("%s (build %d)", windows_string, (int64_t)fow.dwBuildNumber);
		}
	}

	return "";
}

Vector<String> OS_Windows::_get_video_adapter_driver_info_reg(const String &p_name) const {
	Vector<String> info;

	String subkey = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}";
	HKEY hkey = nullptr;
	LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, (LPCWSTR)subkey.utf16().get_data(), 0, KEY_READ, &hkey);
	if (result != ERROR_SUCCESS) {
		return Vector<String>();
	}

	DWORD subkeys = 0;
	result = RegQueryInfoKeyW(hkey, nullptr, nullptr, nullptr, &subkeys, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	if (result != ERROR_SUCCESS) {
		RegCloseKey(hkey);
		return Vector<String>();
	}
	for (DWORD i = 0; i < subkeys; i++) {
		WCHAR key_name[MAX_PATH] = L"";
		DWORD key_name_size = MAX_PATH;
		result = RegEnumKeyExW(hkey, i, key_name, &key_name_size, nullptr, nullptr, nullptr, nullptr);
		if (result != ERROR_SUCCESS) {
			continue;
		}
		String id = String::utf16((const char16_t *)key_name, key_name_size);
		if (!id.is_empty()) {
			HKEY sub_hkey = nullptr;
			result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, (LPCWSTR)(subkey + "\\" + id).utf16().get_data(), 0, KEY_QUERY_VALUE, &sub_hkey);
			if (result != ERROR_SUCCESS) {
				continue;
			}

			WCHAR buffer[4096];
			DWORD buffer_len = 4096;
			DWORD vtype = REG_SZ;
			if (RegQueryValueExW(sub_hkey, L"DriverDesc", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) != ERROR_SUCCESS || buffer_len == 0) {
				buffer_len = 4096;
				if (RegQueryValueExW(sub_hkey, L"HardwareInformation.AdapterString", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) != ERROR_SUCCESS || buffer_len == 0) {
					RegCloseKey(sub_hkey);
					continue;
				}
			}

			String driver_name = String::utf16((const char16_t *)buffer, buffer_len).strip_edges();
			if (driver_name == p_name) {
				String driver_provider = driver_name;
				String driver_version;

				buffer_len = 4096;
				if (RegQueryValueExW(sub_hkey, L"ProviderName", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) == ERROR_SUCCESS && buffer_len != 0) {
					driver_provider = String::utf16((const char16_t *)buffer, buffer_len).strip_edges();
				}
				buffer_len = 4096;
				if (RegQueryValueExW(sub_hkey, L"DriverVersion", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) == ERROR_SUCCESS && buffer_len != 0) {
					driver_version = String::utf16((const char16_t *)buffer, buffer_len).strip_edges();
				}
				if (!driver_version.is_empty()) {
					info.push_back(driver_provider);
					info.push_back(driver_version);

					RegCloseKey(sub_hkey);
					break;
				}
			}
			RegCloseKey(sub_hkey);
		}
	}
	RegCloseKey(hkey);
	return info;
}

Vector<String> OS_Windows::_get_video_adapter_driver_info_wmi(const String &p_name) const {
	Vector<String> info;

	REFCLSID clsid = CLSID_WbemLocator; // Unmarshaler CLSID
	REFIID uuid = IID_IWbemLocator; // Interface UUID
	IWbemLocator *wbemLocator = nullptr; // to get the services
	IWbemServices *wbemServices = nullptr; // to get the class
	IEnumWbemClassObject *iter = nullptr;
	IWbemClassObject *pnpSDriverObject[1]; // contains driver name, version, etc.
	String driver_name;
	String driver_version;

	HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, uuid, (LPVOID *)&wbemLocator);
	if (hr != S_OK) {
		return Vector<String>();
	}
	BSTR resource_name = SysAllocString(L"root\\CIMV2");
	hr = wbemLocator->ConnectServer(resource_name, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &wbemServices);
	SysFreeString(resource_name);

	SAFE_RELEASE(wbemLocator) // from now on, use `wbemServices`
	if (hr != S_OK) {
		SAFE_RELEASE(wbemServices)
		return Vector<String>();
	}

	const String gpu_device_class_query = vformat("SELECT * FROM Win32_PnPSignedDriver WHERE DeviceName = \"%s\"", p_name);
	BSTR query = SysAllocString((const WCHAR *)gpu_device_class_query.utf16().get_data());
	BSTR query_lang = SysAllocString(L"WQL");
	hr = wbemServices->ExecQuery(query_lang, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, nullptr, &iter);
	SysFreeString(query_lang);
	SysFreeString(query);
	if (hr == S_OK) {
		ULONG resultCount;
		hr = iter->Next(5000, 1, pnpSDriverObject, &resultCount); // Get exactly 1. Wait max 5 seconds.

		if (hr == S_OK && resultCount > 0) {
			VARIANT dn;
			VariantInit(&dn);

			BSTR object_name = SysAllocString(L"DriverName");
			hr = pnpSDriverObject[0]->Get(object_name, 0, &dn, nullptr, nullptr);
			SysFreeString(object_name);
			if (hr == S_OK && dn.vt == VT_BSTR) {
				String d_name = String(V_BSTR(&dn));
				if (d_name.is_empty()) {
					object_name = SysAllocString(L"DriverProviderName");
					hr = pnpSDriverObject[0]->Get(object_name, 0, &dn, nullptr, nullptr);
					SysFreeString(object_name);
					if (hr == S_OK) {
						driver_name = String(V_BSTR(&dn));
					}
				} else {
					driver_name = d_name;
				}
			} else {
				object_name = SysAllocString(L"DriverProviderName");
				hr = pnpSDriverObject[0]->Get(object_name, 0, &dn, nullptr, nullptr);
				SysFreeString(object_name);
				if (hr == S_OK && dn.vt == VT_BSTR) {
					driver_name = String(V_BSTR(&dn));
				} else {
					driver_name = "Unknown";
				}
			}

			VARIANT dv;
			VariantInit(&dv);
			object_name = SysAllocString(L"DriverVersion");
			hr = pnpSDriverObject[0]->Get(object_name, 0, &dv, nullptr, nullptr);
			SysFreeString(object_name);
			if (hr == S_OK && dv.vt == VT_BSTR) {
				driver_version = String(V_BSTR(&dv));
			} else {
				driver_version = "Unknown";
			}
			for (ULONG i = 0; i < resultCount; i++) {
				SAFE_RELEASE(pnpSDriverObject[i])
			}
		}
	}

	SAFE_RELEASE(wbemServices)
	SAFE_RELEASE(iter)

	info.push_back(driver_name);
	info.push_back(driver_version);

	return info;
}

Vector<String> OS_Windows::get_video_adapter_driver_info() const {
	if (!Renderer::is_initialized()) {
		return Vector<String>();
	}

	static Vector<String> info;
	if (!info.is_empty()) {
		return info;
	}

	const String device_name = Renderer::get_video_adapter_name();
	if (device_name.is_empty()) {
		return Vector<String>();
	}

	info = _get_video_adapter_driver_info_reg(device_name);
	if (info.is_empty()) {
		info = _get_video_adapter_driver_info_wmi(device_name);
	}
	return info;
}

bool OS_Windows::get_user_prefers_integrated_gpu() const {
	// On Windows 10, the preferred GPU configured in Windows Settings is
	// stored in the registry under the key
	// `HKEY_CURRENT_USER\SOFTWARE\Microsoft\DirectX\UserGpuPreferences`
	// with the name being the app ID or EXE path. The value is in the form of
	// `GpuPreference=1;`, with the value being 1 for integrated GPU and 2
	// for discrete GPU. On Windows 11, there may be more flags, separated
	// by semicolons.

	// If this is a packaged app, use the "application user model ID".
	// Otherwise, use the EXE path.
	WCHAR value_name[32768];
	bool is_packaged = false;
	{
		HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
		if (kernel32) {
			using GetCurrentApplicationUserModelIdPtr = LONG(WINAPI *)(UINT32 * length, PWSTR id);
			GetCurrentApplicationUserModelIdPtr GetCurrentApplicationUserModelId = (GetCurrentApplicationUserModelIdPtr)(void *)GetProcAddress(kernel32, "GetCurrentApplicationUserModelId");

			if (GetCurrentApplicationUserModelId) {
				UINT32 length = std_size(value_name);
				LONG result = GetCurrentApplicationUserModelId(&length, value_name);
				if (result == ERROR_SUCCESS) {
					is_packaged = true;
				}
			}
		}
	}
	if (!is_packaged && GetModuleFileNameW(nullptr, value_name, sizeof(value_name) / sizeof(value_name[0])) >= sizeof(value_name) / sizeof(value_name[0])) {
		// Paths should never be longer than 32767, but just in case.
		return false;
	}

	LPCWSTR subkey = L"SOFTWARE\\Microsoft\\DirectX\\UserGpuPreferences";
	HKEY hkey = nullptr;
	LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER, subkey, 0, KEY_READ, &hkey);
	if (result != ERROR_SUCCESS) {
		return false;
	}

	DWORD size = 0;
	result = RegGetValueW(hkey, nullptr, value_name, RRF_RT_REG_SZ, nullptr, nullptr, &size);
	if (result != ERROR_SUCCESS || size == 0) {
		RegCloseKey(hkey);
		return false;
	}

	Vector<WCHAR> buffer;
	buffer.resize(size / sizeof(WCHAR));
	result = RegGetValueW(hkey, nullptr, value_name, RRF_RT_REG_SZ, nullptr, (LPBYTE)buffer.ptrw(), &size);
	if (result != ERROR_SUCCESS) {
		RegCloseKey(hkey);
		return false;
	}

	RegCloseKey(hkey);
	const String flags = String::utf16((const char16_t *)buffer.ptr(), size / sizeof(WCHAR));

	for (const String &flag : flags.split(";", false)) {
		if (flag == "GpuPreference=1") {
			return true;
		}
	}
	return false;
}

OS::DateTime OS_Windows::get_datetime(bool p_utc) const {
	SYSTEMTIME systemtime;
	if (p_utc) {
		GetSystemTime(&systemtime);
	} else {
		GetLocalTime(&systemtime);
	}

	//Get DST information from Windows, but only if p_utc is false.
	TIME_ZONE_INFORMATION info;
	bool is_daylight = false;
	if (!p_utc && GetTimeZoneInformation(&info) == TIME_ZONE_ID_DAYLIGHT) {
		is_daylight = true;
	}

	DateTime dt;
	dt.year = systemtime.wYear;
	dt.month = Month(systemtime.wMonth);
	dt.day = systemtime.wDay;
	dt.weekday = Weekday(systemtime.wDayOfWeek);
	dt.hour = systemtime.wHour;
	dt.minute = systemtime.wMinute;
	dt.second = systemtime.wSecond;
	dt.dst = is_daylight;
	return dt;
}

OS::TimeZoneInfo OS_Windows::get_time_zone_info() const {
	TIME_ZONE_INFORMATION info;
	bool is_daylight = false;
	if (GetTimeZoneInformation(&info) == TIME_ZONE_ID_DAYLIGHT) {
		is_daylight = true;
	}

	// Daylight Bias needs to be added to the bias if DST is in effect, or else it will not properly update.
	TimeZoneInfo ret;
	if (is_daylight) {
		ret.name = info.DaylightName;
		ret.bias = info.Bias + info.DaylightBias;
	} else {
		ret.name = info.StandardName;
		ret.bias = info.Bias + info.StandardBias;
	}

	// Bias value returned by GetTimeZoneInformation is inverted of what we expect
	// For example, on GMT-3 GetTimeZoneInformation return a Bias of 180, so invert the value to get -180
	ret.bias = -ret.bias;
	return ret;
}

double OS_Windows::get_unix_time() const {
	// 1 Windows tick is 100ns
	const uint64_t WINDOWS_TICKS_PER_SECOND = 10000000;
	const uint64_t TICKS_TO_UNIX_EPOCH = 116444736000000000LL;

	SYSTEMTIME st;
	GetSystemTime(&st);
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	uint64_t ticks_time;
	ticks_time = ft.dwHighDateTime;
	ticks_time <<= 32;
	ticks_time |= ft.dwLowDateTime;

	return (double)(ticks_time - TICKS_TO_UNIX_EPOCH) / WINDOWS_TICKS_PER_SECOND;
}

void OS_Windows::delay_usec(uint32_t p_usec) const {
	if (p_usec < 1000) {
		Sleep(1);
	} else {
		Sleep(p_usec / 1000);
	}
}

uint64_t OS_Windows::get_ticks_usec() const {
	uint64_t ticks;

	// This is the number of clock ticks since start
	QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
	// Subtract the ticks at game start to get
	// the ticks since the game started
	ticks -= ticks_start;

	// Divide by frequency to get the time in seconds
	// original calculation shown below is subject to overflow
	// with high ticks_per_second and a number of days since the last reboot.
	// time = ticks * 1000000L / ticks_per_second;

	// we can prevent this by either using 128 bit math
	// or separating into a calculation for seconds, and the fraction
	uint64_t seconds = ticks / ticks_per_second;

	// compiler will optimize these two into one divide
	uint64_t leftover = ticks % ticks_per_second;

	// remainder
	uint64_t time = (leftover * 1000000L) / ticks_per_second;

	// seconds
	time += seconds * 1000000L;

	return time;
}

String OS_Windows::_quote_command_line_argument(const String &p_text) const {
	for (int i = 0; i < p_text.size(); i++) {
		char32_t c = p_text[i];
		if (c == ' ' || c == '&' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '^' || c == '=' || c == ';' || c == '!' || c == '\'' || c == '+' || c == ',' || c == '`' || c == '~') {
			return "\"" + p_text + "\"";
		}
	}
	return p_text;
}

static void _append_to_pipe(char *p_bytes, int p_size, String *r_pipe, Mutex *p_pipe_mutex) {
	// Try to convert from default ANSI code page to Unicode.
	LocalVector<wchar_t> wchars;
	int total_wchars = MultiByteToWideChar(CP_ACP, 0, p_bytes, p_size, nullptr, 0);
	if (total_wchars > 0) {
		wchars.resize(total_wchars);
		if (MultiByteToWideChar(CP_ACP, 0, p_bytes, p_size, wchars.ptr(), total_wchars) == 0) {
			wchars.clear();
		}
	}

	if (p_pipe_mutex) {
		p_pipe_mutex->lock();
	}
	if (wchars.is_empty()) {
		// Let's hope it's compatible with UTF-8.
		(*r_pipe) += String::utf8(p_bytes, p_size);
	} else {
		(*r_pipe) += String::utf16((char16_t *)wchars.ptr(), total_wchars);
	}
	if (p_pipe_mutex) {
		p_pipe_mutex->unlock();
	}
}

void OS_Windows::_init_encodings() {
	encodings[""] = 0;
	encodings["CP_ACP"] = 0;
	encodings["CP_OEMCP"] = 1;
	encodings["CP_MACCP"] = 2;
	encodings["CP_THREAD_ACP"] = 3;
	encodings["CP_SYMBOL"] = 42;
	encodings["IBM037"] = 37;
	encodings["IBM437"] = 437;
	encodings["IBM500"] = 500;
	encodings["ASMO-708"] = 708;
	encodings["ASMO-449"] = 709;
	encodings["DOS-710"] = 710;
	encodings["DOS-720"] = 720;
	encodings["IBM737"] = 737;
	encodings["IBM775"] = 775;
	encodings["IBM850"] = 850;
	encodings["IBM852"] = 852;
	encodings["IBM855"] = 855;
	encodings["IBM857"] = 857;
	encodings["IBM00858"] = 858;
	encodings["IBM860"] = 860;
	encodings["IBM861"] = 861;
	encodings["DOS-862"] = 862;
	encodings["IBM863"] = 863;
	encodings["IBM864"] = 864;
	encodings["IBM865"] = 865;
	encodings["CP866"] = 866;
	encodings["IBM869"] = 869;
	encodings["IBM870"] = 870;
	encodings["WINDOWS-874"] = 874;
	encodings["CP875"] = 875;
	encodings["SHIFT_JIS"] = 932;
	encodings["GB2312"] = 936;
	encodings["KS_C_5601-1987"] = 949;
	encodings["BIG5"] = 950;
	encodings["IBM1026"] = 1026;
	encodings["IBM01047"] = 1047;
	encodings["IBM01140"] = 1140;
	encodings["IBM01141"] = 1141;
	encodings["IBM01142"] = 1142;
	encodings["IBM01143"] = 1143;
	encodings["IBM01144"] = 1144;
	encodings["IBM01145"] = 1145;
	encodings["IBM01146"] = 1146;
	encodings["IBM01147"] = 1147;
	encodings["IBM01148"] = 1148;
	encodings["IBM01149"] = 1149;
	encodings["UTF-16"] = 1200;
	encodings["UNICODEFFFE"] = 1201;
	encodings["WINDOWS-1250"] = 1250;
	encodings["WINDOWS-1251"] = 1251;
	encodings["WINDOWS-1252"] = 1252;
	encodings["WINDOWS-1253"] = 1253;
	encodings["WINDOWS-1254"] = 1254;
	encodings["WINDOWS-1255"] = 1255;
	encodings["WINDOWS-1256"] = 1256;
	encodings["WINDOWS-1257"] = 1257;
	encodings["WINDOWS-1258"] = 1258;
	encodings["JOHAB"] = 1361;
	encodings["MACINTOSH"] = 10000;
	encodings["X-MAC-JAPANESE"] = 10001;
	encodings["X-MAC-CHINESETRAD"] = 10002;
	encodings["X-MAC-KOREAN"] = 10003;
	encodings["X-MAC-ARABIC"] = 10004;
	encodings["X-MAC-HEBREW"] = 10005;
	encodings["X-MAC-GREEK"] = 10006;
	encodings["X-MAC-CYRILLIC"] = 10007;
	encodings["X-MAC-CHINESESIMP"] = 10008;
	encodings["X-MAC-ROMANIAN"] = 10010;
	encodings["X-MAC-UKRAINIAN"] = 10017;
	encodings["X-MAC-THAI"] = 10021;
	encodings["X-MAC-CE"] = 10029;
	encodings["X-MAC-ICELANDIC"] = 10079;
	encodings["X-MAC-TURKISH"] = 10081;
	encodings["X-MAC-CROATIAN"] = 10082;
	encodings["UTF-32"] = 12000;
	encodings["UTF-32BE"] = 12001;
	encodings["X-CHINESE_CNS"] = 20000;
	encodings["X-CP20001"] = 20001;
	encodings["X_CHINESE-ETEN"] = 20002;
	encodings["X-CP20003"] = 20003;
	encodings["X-CP20004"] = 20004;
	encodings["X-CP20005"] = 20005;
	encodings["X-IA5"] = 20105;
	encodings["X-IA5-GERMAN"] = 20106;
	encodings["X-IA5-SWEDISH"] = 20107;
	encodings["X-IA5-NORWEGIAN"] = 20108;
	encodings["US-ASCII"] = 20127;
	encodings["X-CP20261"] = 20261;
	encodings["X-CP20269"] = 20269;
	encodings["IBM273"] = 20273;
	encodings["IBM277"] = 20277;
	encodings["IBM278"] = 20278;
	encodings["IBM280"] = 20280;
	encodings["IBM284"] = 20284;
	encodings["IBM285"] = 20285;
	encodings["IBM290"] = 20290;
	encodings["IBM297"] = 20297;
	encodings["IBM420"] = 20420;
	encodings["IBM423"] = 20423;
	encodings["IBM424"] = 20424;
	encodings["X-EBCDIC-KOREANEXTENDED"] = 20833;
	encodings["IBM-THAI"] = 20838;
	encodings["KOI8-R"] = 20866;
	encodings["IBM871"] = 20871;
	encodings["IBM880"] = 20880;
	encodings["IBM905"] = 20905;
	encodings["IBM00924"] = 20924;
	encodings["EUC-JP"] = 20932;
	encodings["X-CP20936"] = 20936;
	encodings["X-CP20949"] = 20949;
	encodings["CP1025"] = 21025;
	encodings["KOI8-U"] = 21866;
	encodings["ISO-8859-1"] = 28591;
	encodings["ISO-8859-2"] = 28592;
	encodings["ISO-8859-3"] = 28593;
	encodings["ISO-8859-4"] = 28594;
	encodings["ISO-8859-5"] = 28595;
	encodings["ISO-8859-6"] = 28596;
	encodings["ISO-8859-7"] = 28597;
	encodings["ISO-8859-8"] = 28598;
	encodings["ISO-8859-9"] = 28599;
	encodings["ISO-8859-13"] = 28603;
	encodings["ISO-8859-15"] = 28605;
	encodings["X-EUROPA"] = 29001;
	encodings["ISO-8859-8-I"] = 38598;
	encodings["ISO-2022-JP"] = 50220;
	encodings["CSISO2022JP"] = 50221;
	encodings["ISO-2022-JP"] = 50222;
	encodings["ISO-2022-KR"] = 50225;
	encodings["X-CP50227"] = 50227;
	encodings["EBCDIC-JP"] = 50930;
	encodings["EBCDIC-US-JP"] = 50931;
	encodings["EBCDIC-KR"] = 50933;
	encodings["EBCDIC-CN-eXT"] = 50935;
	encodings["EBCDIC-CN"] = 50936;
	encodings["EBCDIC-US-CN"] = 50937;
	encodings["EBCDIC-JP-EXT"] = 50939;
	encodings["EUC-JP"] = 51932;
	encodings["EUC-CN"] = 51936;
	encodings["EUC-KR"] = 51949;
	encodings["HZ-GB-2312"] = 52936;
	encodings["GB18030"] = 54936;
	encodings["X-ISCII-DE"] = 57002;
	encodings["X-ISCII-BE"] = 57003;
	encodings["X-ISCII-TA"] = 57004;
	encodings["X-ISCII-TE"] = 57005;
	encodings["X-ISCII-AS"] = 57006;
	encodings["X-ISCII-OR"] = 57007;
	encodings["X-ISCII-KA"] = 57008;
	encodings["X-ISCII-MA"] = 57009;
	encodings["X-ISCII-GU"] = 57010;
	encodings["X-ISCII-PA"] = 57011;
	encodings["UTF-7"] = 65000;
	encodings["UTF-8"] = 65001;
}

String OS_Windows::multibyte_to_string(const String &p_encoding, const PackedByteArray &p_array) const {
	const int *encoding = encodings.getptr(p_encoding.to_upper());
	ERR_FAIL_NULL_V_MSG(encoding, String(), "Conversion failed: Unknown encoding");

	LocalVector<wchar_t> wchars;
	int total_wchars = MultiByteToWideChar(*encoding, 0, (const char *)p_array.ptr(), p_array.size(), nullptr, 0);
	if (total_wchars == 0) {
		DWORD err_code = GetLastError();
		ERR_FAIL_V_MSG(String(), vformat("Conversion failed: %s", format_error_message(err_code)));
	}
	wchars.resize(total_wchars);
	if (MultiByteToWideChar(*encoding, 0, (const char *)p_array.ptr(), p_array.size(), wchars.ptr(), total_wchars) == 0) {
		DWORD err_code = GetLastError();
		ERR_FAIL_V_MSG(String(), vformat("Conversion failed: %s", format_error_message(err_code)));
	}

	return String::utf16((const char16_t *)wchars.ptr(), wchars.size());
}

PackedByteArray OS_Windows::string_to_multibyte(const String &p_encoding, const String &p_string) const {
	const int *encoding = encodings.getptr(p_encoding.to_upper());
	ERR_FAIL_NULL_V_MSG(encoding, PackedByteArray(), "Conversion failed: Unknown encoding");

	Char16String charstr = p_string.utf16();
	PackedByteArray ret;
	int total_mbchars = WideCharToMultiByte(*encoding, 0, (const wchar_t *)charstr.ptr(), charstr.size(), nullptr, 0, nullptr, nullptr);
	if (total_mbchars == 0) {
		DWORD err_code = GetLastError();
		ERR_FAIL_V_MSG(PackedByteArray(), vformat("Conversion failed: %s", format_error_message(err_code)));
	}

	ret.resize(total_mbchars);
	if (WideCharToMultiByte(*encoding, 0, (const wchar_t *)charstr.ptr(), charstr.size(), (char *)ret.ptrw(), ret.size(), nullptr, nullptr) == 0) {
		DWORD err_code = GetLastError();
		ERR_FAIL_V_MSG(PackedByteArray(), vformat("Conversion failed: %s", format_error_message(err_code)));
	}

	return ret;
}

Dictionary OS_Windows::get_memory_info() const {
	Dictionary meminfo;

	meminfo["physical"] = -1;
	meminfo["free"] = -1;
	meminfo["available"] = -1;
	meminfo["stack"] = -1;

	PERFORMANCE_INFORMATION pref_info;
	pref_info.cb = sizeof(pref_info);
	GetPerformanceInfo(&pref_info, sizeof(pref_info));

	typedef void(WINAPI * PGetCurrentThreadStackLimits)(PULONG_PTR, PULONG_PTR);
	PGetCurrentThreadStackLimits GetCurrentThreadStackLimits = (PGetCurrentThreadStackLimits)(void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCurrentThreadStackLimits");

	ULONG_PTR LowLimit = 0;
	ULONG_PTR HighLimit = 0;
	if (GetCurrentThreadStackLimits) {
		GetCurrentThreadStackLimits(&LowLimit, &HighLimit);
	}

	if (pref_info.PhysicalTotal * pref_info.PageSize != 0) {
		meminfo["physical"] = static_cast<int64_t>(pref_info.PhysicalTotal * pref_info.PageSize);
	}
	if (pref_info.PhysicalAvailable * pref_info.PageSize != 0) {
		meminfo["free"] = static_cast<int64_t>(pref_info.PhysicalAvailable * pref_info.PageSize);
	}
	if (pref_info.CommitLimit * pref_info.PageSize != 0) {
		meminfo["available"] = static_cast<int64_t>(pref_info.CommitLimit * pref_info.PageSize);
	}
	if (HighLimit - LowLimit != 0) {
		meminfo["stack"] = static_cast<int64_t>(HighLimit - LowLimit);
	}

	return meminfo;
}

Dictionary OS_Windows::execute_with_pipe(const String &p_path, const List<String> &p_arguments, bool p_blocking) {
#define CLEAN_PIPES \
	if (pipe_in[0] != 0) { \
		CloseHandle(pipe_in[0]); \
	} \
	if (pipe_in[1] != 0) { \
		CloseHandle(pipe_in[1]); \
	} \
	if (pipe_out[0] != 0) { \
		CloseHandle(pipe_out[0]); \
	} \
	if (pipe_out[1] != 0) { \
		CloseHandle(pipe_out[1]); \
	} \
	if (pipe_err[0] != 0) { \
		CloseHandle(pipe_err[0]); \
	} \
	if (pipe_err[1] != 0) { \
		CloseHandle(pipe_err[1]); \
	}

	Dictionary ret;

	String path = p_path.is_absolute_path() ? fix_path(p_path) : p_path;
	String command = _quote_command_line_argument(path);
	for (const String &E : p_arguments) {
		command += " " + _quote_command_line_argument(E);
	}

	// Create pipes.
	HANDLE pipe_in[2] = { nullptr, nullptr };
	HANDLE pipe_out[2] = { nullptr, nullptr };
	HANDLE pipe_err[2] = { nullptr, nullptr };

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = true;
	sa.lpSecurityDescriptor = nullptr;

	ERR_FAIL_COND_V(!CreatePipe(&pipe_in[0], &pipe_in[1], &sa, 0), ret);
	if (!CreatePipe(&pipe_out[0], &pipe_out[1], &sa, 0)) {
		CLEAN_PIPES
		ERR_FAIL_V(ret);
	}
	if (!CreatePipe(&pipe_err[0], &pipe_err[1], &sa, 0)) {
		CLEAN_PIPES
		ERR_FAIL_V(ret);
	}
	ERR_FAIL_COND_V(!SetHandleInformation(pipe_err[0], HANDLE_FLAG_INHERIT, 0), ret);

	// Create process.
	ProcessInfo pi;
	ZeroMemory(&pi.si, sizeof(pi.si));
	pi.si.StartupInfo.cb = sizeof(pi.si);
	ZeroMemory(&pi.pi, sizeof(pi.pi));
	LPSTARTUPINFOW si_w = (LPSTARTUPINFOW)&pi.si.StartupInfo;

	pi.si.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
	pi.si.StartupInfo.hStdInput = pipe_in[0];
	pi.si.StartupInfo.hStdOutput = pipe_out[1];
	pi.si.StartupInfo.hStdError = pipe_err[1];

	SIZE_T attr_list_size = 0;
	InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_list_size);
	pi.si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)alloca(attr_list_size);
	if (!InitializeProcThreadAttributeList(pi.si.lpAttributeList, 1, 0, &attr_list_size)) {
		CLEAN_PIPES
		ERR_FAIL_V(ret);
	}
	HANDLE handles_to_inherit[] = { pipe_in[0], pipe_out[1], pipe_err[1] };
	if (!UpdateProcThreadAttribute(
				pi.si.lpAttributeList,
				0,
				PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
				handles_to_inherit,
				sizeof(handles_to_inherit),
				nullptr,
				nullptr)) {
		CLEAN_PIPES
		DeleteProcThreadAttributeList(pi.si.lpAttributeList);
		ERR_FAIL_V(ret);
	}

	DWORD creation_flags = NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT;

	Char16String current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
	if (current_dir_name.size() >= MAX_PATH) {
		Char16String current_short_dir_name;
		str_len = GetShortPathNameW((LPCWSTR)current_dir_name.ptr(), nullptr, 0);
		current_short_dir_name.resize_uninitialized(str_len);
		GetShortPathNameW((LPCWSTR)current_dir_name.ptr(), (LPWSTR)current_short_dir_name.ptrw(), current_short_dir_name.size());
		current_dir_name = current_short_dir_name;
	}

	if (!CreateProcessW(nullptr, (LPWSTR)(command.utf16().ptrw()), nullptr, nullptr, true, creation_flags, nullptr, (LPWSTR)current_dir_name.ptr(), si_w, &pi.pi)) {
		CLEAN_PIPES
		DeleteProcThreadAttributeList(pi.si.lpAttributeList);
		ERR_FAIL_V_MSG(ret, "Could not create child process: " + command);
	}
	CloseHandle(pipe_in[0]);
	CloseHandle(pipe_out[1]);
	CloseHandle(pipe_err[1]);
	DeleteProcThreadAttributeList(pi.si.lpAttributeList);

	ProcessID pid = pi.pi.dwProcessId;
	process_map_mutex.lock();
	process_map->insert(pid, pi);
	process_map_mutex.unlock();

	Ref<FileAccessWindowsPipe> main_pipe;
	main_pipe.instantiate();
	main_pipe->open_existing(pipe_out[0], pipe_in[1], p_blocking);

	Ref<FileAccessWindowsPipe> err_pipe;
	err_pipe.instantiate();
	err_pipe->open_existing(pipe_err[0], nullptr, p_blocking);

	ret["stdio"] = main_pipe;
	ret["stderr"] = err_pipe;
	ret["pid"] = pid;

#undef CLEAN_PIPES
	return ret;
}

Error OS_Windows::execute(const String &p_path, const List<String> &p_arguments, String *r_pipe, int *r_exitcode, bool read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	String path = p_path.is_absolute_path() ? fix_path(p_path) : p_path;
	String command = _quote_command_line_argument(path);
	for (const String &E : p_arguments) {
		command += " " + _quote_command_line_argument(E);
	}

	ProcessInfo pi;
	ZeroMemory(&pi.si, sizeof(pi.si));
	pi.si.StartupInfo.cb = sizeof(pi.si);
	ZeroMemory(&pi.pi, sizeof(pi.pi));
	LPSTARTUPINFOW si_w = (LPSTARTUPINFOW)&pi.si.StartupInfo;

	bool inherit_handles = false;
	HANDLE pipe[2] = { nullptr, nullptr };
	if (r_pipe) {
		// Create pipe for StdOut and StdErr.
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = true;
		sa.lpSecurityDescriptor = nullptr;

		ERR_FAIL_COND_V(!CreatePipe(&pipe[0], &pipe[1], &sa, 0), ERR_CANT_FORK);

		pi.si.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
		pi.si.StartupInfo.hStdOutput = pipe[1];
		if (read_stderr) {
			pi.si.StartupInfo.hStdError = pipe[1];
		}

		SIZE_T attr_list_size = 0;
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_list_size);
		pi.si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)alloca(attr_list_size);
		if (!InitializeProcThreadAttributeList(pi.si.lpAttributeList, 1, 0, &attr_list_size)) {
			CloseHandle(pipe[0]); // Cleanup pipe handles.
			CloseHandle(pipe[1]);
			ERR_FAIL_V(ERR_CANT_FORK);
		}
		if (!UpdateProcThreadAttribute(
					pi.si.lpAttributeList,
					0,
					PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
					&pipe[1],
					sizeof(HANDLE),
					nullptr,
					nullptr)) {
			CloseHandle(pipe[0]); // Cleanup pipe handles.
			CloseHandle(pipe[1]);
			DeleteProcThreadAttributeList(pi.si.lpAttributeList);
			ERR_FAIL_V(ERR_CANT_FORK);
		}
		inherit_handles = true;
	}
	DWORD creation_flags = NORMAL_PRIORITY_CLASS;
	if (inherit_handles) {
		creation_flags |= EXTENDED_STARTUPINFO_PRESENT;
	}
	if (p_open_console) {
		creation_flags |= CREATE_NEW_CONSOLE;
	} else {
		creation_flags |= CREATE_NO_WINDOW;
	}

	Char16String current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
	if (current_dir_name.size() >= MAX_PATH) {
		Char16String current_short_dir_name;
		str_len = GetShortPathNameW((LPCWSTR)current_dir_name.ptr(), nullptr, 0);
		current_short_dir_name.resize_uninitialized(str_len);
		GetShortPathNameW((LPCWSTR)current_dir_name.ptr(), (LPWSTR)current_short_dir_name.ptrw(), current_short_dir_name.size());
		current_dir_name = current_short_dir_name;
	}

	int ret = CreateProcessW(nullptr, (LPWSTR)(command.utf16().ptrw()), nullptr, nullptr, inherit_handles, creation_flags, nullptr, (LPWSTR)current_dir_name.ptr(), si_w, &pi.pi);
	if (!ret && r_pipe) {
		CloseHandle(pipe[0]); // Cleanup pipe handles.
		CloseHandle(pipe[1]);
		DeleteProcThreadAttributeList(pi.si.lpAttributeList);
	}
	ERR_FAIL_COND_V_MSG(ret == 0, ERR_CANT_FORK, "Could not create child process: " + command);

	if (r_pipe) {
		CloseHandle(pipe[1]); // Close pipe write handle (only child process is writing).

		LocalVector<char> bytes;
		int bytes_in_buffer = 0;

		const int CHUNK_SIZE = 4096;
		DWORD read = 0;
		for (;;) { // Read StdOut and StdErr from pipe.
			bytes.resize(bytes_in_buffer + CHUNK_SIZE);
			const bool success = ReadFile(pipe[0], bytes.ptr() + bytes_in_buffer, CHUNK_SIZE, &read, nullptr);
			if (!success || read == 0) {
				break;
			}

			// Assume that all possible encodings are ASCII-compatible.
			// Break at newline to allow receiving long output in portions.
			int newline_index = -1;
			for (int i = read - 1; i >= 0; i--) {
				if (bytes[bytes_in_buffer + i] == '\n') {
					newline_index = i;
					break;
				}
			}
			if (newline_index == -1) {
				bytes_in_buffer += read;
				continue;
			}

			const int bytes_to_convert = bytes_in_buffer + (newline_index + 1);
			_append_to_pipe(bytes.ptr(), bytes_to_convert, r_pipe, p_pipe_mutex);

			bytes_in_buffer = read - (newline_index + 1);
			memmove(bytes.ptr(), bytes.ptr() + bytes_to_convert, bytes_in_buffer);
		}

		if (bytes_in_buffer > 0) {
			_append_to_pipe(bytes.ptr(), bytes_in_buffer, r_pipe, p_pipe_mutex);
		}

		CloseHandle(pipe[0]); // Close pipe read handle.
	}
	WaitForSingleObject(pi.pi.hProcess, INFINITE);

	if (r_exitcode) {
		DWORD ret2;
		GetExitCodeProcess(pi.pi.hProcess, &ret2);
		*r_exitcode = ret2;
	}

	CloseHandle(pi.pi.hProcess);
	CloseHandle(pi.pi.hThread);
	if (r_pipe) {
		DeleteProcThreadAttributeList(pi.si.lpAttributeList);
	}

	return OK;
}

Error OS_Windows::create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id, bool p_open_console) {
	String path = p_path.is_absolute_path() ? fix_path(p_path) : p_path;
	String command = _quote_command_line_argument(path);
	for (const String &E : p_arguments) {
		command += " " + _quote_command_line_argument(E);
	}

	ProcessInfo pi;
	ZeroMemory(&pi.si, sizeof(pi.si));
	pi.si.StartupInfo.cb = sizeof(pi.si.StartupInfo);
	ZeroMemory(&pi.pi, sizeof(pi.pi));
	LPSTARTUPINFOW si_w = (LPSTARTUPINFOW)&pi.si.StartupInfo;

	DWORD creation_flags = NORMAL_PRIORITY_CLASS;
	if (p_open_console) {
		creation_flags |= CREATE_NEW_CONSOLE;
	} else {
		creation_flags |= CREATE_NO_WINDOW;
	}

	Char16String current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
	if (current_dir_name.size() >= MAX_PATH) {
		Char16String current_short_dir_name;
		str_len = GetShortPathNameW((LPCWSTR)current_dir_name.ptr(), nullptr, 0);
		current_short_dir_name.resize_uninitialized(str_len);
		GetShortPathNameW((LPCWSTR)current_dir_name.ptr(), (LPWSTR)current_short_dir_name.ptrw(), current_short_dir_name.size());
		current_dir_name = current_short_dir_name;
	}

	int ret = CreateProcessW(nullptr, (LPWSTR)(command.utf16().ptrw()), nullptr, nullptr, false, creation_flags, nullptr, (LPWSTR)current_dir_name.ptr(), si_w, &pi.pi);
	ERR_FAIL_COND_V_MSG(ret == 0, ERR_CANT_FORK, "Could not create child process: " + command);

	ProcessID pid = pi.pi.dwProcessId;
	if (r_child_id) {
		*r_child_id = pid;
	}
	process_map_mutex.lock();
	process_map->insert(pid, pi);
	process_map_mutex.unlock();

	return OK;
}

Error OS_Windows::kill(const ProcessID &p_pid) {
	int ret = 0;
	MutexLock lock(process_map_mutex);
	if (process_map->has(p_pid)) {
		const PROCESS_INFORMATION pi = (*process_map)[p_pid].pi;
		process_map->erase(p_pid);

		ret = TerminateProcess(pi.hProcess, 0);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} else {
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, (DWORD)p_pid);
		if (hProcess != nullptr) {
			ret = TerminateProcess(hProcess, 0);

			CloseHandle(hProcess);
		}
	}

	return ret != 0 ? OK : FAILED;
}

int OS_Windows::get_process_id() const {
	return _getpid();
}

bool OS_Windows::is_process_running(const ProcessID &p_pid) const {
	MutexLock lock(process_map_mutex);
	if (!process_map->has(p_pid)) {
		return false;
	}

	const ProcessInfo &info = (*process_map)[p_pid];
	if (!info.is_running) {
		return false;
	}

	const PROCESS_INFORMATION &pi = info.pi;
	DWORD dw_exit_code = 0;
	if (!GetExitCodeProcess(pi.hProcess, &dw_exit_code)) {
		return false;
	}

	if (dw_exit_code != STILL_ACTIVE) {
		info.is_running = false;
		info.exit_code = dw_exit_code;
		return false;
	}

	return true;
}

int OS_Windows::get_process_exit_code(const ProcessID &p_pid) const {
	MutexLock lock(process_map_mutex);
	if (!process_map->has(p_pid)) {
		return -1;
	}

	const ProcessInfo &info = (*process_map)[p_pid];
	if (!info.is_running) {
		return info.exit_code;
	}

	const PROCESS_INFORMATION &pi = info.pi;

	DWORD dw_exit_code = 0;
	if (!GetExitCodeProcess(pi.hProcess, &dw_exit_code)) {
		return -1;
	}

	if (dw_exit_code == STILL_ACTIVE) {
		return -1;
	}

	info.is_running = false;
	info.exit_code = dw_exit_code;
	return dw_exit_code;
}

Error OS_Windows::set_cwd(const String &p_cwd) {
	if (_wchdir((LPCWSTR)(p_cwd.utf16().get_data())) != 0) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

String OS_Windows::get_cwd() const {
	Char16String real_current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	real_current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
	return String::utf16((const char16_t *)real_current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace_char('\\', '/');
}

Vector<String> OS_Windows::get_system_fonts() const {
	if (!dwrite_init) {
		return Vector<String>();
	}

	Vector<String> ret;
	HashSet<String> font_names;

	UINT32 family_count = font_collection->GetFontFamilyCount();
	for (UINT32 i = 0; i < family_count; i++) {
		ComAutoreleaseRef<IDWriteFontFamily> family;
		HRESULT hr = font_collection->GetFontFamily(i, &family.reference);
		ERR_CONTINUE(FAILED(hr) || family.is_null());

		ComAutoreleaseRef<IDWriteLocalizedStrings> family_names;
		hr = family->GetFamilyNames(&family_names.reference);
		ERR_CONTINUE(FAILED(hr) || family_names.is_null());

		UINT32 index = 0;
		BOOL exists = false;
		UINT32 length = 0;
		Char16String name;

		hr = family_names->FindLocaleName(L"en-us", &index, &exists);
		ERR_CONTINUE(FAILED(hr));

		hr = family_names->GetStringLength(index, &length);
		ERR_CONTINUE(FAILED(hr));

		name.resize_uninitialized(length + 1);
		hr = family_names->GetString(index, (WCHAR *)name.ptrw(), length + 1);
		ERR_CONTINUE(FAILED(hr));

		font_names.insert(String::utf16(name.ptr(), length));
	}

	for (const String &E : font_names) {
		ret.push_back(E);
	}
	return ret;
}

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wnon-virtual-dtor") // Silence warning due to a COM API weirdness.

class FallbackTextAnalysisSource : public IDWriteTextAnalysisSource {
	LONG _cRef = 1;

	bool rtl = false;
	Char16String string;
	Char16String locale;
	IDWriteNumberSubstitution *n_sub = nullptr;

public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) override {
		if (IID_IUnknown == riid) {
			AddRef();
			*ppvInterface = (IUnknown *)this;
		} else if (__uuidof(IMMNotificationClient) == riid) {
			AddRef();
			*ppvInterface = (IMMNotificationClient *)this;
		} else {
			*ppvInterface = nullptr;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() override {
		return InterlockedIncrement(&_cRef);
	}

	ULONG STDMETHODCALLTYPE Release() override {
		ULONG ulRef = InterlockedDecrement(&_cRef);
		if (0 == ulRef) {
			delete this;
		}
		return ulRef;
	}

	HRESULT STDMETHODCALLTYPE GetTextAtPosition(UINT32 p_text_position, WCHAR const **r_text_string, UINT32 *r_text_length) override {
		if (p_text_position >= (UINT32)string.length()) {
			*r_text_string = nullptr;
			*r_text_length = 0;
			return S_OK;
		}
		*r_text_string = reinterpret_cast<const wchar_t *>(string.get_data()) + p_text_position;
		*r_text_length = string.length() - p_text_position;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetTextBeforePosition(UINT32 p_text_position, WCHAR const **r_text_string, UINT32 *r_text_length) override {
		if (p_text_position < 1 || p_text_position >= (UINT32)string.length()) {
			*r_text_string = nullptr;
			*r_text_length = 0;
			return S_OK;
		}
		*r_text_string = reinterpret_cast<const wchar_t *>(string.get_data());
		*r_text_length = p_text_position;
		return S_OK;
	}

	DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() override {
		return (rtl) ? DWRITE_READING_DIRECTION_RIGHT_TO_LEFT : DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
	}

	HRESULT STDMETHODCALLTYPE GetLocaleName(UINT32 p_text_position, UINT32 *r_text_length, WCHAR const **r_locale_name) override {
		*r_locale_name = reinterpret_cast<const wchar_t *>(locale.get_data());
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetNumberSubstitution(UINT32 p_text_position, UINT32 *r_text_length, IDWriteNumberSubstitution **r_number_substitution) override {
		*r_number_substitution = n_sub;
		return S_OK;
	}

	FallbackTextAnalysisSource(const Char16String &p_text, const Char16String &p_locale, bool p_rtl, IDWriteNumberSubstitution *p_nsub) {
		_cRef = 1;
		string = p_text;
		locale = p_locale;
		n_sub = p_nsub;
		rtl = p_rtl;
	}

	virtual ~FallbackTextAnalysisSource() {}
};

VLTR_GCC_WARNING_POP

String OS_Windows::_get_default_fontname(const String &p_font_name) const {
	String font_name = p_font_name;
	if (font_name.to_lower() == "sans-serif") {
		font_name = "Arial";
	} else if (font_name.to_lower() == "serif") {
		font_name = "Times New Roman";
	} else if (font_name.to_lower() == "monospace") {
		font_name = "Courier New";
	} else if (font_name.to_lower() == "cursive") {
		font_name = "Comic Sans MS";
	} else if (font_name.to_lower() == "fantasy") {
		font_name = "Gabriola";
	}
	return font_name;
}

DWRITE_FONT_WEIGHT OS_Windows::_weight_to_dw(int p_weight) const {
	if (p_weight < 150) {
		return DWRITE_FONT_WEIGHT_THIN;
	} else if (p_weight < 250) {
		return DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
	} else if (p_weight < 325) {
		return DWRITE_FONT_WEIGHT_LIGHT;
	} else if (p_weight < 375) {
		return DWRITE_FONT_WEIGHT_SEMI_LIGHT;
	} else if (p_weight < 450) {
		return DWRITE_FONT_WEIGHT_NORMAL;
	} else if (p_weight < 550) {
		return DWRITE_FONT_WEIGHT_MEDIUM;
	} else if (p_weight < 650) {
		return DWRITE_FONT_WEIGHT_DEMI_BOLD;
	} else if (p_weight < 750) {
		return DWRITE_FONT_WEIGHT_BOLD;
	} else if (p_weight < 850) {
		return DWRITE_FONT_WEIGHT_EXTRA_BOLD;
	} else if (p_weight < 925) {
		return DWRITE_FONT_WEIGHT_BLACK;
	} else {
		return DWRITE_FONT_WEIGHT_EXTRA_BLACK;
	}
}

DWRITE_FONT_STRETCH OS_Windows::_stretch_to_dw(int p_stretch) const {
	if (p_stretch < 56) {
		return DWRITE_FONT_STRETCH_ULTRA_CONDENSED;
	} else if (p_stretch < 69) {
		return DWRITE_FONT_STRETCH_EXTRA_CONDENSED;
	} else if (p_stretch < 81) {
		return DWRITE_FONT_STRETCH_CONDENSED;
	} else if (p_stretch < 93) {
		return DWRITE_FONT_STRETCH_SEMI_CONDENSED;
	} else if (p_stretch < 106) {
		return DWRITE_FONT_STRETCH_NORMAL;
	} else if (p_stretch < 137) {
		return DWRITE_FONT_STRETCH_SEMI_EXPANDED;
	} else if (p_stretch < 144) {
		return DWRITE_FONT_STRETCH_EXPANDED;
	} else if (p_stretch < 162) {
		return DWRITE_FONT_STRETCH_EXTRA_EXPANDED;
	} else {
		return DWRITE_FONT_STRETCH_ULTRA_EXPANDED;
	}
}

Vector<String> OS_Windows::get_system_font_path_for_text(const String &p_font_name, const String &p_text, const String &p_locale, const String &p_script, int p_weight, int p_stretch, bool p_italic) const {
	// This may be called before TextServerManager has been created, which would cause a crash downstream if we do not check here
	if (!dwrite2_init || !TextServerManager::is_initialized()) {
		return Vector<String>();
	}

	String font_name = _get_default_fontname(p_font_name);

	bool rtl = TS->is_locale_right_to_left(p_locale);
	Char16String text = p_text.utf16();
	Char16String locale = p_locale.utf16();

	ComAutoreleaseRef<IDWriteNumberSubstitution> number_substitution;
	HRESULT hr = dwrite_factory->CreateNumberSubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE, reinterpret_cast<const wchar_t *>(locale.get_data()), true, &number_substitution.reference);
	ERR_FAIL_COND_V(FAILED(hr) || number_substitution.is_null(), Vector<String>());

	FallbackTextAnalysisSource fs = FallbackTextAnalysisSource(text, locale, rtl, number_substitution.reference);
	UINT32 mapped_length = 0;
	FLOAT scale = 0.0;
	ComAutoreleaseRef<IDWriteFont> dwrite_font;
	hr = system_font_fallback->MapCharacters(
			&fs,
			0,
			(UINT32)text.length(),
			font_collection,
			reinterpret_cast<const wchar_t *>(font_name.utf16().get_data()),
			_weight_to_dw(p_weight),
			p_italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
			_stretch_to_dw(p_stretch),
			&mapped_length,
			&dwrite_font.reference,
			&scale);

	if (FAILED(hr) || dwrite_font.is_null()) {
		return Vector<String>();
	}

	ComAutoreleaseRef<IDWriteFontFace> dwrite_face;
	hr = dwrite_font->CreateFontFace(&dwrite_face.reference);
	if (FAILED(hr) || dwrite_face.is_null()) {
		return Vector<String>();
	}

	UINT32 number_of_files = 0;
	hr = dwrite_face->GetFiles(&number_of_files, nullptr);
	if (FAILED(hr)) {
		return Vector<String>();
	}
	Vector<ComAutoreleaseRef<IDWriteFontFile>> files;
	files.resize(number_of_files);
	hr = dwrite_face->GetFiles(&number_of_files, (IDWriteFontFile **)files.ptrw());
	if (FAILED(hr)) {
		return Vector<String>();
	}

	Vector<String> ret;
	for (UINT32 i = 0; i < number_of_files; i++) {
		const void *reference_key = nullptr;
		UINT32 reference_key_size = 0;
		ComAutoreleaseRef<IDWriteLocalFontFileLoader> loader;

		hr = files.write[i]->GetLoader((IDWriteFontFileLoader **)&loader.reference);
		if (FAILED(hr) || loader.is_null()) {
			continue;
		}
		hr = files.write[i]->GetReferenceKey(&reference_key, &reference_key_size);
		if (FAILED(hr)) {
			continue;
		}

		WCHAR file_path[32767];
		hr = loader->GetFilePathFromKey(reference_key, reference_key_size, &file_path[0], 32767);
		if (FAILED(hr)) {
			continue;
		}
		String fpath = String::utf16((const char16_t *)&file_path[0]).replace_char('\\', '/');

		WIN32_FIND_DATAW d;
		HANDLE fnd = FindFirstFileW((LPCWSTR)&file_path[0], &d);
		if (fnd != INVALID_HANDLE_VALUE) {
			String fname = String::utf16((const char16_t *)d.cFileName);
			if (!fname.is_empty()) {
				fpath = fpath.get_base_dir().path_join(fname);
			}
			FindClose(fnd);
		}
		ret.push_back(fpath);
	}
	return ret;
}

String OS_Windows::get_system_font_path(const String &p_font_name, int p_weight, int p_stretch, bool p_italic) const {
	if (!dwrite_init) {
		return String();
	}

	String font_name = _get_default_fontname(p_font_name);

	UINT32 index = 0;
	BOOL exists = false;
	HRESULT hr = font_collection->FindFamilyName((const WCHAR *)font_name.utf16().get_data(), &index, &exists);
	if (FAILED(hr) || !exists) {
		return String();
	}

	ComAutoreleaseRef<IDWriteFontFamily> family;
	hr = font_collection->GetFontFamily(index, &family.reference);
	if (FAILED(hr) || family.is_null()) {
		return String();
	}

	ComAutoreleaseRef<IDWriteFont> dwrite_font;
	hr = family->GetFirstMatchingFont(_weight_to_dw(p_weight), _stretch_to_dw(p_stretch), p_italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, &dwrite_font.reference);
	if (FAILED(hr) || dwrite_font.is_null()) {
		return String();
	}

	ComAutoreleaseRef<IDWriteFontFace> dwrite_face;
	hr = dwrite_font->CreateFontFace(&dwrite_face.reference);
	if (FAILED(hr) || dwrite_face.is_null()) {
		return String();
	}

	UINT32 number_of_files = 0;
	hr = dwrite_face->GetFiles(&number_of_files, nullptr);
	if (FAILED(hr)) {
		return String();
	}
	Vector<ComAutoreleaseRef<IDWriteFontFile>> files;
	files.resize(number_of_files);
	hr = dwrite_face->GetFiles(&number_of_files, (IDWriteFontFile **)files.ptrw());
	if (FAILED(hr)) {
		return String();
	}

	for (UINT32 i = 0; i < number_of_files; i++) {
		const void *reference_key = nullptr;
		UINT32 reference_key_size = 0;
		ComAutoreleaseRef<IDWriteLocalFontFileLoader> loader;

		hr = files.write[i]->GetLoader((IDWriteFontFileLoader **)&loader.reference);
		if (FAILED(hr) || loader.is_null()) {
			continue;
		}
		hr = files.write[i]->GetReferenceKey(&reference_key, &reference_key_size);
		if (FAILED(hr)) {
			continue;
		}

		WCHAR file_path[32767];
		hr = loader->GetFilePathFromKey(reference_key, reference_key_size, &file_path[0], 32767);
		if (FAILED(hr)) {
			continue;
		}
		String fpath = String::utf16((const char16_t *)&file_path[0]).replace_char('\\', '/');

		WIN32_FIND_DATAW d;
		HANDLE fnd = FindFirstFileW((LPCWSTR)&file_path[0], &d);
		if (fnd != INVALID_HANDLE_VALUE) {
			String fname = String::utf16((const char16_t *)d.cFileName);
			if (!fname.is_empty()) {
				fpath = fpath.get_base_dir().path_join(fname);
			}
			FindClose(fnd);
		}

		return fpath;
	}
	return String();
}

String OS_Windows::get_executable_path() const {
	WCHAR bufname[4096];
	GetModuleFileNameW(nullptr, bufname, 4096);
	String s = String::utf16((const char16_t *)bufname).replace_char('\\', '/');
	return s;
}

bool OS_Windows::has_environment(const String &p_var) const {
	return GetEnvironmentVariableW((LPCWSTR)(p_var.utf16().get_data()), nullptr, 0) > 0;
}

String OS_Windows::get_environment(const String &p_var) const {
	WCHAR wval[0x7fff]; // MSDN says 32767 char is the maximum
	int wlen = GetEnvironmentVariableW((LPCWSTR)(p_var.utf16().get_data()), wval, 0x7fff);
	if (wlen > 0) {
		return String::utf16((const char16_t *)wval);
	}
	return "";
}

void OS_Windows::set_environment(const String &p_var, const String &p_value) const {
	ERR_FAIL_COND_MSG(p_var.is_empty() || p_var.contains_char('='), vformat("Invalid environment variable name '%s', cannot be empty or include '='.", p_var));
	Char16String var = p_var.utf16();
	Char16String value = p_value.utf16();
	ERR_FAIL_COND_MSG(var.length() + value.length() + 2 > 32767, vformat("Invalid definition for environment variable '%s', cannot exceed 32767 characters.", p_var));
	SetEnvironmentVariableW((LPCWSTR)(var.get_data()), (LPCWSTR)(value.get_data()));
}

void OS_Windows::unset_environment(const String &p_var) const {
	ERR_FAIL_COND_MSG(p_var.is_empty() || p_var.contains_char('='), vformat("Invalid environment variable name '%s', cannot be empty or include '='.", p_var));
	SetEnvironmentVariableW((LPCWSTR)(p_var.utf16().get_data()), nullptr); // Null to delete.
}

String OS_Windows::get_stdin_string(int64_t p_buffer_size) {
	if (get_stdin_type() == STD_HANDLE_INVALID) {
		return String();
	}

	Vector<uint8_t> data;
	data.resize(p_buffer_size);
	DWORD count = 0;
	if (ReadFile(GetStdHandle(STD_INPUT_HANDLE), data.ptrw(), data.size(), &count, nullptr)) {
		return String::utf8((const char *)data.ptr(), count).replace("\r\n", "\n").rstrip("\n");
	}

	return String();
}

PackedByteArray OS_Windows::get_stdin_buffer(int64_t p_buffer_size) {
	Vector<uint8_t> data;
	data.resize(p_buffer_size);
	DWORD count = 0;
	if (ReadFile(GetStdHandle(STD_INPUT_HANDLE), data.ptrw(), data.size(), &count, nullptr)) {
		return data;
	}

	return PackedByteArray();
}

OS_Windows::StdHandleType OS_Windows::get_stdin_type() const {
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	if (h == nullptr || h == INVALID_HANDLE_VALUE) {
		return STD_HANDLE_INVALID;
	}
	DWORD ftype = GetFileType(h);
	if (ftype == FILE_TYPE_UNKNOWN && GetLastError() != ERROR_SUCCESS) {
		return STD_HANDLE_UNKNOWN;
	}
	ftype &= ~(FILE_TYPE_REMOTE);

	if (ftype == FILE_TYPE_DISK) {
		return STD_HANDLE_FILE;
	} else if (ftype == FILE_TYPE_PIPE) {
		return STD_HANDLE_PIPE;
	} else {
		DWORD conmode = 0;
		BOOL res = GetConsoleMode(h, &conmode);
		if (!res && (GetLastError() == ERROR_INVALID_HANDLE)) {
			return STD_HANDLE_UNKNOWN; // Unknown character device.
		} else {
#ifndef WINDOWS_SUBSYSTEM_CONSOLE
			if (!is_using_con_wrapper()) {
				return STD_HANDLE_INVALID; // Window app can't read stdin input without werapper.
			}
#endif
			return STD_HANDLE_CONSOLE;
		}
	}
}

OS_Windows::StdHandleType OS_Windows::get_stdout_type() const {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (h == nullptr || h == INVALID_HANDLE_VALUE) {
		return STD_HANDLE_INVALID;
	}
	DWORD ftype = GetFileType(h);
	if (ftype == FILE_TYPE_UNKNOWN && GetLastError() != ERROR_SUCCESS) {
		return STD_HANDLE_UNKNOWN;
	}
	ftype &= ~(FILE_TYPE_REMOTE);

	if (ftype == FILE_TYPE_DISK) {
		return STD_HANDLE_FILE;
	} else if (ftype == FILE_TYPE_PIPE) {
		return STD_HANDLE_PIPE;
	} else {
		DWORD conmode = 0;
		BOOL res = GetConsoleMode(h, &conmode);
		if (!res && (GetLastError() == ERROR_INVALID_HANDLE)) {
			return STD_HANDLE_UNKNOWN; // Unknown character device.
		} else {
			return STD_HANDLE_CONSOLE;
		}
	}
}

OS_Windows::StdHandleType OS_Windows::get_stderr_type() const {
	HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
	if (h == nullptr || h == INVALID_HANDLE_VALUE) {
		return STD_HANDLE_INVALID;
	}
	DWORD ftype = GetFileType(h);
	if (ftype == FILE_TYPE_UNKNOWN && GetLastError() != ERROR_SUCCESS) {
		return STD_HANDLE_UNKNOWN;
	}
	ftype &= ~(FILE_TYPE_REMOTE);

	if (ftype == FILE_TYPE_DISK) {
		return STD_HANDLE_FILE;
	} else if (ftype == FILE_TYPE_PIPE) {
		return STD_HANDLE_PIPE;
	} else {
		DWORD conmode = 0;
		BOOL res = GetConsoleMode(h, &conmode);
		if (!res && (GetLastError() == ERROR_INVALID_HANDLE)) {
			return STD_HANDLE_UNKNOWN; // Unknown character device.
		} else {
			return STD_HANDLE_CONSOLE;
		}
	}
}

Error OS_Windows::shell_open(const String &p_uri) {
	INT_PTR ret = (INT_PTR)ShellExecuteW(nullptr, nullptr, (LPCWSTR)(p_uri.utf16().get_data()), nullptr, nullptr, SW_SHOWNORMAL);
	if (ret > 32) {
		return OK;
	} else {
		switch (ret) {
			case ERROR_FILE_NOT_FOUND:
			case SE_ERR_DLLNOTFOUND:
				return ERR_FILE_NOT_FOUND;
			case ERROR_PATH_NOT_FOUND:
				return ERR_FILE_BAD_PATH;
			case ERROR_BAD_FORMAT:
				return ERR_FILE_CORRUPT;
			case SE_ERR_ACCESSDENIED:
				return ERR_UNAUTHORIZED;
			case 0:
			case SE_ERR_OOM:
				return ERR_OUT_OF_MEMORY;
			default:
				return FAILED;
		}
	}
}

Error OS_Windows::shell_show_in_file_manager(String p_path, bool p_open_folder) {
	bool open_folder = false;
	if (DirAccess::dir_exists_absolute(p_path) && p_open_folder) {
		open_folder = true;
	}

	if (!p_path.is_quoted()) {
		p_path = p_path.quote();
	}
	p_path = fix_path(p_path);

	INT_PTR ret = OK;
	if (open_folder) {
		ret = (INT_PTR)ShellExecuteW(nullptr, nullptr, L"explorer.exe", LPCWSTR(p_path.utf16().get_data()), nullptr, SW_SHOWNORMAL);
	} else {
		ret = (INT_PTR)ShellExecuteW(nullptr, nullptr, L"explorer.exe", LPCWSTR((String("/select,") + p_path).utf16().get_data()), nullptr, SW_SHOWNORMAL);
	}

	if (ret > 32) {
		return OK;
	} else {
		switch (ret) {
			case ERROR_FILE_NOT_FOUND:
			case SE_ERR_DLLNOTFOUND:
				return ERR_FILE_NOT_FOUND;
			case ERROR_PATH_NOT_FOUND:
				return ERR_FILE_BAD_PATH;
			case ERROR_BAD_FORMAT:
				return ERR_FILE_CORRUPT;
			case SE_ERR_ACCESSDENIED:
				return ERR_UNAUTHORIZED;
			case 0:
			case SE_ERR_OOM:
				return ERR_OUT_OF_MEMORY;
			default:
				return FAILED;
		}
	}
}

String OS_Windows::get_locale() const {
	const _WinLocale *wl = &_win_locales[0];

	LANGID langid = GetUserDefaultUILanguage();
	String neutral;
	int lang = PRIMARYLANGID(langid);
	int sublang = SUBLANGID(langid);

	while (wl->locale) {
		if (wl->main_lang == lang && wl->sublang == SUBLANG_NEUTRAL) {
			neutral = wl->locale;
		}

		if (lang == wl->main_lang && sublang == wl->sublang) {
			return String(wl->locale).replace_char('-', '_');
		}

		wl++;
	}

	if (!neutral.is_empty()) {
		return String(neutral).replace_char('-', '_');
	}

	return "en";
}

Vector<String> OS_Windows::get_preferred_locales() const {
	Vector<String> out = WinRTUtils::get_preferred_locales();
	if (out.is_empty()) {
		out.push_back(get_locale());
	}
	return out;
}

String OS_Windows::get_model_name() const {
	HKEY hkey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\BIOS", 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS) {
		return OS::get_model_name();
	}

	String sys_name;
	String board_name;
	WCHAR buffer[256];
	DWORD buffer_len = 256;
	DWORD vtype = REG_SZ;
	if (RegQueryValueExW(hkey, L"SystemProductName", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) == ERROR_SUCCESS && buffer_len != 0) {
		sys_name = String::utf16((const char16_t *)buffer, buffer_len).strip_edges();
	}
	buffer_len = 256;
	if (RegQueryValueExW(hkey, L"BaseBoardProduct", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) == ERROR_SUCCESS && buffer_len != 0) {
		board_name = String::utf16((const char16_t *)buffer, buffer_len).strip_edges();
	}
	RegCloseKey(hkey);
	if (!sys_name.is_empty() && sys_name.to_lower() != "system product name") {
		return sys_name;
	}
	if (!board_name.is_empty() && board_name.to_lower() != "base board product") {
		return board_name;
	}
	return OS::get_model_name();
}

String OS_Windows::get_processor_name() const {
	const String id = "Hardware\\Description\\System\\CentralProcessor\\0";

	HKEY hkey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, (LPCWSTR)(id.utf16().get_data()), 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS) {
		ERR_FAIL_V_MSG("", String("Couldn't get the CPU model name. Returning an empty string."));
	}

	WCHAR buffer[256];
	DWORD buffer_len = 256;
	DWORD vtype = REG_SZ;
	if (RegQueryValueExW(hkey, L"ProcessorNameString", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) == ERROR_SUCCESS) {
		RegCloseKey(hkey);
		return String::utf16((const char16_t *)buffer, buffer_len).strip_edges();
	} else {
		RegCloseKey(hkey);
		ERR_FAIL_V_MSG("", String("Couldn't get the CPU model name. Returning an empty string."));
	}
}

void OS_Windows::run() {
	if (!MainLoop::is_active()) {
		return;
	}

	MainLoop::initialize();

	while (true) {
		GodotProfileFrameMark;
		GodotProfileZone("OS_Windows::run");
		DisplayServer::process_events(); // get rid of pending events
		if (Main::iteration()) {
			break;
		}
	}

	MainLoop::finalize();
}

uint64_t OS_Windows::get_embedded_pck_offset() const {
	Ref<FileAccess> f = FileAccess::open(get_executable_path(), FileAccess::READ);
	if (f.is_null()) {
		return 0;
	}

	// Process header.
	{
		f->seek(0x3c);
		uint32_t pe_pos = f->get_32();

		f->seek(pe_pos);
		uint32_t magic = f->get_32();
		if (magic != 0x00004550) {
			return 0;
		}
	}

	int num_sections;
	{
		int64_t header_pos = f->get_position();

		f->seek(header_pos + 2);
		num_sections = f->get_16();
		f->seek(header_pos + 16);
		uint16_t opt_header_size = f->get_16();

		// Skip rest of header + optional header to go to the section headers.
		f->seek(f->get_position() + 2 + opt_header_size);
	}
	int64_t section_table_pos = f->get_position();

	// Search for the "pck" section.
	int64_t off = 0;
	for (int i = 0; i < num_sections; ++i) {
		int64_t section_header_pos = section_table_pos + i * 40;
		f->seek(section_header_pos);

		uint8_t section_name[9];
		f->get_buffer(section_name, 8);
		section_name[8] = '\0';

		if (strcmp((char *)section_name, "pck") == 0) {
			f->seek(section_header_pos + 20);
			off = f->get_32();
			break;
		}
	}

	return off;
}

String OS_Windows::get_config_path() const {
	if (has_environment("APPDATA")) {
		return get_environment("APPDATA").replace_char('\\', '/');
	}
	return ".";
}

String OS_Windows::get_data_path() const {
	return get_config_path();
}

String OS_Windows::get_cache_path() const {
	static String cache_path_cache;
	if (cache_path_cache.is_empty()) {
		if (has_environment("LOCALAPPDATA")) {
			cache_path_cache = get_environment("LOCALAPPDATA").replace_char('\\', '/');
		}
		if (cache_path_cache.is_empty()) {
			cache_path_cache = get_temp_path();
		}
	}
	return cache_path_cache;
}

String OS_Windows::get_temp_path() const {
	static String temp_path_cache;
	if (temp_path_cache.is_empty()) {
		{
			Vector<WCHAR> temp_path;
			// The maximum possible size is MAX_PATH+1 (261) + terminating null character.
			temp_path.resize(MAX_PATH + 2);
			DWORD temp_path_length = GetTempPathW(temp_path.size(), temp_path.ptrw());
			if (temp_path_length > 0 && temp_path_length < temp_path.size()) {
				temp_path_cache = String::utf16((const char16_t *)temp_path.ptr());
				// Let's try to get the long path instead of the short path (with tildes ~).
				DWORD temp_path_long_length = GetLongPathNameW(temp_path.ptr(), temp_path.ptrw(), temp_path.size());
				if (temp_path_long_length > 0 && temp_path_long_length < temp_path.size()) {
					temp_path_cache = String::utf16((const char16_t *)temp_path.ptr());
				}
			}
		}
		if (temp_path_cache.is_empty()) {
			temp_path_cache = get_config_path();
		}
	}
	return temp_path_cache.replace_char('\\', '/').trim_suffix("/");
}

// Get properly capitalized engine name for system paths
String OS_Windows::get_godot_dir_name() const {
	return String(VLTR_VERSION_SHORT_NAME).capitalize();
}

String OS_Windows::get_system_dir(SystemDir p_dir, bool p_shared_storage) const {
	KNOWNFOLDERID id;

	switch (p_dir) {
		case SYSTEM_DIR_DESKTOP: {
			id = FOLDERID_Desktop;
		} break;
		case SYSTEM_DIR_DCIM: {
			id = FOLDERID_Pictures;
		} break;
		case SYSTEM_DIR_DOCUMENTS: {
			id = FOLDERID_Documents;
		} break;
		case SYSTEM_DIR_DOWNLOADS: {
			id = FOLDERID_Downloads;
		} break;
		case SYSTEM_DIR_MOVIES: {
			id = FOLDERID_Videos;
		} break;
		case SYSTEM_DIR_MUSIC: {
			id = FOLDERID_Music;
		} break;
		case SYSTEM_DIR_PICTURES: {
			id = FOLDERID_Pictures;
		} break;
		case SYSTEM_DIR_RINGTONES: {
			id = FOLDERID_Music;
		} break;
	}

	PWSTR szPath;
	HRESULT res = SHGetKnownFolderPath(id, 0, nullptr, &szPath);
	ERR_FAIL_COND_V(res != S_OK, String());
	String path = String::utf16((const char16_t *)szPath).replace_char('\\', '/');
	CoTaskMemFree(szPath);
	return path;
}

String OS_Windows::get_user_data_dir(const String &p_user_dir) const {
	return get_data_path().path_join(p_user_dir).replace_char('\\', '/');
}

String OS_Windows::expand_path(const String &p_path) const {
	String path = p_path;

	if (path.begins_with("~/") || path.begins_with("~\\") || path == "~") {
		String home = get_environment("USERPROFILE");
		if (!home.is_empty()) {
			path = home + path.substr(1);
		}
	}

	return path;
}

String OS_Windows::get_unique_id() const {
	HW_PROFILE_INFOA HwProfInfo;
	ERR_FAIL_COND_V(!GetCurrentHwProfileA(&HwProfInfo), "");

	// Note: Windows API returns a GUID with null termination.
	return String::ascii(Span<char>(HwProfInfo.szHwProfileGuid, strnlen(HwProfInfo.szHwProfileGuid, HW_PROFILE_GUIDLEN)));
}

bool OS_Windows::_check_internal_feature_support(const String &p_feature) {
	if (p_feature == "system_fonts") {
		return dwrite_init;
	}
	if (p_feature == "pc") {
		return true;
	}

	return false;
}

void OS_Windows::disable_crash_handler() {
	crash_handler.disable();
}

bool OS_Windows::is_disable_crash_handler() const {
	return crash_handler.is_disabled();
}

Error OS_Windows::move_to_trash(const String &p_path) {
	SHFILEOPSTRUCTW sf;

	Char16String utf16 = p_path.utf16();
	WCHAR *from = new WCHAR[utf16.length() + 2];
	wcscpy_s(from, utf16.length() + 1, (LPCWSTR)(utf16.get_data()));
	from[utf16.length() + 1] = 0;

	sf.hwnd = main_window;
	sf.wFunc = FO_DELETE;
	sf.pFrom = from;
	sf.pTo = nullptr;
	sf.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
	sf.fAnyOperationsAborted = FALSE;
	sf.hNameMappings = nullptr;
	sf.lpszProgressTitle = nullptr;

	int ret = SHFileOperationW(&sf);
	delete[] from;

	if (ret) {
		ERR_PRINT("SHFileOperation error: " + itos(ret));
		return FAILED;
	}

	return OK;
}

String OS_Windows::get_system_ca_certificates() {
	HCERTSTORE cert_store = CertOpenSystemStoreA(0, "ROOT");
	ERR_FAIL_NULL_V_MSG(cert_store, "", "Failed to read the root certificate store.");

	FILETIME curr_time;
	GetSystemTimeAsFileTime(&curr_time);

	String certs;
	PCCERT_CONTEXT curr = CertEnumCertificatesInStore(cert_store, nullptr);
	while (curr) {
		FILETIME ft;
		DWORD size = sizeof(ft);
		// Check if the certificate is disallowed.
		if (CertGetCertificateContextProperty(curr, CERT_DISALLOWED_FILETIME_PROP_ID, &ft, &size) && CompareFileTime(&curr_time, &ft) != -1) {
			curr = CertEnumCertificatesInStore(cert_store, curr);
			continue;
		}
		// Encode and add to certificate list.
		bool success = CryptBinaryToStringA(curr->pbCertEncoded, curr->cbCertEncoded, CRYPT_STRING_BASE64HEADER | CRYPT_STRING_NOCR, nullptr, &size);
		ERR_CONTINUE(!success);
		PackedByteArray pba;
		pba.resize(size + 1);
		CryptBinaryToStringA(curr->pbCertEncoded, curr->cbCertEncoded, CRYPT_STRING_BASE64HEADER | CRYPT_STRING_NOCR, (char *)pba.ptrw(), &size);
		pba.write[size] = 0;
		certs += String::ascii(Span((const char *)pba.ptr(), strlen((const char *)pba.ptr())));
		curr = CertEnumCertificatesInStore(cert_store, curr);
	}
	CertCloseStore(cert_store, 0);
	return certs;
}

void OS_Windows::add_frame_delay(bool p_can_draw, bool p_wake_for_events) {
	if (p_wake_for_events) {
		uint64_t delay = get_frame_delay(p_can_draw);
		if (delay == 0) {
			return;
		}

		DisplayServerWindows *ds_win = DisplayServer::driver_as<DisplayServerWindows>();
		if (ds_win) {
			MsgWaitForMultipleObjects(0, nullptr, false, Math::floor(double(delay) / 1000.0), QS_ALLINPUT);
			return;
		}
	}

	const uint32_t frame_delay = Engine::get_frame_delay();
	if (frame_delay) {
		// Add fixed frame delay to decrease CPU/GPU usage. This doesn't take
		// the actual frame time into account.
		// Due to the high fluctuation of the actual sleep duration, it's not recommended
		// to use this as a FPS limiter.
		delay_usec(frame_delay * 1000);
	}

	// Add a dynamic frame delay to decrease CPU/GPU usage. This takes the
	// previous frame time into account for a smoother result.
	uint64_t dynamic_delay = 0;
	if (is_in_low_processor_usage_mode() || !p_can_draw) {
		dynamic_delay = get_low_processor_usage_mode_sleep_usec();
	}
	const int max_fps = Engine::get_max_fps();
	if (max_fps > 0 && !Engine::is_editor_hint()) {
		// Override the low processor usage mode sleep delay if the target FPS is lower.
		dynamic_delay = MAX(dynamic_delay, (uint64_t)(1000000 / max_fps));
	}

	if (dynamic_delay > 0) {
		target_ticks += dynamic_delay;
		uint64_t current_ticks = get_ticks_usec();

		if (!is_in_low_processor_usage_mode()) {
			if (target_ticks > current_ticks + delay_resolution) {
				uint64_t delay_time = target_ticks - current_ticks - delay_resolution;
				// Make sure we always sleep for a multiple of delay_resolution to avoid overshooting.
				// Refer to: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep#remarks
				delay_time = (delay_time / delay_resolution) * delay_resolution;
				if (delay_time > 0) {
					delay_usec(delay_time);
				}
			}
			// Busy wait for the remainder of time.
			while (get_ticks_usec() < target_ticks) {
				YieldProcessor();
			}
		} else {
			// Use a more relaxed approach for low processor usage mode.
			// This has worse frame pacing but is more power efficient.
			if (current_ticks < target_ticks) {
				delay_usec(target_ticks - current_ticks);
			}
		}

		current_ticks = get_ticks_usec();
		target_ticks = MIN(MAX(target_ticks, current_ticks - dynamic_delay), current_ticks + dynamic_delay);
	}
}

#ifdef TOOLS_ENABLED
bool OS_Windows::_test_create_rendering_device(const String &p_display_driver) const {
	// Tests Rendering Device creation.

	bool ok = false;
#if defined(RD_ENABLED)
	Error err;
	RenderingContextDriver *rcd = nullptr;

#if defined(VULKAN_ENABLED)
	rcd = memnew(RenderingContextDriverVulkan);
#endif
#ifdef D3D12_ENABLED
	if (rcd == nullptr) {
		rcd = memnew(RenderingContextDriverD3D12);
	}
#endif
	if (rcd != nullptr) {
		err = rcd->initialize();
		if (err == OK) {
			RenderingDevice *rd = memnew(RenderingDevice);
			err = rd->initialize(rcd);
			memdelete(rd);
			rd = nullptr;
			if (err == OK) {
				ok = true;
			}
		}
		memdelete(rcd);
		rcd = nullptr;
	}
#endif

	return ok;
}

bool OS_Windows::_test_create_rendering_device_and_gl(const String &p_display_driver) const {
	// Tests OpenGL context and Rendering Device simultaneous creation. This function is expected to crash on some NVIDIA drivers.

	WNDCLASSEXW wc_probe;
	memset(&wc_probe, 0, sizeof(WNDCLASSEXW));
	wc_probe.cbSize = sizeof(WNDCLASSEXW);
	wc_probe.style = CS_OWNDC | CS_DBLCLKS;
	wc_probe.lpfnWndProc = (WNDPROC)::DefWindowProcW;
	wc_probe.cbClsExtra = 0;
	wc_probe.cbWndExtra = 0;
	wc_probe.hInstance = GetModuleHandle(nullptr);
	wc_probe.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc_probe.hCursor = nullptr;
	wc_probe.hbrBackground = nullptr;
	wc_probe.lpszMenuName = nullptr;
	wc_probe.lpszClassName = L"Engine probe window";

	if (!RegisterClassExW(&wc_probe)) {
		return false;
	}

	HWND hWnd = CreateWindowExW(WS_EX_WINDOWEDGE, L"Engine probe window", L"", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	if (!hWnd) {
		UnregisterClassW(L"Engine probe window", GetModuleHandle(nullptr));
		return false;
	}

	bool ok = true;
#ifdef GLES3_ENABLED
	GLManagerNative_Windows *test_gl_manager_native = memnew(GLManagerNative_Windows);
	if (test_gl_manager_native->window_create(DisplayServerEnums::MAIN_WINDOW_ID, hWnd, GetModuleHandle(nullptr), 800, 600) == OK) {
		RasterizerGLES3::make_current(true);
	} else {
		ok = false;
	}
#endif

	MSG msg = {};
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	if (ok) {
		ok = _test_create_rendering_device(p_display_driver);
	}

#ifdef GLES3_ENABLED
	memdelete(test_gl_manager_native);
#endif

	DestroyWindow(hWnd);
	UnregisterClassW(L"Engine probe window", GetModuleHandle(nullptr));
	return ok;
}
#endif

#ifdef _MSC_VER
#define IAT_HOOK_CALL __declspec(guard(nocf))
#else
#define IAT_HOOK_CALL
#endif

using GetProcAddressType = FARPROC(__stdcall *)(HMODULE, LPCSTR);
GetProcAddressType Original_GetProcAddress = nullptr;

using HidD_GetProductStringType = BOOLEAN(__stdcall *)(HANDLE, void *, ULONG);
HidD_GetProductStringType Original_HidD_GetProductString = nullptr;

#ifndef HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER
#define HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER 0x08
#endif

bool _hid_is_controller(HANDLE p_hid_handle) {
	PHIDP_PREPARSED_DATA hid_preparsed = nullptr;
	BOOLEAN preparsed_res = HidD_GetPreparsedData(p_hid_handle, &hid_preparsed);
	if (!preparsed_res) {
		return false;
	}

	HIDP_CAPS hid_caps = {};
	NTSTATUS caps_res = HidP_GetCaps(hid_preparsed, &hid_caps);
	HidD_FreePreparsedData(hid_preparsed);
	if (caps_res != HIDP_STATUS_SUCCESS) {
		return false;
	}

	if (hid_caps.UsagePage != HID_USAGE_PAGE_GENERIC) {
		return false;
	}

	if (hid_caps.Usage == HID_USAGE_GENERIC_JOYSTICK || hid_caps.Usage == HID_USAGE_GENERIC_GAMEPAD || hid_caps.Usage == HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER) {
		return true;
	}

	return false;
}

IAT_HOOK_CALL BOOLEAN __stdcall Hook_HidD_GetProductString(HANDLE p_object, void *p_buffer, ULONG p_buffer_length) {
	constexpr const wchar_t unknown_product_string[] = L"Unknown HID Device";
	constexpr size_t unknown_product_length = sizeof(unknown_product_string);

	if (_hid_is_controller(p_object)) {
		return HidD_GetProductString(p_object, p_buffer, p_buffer_length);
	}

	// The HID is (probably) not a controller, so we don't care about returning its actual product string.
	// This avoids stalls on `EnumDevices` because DirectInput attempts to enumerate all HIDs, including some DACs
	// and other devices which take too long to respond to those requests, added to the lack of a shorter timeout.
	if (p_buffer_length >= unknown_product_length) {
		memcpy(p_buffer, unknown_product_string, unknown_product_length);
		return TRUE;
	}
	return FALSE;
}

IAT_HOOK_CALL FARPROC __stdcall Hook_GetProcAddress(HMODULE p_module, LPCSTR p_name) {
	if (String(p_name) == "HidD_GetProductString") {
		return (FARPROC)(LPVOID)Hook_HidD_GetProductString;
	}
	if (Original_GetProcAddress) {
		return Original_GetProcAddress(p_module, p_name);
	}
	return nullptr;
}

LPVOID install_iat_hook(const String &p_target, const String &p_module, const String &p_symbol, LPVOID p_hook_func) {
	LPVOID image_base = LoadLibraryA(p_target.ascii().get_data());
	if (image_base) {
		PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)((DWORD_PTR)image_base + ((PIMAGE_DOS_HEADER)image_base)->e_lfanew);
		PIMAGE_IMPORT_DESCRIPTOR import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)image_base + nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		while (import_descriptor->Name != 0) {
			LPCSTR library_name = (LPCSTR)((DWORD_PTR)image_base + import_descriptor->Name);
			if (String(library_name).to_lower() == p_module) {
				PIMAGE_THUNK_DATA original_first_thunk = (PIMAGE_THUNK_DATA)((DWORD_PTR)image_base + import_descriptor->OriginalFirstThunk);
				PIMAGE_THUNK_DATA first_thunk = (PIMAGE_THUNK_DATA)((DWORD_PTR)image_base + import_descriptor->FirstThunk);

				while ((LPVOID)original_first_thunk->u1.AddressOfData != nullptr) {
					PIMAGE_IMPORT_BY_NAME function_import = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)image_base + original_first_thunk->u1.AddressOfData);
					if (String(function_import->Name).to_lower() == p_symbol.to_lower()) {
						DWORD old_protect = 0;
						VirtualProtect((LPVOID)(&first_thunk->u1.Function), 8, PAGE_READWRITE, &old_protect);

						LPVOID old_func = (LPVOID)first_thunk->u1.Function;
						first_thunk->u1.Function = (DWORD_PTR)p_hook_func;

						VirtualProtect((LPVOID)(&first_thunk->u1.Function), 8, old_protect, nullptr);
						return old_func;
					}
					original_first_thunk++;
					first_thunk++;
				}
			}
			import_descriptor++;
		}
	}
	return nullptr;
}

OS_Windows::OS_Windows(HINSTANCE _hInstance) {
	hInstance = _hInstance;

	Original_GetProcAddress = (GetProcAddressType)install_iat_hook("dinput8.dll", "kernel32.dll", "GetProcAddress", (LPVOID)Hook_GetProcAddress);
	Original_HidD_GetProductString = (HidD_GetProductStringType)install_iat_hook("dinput8.dll", "hid.dll", "HidD_GetProductString", (LPVOID)Hook_HidD_GetProductString);

	_init_encodings();

	// Reset CWD to ensure long path is used.
	Char16String current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());

	Char16String new_current_dir_name;
	str_len = GetLongPathNameW((LPCWSTR)current_dir_name.get_data(), nullptr, 0);
	new_current_dir_name.resize_uninitialized(str_len + 1);
	GetLongPathNameW((LPCWSTR)current_dir_name.get_data(), (LPWSTR)new_current_dir_name.ptrw(), new_current_dir_name.size());

	SetCurrentDirectoryW((LPCWSTR)new_current_dir_name.get_data());

#ifndef WINDOWS_SUBSYSTEM_CONSOLE
	RedirectIOToConsole();
#endif

	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

#ifdef WASAPI_ENABLED
	AudioDriverManager::add_driver(&driver_wasapi);
#endif
#ifdef XAUDIO2_ENABLED
	AudioDriverManager::add_driver(&driver_xaudio2);
#endif

	DisplayServerWindows::register_windows_driver();

	// Enable ANSI escape code support on Windows 10 v1607 (Anniversary Update) and later.
	// This lets the engine and projects use ANSI escape codes to color text just like on macOS and Linux.
	//
	// NOTE: The engine does not use ANSI escape codes to color error/warning messages; it uses Windows API calls instead.
	// Therefore, error/warning messages are still colored on Windows versions older than 10.
	HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD outMode = 0;
	GetConsoleMode(stdoutHandle, &outMode);
	outMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(stdoutHandle, outMode)) {
		// Windows 10 prior to Anniversary Update.
		print_verbose("Can't set the ENABLE_VIRTUAL_TERMINAL_PROCESSING Windows console mode. `print_rich()` will not work as expected.");
	}

	Vector<Logger *> loggers;
	loggers.push_back(memnew(Logger(Logger::BACKEND_WINDOWS_TERMINAL)));
	_set_logger(memnew(CompositeLogger(loggers)));
}

OS_Windows::~OS_Windows() {
	CoUninitialize();
}


// --- Merged from platform/windows/windows_terminal_logger.cpp ---
#include "servers/display/display_server.h"

#ifdef WINDOWS_ENABLED

#include "core/os/os.h"

#include <windows.h>

#include <cstdio>


#endif // WINDOWS_ENABLED


// --- Merged from platform/windows/windows_utils.cpp ---
#include "servers/display/display_server.h"

#ifdef WINDOWS_ENABLED

#include "core/typedefs.h"
#include "core/io/net_socket.h"
#include "core/io/file_access.h"

#include <windows.h>
#undef FAILED // Overrides Error::FAILED

// dbghelp is linked only in DEBUG_ENABLED builds.
#ifdef DEBUG_ENABLED
#include <dbghelp.h>
#endif
#include <winnt.h>

HashMap<String, Vector<String>> WindowsUtils::temp_pdbs;

Error WindowsUtils::copy_and_rename_pdb(const String &p_dll_path) {
#ifdef DEBUG_ENABLED
	// 1000 ought to be enough for anybody, in case the debugger does not unblock previous PDBs.
	// Usually no more than 2 will be used.
	const int max_pdb_names = 1000;

	uint32_t pdb_res_address = 0;
	String pdb_res_path;

	// Open and read the PDB information if available.
	{
		ULONG dbg_info_size = 0;
		DWORD dbg_info_position = 0;

		{
			// The custom LoadLibraryExW is used instead of open_dynamic_library
			// to avoid loading the original PDB into the debugger.
			HMODULE library_ptr = LoadLibraryExW((LPCWSTR)(p_dll_path.utf16().get_data()), nullptr, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);

			ERR_FAIL_NULL_V_MSG(library_ptr, ERR_FILE_CANT_OPEN, vformat("Failed to load library '%s'.", p_dll_path));

			IMAGE_DEBUG_DIRECTORY *dbg_dir = (IMAGE_DEBUG_DIRECTORY *)ImageDirectoryEntryToDataEx(library_ptr, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &dbg_info_size, nullptr);

			bool has_debug = dbg_dir && dbg_dir->Type == IMAGE_DEBUG_TYPE_CODEVIEW;
			if (has_debug) {
				dbg_info_position = dbg_dir->PointerToRawData;
				dbg_info_size = dbg_dir->SizeOfData;
			}

			ERR_FAIL_COND_V_MSG(!FreeLibrary((HMODULE)library_ptr), FAILED, vformat("Failed to free library '%s'.", p_dll_path));

			if (!has_debug) {
				// Skip with no debugging symbols.
				return ERR_SKIP;
			}
		}

		struct CV_HEADER {
			DWORD Signature;
			DWORD Offset;
		};

		const DWORD nb10_magic = 0x3031424e; // "01BN" (little-endian)
		struct CV_INFO_PDB20 {
			CV_HEADER CvHeader; // CvHeader.Signature = "NB10"
			DWORD Signature;
			DWORD Age;
			BYTE PdbFileName[1];
		};

		const DWORD rsds_magic = 0x53445352; // "SDSR" (little-endian)
		struct CV_INFO_PDB70 {
			DWORD Signature; // "RSDS"
			BYTE Guid[16];
			DWORD Age;
			BYTE PdbFileName[1];
		};

		Vector<uint8_t> dll_data;

		{
			Error err = OK;
			Ref<FileAccess> file = FileAccess::open(p_dll_path, FileAccess::READ, &err);
			ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Failed to read library '%s'.", p_dll_path));

			file->seek(dbg_info_position);
			dll_data = file->get_buffer(dbg_info_size);
			ERR_FAIL_COND_V_MSG(file->get_error() != OK, file->get_error(), vformat("Failed to read data from library '%s'.", p_dll_path));
		}

		const char *raw_pdb_path = nullptr;
		int raw_pdb_offset = 0;
		DWORD *pdb_info_signature = (DWORD *)dll_data.ptr();

		if (*pdb_info_signature == rsds_magic) {
			raw_pdb_path = (const char *)(((CV_INFO_PDB70 *)pdb_info_signature)->PdbFileName);
			raw_pdb_offset = offsetof(CV_INFO_PDB70, PdbFileName);
		} else if (*pdb_info_signature == nb10_magic) {
			// Not even sure if this format still exists anywhere...
			raw_pdb_path = (const char *)(((CV_INFO_PDB20 *)pdb_info_signature)->PdbFileName);
			raw_pdb_offset = offsetof(CV_INFO_PDB20, PdbFileName);
		} else {
			ERR_FAIL_V_MSG(FAILED, vformat("Unknown PDB format in '%s'.", p_dll_path));
		}

		String utf_path;
		Error err = utf_path.append_utf8(raw_pdb_path);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Failed to read PDB path from '%s'.", p_dll_path));

		pdb_res_path = utf_path;
		pdb_res_address = dbg_info_position + raw_pdb_offset;
	}

	String dll_base_dir = p_dll_path.get_base_dir();
	String copy_pdb_path = pdb_res_path;

	// Attempting to find the PDB by absolute and relative paths.
	if (copy_pdb_path.is_relative_path()) {
		copy_pdb_path = dll_base_dir.path_join(copy_pdb_path);
		if (!FileAccess::exists(copy_pdb_path)) {
			copy_pdb_path = dll_base_dir.path_join(copy_pdb_path.get_file());
		}
	} else if (!FileAccess::exists(copy_pdb_path)) {
		copy_pdb_path = dll_base_dir.path_join(copy_pdb_path.get_file());
	}
	if (!FileAccess::exists(copy_pdb_path)) {
		// The PDB file may be distributed separately on purpose, so we don't consider this an error.
		WARN_VERBOSE(vformat("PDB file '%s' for library '%s' was not found, skipping copy/rename.", copy_pdb_path, p_dll_path));
		return ERR_SKIP;
	}

	String new_pdb_base_name = p_dll_path.get_file().get_basename() + "_";

	// Checking the available space for the updated string
	// and trying to shorten it if there is not much space.
	{
		// e.g. 999.pdb
		const uint8_t suffix_size = String::num_characters((int64_t)max_pdb_names - 1) + 4;
		// e.g. ~lib_ + 1 for the \0
		const uint8_t min_base_size = 5 + 1;
		int original_path_size = pdb_res_path.utf8().length();
		CharString utf8_name = new_pdb_base_name.utf8();
		int new_expected_buffer_size = utf8_name.length() + suffix_size;

		// Since we have limited space inside the DLL to patch the path to the PDB,
		// it is necessary to limit the size based on the number of bytes occupied by the string.
		if (new_expected_buffer_size > original_path_size) {
			ERR_FAIL_COND_V_MSG(original_path_size < min_base_size + suffix_size, FAILED, vformat("The original PDB path size in bytes is too small: '%s'. Expected size: %d or more bytes, but available %d.", pdb_res_path, min_base_size + suffix_size, original_path_size));

			new_pdb_base_name.clear();
			new_pdb_base_name.append_utf8(utf8_name.get_data(), original_path_size - suffix_size);
			new_pdb_base_name[new_pdb_base_name.length() - 1] = '_'; // Restore the last '_'
			WARN_PRINT(vformat("The original path size of '%s' in bytes was too small to fit the new name, so it was shortened to '%s%d.pdb'.", pdb_res_path, new_pdb_base_name, max_pdb_names - 1));
		}
	}

	// Delete old PDB files.
	for (const String &file : DirAccess::get_files_at(dll_base_dir)) {
		if (file.begins_with(new_pdb_base_name) && file.ends_with(".pdb")) {
			String path = dll_base_dir.path_join(file);

			// Just try to delete without showing any errors.
			Error err = DirAccess::remove_absolute(path);
			if (err == OK && temp_pdbs[p_dll_path].has(path)) {
				temp_pdbs[p_dll_path].erase(path);
			}
		}
	}

	// Try to copy PDB with new name and patch DLL.
	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	for (int i = 0; i < max_pdb_names; i++) {
		String new_pdb_name = vformat("%s%d.pdb", new_pdb_base_name, i);
		String new_pdb_path = dll_base_dir.path_join(new_pdb_name);
		Error err = OK;

		Ref<FileAccess> test_pdb_is_locked = FileAccess::open(new_pdb_path, FileAccess::READ_WRITE, &err);
		if (err == ERR_FILE_CANT_OPEN) {
			// If the file is blocked, continue searching.
			continue;
		} else if (err != OK && err != ERR_FILE_NOT_FOUND) {
			ERR_FAIL_V_MSG(err, vformat("Failed to open '%s' to check if it is locked.", new_pdb_path));
		}

		err = d->copy(copy_pdb_path, new_pdb_path);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Failed to copy PDB from '%s' to '%s'.", copy_pdb_path, new_pdb_path));
		temp_pdbs[p_dll_path].append(new_pdb_path);

		Ref<FileAccess> file = FileAccess::open(p_dll_path, FileAccess::READ_WRITE, &err);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Failed to open '%s' to patch the PDB path.", p_dll_path));

		int original_path_size = pdb_res_path.utf8().length();
		// Double-check file bounds.
		ERR_FAIL_UNSIGNED_INDEX_V_MSG(pdb_res_address + original_path_size, file->get_length(), FAILED, vformat("Failed to write a new PDB path. Probably '%s' has been changed.", p_dll_path));

		Vector<uint8_t> u8 = new_pdb_name.to_utf8_buffer();
		file->seek(pdb_res_address);
		file->store_buffer(u8);

		// Terminate string and fill the remaining part of the original string with the '\0'.
		Vector<uint8_t> padding_buffer;
		padding_buffer.resize_initialized((int64_t)original_path_size - u8.size());
		file->store_buffer(padding_buffer);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Failed to write a new PDB path to '%s'.", p_dll_path));

		return OK;
	}

	ERR_FAIL_V_MSG(FAILED, vformat("Failed to find an unblocked PDB name for '%s' among %d files.", p_dll_path, max_pdb_names));
#else
	WARN_PRINT_ONCE("Renaming PDB files is only available in debug builds. If your libraries use PDB files, then the original ones will be used.");
	return ERR_SKIP;
#endif
}

void WindowsUtils::remove_temp_pdbs(const String &p_dll_path) {
#ifdef DEBUG_ENABLED
	if (temp_pdbs.has(p_dll_path)) {
		Vector<String> removed;
		int failed = 0;
		const int failed_limit = 10;
		for (const String &pdb : temp_pdbs[p_dll_path]) {
			if (FileAccess::exists(pdb)) {
				Error err = DirAccess::remove_absolute(pdb);
				if (err == OK) {
					removed.append(pdb);
				} else {
					failed++;
					if (failed <= failed_limit) {
						print_verbose("Failed to remove temp PDB: " + pdb);
					}
				}
			} else {
				removed.append(pdb);
			}
		}

		if (failed > failed_limit) {
			print_verbose(vformat("And %d more PDB files could not be removed....", failed - failed_limit));
		}

		for (const String &pdb : removed) {
			temp_pdbs[p_dll_path].erase(pdb);
		}
	}
#endif
}

#endif // WINDOWS_ENABLED


// --- Merged from platform/windows/godot_windows.cpp ---
#if !defined(LIBGODOT_ENABLED)
#include "servers/display/display_server.h"


#include <clocale>

// For export templates, add a section; the exporter will patch it to enclose
// the data appended to the executable (bundled PCK).
#ifndef TOOLS_ENABLED
#if defined _MSC_VER
#pragma section("pck", read)
__declspec(allocate("pck")) static char dummy[8] = { 0 };

// Dummy function to prevent LTO from discarding "pck" section.
extern "C" char *__cdecl pck_section_dummy_call() {
	return &dummy[0];
}
#if defined _AMD64_
#pragma comment(linker, "/include:pck_section_dummy_call")
#elif defined _X86_
#pragma comment(linker, "/include:_pck_section_dummy_call")
#endif

#elif defined __GNUC__
static const char dummy[8] __attribute__((section("pck"), used)) = { 0 };
#endif
#endif

char *wc_to_utf8(const wchar_t *wc) {
	int ulen = WideCharToMultiByte(CP_UTF8, 0, wc, -1, nullptr, 0, nullptr, nullptr);
	char *ubuf = new char[ulen + 1];
	WideCharToMultiByte(CP_UTF8, 0, wc, -1, ubuf, ulen, nullptr, nullptr);
	ubuf[ulen] = 0;
	return ubuf;
}

int widechar_main(int argc, wchar_t **argv) {
	godot_init_profiler();

	OS_Windows os(nullptr);

	setlocale(LC_CTYPE, "");

	char **argv_utf8 = new char *[argc];

	for (int i = 0; i < argc; ++i) {
		argv_utf8[i] = wc_to_utf8(argv[i]);
	}

	TEST_MAIN_PARAM_OVERRIDE(argc, argv_utf8)

	Error err = Main::setup(argv_utf8[0], argc - 1, &argv_utf8[1]);

	if (err != OK) {
		for (int i = 0; i < argc; ++i) {
			delete[] argv_utf8[i];
		}
		delete[] argv_utf8;

		if (err == ERR_HELP) { // Returned by --help and --version, so success.
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}

	if (Main::start() == EXIT_SUCCESS) {
		os.run();
	} else {
		os.set_exit_code(EXIT_FAILURE);
	}
	Main::cleanup();

	for (int i = 0; i < argc; ++i) {
		delete[] argv_utf8[i];
	}
	delete[] argv_utf8;

	godot_cleanup_profiler();
	return os.get_exit_code();
}

int _main() {
	LPWSTR *wc_argv;
	int argc;
	int result;

	wc_argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (nullptr == wc_argv) {
		wprintf(L"CommandLineToArgvW failed\n");
		return 0;
	}

	result = widechar_main(argc, wc_argv);

	LocalFree(wc_argv);
	return result;
}

int main(int argc, char **argv) {
	// override the arguments for the test handler / if symbol is provided
	// TEST_MAIN_OVERRIDE

	// _argc and _argv are ignored
	// we are going to use the WideChar version of them instead
#if defined(CRASH_HANDLER_EXCEPTION) && defined(_MSC_VER)
	__try {
		return _main();
	} __except (CrashHandlerException(GetExceptionInformation())) {
		return 1;
	}
#else
	return _main();
#endif
}

HINSTANCE godot_hinstance = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	godot_hinstance = hInstance;
	return main(0, nullptr);
}
#endif // !defined(LIBGODOT_ENABLED)


// --- Merged from platform/windows/libgodot_windows.cpp ---
#if defined(LIBGODOT_ENABLED)
#include "servers/display/display_server.h"

#include "core/extension/godot_instance.h"
#include "core/extension/libgodot.h"
#include "servers/display/display_server.h"

static OS_Windows *os = nullptr;

static GodotInstance *instance = nullptr;

GDExtensionObjectPtr libgodot_create_godot_instance(int p_argc, char *p_argv[], GDExtensionInitializationFunction p_init_func) {
	ERR_FAIL_COND_V_MSG(instance != nullptr, nullptr, "Only one Godot Instance may be created at a time.");

	os = new OS_Windows(GetModuleHandle(nullptr));

	Error err = Main::setup(p_argv[0], p_argc - 1, &p_argv[1], false);
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
#endif // defined(LIBGODOT_ENABLED)

// === Merged from platform/windows/display_server_windows.cpp ===
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


#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/io/file_access.h"
#include "core/io/net_socket.h"
#include "core/os/os.h"
#include "core/typedefs.h"
#include "core/io/net_socket.h"
#include "servers/display/display_server.h"

#ifdef SDL_ENABLED
#include "core/os/os.h"
#endif

#if defined(VULKAN_ENABLED)
#include "servers/display/display_server.h"
#endif
#if defined(D3D12_ENABLED)
#include "servers/display/display_server.h"

#include <dxgi1_6.h>
#endif
#if defined(GLES3_ENABLED)
#include "servers/display/display_server.h"
#endif

#include <avrt.h>
#include <dwmapi.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <wbemcli.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWCP_DEFAULT
#define DWMWCP_DEFAULT 0
#endif

#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif

#define WM_INDICATOR_CALLBACK_MESSAGE (WM_USER + 1)

static String format_ds_error_message(DWORD id) {
	LPWSTR messageBuffer = nullptr;
	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);

	String msg = "Error " + itos(id) + ": " + String::utf16((const char16_t *)messageBuffer, size);

	LocalFree(messageBuffer);

	return msg;
}

static void track_mouse_leave_event(HWND hWnd) {
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = hWnd;
	tme.dwHoverTime = HOVER_DEFAULT;
	TrackMouseEvent(&tme);
}

static void update_ime_form_positions(HIMC p_himc, const Point2i &p_pos) {
	if (p_himc == (HIMC) nullptr) {
		return;
	}

	COMPOSITIONFORM cps = {};
	cps.dwStyle = CFS_POINT;
	cps.ptCurrentPos.x = p_pos.x;
	cps.ptCurrentPos.y = p_pos.y;
	ImmSetCompositionWindow(p_himc, &cps);

	LOGFONT logFont = {};

	logFont.lfHeight = 1; // em height
	logFont.lfQuality = CLEARTYPE_QUALITY;

	ImmSetCompositionFontA(p_himc, &logFont);

	CANDIDATEFORM cf = {};
	cf.dwIndex = 0;

	cf.dwStyle = CFS_CANDIDATEPOS;
	cf.ptCurrentPos.x = p_pos.x;
	cf.ptCurrentPos.y = p_pos.y;
	ImmSetCandidateWindow(p_himc, &cf);

	cf.dwStyle = CFS_EXCLUDE;
	cf.rcArea.left = p_pos.x;
	cf.rcArea.right = p_pos.x;
	cf.rcArea.top = p_pos.y;
	cf.rcArea.bottom = p_pos.y;
	ImmSetCandidateWindow(p_himc, &cf);
}

bool DisplayServerWindows::has_feature(DisplayServerEnums::Feature p_feature) const {
	switch (p_feature) {
#ifndef DISABLE_DEPRECATED
		case DisplayServerEnums::FEATURE_GLOBAL_MENU: {
			return (native_menu && native_menu->has_feature(NativeMenu::FEATURE_GLOBAL_MENU));
		} break;
#endif
		case DisplayServerEnums::FEATURE_SUBWINDOWS:
		case DisplayServerEnums::FEATURE_TOUCHSCREEN:
		case DisplayServerEnums::FEATURE_MOUSE:
		case DisplayServerEnums::FEATURE_MOUSE_WARP:
		case DisplayServerEnums::FEATURE_CLIPBOARD:
		case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
		case DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE:
		case DisplayServerEnums::FEATURE_IME:
		case DisplayServerEnums::FEATURE_WINDOW_TRANSPARENCY:
		case DisplayServerEnums::FEATURE_HIDPI:
		case DisplayServerEnums::FEATURE_ICON:
		case DisplayServerEnums::FEATURE_NATIVE_ICON:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_INPUT:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA:
		//case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_MIME:
		case DisplayServerEnums::FEATURE_SWAP_BUFFERS:
		case DisplayServerEnums::FEATURE_KEEP_SCREEN_ON:
		case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH:
		case DisplayServerEnums::FEATURE_SCREEN_CAPTURE:
		case DisplayServerEnums::FEATURE_STATUS_INDICATOR:
		case DisplayServerEnums::FEATURE_WINDOW_EMBEDDING:
		case DisplayServerEnums::FEATURE_WINDOW_DRAG:
		case DisplayServerEnums::FEATURE_HDR_OUTPUT:
			return true;
		case DisplayServerEnums::FEATURE_SCREEN_EXCLUDE_FROM_CAPTURE:
			return (os_ver.dwBuildNumber >= 19041); // Fully supported on Windows 10 Vibranium R1 (2004)+ only, captured as black rect on older versions.
		case DisplayServerEnums::FEATURE_EMOJI_AND_SYMBOL_PICKER:
			return (os_ver.dwBuildNumber >= 17134); // Windows 10 Redstone 4 (1803)+ only.
		case DisplayServerEnums::FEATURE_ACCESSIBILITY_SCREEN_READER: {
			return AccessibilityServer::is_supported();
		} break;
		default:
			return false;
	}
}

String DisplayServerWindows::get_name() const {
	return "Windows";
}

Vector2i _logical_to_physical(const Vector2i &p_point) {
	POINT p1;
	p1.x = p_point.x;
	p1.y = p_point.y;
	LogicalToPhysicalPointForPerMonitorDPI(nullptr, &p1);
	return Vector2i(p1.x, p1.y);
}

Vector2i DisplayServerWindows::_get_screen_expand_offset(int p_screen) const {
	Vector2i p1 = _logical_to_physical(screen_get_position(p_screen));
	Vector2i p2 = _logical_to_physical(screen_get_position(p_screen) + screen_get_size(p_screen));

	int screen_down = -1;
	int screen_right = -1;
	for (int i = 0; i < get_screen_count(); i++) {
		if (i == p_screen) {
			continue;
		}
		Vector2i sp1 = _logical_to_physical(screen_get_position(i));
		Vector2i sp2 = _logical_to_physical(screen_get_position(i) + screen_get_size(i));
		if (sp1.y >= p2.y - 5 && sp1.y <= p2.y + 5 && sp1.x <= p2.x && p1.x <= sp2.x) {
			screen_down = i;
		}
		if (sp1.x >= p2.x - 5 && sp1.x <= p2.x + 5 && sp1.y <= p2.y && p1.y <= sp2.y) {
			screen_right = i;
		}
	}

	if (screen_down == -1) {
		return Vector2i(0, 2);
	} else if (screen_right == -1) {
		return Vector2i(2, 0);
	} else {
		int diff_d = screen_get_refresh_rate(p_screen) - screen_get_refresh_rate(screen_down);
		int diff_r = screen_get_refresh_rate(p_screen) - screen_get_refresh_rate(screen_right);
		if (diff_d < diff_r) {
			return Vector2i(0, 2);
		} else {
			return Vector2i(2, 0);
		}
	}
}

void DisplayServerWindows::_set_mouse_mode_impl(DisplayServerEnums::MouseMode p_mode) {
	if (p_mode == DisplayServerEnums::MOUSE_MODE_HIDDEN || p_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED || p_mode == DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN) {
		// Hide cursor before moving.
		if (hCursor == nullptr) {
			hCursor = SetCursor(nullptr);
		} else {
			SetCursor(nullptr);
		}
	}

	if (windows.has(DisplayServerEnums::MAIN_WINDOW_ID) && (p_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED || p_mode == DisplayServerEnums::MOUSE_MODE_CONFINED || p_mode == DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN)) {
		// Mouse is grabbed (captured or confined).
		DisplayServerEnums::WindowID window_id = _get_focused_window_or_popup();
		if (!windows.has(window_id)) {
			window_id = DisplayServerEnums::MAIN_WINDOW_ID;
		}

		WindowData &wd = windows[window_id];

		Vector2i off = (wd.multiwindow_fs || (!wd.fullscreen && wd.borderless && wd.maximized)) ? _get_screen_expand_offset(window_get_current_screen(window_id)) : Vector2i();

		RECT clipRect;
		GetClientRect(wd.hWnd, &clipRect);
		clipRect.right -= off.x;
		clipRect.bottom -= off.y;
		ClientToScreen(wd.hWnd, (POINT *)&clipRect.left);
		ClientToScreen(wd.hWnd, (POINT *)&clipRect.right);
		ClipCursor(&clipRect);
		if (p_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
			center = window_get_size() / 2;
			POINT pos = { (int)center.x, (int)center.y };
			ClientToScreen(wd.hWnd, &pos);
			SetCursorPos(pos.x, pos.y);
			SetCapture(wd.hWnd);

			_register_raw_input_devices(window_id);
		}
	} else {
		// Mouse is free to move around (not captured or confined).
		// When the user is moving a window, it's important to not ReleaseCapture because it will cause
		// the window movement to stop and if the user tries to move the Windows when it's not activated,
		// it will prevent the window movement. It's probably impossible to move the Window while it's captured anyway.
		if (!_has_moving_window()) {
			ReleaseCapture();
		}
		ClipCursor(nullptr);

		_register_raw_input_devices(DisplayServerEnums::INVALID_WINDOW_ID);
	}

	if (p_mode == DisplayServerEnums::MOUSE_MODE_VISIBLE || p_mode == DisplayServerEnums::MOUSE_MODE_CONFINED) {
		// Show cursor.
		DisplayServerEnums::CursorShape c = cursor_shape;
		cursor_shape = DisplayServerEnums::CURSOR_MAX;
		cursor_set_shape(c);
	}
}

DisplayServerEnums::WindowID DisplayServerWindows::_get_focused_window_or_popup() const {
	const List<DisplayServerEnums::WindowID>::Element *E = popup_list.back();
	if (E) {
		return E->get();
	}

	return last_focused_window;
}

bool DisplayServerWindows::_has_moving_window() const {
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (E.value.move_timer_id) {
			return true;
		}
	}
	return false;
}

void DisplayServerWindows::_register_raw_input_devices(DisplayServerEnums::WindowID p_target_window) {
	use_raw_input = true;

	RAWINPUTDEVICE rid[2] = {};
	rid[0].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
	rid[0].usUsage = 0x02; // HID_USAGE_GENERIC_MOUSE
	rid[0].dwFlags = 0;

	rid[1].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
	rid[1].usUsage = 0x06; // HID_USAGE_GENERIC_KEYBOARD
	rid[1].dwFlags = 0;

	if (p_target_window != DisplayServerEnums::INVALID_WINDOW_ID && windows.has(p_target_window)) {
		// Follow the defined window
		rid[0].hwndTarget = windows[p_target_window].hWnd;
		rid[1].hwndTarget = windows[p_target_window].hWnd;
	} else {
		// Follow the keyboard focus
		rid[0].hwndTarget = nullptr;
		rid[1].hwndTarget = nullptr;
	}

	if (RegisterRawInputDevices(rid, 2, sizeof(rid[0])) == FALSE) {
		// Registration failed.
		use_raw_input = false;
	}
}

void DisplayServerWindows::initialize_tts() const {
	TTS_Windows::initialize();
}

bool DisplayServerWindows::tts_is_speaking() const {
	if (unlikely(!TTS_Windows::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND_V(!TTS_Windows::is_initialized(), false);
	return TTS_Windows::is_speaking();
}

bool DisplayServerWindows::tts_is_paused() const {
	if (unlikely(!TTS_Windows::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND_V(!TTS_Windows::is_initialized(), false);
	return TTS_Windows::is_paused();
}

TypedArray<Dictionary> DisplayServerWindows::tts_get_voices() const {
	if (unlikely(!TTS_Windows::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND_V(!TTS_Windows::is_initialized(), TypedArray<Dictionary>());
	return TTS_Windows::get_voices();
}

void DisplayServerWindows::tts_speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	if (unlikely(!TTS_Windows::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND(!TTS_Windows::is_initialized());
	TTS_Windows::speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
}

void DisplayServerWindows::tts_pause() {
	if (unlikely(!TTS_Windows::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND(!TTS_Windows::is_initialized());
	TTS_Windows::pause();
}

void DisplayServerWindows::tts_resume() {
	if (unlikely(!TTS_Windows::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND(!TTS_Windows::is_initialized());
	TTS_Windows::resume();
}

void DisplayServerWindows::tts_stop() {
	if (unlikely(!TTS_Windows::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND(!TTS_Windows::is_initialized());
	TTS_Windows::stop();
}

Error DisplayServerWindows::file_dialog_show(const String &p_title, const String &p_current_directory, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) {
	return _file_dialog_with_options_show(p_title, p_current_directory, String(), p_filename, p_show_hidden, p_mode, p_filters, TypedArray<Dictionary>(), p_callback, false, p_window_id);
}

Error DisplayServerWindows::file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) {
	return _file_dialog_with_options_show(p_title, p_current_directory, p_root, p_filename, p_show_hidden, p_mode, p_filters, p_options, p_callback, true, p_window_id);
}

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wnon-virtual-dtor") // Silence warning due to a COM API weirdness.

class FileDialogEventHandler : public IFileDialogEvents, public IFileDialogControlEvents {
	LONG ref_count = 1;
	int ctl_id = 1;

	HashMap<int, String> ctls;
	Dictionary selected;
	String root;

public:
	// IUnknown methods
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) {
		static const QITAB qit[] = {
#ifdef __MINGW32__
			{ &__uuidof(IFileDialogEvents), static_cast<decltype(qit[0].dwOffset)>(OFFSETOFCLASS(IFileDialogEvents, FileDialogEventHandler)) },
			{ &__uuidof(IFileDialogControlEvents), static_cast<decltype(qit[0].dwOffset)>(OFFSETOFCLASS(IFileDialogControlEvents, FileDialogEventHandler)) },
#else
			QITABENT(FileDialogEventHandler, IFileDialogEvents),
			QITABENT(FileDialogEventHandler, IFileDialogControlEvents),
#endif
			{ nullptr, 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	ULONG STDMETHODCALLTYPE AddRef() {
		return InterlockedIncrement(&ref_count);
	}

	ULONG STDMETHODCALLTYPE Release() {
		long ref = InterlockedDecrement(&ref_count);
		if (!ref) {
			delete this;
		}
		return ref;
	}

	// IFileDialogEvents methods
	HRESULT STDMETHODCALLTYPE OnFileOk(IFileDialog *) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnFolderChange(IFileDialog *) { return S_OK; }

	HRESULT STDMETHODCALLTYPE OnFolderChanging(IFileDialog *p_pfd, IShellItem *p_item) {
		if (root.is_empty()) {
			return S_OK;
		}

		LPWSTR lpw_path = nullptr;
		p_item->GetDisplayName(SIGDN_FILESYSPATH, &lpw_path);
		if (!lpw_path) {
			return S_FALSE;
		}
		String path = String::utf16((const char16_t *)lpw_path).replace_char('\\', '/').trim_prefix(R"(\\?\)").simplify_path();
		if (!path.begins_with(root.simplify_path())) {
			return S_FALSE;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnHelp(IFileDialog *) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnSelectionChange(IFileDialog *) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnTypeChange(IFileDialog *pfd) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *) { return S_OK; }

	// IFileDialogControlEvents methods
	HRESULT STDMETHODCALLTYPE OnItemSelected(IFileDialogCustomize *p_pfdc, DWORD p_ctl_id, DWORD p_item_idx) {
		if (ctls.has(p_ctl_id)) {
			selected[ctls[p_ctl_id]] = (int)p_item_idx;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnButtonClicked(IFileDialogCustomize *, DWORD) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnCheckButtonToggled(IFileDialogCustomize *p_pfdc, DWORD p_ctl_id, BOOL p_checked) {
		if (ctls.has(p_ctl_id)) {
			selected[ctls[p_ctl_id]] = (bool)p_checked;
		}
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE OnControlActivating(IFileDialogCustomize *, DWORD) { return S_OK; }

	Dictionary get_selected() {
		return selected;
	}

	void set_root(const String &p_root) {
		root = p_root;
	}

	void add_option(IFileDialogCustomize *p_pfdc, const String &p_name, const Vector<String> &p_options, int p_default) {
		int gid = ctl_id++;
		int cid = ctl_id++;

		if (p_options.is_empty()) {
			// Add check box.
			p_pfdc->StartVisualGroup(gid, L"");
			p_pfdc->AddCheckButton(cid, (LPCWSTR)p_name.utf16().get_data(), p_default);
			p_pfdc->SetControlState(cid, CDCS_VISIBLE | CDCS_ENABLED);
			p_pfdc->EndVisualGroup();
			selected[p_name] = (bool)p_default;
		} else {
			// Add combo box.
			p_pfdc->StartVisualGroup(gid, (LPCWSTR)p_name.utf16().get_data());
			p_pfdc->AddComboBox(cid);
			p_pfdc->SetControlState(cid, CDCS_VISIBLE | CDCS_ENABLED);
			for (int i = 0; i < p_options.size(); i++) {
				p_pfdc->AddControlItem(cid, i, (LPCWSTR)p_options[i].utf16().get_data());
			}
			p_pfdc->SetSelectedControlItem(cid, p_default);
			p_pfdc->EndVisualGroup();
			selected[p_name] = p_default;
		}
		ctls[cid] = p_name;
	}

	virtual ~FileDialogEventHandler() {}
};

VLTR_GCC_WARNING_POP

LRESULT CALLBACK WndProcFileDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	DisplayServerWindows *ds_win = DisplayServer::driver_as<DisplayServerWindows>();
	if (ds_win) {
		return ds_win->WndProcFileDialog(hWnd, uMsg, wParam, lParam);
	} else {
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

LRESULT DisplayServerWindows::WndProcFileDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	MutexLock lock(file_dialog_mutex);
	if (file_dialog_wnd.has(hWnd)) {
		if (file_dialog_wnd[hWnd]->close_requested.is_set()) {
			IPropertyStore *prop_store;
			HRESULT hr = SHGetPropertyStoreForWindow(hWnd, IID_IPropertyStore, (void **)&prop_store);
			if (hr == S_OK) {
				PROPVARIANT val;
				PropVariantInit(&val);
				prop_store->SetValue(PKEY_AppUserModel_ID, val);
				prop_store->Release();
			}
			DestroyWindow(hWnd);
			file_dialog_wnd.erase(hWnd);
		}
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void DisplayServerWindows::_thread_fd_monitor(void *p_ud) {
	DisplayServerWindows *ds = DisplayServer::driver_as<DisplayServerWindows>();
	FileDialogData *fd = (FileDialogData *)p_ud;

	if (fd->mode < 0 && fd->mode >= DisplayServerEnums::FILE_DIALOG_MODE_SAVE_MAX) {
		fd->finished.set();
		return;
	}
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	int64_t x = fd->wrect.position.x;
	int64_t y = fd->wrect.position.y;
	int64_t w = fd->wrect.size.x;
	int64_t h = fd->wrect.size.y;

	WNDCLASSW wc = {};
	wc.lpfnWndProc = (WNDPROC)::WndProcFileDialog;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = L"Engine File Dialog";
	RegisterClassW(&wc);

	HWND hwnd_dialog = CreateWindowExW(WS_EX_APPWINDOW, L"Engine File Dialog", L"", WS_OVERLAPPEDWINDOW, x, y, w, h, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	if (hwnd_dialog) {
		{
			MutexLock lock(ds->file_dialog_mutex);
			ds->file_dialog_wnd[hwnd_dialog] = fd;
		}

		HICON w_icon = (HICON)SendMessage(fd->hwnd_owner, WM_GETICON, ICON_SMALL, 0);
		if (w_icon) {
			SendMessage(hwnd_dialog, WM_SETICON, ICON_SMALL, (LPARAM)w_icon);
		}
		w_icon = (HICON)SendMessage(fd->hwnd_owner, WM_GETICON, ICON_BIG, 0);
		if (w_icon) {
			SendMessage(hwnd_dialog, WM_SETICON, ICON_BIG, (LPARAM)w_icon);
		}
		IPropertyStore *prop_store;
		HRESULT hr = SHGetPropertyStoreForWindow(hwnd_dialog, IID_IPropertyStore, (void **)&prop_store);
		if (hr == S_OK) {
			PROPVARIANT val;
			InitPropVariantFromString((PCWSTR)fd->appid.utf16().get_data(), &val);
			prop_store->SetValue(PKEY_AppUserModel_ID, val);
			prop_store->Release();
		}
	}

	SetCurrentProcessExplicitAppUserModelID((PCWSTR)fd->appid.utf16().get_data());

	Vector<Char16String> filter_names;
	Vector<Char16String> filter_exts;
	for (const String &E : fd->filters) {
		Vector<String> tokens = E.split(";");
		if (tokens.size() >= 1) {
			String flt = tokens[0].strip_edges();
			int filter_slice_count = flt.get_slice_count(",");
			Vector<String> exts;
			for (int j = 0; j < filter_slice_count; j++) {
				String str = (flt.get_slicec(',', j).strip_edges());
				if (!str.is_empty()) {
					exts.push_back(str);
				}
			}
			if (!exts.is_empty()) {
				String str = String(";").join(exts);
				filter_exts.push_back(str.utf16());
				if (tokens.size() >= 2) {
					filter_names.push_back(tokens[1].strip_edges().utf16());
				} else {
					filter_names.push_back(str.utf16());
				}
			}
		}
	}
	if (filter_names.is_empty()) {
		filter_exts.push_back(String("*.*").utf16());
		filter_names.push_back((RTR("All Files") + " (*.*)").utf16());
	}

	Vector<COMDLG_FILTERSPEC> filters;
	for (int i = 0; i < filter_names.size(); i++) {
		filters.push_back({ (LPCWSTR)filter_names[i].ptr(), (LPCWSTR)filter_exts[i].ptr() });
	}

	HRESULT hr = S_OK;
	IFileDialog *pfd = nullptr;
	if (fd->mode == DisplayServerEnums::FILE_DIALOG_MODE_SAVE_FILE) {
		hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileSaveDialog, (void **)&pfd);
	} else {
		hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void **)&pfd);
	}
	if (SUCCEEDED(hr)) {
		IFileDialogEvents *pfde = nullptr;
		FileDialogEventHandler *event_handler = new FileDialogEventHandler();
		hr = event_handler->QueryInterface(IID_PPV_ARGS(&pfde));

		DWORD cookie = 0;
		hr = pfd->Advise(pfde, &cookie);

		IFileDialogCustomize *pfdc = nullptr;
		hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdc));

		for (int i = 0; i < fd->options.size(); i++) {
			const Dictionary &item = fd->options[i];
			if (!item.has("name") || !item.has("values") || !item.has("default")) {
				continue;
			}
			event_handler->add_option(pfdc, item["name"], item["values"], item["default"]);
		}
		event_handler->set_root(fd->root);

		pfdc->Release();

		DWORD flags;
		pfd->GetOptions(&flags);
		if (fd->mode == DisplayServerEnums::FILE_DIALOG_MODE_OPEN_FILES) {
			flags |= FOS_ALLOWMULTISELECT;
		}
		if (fd->mode == DisplayServerEnums::FILE_DIALOG_MODE_OPEN_DIR) {
			flags |= FOS_PICKFOLDERS;
		}
		if (fd->show_hidden) {
			flags |= FOS_FORCESHOWHIDDEN;
		}
		pfd->SetOptions(flags | FOS_FORCEFILESYSTEM);
		pfd->SetTitle((LPCWSTR)fd->title.utf16().get_data());

		String dir = ProjectSettings::globalize_path(fd->current_directory);
		if (dir == ".") {
			dir = OS::get_executable_path().get_base_dir();
		}
		if (dir.is_relative_path() || dir == ".") {
			Char16String current_dir_name;
			size_t str_len = GetCurrentDirectoryW(0, nullptr);
			current_dir_name.resize_uninitialized(str_len + 1);
			GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
			if (dir == ".") {
				dir = String::utf16((const char16_t *)current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace_char('\\', '/');
			} else {
				dir = String::utf16((const char16_t *)current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace_char('\\', '/').path_join(dir);
			}
		}
		dir = dir.simplify_path();
		dir = dir.trim_prefix(R"(\\?\)").replace_char('/', '\\');

		IShellItem *shellitem = nullptr;
		hr = SHCreateItemFromParsingName((LPCWSTR)dir.utf16().ptr(), nullptr, IID_IShellItem, (void **)&shellitem);
		if (SUCCEEDED(hr)) {
			pfd->SetDefaultFolder(shellitem);
			pfd->SetFolder(shellitem);
		}

		pfd->SetFileName((LPCWSTR)fd->filename.utf16().get_data());
		pfd->SetFileTypes(filters.size(), filters.ptr());
		pfd->SetFileTypeIndex(0);

		hr = pfd->Show(hwnd_dialog);
		pfd->Unadvise(cookie);

		Dictionary options = event_handler->get_selected();

		pfde->Release();
		event_handler->Release();

		UINT index = 0;
		pfd->GetFileTypeIndex(&index);
		if (index > 0) {
			index = index - 1;
		}

		if (SUCCEEDED(hr)) {
			Vector<String> file_names;

			if (fd->mode == DisplayServerEnums::FILE_DIALOG_MODE_OPEN_FILES) {
				IShellItemArray *results;
				hr = static_cast<IFileOpenDialog *>(pfd)->GetResults(&results);
				if (SUCCEEDED(hr)) {
					DWORD count = 0;
					results->GetCount(&count);
					for (DWORD i = 0; i < count; i++) {
						IShellItem *result;
						results->GetItemAt(i, &result);

						PWSTR file_path = nullptr;
						hr = result->GetDisplayName(SIGDN_FILESYSPATH, &file_path);
						if (SUCCEEDED(hr)) {
							file_names.push_back(String::utf16((const char16_t *)file_path).replace_char('\\', '/').trim_prefix(R"(\\?\)"));
							CoTaskMemFree(file_path);
						}
						result->Release();
					}
					results->Release();
				}
			} else {
				IShellItem *result;
				hr = pfd->GetResult(&result);
				if (SUCCEEDED(hr)) {
					PWSTR file_path = nullptr;
					hr = result->GetDisplayName(SIGDN_FILESYSPATH, &file_path);
					if (SUCCEEDED(hr)) {
						file_names.push_back(String::utf16((const char16_t *)file_path).replace_char('\\', '/').trim_prefix(R"(\\?\)"));
						CoTaskMemFree(file_path);
					}
					result->Release();
				}
			}
			if (fd->callback.is_valid()) {
				MutexLock lock(ds->file_dialog_mutex);
				FileDialogCallback cb;
				cb.callback = fd->callback;
				cb.status = true;
				cb.files = file_names;
				cb.index = index;
				cb.options = options;
				cb.opt_in_cb = fd->options_in_cb;
				ds->pending_cbs.push_back(cb);
			}
		} else {
			if (fd->callback.is_valid()) {
				MutexLock lock(ds->file_dialog_mutex);
				FileDialogCallback cb;
				cb.callback = fd->callback;
				cb.status = false;
				cb.files = Vector<String>();
				cb.index = index;
				cb.options = options;
				cb.opt_in_cb = fd->options_in_cb;
				ds->pending_cbs.push_back(cb);
			}
		}
		pfd->Release();
	} else {
		if (fd->callback.is_valid()) {
			MutexLock lock(ds->file_dialog_mutex);
			FileDialogCallback cb;
			cb.callback = fd->callback;
			cb.status = false;
			cb.files = Vector<String>();
			cb.index = 0;
			cb.options = Dictionary();
			cb.opt_in_cb = fd->options_in_cb;
			ds->pending_cbs.push_back(cb);
		}
	}
	{
		MutexLock lock(ds->file_dialog_mutex);
		if (hwnd_dialog && ds->file_dialog_wnd.has(hwnd_dialog)) {
			IPropertyStore *prop_store;
			hr = SHGetPropertyStoreForWindow(hwnd_dialog, IID_IPropertyStore, (void **)&prop_store);
			if (hr == S_OK) {
				PROPVARIANT val;
				PropVariantInit(&val);
				prop_store->SetValue(PKEY_AppUserModel_ID, val);
				prop_store->Release();
			}
			DestroyWindow(hwnd_dialog);
			ds->file_dialog_wnd.erase(hwnd_dialog);
		}
	}
	UnregisterClassW(L"Engine File Dialog", GetModuleHandle(nullptr));
	CoUninitialize();

	fd->finished.set();

	if (fd->window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		callable_mp_static(&DisplayServer::window_move_to_foreground).call_deferred(fd->window_id);
	}
}

Error DisplayServerWindows::_file_dialog_with_options_show(const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, bool p_options_in_cb, DisplayServerEnums::WindowID p_window_id) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_INDEX_V(int(p_mode), DisplayServerEnums::FILE_DIALOG_MODE_SAVE_MAX, FAILED);

	String appname;
	if (Engine::is_editor_hint()) {
		appname = "Godot.GodotEditor." + String(GODOT_VERSION_BRANCH);
	} else {
		String name = GLOBAL_GET("application/config/name");
		String version = GLOBAL_GET("application/config/version");
		if (version.is_empty()) {
			version = "0";
		}
		String clean_app_name = name.to_pascal_case();
		for (int i = 0; i < clean_app_name.length(); i++) {
			if (!is_ascii_alphanumeric_char(clean_app_name[i]) && clean_app_name[i] != '_' && clean_app_name[i] != '.') {
				clean_app_name[i] = '_';
			}
		}
		clean_app_name = clean_app_name.substr(0, 120 - version.length()).trim_suffix(".");
		appname = "Godot." + clean_app_name + "." + version;
	}

	FileDialogData *fd = memnew(FileDialogData);
	if (windows.has(p_window_id) && !windows[p_window_id].is_popup) {
		fd->hwnd_owner = windows[p_window_id].hWnd;
		RECT crect;
		GetWindowRect(fd->hwnd_owner, &crect);
		fd->wrect = Rect2i(crect.left, crect.top, crect.right - crect.left, crect.bottom - crect.top);
	} else {
		fd->hwnd_owner = nullptr;
		fd->wrect = Rect2i(CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT);
	}
	fd->appid = appname;
	fd->title = p_title;
	fd->current_directory = p_current_directory;
	fd->root = p_root;
	fd->filename = p_filename;
	fd->show_hidden = p_show_hidden;
	fd->mode = p_mode;
	fd->window_id = p_window_id;
	fd->filters = p_filters;
	fd->options = p_options;
	fd->callback = p_callback;
	fd->options_in_cb = p_options_in_cb;
	fd->finished.clear();
	fd->close_requested.clear();

	fd->listener_thread.start(DisplayServerWindows::_thread_fd_monitor, fd);

	file_dialogs.push_back(fd);

	return OK;
}

void DisplayServerWindows::process_file_dialog_callbacks() {
	MutexLock lock(file_dialog_mutex);
	while (!pending_cbs.is_empty()) {
		FileDialogCallback cb = pending_cbs.front()->get();
		pending_cbs.pop_front();

		if (cb.opt_in_cb) {
			Variant ret;
			Callable::CallError ce;
			const Variant *args[4] = { &cb.status, &cb.files, &cb.index, &cb.options };

			cb.callback.callp(args, 4, ret, ce);
			if (ce.error != Callable::CallError::CALL_OK) {
				ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(cb.callback, args, 4, ce)));
			}
		} else {
			Variant ret;
			Callable::CallError ce;
			const Variant *args[3] = { &cb.status, &cb.files, &cb.index };

			cb.callback.callp(args, 3, ret, ce);
			if (ce.error != Callable::CallError::CALL_OK) {
				ERR_PRINT(vformat("Failed to execute file dialog callback: %s.", Variant::get_callable_error_text(cb.callback, args, 3, ce)));
			}
		}
	}
}

void DisplayServerWindows::beep() const {
	MessageBeep(MB_OK);
}

void DisplayServerWindows::_mouse_update_mode() {
	_THREAD_SAFE_METHOD_

	DisplayServerEnums::MouseMode wanted_mouse_mode = mouse_mode_override_enabled
			? mouse_mode_override
			: mouse_mode_base;

	if (mouse_mode == wanted_mouse_mode) {
		// Already in the same mode; do nothing.
		return;
	}

	mouse_mode = wanted_mouse_mode;

	_set_mouse_mode_impl(wanted_mouse_mode);
}

void DisplayServerWindows::mouse_set_mode(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_base) {
		return;
	}
	mouse_mode_base = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerWindows::mouse_get_mode() const {
	return mouse_mode;
}

void DisplayServerWindows::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_override) {
		return;
	}
	mouse_mode_override = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerWindows::mouse_get_mode_override() const {
	return mouse_mode_override;
}

void DisplayServerWindows::mouse_set_mode_override_enabled(bool p_override_enabled) {
	if (p_override_enabled == mouse_mode_override_enabled) {
		return;
	}
	mouse_mode_override_enabled = p_override_enabled;
	_mouse_update_mode();
}

bool DisplayServerWindows::mouse_is_mode_override_enabled() const {
	return mouse_mode_override_enabled;
}

void DisplayServerWindows::warp_mouse(const Point2i &p_position) {
	_THREAD_SAFE_METHOD_

	DisplayServerEnums::WindowID window_id = _get_focused_window_or_popup();

	if (!windows.has(window_id)) {
		return; // No focused window?
	}

	if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
		old_x = p_position.x;
		old_y = p_position.y;
	} else {
		POINT p;
		p.x = p_position.x;
		p.y = p_position.y;
		ClientToScreen(windows[window_id].hWnd, &p);

		SetCursorPos(p.x, p.y);
	}
}

Point2i DisplayServerWindows::mouse_get_position() const {
	POINT p;
	GetCursorPos(&p);
	return Point2i(p.x, p.y) - _get_screens_origin();
}

uint32_t DisplayServerWindows::mouse_get_button_state() const {
	uint32_t last_button_state = MouseButtonMask::NONE;

	if (GetKeyState(VK_LBUTTON) & (1 << 15)) {
		last_button_state.set_flag(MouseButtonMask::LEFT);
	}
	if (GetKeyState(VK_RBUTTON) & (1 << 15)) {
		last_button_state.set_flag(MouseButtonMask::RIGHT);
	}
	if (GetKeyState(VK_MBUTTON) & (1 << 15)) {
		last_button_state.set_flag(MouseButtonMask::MIDDLE);
	}
	if (GetKeyState(VK_XBUTTON1) & (1 << 15)) {
		last_button_state.set_flag(MouseButtonMask::MB_XBUTTON1);
	}
	if (GetKeyState(VK_XBUTTON2) & (1 << 15)) {
		last_button_state.set_flag(MouseButtonMask::MB_XBUTTON2);
	}

	return last_button_state;
}

void DisplayServerWindows::clipboard_set(const String &p_text) {
	_THREAD_SAFE_METHOD_

	if (!windows.has(DisplayServerEnums::MAIN_WINDOW_ID)) {
		return;
	}

	// Convert LF line endings to CRLF in clipboard content.
	// Otherwise, line endings won't be visible when pasted in other software.
	String text = p_text.replace("\r\n", "\n").replace("\n", "\r\n"); // Avoid \r\r\n.

	if (!OpenClipboard(windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd)) {
		ERR_FAIL_MSG("Unable to open clipboard.");
	}
	EmptyClipboard();

	Char16String utf16 = text.utf16();
	HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (utf16.length() + 1) * sizeof(WCHAR));
	ERR_FAIL_NULL_MSG(mem, "Unable to allocate memory for clipboard contents.");

	LPWSTR lptstrCopy = (LPWSTR)GlobalLock(mem);
	memcpy(lptstrCopy, utf16.get_data(), (utf16.length() + 1) * sizeof(WCHAR));
	GlobalUnlock(mem);

	SetClipboardData(CF_UNICODETEXT, mem);

	// Set the CF_TEXT version (not needed?).
	CharString utf8 = text.utf8();
	mem = GlobalAlloc(GMEM_MOVEABLE, utf8.length() + 1);
	ERR_FAIL_NULL_MSG(mem, "Unable to allocate memory for clipboard contents.");

	LPTSTR ptr = (LPTSTR)GlobalLock(mem);
	memcpy(ptr, utf8.get_data(), utf8.length());
	ptr[utf8.length()] = 0;
	GlobalUnlock(mem);

	SetClipboardData(CF_TEXT, mem);

	CloseClipboard();
}

String DisplayServerWindows::clipboard_get() const {
	_THREAD_SAFE_METHOD_

	if (!windows.has(DisplayServerEnums::MAIN_WINDOW_ID)) {
		return String();
	}

	String ret;
	if (!OpenClipboard(windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd)) {
		ERR_FAIL_V_MSG("", "Unable to open clipboard.");
	}

	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
		if (mem != nullptr) {
			LPWSTR ptr = (LPWSTR)GlobalLock(mem);
			if (ptr != nullptr) {
				ret = String::utf16((const char16_t *)ptr);
				GlobalUnlock(mem);
			}
		}

	} else if (IsClipboardFormatAvailable(CF_TEXT)) {
		HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
		if (mem != nullptr) {
			LPTSTR ptr = (LPTSTR)GlobalLock(mem);
			if (ptr != nullptr) {
				ret.append_utf8((const char *)ptr);
				GlobalUnlock(mem);
			}
		}
	}

	CloseClipboard();

	return ret;
}

Ref<Image> DisplayServerWindows::clipboard_get_image() const {
	Ref<Image> image;
	if (!windows.has(last_focused_window)) {
		return image; // No focused window?
	}
	if (!OpenClipboard(windows[last_focused_window].hWnd)) {
		ERR_FAIL_V_MSG(image, "Unable to open clipboard.");
	}
	UINT png_format = RegisterClipboardFormatA("PNG");
	if (png_format && IsClipboardFormatAvailable(png_format)) {
		HANDLE png_handle = GetClipboardData(png_format);
		if (png_handle) {
			size_t png_size = GlobalSize(png_handle);
			uint8_t *png_data = (uint8_t *)GlobalLock(png_handle);
			image.instantiate();

			PNGDriverCommon::png_to_image(png_data, png_size, false, image);

			GlobalUnlock(png_handle);
		}
	} else if (IsClipboardFormatAvailable(CF_DIB)) {
		HGLOBAL mem = GetClipboardData(CF_DIB);
		if (mem != nullptr) {
			BITMAPINFO *ptr = static_cast<BITMAPINFO *>(GlobalLock(mem));

			if (ptr != nullptr) {
				BITMAPINFOHEADER *info = &ptr->bmiHeader;
				void *dib_bits = (void *)(ptr->bmiColors);

				// Draw DIB image to temporary DC surface and read it back as BGRA8.
				HDC dc = GetDC(nullptr);
				if (dc) {
					HDC hdc = CreateCompatibleDC(dc);
					if (hdc) {
						HBITMAP hbm = CreateCompatibleBitmap(dc, info->biWidth, std::abs(info->biHeight));
						if (hbm) {
							SelectObject(hdc, hbm);
							SetDIBitsToDevice(hdc, 0, 0, info->biWidth, std::abs(info->biHeight), 0, 0, 0, std::abs(info->biHeight), dib_bits, ptr, DIB_RGB_COLORS);

							BITMAPINFO bmp_info = {};
							bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);
							bmp_info.bmiHeader.biWidth = info->biWidth;
							bmp_info.bmiHeader.biHeight = -std::abs(info->biHeight);
							bmp_info.bmiHeader.biPlanes = 1;
							bmp_info.bmiHeader.biBitCount = 32;
							bmp_info.bmiHeader.biCompression = BI_RGB;

							Vector<uint8_t> img_data;
							img_data.resize(info->biWidth * std::abs(info->biHeight) * 4);
							GetDIBits(hdc, hbm, 0, std::abs(info->biHeight), img_data.ptrw(), &bmp_info, DIB_RGB_COLORS);

							uint8_t *wr = (uint8_t *)img_data.ptrw();
							for (int i = 0; i < info->biWidth * std::abs(info->biHeight); i++) {
								SWAP(wr[i * 4 + 0], wr[i * 4 + 2]); // Swap B and R.
								if (info->biBitCount != 32) {
									wr[i * 4 + 3] = 255; // Set A to solid if it's not in the source image.
								}
							}
							image = Image::create_from_data(info->biWidth, std::abs(info->biHeight), false, Image::Format::FORMAT_RGBA8, img_data);

							DeleteObject(hbm);
						}
						DeleteDC(hdc);
					}
					ReleaseDC(nullptr, dc);
				}
				GlobalUnlock(mem);
			}
		}
	}
	CloseClipboard();

	return image;
}

bool DisplayServerWindows::clipboard_has() const {
	return (IsClipboardFormatAvailable(CF_TEXT) ||
			IsClipboardFormatAvailable(CF_UNICODETEXT) ||
			IsClipboardFormatAvailable(CF_OEMTEXT));
}

bool DisplayServerWindows::clipboard_has_image() const {
	UINT png_format = RegisterClipboardFormatA("PNG");
	return ((png_format && IsClipboardFormatAvailable(png_format)) || IsClipboardFormatAvailable(CF_DIB));
}

typedef struct {
	int count;
	int screen;
	HMONITOR monitor;
} EnumScreenData;

static BOOL CALLBACK _MonitorEnumProcPrim(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumScreenData *data = (EnumScreenData *)dwData;
	if ((lprcMonitor->left == 0) && (lprcMonitor->top == 0)) {
		data->screen = data->count;
		return FALSE;
	}

	data->count++;
	return TRUE;
}

static BOOL CALLBACK _MonitorEnumProcScreen(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumScreenData *data = (EnumScreenData *)dwData;
	if (data->monitor == hMonitor) {
		data->screen = data->count;
	}

	data->count++;
	return TRUE;
}

static BOOL CALLBACK _MonitorEnumProcCount(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	int *data = (int *)dwData;
	(*data)++;
	return TRUE;
}

static BOOL CALLBACK _MonitorEnumProcMonitor(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumScreenData *data = (EnumScreenData *)dwData;

	if (data->screen == data->count) {
		data->monitor = hMonitor;
		return FALSE;
	}

	data->count++;
	return TRUE;
}

static HMONITOR _get_hmonitor_of_screen(int p_screen) {
	EnumScreenData data = { 0, p_screen, nullptr };
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcMonitor, (LPARAM)&data);
	return data.monitor;
}

int DisplayServerWindows::get_screen_count() const {
	_THREAD_SAFE_METHOD_

	int data = 0;
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcCount, (LPARAM)&data);
	return data;
}

int DisplayServerWindows::get_primary_screen() const {
	EnumScreenData data = { 0, 0, nullptr };
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcPrim, (LPARAM)&data);
	return data.screen;
}

int DisplayServerWindows::get_keyboard_focus_screen() const {
	HWND hwnd = GetForegroundWindow();
	if (hwnd) {
		EnumScreenData data = { 0, 0, MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST) };
		EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcScreen, (LPARAM)&data);
		return data.screen;
	} else {
		return get_primary_screen();
	}
}

typedef struct {
	int count;
	int screen;
	Point2 pos;
} EnumPosData;

static BOOL CALLBACK _MonitorEnumProcPos(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumPosData *data = (EnumPosData *)dwData;
	if (data->count == data->screen) {
		data->pos.x = lprcMonitor->left;
		data->pos.y = lprcMonitor->top;
	}

	data->count++;
	return TRUE;
}

static BOOL CALLBACK _MonitorEnumProcOrigin(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumPosData *data = (EnumPosData *)dwData;
	data->pos = data->pos.min(Point2(lprcMonitor->left, lprcMonitor->top));

	return TRUE;
}

Point2i DisplayServerWindows::_get_screens_origin() const {
	_THREAD_SAFE_METHOD_

	EnumPosData data = { 0, 0, Point2() };
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcOrigin, (LPARAM)&data);
	return data.pos;
}

Point2i DisplayServerWindows::screen_get_position(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Point2i());

	EnumPosData data = { 0, p_screen, Point2() };
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcPos, (LPARAM)&data);
	return data.pos - _get_screens_origin();
}

typedef struct {
	int count;
	int screen;
	Size2 size;
} EnumSizeData;

typedef struct {
	int count;
	int screen;
	Rect2i rect;
} EnumRectData;

typedef struct {
	Vector<DISPLAYCONFIG_PATH_INFO> paths;
	Vector<DISPLAYCONFIG_MODE_INFO> modes;
	int count;
	int screen;
	float rate;
} EnumRefreshRateData;

static BOOL CALLBACK _MonitorEnumProcSize(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumSizeData *data = (EnumSizeData *)dwData;
	if (data->count == data->screen) {
		data->size.x = lprcMonitor->right - lprcMonitor->left;
		data->size.y = lprcMonitor->bottom - lprcMonitor->top;
	}

	data->count++;
	return TRUE;
}

Size2i DisplayServerWindows::screen_get_size(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Size2i());

	EnumSizeData data = { 0, p_screen, Size2() };
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcSize, (LPARAM)&data);
	return data.size;
}

static BOOL CALLBACK _MonitorEnumProcUsableSize(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumRectData *data = (EnumRectData *)dwData;
	if (data->count == data->screen) {
		MONITORINFO minfo;
		memset(&minfo, 0, sizeof(MONITORINFO));
		minfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfoA(hMonitor, &minfo);

		data->rect.position.x = minfo.rcWork.left;
		data->rect.position.y = minfo.rcWork.top;
		data->rect.size.x = minfo.rcWork.right - minfo.rcWork.left;
		data->rect.size.y = minfo.rcWork.bottom - minfo.rcWork.top;
	}

	data->count++;
	return TRUE;
}

static BOOL CALLBACK _MonitorEnumProcRefreshRate(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumRefreshRateData *data = (EnumRefreshRateData *)dwData;
	if (data->count == data->screen) {
		MONITORINFOEXW minfo;
		memset(&minfo, 0, sizeof(minfo));
		minfo.cbSize = sizeof(minfo);
		GetMonitorInfoW(hMonitor, &minfo);

		bool found = false;
		for (const DISPLAYCONFIG_PATH_INFO &path : data->paths) {
			DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name;
			memset(&source_name, 0, sizeof(source_name));
			source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			source_name.header.size = sizeof(source_name);
			source_name.header.adapterId = path.sourceInfo.adapterId;
			source_name.header.id = path.sourceInfo.id;
			if (DisplayConfigGetDeviceInfo(&source_name.header) == ERROR_SUCCESS) {
				if (wcscmp(minfo.szDevice, source_name.viewGdiDeviceName) == 0 && path.targetInfo.refreshRate.Numerator != 0 && path.targetInfo.refreshRate.Denominator != 0) {
					data->rate = (double)path.targetInfo.refreshRate.Numerator / (double)path.targetInfo.refreshRate.Denominator;
					found = true;
					break;
				}
			}
		}
		if (!found) {
			DEVMODEW dm;
			memset(&dm, 0, sizeof(dm));
			dm.dmSize = sizeof(dm);
			EnumDisplaySettingsW(minfo.szDevice, ENUM_CURRENT_SETTINGS, &dm);

			data->rate = dm.dmDisplayFrequency;
		}
	}

	data->count++;
	return TRUE;
}

Rect2i DisplayServerWindows::screen_get_usable_rect(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	EnumRectData data = { 0, p_screen, Rect2i() };
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcUsableSize, (LPARAM)&data);
	data.rect.position -= _get_screens_origin();
	return data.rect;
}

typedef struct {
	int current_index;
	int screen;
	int dpi;
} EnumDpiData;

static int QueryDpiForMonitor(HMONITOR hmon) {
	int dpiX = 96, dpiY = 96;

	UINT x = 0, y = 0;
	if (hmon) {
		HRESULT hr = GetDpiForMonitor(hmon, MDT_DEFAULT, &x, &y);
		if (SUCCEEDED(hr) && (x > 0) && (y > 0)) {
			dpiX = (int)x;
			dpiY = (int)y;
		}
	} else {
		static int overallX = 0, overallY = 0;
		if (overallX <= 0 || overallY <= 0) {
			HDC hdc = GetDC(nullptr);
			if (hdc) {
				overallX = GetDeviceCaps(hdc, LOGPIXELSX);
				overallY = GetDeviceCaps(hdc, LOGPIXELSY);
				ReleaseDC(nullptr, hdc);
			}
		}
		if (overallX > 0 && overallY > 0) {
			dpiX = overallX;
			dpiY = overallY;
		}
	}

	return (dpiX + dpiY) / 2;
}

static BOOL CALLBACK _MonitorEnumProcDpi(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	EnumDpiData *data = (EnumDpiData *)dwData;

	data->current_index++;

	if (data->current_index == data->screen) {
		data->dpi = QueryDpiForMonitor(hMonitor);
		return FALSE;
	}
	return TRUE;
}

int DisplayServerWindows::screen_get_dpi(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);

	EnumDpiData data = { -1, p_screen, 96 };
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcDpi, (LPARAM)&data);

	ERR_FAIL_COND_V_MSG(data.current_index < p_screen, 96, vformat("Screen index %d out of range [0, %d].", p_screen, data.current_index));
	return data.dpi;
}

Color DisplayServerWindows::screen_get_pixel(const Point2i &p_position) const {
	Point2i pos = p_position + _get_screens_origin();

	POINT p;
	p.x = pos.x;
	p.y = pos.y;
	LogicalToPhysicalPointForPerMonitorDPI(nullptr, &p);

	HDC dc = GetDC(nullptr);
	if (dc) {
		COLORREF col = GetPixel(dc, p.x, p.y);
		if (col != CLR_INVALID) {
			ReleaseDC(nullptr, dc);
			return Color(float(col & 0x000000FF) / 255.0f, float((col & 0x0000FF00) >> 8) / 255.0f, float((col & 0x00FF0000) >> 16) / 255.0f, 1.0f);
		}
		ReleaseDC(nullptr, dc);
	}

	return Color();
}

Ref<Image> DisplayServerWindows::screen_get_image(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Ref<Image>());

	Point2i pos = screen_get_position(p_screen) + _get_screens_origin();
	Size2i size = screen_get_size(p_screen);

	POINT p1;
	p1.x = pos.x;
	p1.y = pos.y;

	POINT p2;
	p2.x = pos.x + size.x;
	p2.y = pos.y + size.y;
	LogicalToPhysicalPointForPerMonitorDPI(nullptr, &p1);
	LogicalToPhysicalPointForPerMonitorDPI(nullptr, &p2);

	Ref<Image> img;
	HDC dc = GetDC(nullptr);
	if (dc) {
		HDC hdc = CreateCompatibleDC(dc);
		int width = p2.x - p1.x;
		int height = p2.y - p1.y;
		if (hdc) {
			HBITMAP hbm = CreateCompatibleBitmap(dc, width, height);
			if (hbm) {
				SelectObject(hdc, hbm);
				BitBlt(hdc, 0, 0, width, height, dc, p1.x, p1.y, SRCCOPY);

				BITMAPINFO bmp_info = {};
				bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);
				bmp_info.bmiHeader.biWidth = width;
				bmp_info.bmiHeader.biHeight = -height;
				bmp_info.bmiHeader.biPlanes = 1;
				bmp_info.bmiHeader.biBitCount = 32;
				bmp_info.bmiHeader.biCompression = BI_RGB;

				Vector<uint8_t> img_data;
				img_data.resize(width * height * 4);
				GetDIBits(hdc, hbm, 0, height, img_data.ptrw(), &bmp_info, DIB_RGB_COLORS);

				uint8_t *wr = (uint8_t *)img_data.ptrw();
				for (int i = 0; i < width * height; i++) {
					SWAP(wr[i * 4 + 0], wr[i * 4 + 2]); // Swap B and R.
				}
				img = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, img_data);

				DeleteObject(hbm);
			}
			DeleteDC(hdc);
		}
		ReleaseDC(nullptr, dc);
	}

	return img;
}

Ref<Image> DisplayServerWindows::screen_get_image_rect(const Rect2i &p_rect) const {
	Point2i pos = p_rect.position + _get_screens_origin();
	Size2i size = p_rect.size;

	POINT p1;
	p1.x = pos.x;
	p1.y = pos.y;

	POINT p2;
	p2.x = pos.x + size.x;
	p2.y = pos.y + size.y;
	LogicalToPhysicalPointForPerMonitorDPI(nullptr, &p1);
	LogicalToPhysicalPointForPerMonitorDPI(nullptr, &p2);

	Ref<Image> img;
	HDC dc = GetDC(nullptr);
	if (dc) {
		HDC hdc = CreateCompatibleDC(dc);
		int width = p2.x - p1.x;
		int height = p2.y - p1.y;
		if (hdc) {
			HBITMAP hbm = CreateCompatibleBitmap(dc, width, height);
			if (hbm) {
				SelectObject(hdc, hbm);
				BitBlt(hdc, 0, 0, width, height, dc, p1.x, p1.y, SRCCOPY);

				BITMAPINFO bmp_info = {};
				bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);
				bmp_info.bmiHeader.biWidth = width;
				bmp_info.bmiHeader.biHeight = -height;
				bmp_info.bmiHeader.biPlanes = 1;
				bmp_info.bmiHeader.biBitCount = 32;
				bmp_info.bmiHeader.biCompression = BI_RGB;

				Vector<uint8_t> img_data;
				img_data.resize(width * height * 4);
				GetDIBits(hdc, hbm, 0, height, img_data.ptrw(), &bmp_info, DIB_RGB_COLORS);

				uint8_t *wr = (uint8_t *)img_data.ptrw();
				for (int i = 0; i < width * height; i++) {
					SWAP(wr[i * 4 + 0], wr[i * 4 + 2]); // Swap B and R.
				}
				img = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, img_data);

				DeleteObject(hbm);
			}
			DeleteDC(hdc);
		}
		ReleaseDC(nullptr, dc);
	}

	return img;
}

#ifdef D3D12_ENABLED
static bool _get_monitor_desc(HMONITOR p_monitor, IDXGIFactory2 *dxgi_factory, DXGI_OUTPUT_DESC1 &r_Desc) {
	r_Desc = {};

	// Note: As of August, 2025 Microsoft's sample code only checks the default
	// adapter, but sometimes p_monitor may belong to another adapter.
	Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter;
	UINT adapter_i = 0;
	while (dxgi_factory->EnumAdapters1(adapter_i, &dxgiAdapter) == S_OK) {
		Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
		DXGI_OUTPUT_DESC1 desc1;
		UINT output_i = 0;
		while (dxgiAdapter->EnumOutputs(output_i, &dxgiOutput) != DXGI_ERROR_NOT_FOUND) {
			Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
			if (FAILED(dxgiOutput.As(&output6))) {
				continue;
			}

			if (FAILED(output6->GetDesc1(&desc1))) {
				continue;
			}

			if (desc1.Monitor == p_monitor) {
				r_Desc = desc1;
				return true;
			}

			output_i++;
		}
		adapter_i++;
	}

	return false;
}
#endif // D3D12_ENABLED

// Store a list of displays that have failed to get their SDR white level so we don't spam the error log.
static HashSet<Vector3i> displays_with_white_level_error;

static float _get_sdr_white_level_for_hmonitor(HMONITOR p_monitor, const LocalVector<DISPLAYCONFIG_PATH_INFO> &p_paths) {
	if (!p_monitor) {
		return 0.0f;
	}

	MONITORINFOEXW minfo;
	memset(&minfo, 0, sizeof(minfo));
	minfo.cbSize = sizeof(minfo);
	if (!GetMonitorInfoW(p_monitor, &minfo)) {
		ERR_FAIL_V_MSG(0.0f, vformat("Failed to get monitor info for HMONITOR: 0x%X", (intptr_t)p_monitor));
	}

	for (const DISPLAYCONFIG_PATH_INFO &path : p_paths) {
		const Vector3i key = Vector3i((int32_t)path.sourceInfo.adapterId.HighPart, (int32_t)path.sourceInfo.adapterId.LowPart, (int32_t)path.sourceInfo.id);

		DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name;
		memset(&source_name, 0, sizeof(source_name));
		source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
		source_name.header.size = sizeof(source_name);
		source_name.header.adapterId = path.sourceInfo.adapterId;
		source_name.header.id = path.sourceInfo.id;

		if (DisplayConfigGetDeviceInfo(&source_name.header) != ERROR_SUCCESS) {
			if (!displays_with_white_level_error.has(key)) {
				WARN_PRINT(vformat("Failed to get source name for adapterId: 0x%08X%08X, id: %d", (uint32_t)path.sourceInfo.adapterId.HighPart, (uint32_t)path.sourceInfo.adapterId.LowPart, path.sourceInfo.id));
				displays_with_white_level_error.insert(key);
			}
			continue;
		}

		if (wcscmp(minfo.szDevice, source_name.viewGdiDeviceName) != 0) {
			continue;
		}

		// Query the SDR white level.
		DISPLAYCONFIG_SDR_WHITE_LEVEL sdr_white_level;
		memset(&sdr_white_level, 0, sizeof(sdr_white_level));
		sdr_white_level.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
		sdr_white_level.header.size = sizeof(sdr_white_level);
		sdr_white_level.header.adapterId = path.targetInfo.adapterId;
		sdr_white_level.header.id = path.targetInfo.id;

		if (DisplayConfigGetDeviceInfo(&sdr_white_level.header) != ERROR_SUCCESS) {
			if (!displays_with_white_level_error.has(key)) {
				WARN_PRINT(vformat("Failed to get SDR white level for adapterId: 0x%08X%08X, id: %d", (uint32_t)path.targetInfo.adapterId.HighPart, (uint32_t)path.targetInfo.adapterId.LowPart, path.targetInfo.id));
				displays_with_white_level_error.insert(key);
			}
			continue;
		}

		return (float)sdr_white_level.SDRWhiteLevel / 1000.0f * 80.0f;
	}

	return 0.0f;
}

float DisplayServerWindows::screen_get_refresh_rate(int p_screen) const {
	_THREAD_SAFE_METHOD_

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, SCREEN_REFRESH_RATE_FALLBACK);

	EnumRefreshRateData data = { Vector<DISPLAYCONFIG_PATH_INFO>(), Vector<DISPLAYCONFIG_MODE_INFO>(), 0, p_screen, SCREEN_REFRESH_RATE_FALLBACK };

	uint32_t path_count = 0;
	uint32_t mode_count = 0;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count, &mode_count) == ERROR_SUCCESS) {
		data.paths.resize(path_count);
		data.modes.resize(mode_count);
		if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_count, data.paths.ptrw(), &mode_count, data.modes.ptrw(), nullptr) != ERROR_SUCCESS) {
			data.paths.clear();
			data.modes.clear();
		}
	}

	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcRefreshRate, (LPARAM)&data);
	return data.rate;
}

void DisplayServerWindows::screen_set_keep_on(bool p_enable) {
	if (keep_screen_on == p_enable) {
		return;
	}

	if (p_enable) {
		const String reason = "Godot Engine running with display/window/energy_saving/keep_screen_on = true";
		Char16String reason_utf16 = reason.utf16();
		REASON_CONTEXT context;
		context.Version = POWER_REQUEST_CONTEXT_VERSION;
		context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
		context.Reason.SimpleReasonString = (LPWSTR)(reason_utf16.ptrw());
		power_request = PowerCreateRequest(&context);
		if (power_request == INVALID_HANDLE_VALUE) {
			print_error("Failed to enable screen_keep_on.");
			return;
		}
		if (PowerSetRequest(power_request, POWER_REQUEST_TYPE::PowerRequestSystemRequired) == 0) {
			print_error("Failed to request system sleep override.");
			return;
		}
		if (PowerSetRequest(power_request, POWER_REQUEST_TYPE::PowerRequestDisplayRequired) == 0) {
			print_error("Failed to request display timeout override.");
			return;
		}
	} else {
		PowerClearRequest(power_request, POWER_REQUEST_TYPE::PowerRequestSystemRequired);
		PowerClearRequest(power_request, POWER_REQUEST_TYPE::PowerRequestDisplayRequired);
		CloseHandle(power_request);
		power_request = nullptr;
	}

	keep_screen_on = p_enable;
}

bool DisplayServerWindows::screen_is_kept_on() const {
	return keep_screen_on;
}

Vector<DisplayServerEnums::WindowID> DisplayServerWindows::get_window_list() const {
	_THREAD_SAFE_METHOD_

	Vector<DisplayServerEnums::WindowID> ret;
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		ret.push_back(E.key);
	}
	return ret;
}

DisplayServerEnums::WindowID DisplayServerWindows::get_window_at_screen_position(const Point2i &p_position) const {
	Point2i offset = _get_screens_origin();
	POINT p;
	p.x = p_position.x + offset.x;
	p.y = p_position.y + offset.y;
	HWND hwnd = WindowFromPoint(p);
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (E.value.hWnd == hwnd) {
			return E.key;
		}
	}

	return DisplayServerEnums::INVALID_WINDOW_ID;
}

DisplayServerEnums::WindowID DisplayServerWindows::create_sub_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i &p_rect, bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent) {
	_THREAD_SAFE_METHOD_

	bool no_redirection_bitmap = false;
#ifdef DCOMP_ENABLED
	no_redirection_bitmap = OS::is_layered_allowed() && rendering_driver == "d3d12";
#endif

	DisplayServerEnums::WindowID window_id = window_id_counter;
	Error err = _create_window(window_id, p_mode, p_flags, p_rect, p_exclusive, p_transient_parent, nullptr, no_redirection_bitmap);
	ERR_FAIL_COND_V_MSG(err != OK, DisplayServerEnums::INVALID_WINDOW_ID, "Failed to create sub window.");
	++window_id_counter;

#ifdef RD_ENABLED
	if (rendering_context != nullptr) {
		_create_rendering_context_window(window_id, rendering_driver);
	}
#endif
#ifdef GLES3_ENABLED
	_create_gl_window(window_id);
#endif

	window_set_vsync_mode(p_vsync_mode, window_id);

	WindowData &wd = windows[window_id];

	if (p_flags & DisplayServerEnums::WINDOW_FLAG_RESIZE_DISABLED_BIT) {
		wd.resizable = false;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_MINIMIZE_DISABLED_BIT) {
		wd.no_min_btn = true;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_MAXIMIZE_DISABLED_BIT) {
		wd.no_max_btn = true;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_BORDERLESS_BIT) {
		wd.borderless = true;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_ALWAYS_ON_TOP_BIT && p_mode != DisplayServerEnums::WINDOW_MODE_FULLSCREEN && p_mode != DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
		wd.always_on_top = true;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_SHARP_CORNERS_BIT) {
		wd.sharp_corners = true;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_NO_FOCUS_BIT) {
		wd.no_focus = true;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_MOUSE_PASSTHROUGH_BIT) {
		wd.mpass = true;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_EXCLUDE_FROM_CAPTURE_BIT) {
		wd.hide_from_capture = true;
		if (os_ver.dwBuildNumber >= 19041) {
			SetWindowDisplayAffinity(wd.hWnd, WDA_EXCLUDEFROMCAPTURE);
		} else {
			SetWindowDisplayAffinity(wd.hWnd, WDA_MONITOR);
		}
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_POPUP_BIT) {
		wd.is_popup = true;
	}
	if (p_flags & DisplayServerEnums::WINDOW_FLAG_TRANSPARENT_BIT) {
		if (OS::is_layered_allowed()) {
			DWM_BLURBEHIND bb;
			ZeroMemory(&bb, sizeof(bb));
			HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
			bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
			bb.hRgnBlur = hRgn;
			bb.fEnable = TRUE;
			DwmEnableBlurBehindWindow(wd.hWnd, &bb);
		}

		wd.layered_window = true;
	}

	// Inherit icons from MAIN_WINDOW for all sub windows.
	HICON mainwindow_icon = (HICON)SendMessage(windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd, WM_GETICON, ICON_SMALL, 0);
	if (mainwindow_icon) {
		SendMessage(windows[window_id].hWnd, WM_SETICON, ICON_SMALL, (LPARAM)mainwindow_icon);
	}
	mainwindow_icon = (HICON)SendMessage(windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd, WM_GETICON, ICON_BIG, 0);
	if (mainwindow_icon) {
		SendMessage(windows[window_id].hWnd, WM_SETICON, ICON_BIG, (LPARAM)mainwindow_icon);
	}
#ifdef RD_ENABLED
	if (rendering_device) {
		rendering_device->screen_create(window_id);
	}
#endif
	return window_id;
}

bool DisplayServerWindows::_is_always_on_top_recursive(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(!windows.has(p_window), false);

	const WindowData &wd = windows[p_window];
	if (wd.always_on_top) {
		return true;
	}

	if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
		return _is_always_on_top_recursive(wd.transient_parent);
	}

	return false;
}

void DisplayServerWindows::show_window(DisplayServerEnums::WindowID p_id) {
	ERR_FAIL_COND(!windows.has(p_id));

	WindowData &wd = windows[p_id];
	popup_open(p_id);

	if (p_id != DisplayServerEnums::MAIN_WINDOW_ID) {
		_update_window_style(p_id);
	}
	wd.initialized = true;

	if (wd.maximized) {
		ShowWindow(wd.hWnd, SW_SHOWMAXIMIZED);
		SetForegroundWindow(wd.hWnd); // Slightly higher priority.
		SetFocus(wd.hWnd); // Set keyboard focus.
	} else if (wd.minimized) {
		ShowWindow(wd.hWnd, SW_SHOWMINIMIZED);
	} else if (wd.no_focus) {
		// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
		ShowWindow(wd.hWnd, SW_SHOWNA);
	} else if (wd.is_popup) {
		ShowWindow(wd.hWnd, SW_SHOWNA);
		SetFocus(wd.hWnd); // Set keyboard focus.
	} else {
		ShowWindow(wd.hWnd, SW_SHOW);
		SetForegroundWindow(wd.hWnd); // Slightly higher priority.
		SetFocus(wd.hWnd); // Set keyboard focus.
	}
	if (_is_always_on_top_recursive(p_id)) {
		SetWindowPos(wd.hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | ((wd.no_focus || wd.is_popup) ? SWP_NOACTIVATE : 0));
	}
}

void DisplayServerWindows::delete_sub_window(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	ERR_FAIL_COND_MSG(p_window == DisplayServerEnums::MAIN_WINDOW_ID, "Main window cannot be deleted.");

	popup_close(p_window);

	WindowData &wd = windows[p_window];

	while (wd.transient_children.size()) {
		DisplayServerEnums::WindowID wid = *wd.transient_children.begin();
		if (windows.has(wid)) {
			WindowData &wd_window = windows[wid];
			wd_window.transient_parent = DisplayServerEnums::INVALID_WINDOW_ID;
			if (wd_window.exclusive) {
				SetWindowLongPtr(wd_window.hWnd, GWLP_HWNDPARENT, (LONG_PTR) nullptr);
			}
		}
		wd.transient_children.erase(wid);
	}

	if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
		window_set_transient(p_window, DisplayServerEnums::INVALID_WINDOW_ID);
	}

#ifdef RD_ENABLED
	if (rendering_device) {
		rendering_device->screen_free(p_window);
	}

	if (rendering_context) {
		rendering_context->window_destroy(p_window);
	}
#endif
#ifdef GLES3_ENABLED
#ifdef ANGLE_ENABLED
	if (gl_manager_angle) {
		gl_manager_angle->window_destroy(p_window);
	}
#endif
	if (gl_manager_native) {
		gl_manager_native->window_destroy(p_window);
	}
#endif

	_destroy_window(p_window);

	if (last_focused_window == p_window) {
		last_focused_window = DisplayServerEnums::INVALID_WINDOW_ID;
	}
}

void DisplayServerWindows::gl_window_make_current(DisplayServerEnums::WindowID p_window_id) {
#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->window_make_current(p_window_id);
	}
#endif
	if (gl_manager_native) {
		gl_manager_native->window_make_current(p_window_id);
	}
#endif
}

int64_t DisplayServerWindows::window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(!windows.has(p_window), 0);
	switch (p_handle_type) {
		case DisplayServerEnums::DISPLAY_HANDLE: {
			return 0; // Not supported.
		}
		case DisplayServerEnums::WINDOW_HANDLE: {
			return (int64_t)windows[p_window].hWnd;
		}
#if defined(GLES3_ENABLED)
		case DisplayServerEnums::WINDOW_VIEW: {
			if (gl_manager_native) {
				return (int64_t)gl_manager_native->get_hdc(p_window);
			} else {
				return (int64_t)GetDC(windows[p_window].hWnd);
			}
		}
		case DisplayServerEnums::OPENGL_CONTEXT: {
			if (gl_manager_native) {
				return (int64_t)gl_manager_native->get_hglrc(p_window);
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

void DisplayServerWindows::window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	windows[p_window].instance_id = p_instance;
}

ObjectID DisplayServerWindows::window_get_attached_instance_id(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), ObjectID());
	return windows[p_window].instance_id;
}

void DisplayServerWindows::window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	windows[p_window].rect_changed_callback = p_callable;
}

void DisplayServerWindows::window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	windows[p_window].event_callback = p_callable;
}

void DisplayServerWindows::window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	windows[p_window].input_event_callback = p_callable;
}

void DisplayServerWindows::window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	windows[p_window].input_text_callback = p_callable;
}

void DisplayServerWindows::window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &window_data = windows[p_window];

	window_data.drop_files_callback = p_callable;

	if (window_data.drop_target == nullptr) {
		window_data.drop_target = memnew(DropTargetWindows(&window_data));
		ERR_FAIL_COND(RegisterDragDrop(window_data.hWnd, window_data.drop_target) != S_OK);
	}
}

void DisplayServerWindows::window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	SetWindowTextW(windows[p_window].hWnd, (LPCWSTR)(p_title.utf16().get_data()));
}

Size2i DisplayServerWindows::window_get_title_size(const String &p_title, DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	Size2i size;
	ERR_FAIL_COND_V(!windows.has(p_window), size);

	const WindowData &wd = windows[p_window];
	if (wd.fullscreen || wd.minimized || wd.borderless) {
		return size;
	}

	HDC hdc = GetDCEx(wd.hWnd, nullptr, DCX_WINDOW);
	if (hdc) {
		Char16String s = p_title.utf16();
		SIZE text_size;
		if (GetTextExtentPoint32W(hdc, (LPCWSTR)(s.get_data()), s.length(), &text_size)) {
			size.x = text_size.cx;
			size.y = text_size.cy;
		}

		ReleaseDC(wd.hWnd, hdc);
	}
	RECT rect;
	if (DwmGetWindowAttribute(wd.hWnd, DWMWA_CAPTION_BUTTON_BOUNDS, &rect, sizeof(RECT)) == S_OK) {
		if (rect.right - rect.left > 0) {
			ClientToScreen(wd.hWnd, (POINT *)&rect.left);
			ClientToScreen(wd.hWnd, (POINT *)&rect.right);

			PhysicalToLogicalPointForPerMonitorDPI(nullptr, (POINT *)&rect.left);
			PhysicalToLogicalPointForPerMonitorDPI(nullptr, (POINT *)&rect.right);

			size.x += (rect.right - rect.left);
			size.y = MAX(size.y, rect.bottom - rect.top);
		}
	}
	if (wd.icon_big || wd.icon_small) {
		size.x += 32;
	} else {
		size.x += 16;
	}
	return size;
}

void DisplayServerWindows::window_set_mouse_passthrough(const Vector<Vector2> &p_region, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	windows[p_window].mpath = p_region;
	_update_window_mouse_passthrough(p_window);
}

void DisplayServerWindows::_update_window_mouse_passthrough(DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(!windows.has(p_window));

	const WindowData &wd = windows[p_window];
	bool clip_pixel = (wd.multiwindow_fs || (wd.borderless && wd.maximized));
	bool pass_set = (wd.mpath.size() > 0);
	if (!clip_pixel && !pass_set) {
		SetWindowRgn(wd.hWnd, nullptr, TRUE);
	} else {
		HRGN region = nullptr;
		if (pass_set) {
			Vector<POINT> points;
			points.resize(wd.mpath.size());
			POINT *points_ptr = points.ptrw();
			for (int i = 0; i < wd.mpath.size(); i++) {
				if (wd.borderless) {
					points_ptr[i].x = wd.mpath[i].x;
					points_ptr[i].y = wd.mpath[i].y;
				} else {
					points_ptr[i].x = wd.mpath[i].x + GetSystemMetrics(SM_CXSIZEFRAME);
					points_ptr[i].y = wd.mpath[i].y + GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CYCAPTION);
				}
			}
			region = CreatePolygonRgn(points.ptr(), points.size(), ALTERNATE);
		} else {
			region = CreateRectRgn(0, 0, wd.width, wd.height);
		}
		if (clip_pixel) {
			HRGN region_clip = CreateRectRgn(0, 0, wd.width, wd.height);
			CombineRgn(region, region, region_clip, RGN_AND);
			DeleteObject(region_clip);
		}
		SetWindowRgn(wd.hWnd, region, FALSE);
	}
}

int DisplayServerWindows::window_get_current_screen(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), DisplayServerEnums::INVALID_SCREEN);

	EnumScreenData data = { 0, 0, MonitorFromWindow(windows[p_window].hWnd, MONITOR_DEFAULTTONEAREST) };
	EnumDisplayMonitors(nullptr, nullptr, _MonitorEnumProcScreen, (LPARAM)&data);
	return data.screen;
}

void DisplayServerWindows::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX(p_screen, screen_count);

	if (window_get_current_screen(p_window) == p_screen) {
		return;
	}
	const WindowData &wd = windows[p_window];

	if (wd.parent_hwnd) {
		print_line("Embedded window can't be moved to another screen.");
		return;
	}
	if (wd.fullscreen) {
		Point2 pos = screen_get_position(p_screen) + _get_screens_origin();
		Size2 size = screen_get_size(p_screen);
		Vector2i off = (wd.multiwindow_fs || (!wd.fullscreen && wd.borderless && wd.maximized)) ? _get_screen_expand_offset(p_screen) : Vector2i();

		MoveWindow(wd.hWnd, pos.x, pos.y, size.width + off.x, size.height + off.y, TRUE);
	} else if (wd.maximized) {
		Point2 pos = screen_get_position(p_screen) + _get_screens_origin();
		Size2 size = screen_get_size(p_screen);
		Vector2i off = (wd.multiwindow_fs || (!wd.fullscreen && wd.borderless && wd.maximized)) ? _get_screen_expand_offset(p_screen) : Vector2i();

		ShowWindow(wd.hWnd, SW_RESTORE);
		MoveWindow(wd.hWnd, pos.x, pos.y, size.width + off.x, size.height + off.y, TRUE);
		ShowWindow(wd.hWnd, SW_MAXIMIZE);
	} else {
		Rect2i srect = screen_get_usable_rect(p_screen);
		Point2i wpos = window_get_position(p_window) - screen_get_position(window_get_current_screen(p_window));
		Size2i wsize = window_get_size(p_window);
		wpos += srect.position;

		wpos = wpos.clamp(srect.position, srect.position + srect.size - wsize / 3);
		window_set_position(wpos, p_window);
	}
}

Point2i DisplayServerWindows::window_get_position(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Point2i());
	const WindowData &wd = windows[p_window];

	if (wd.minimized) {
		return wd.last_pos;
	}

	POINT point;
	point.x = 0;
	point.y = 0;

	ClientToScreen(wd.hWnd, &point);

	return Point2i(point.x, point.y) - _get_screens_origin();
}

Point2i DisplayServerWindows::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Point2i());
	const WindowData &wd = windows[p_window];

	if (wd.minimized) {
		return wd.last_pos;
	}

	RECT r;
	if (GetWindowRect(wd.hWnd, &r)) {
		return Point2i(r.left, r.top) - _get_screens_origin();
	}

	return Point2i();
}

void DisplayServerWindows::_update_real_mouse_position(DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(!windows.has(p_window));

	POINT mouse_pos;
	if (GetCursorPos(&mouse_pos) && ScreenToClient(windows[p_window].hWnd, &mouse_pos)) {
		if (mouse_pos.x > 0 && mouse_pos.y > 0 && mouse_pos.x <= windows[p_window].width && mouse_pos.y <= windows[p_window].height) {
			old_x = mouse_pos.x;
			old_y = mouse_pos.y;
			old_invalid = false;
			Input::set_mouse_position(Point2i(mouse_pos.x, mouse_pos.y));
		}
	}
}

void DisplayServerWindows::window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.parent_hwnd) {
		print_line("Embedded window can't be moved.");
		return;
	}

	if (wd.fullscreen || wd.maximized) {
		return;
	}

	Point2i offset = _get_screens_origin();

	RECT rc;
	rc.left = p_position.x + offset.x;
	rc.right = p_position.x + wd.width + offset.x;
	rc.bottom = p_position.y + wd.height + offset.y;
	rc.top = p_position.y + offset.y;

	const DWORD style = GetWindowLongPtr(wd.hWnd, GWL_STYLE);
	const DWORD exStyle = GetWindowLongPtr(wd.hWnd, GWL_EXSTYLE);

	AdjustWindowRectEx(&rc, style, false, exStyle);
	MoveWindow(wd.hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

	wd.last_pos = p_position;
	_update_real_mouse_position(p_window);
}

void DisplayServerWindows::window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive) {
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	if (wd.exclusive != p_exclusive) {
		wd.exclusive = p_exclusive;
		if (wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
			if (wd.exclusive) {
				WindowData &wd_parent = windows[wd.transient_parent];
				SetWindowLongPtr(wd.hWnd, GWLP_HWNDPARENT, (LONG_PTR)wd_parent.hWnd);
			} else {
				SetWindowLongPtr(wd.hWnd, GWLP_HWNDPARENT, (LONG_PTR) nullptr);
			}
		}
	}
}

void DisplayServerWindows::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(p_window == p_parent);
	ERR_FAIL_COND(!windows.has(p_window));

	WindowData &wd_window = windows[p_window];

	ERR_FAIL_COND(wd_window.transient_parent == p_parent);
	ERR_FAIL_COND_MSG(wd_window.always_on_top, "Windows with the 'on top' can't become transient.");

	if (p_parent == DisplayServerEnums::INVALID_WINDOW_ID) {
		// Remove transient.

		ERR_FAIL_COND(wd_window.transient_parent == DisplayServerEnums::INVALID_WINDOW_ID);
		ERR_FAIL_COND(!windows.has(wd_window.transient_parent));

		WindowData &wd_parent = windows[wd_window.transient_parent];

		wd_window.transient_parent = DisplayServerEnums::INVALID_WINDOW_ID;
		wd_parent.transient_children.erase(p_window);

		if (wd_window.exclusive) {
			SetWindowLongPtr(wd_window.hWnd, GWLP_HWNDPARENT, (LONG_PTR) nullptr);
		}
	} else {
		ERR_FAIL_COND(!windows.has(p_parent));
		ERR_FAIL_COND_MSG(wd_window.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID, "Window already has a transient parent");
		WindowData &wd_parent = windows[p_parent];

		wd_window.transient_parent = p_parent;
		wd_parent.transient_children.insert(p_window);

		if (wd_window.exclusive) {
			SetWindowLongPtr(wd_window.hWnd, GWLP_HWNDPARENT, (LONG_PTR)wd_parent.hWnd);
		}
	}
}

void DisplayServerWindows::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.parent_hwnd) {
		print_line("Embedded windows can't have a maximum size.");
		return;
	}

	if ((p_size != Size2()) && ((p_size.x < wd.min_size.x) || (p_size.y < wd.min_size.y))) {
		ERR_PRINT("Maximum window size can't be smaller than minimum window size!");
		return;
	}
	wd.max_size = p_size;
}

Size2i DisplayServerWindows::window_get_max_size(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Size2i());
	const WindowData &wd = windows[p_window];
	return wd.max_size;
}

void DisplayServerWindows::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.parent_hwnd) {
		print_line("Embedded windows can't have a minimum size.");
		return;
	}

	if ((p_size != Size2()) && (wd.max_size != Size2()) && ((p_size.x > wd.max_size.x) || (p_size.y > wd.max_size.y))) {
		ERR_PRINT("Minimum window size can't be larger than maximum window size!");
		return;
	}
	wd.min_size = p_size;
}

Size2i DisplayServerWindows::window_get_min_size(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Size2i());
	const WindowData &wd = windows[p_window];
	return wd.min_size;
}

void DisplayServerWindows::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.parent_hwnd) {
		print_line("Embedded window can't be resized.");
		return;
	}

	if (wd.fullscreen || wd.maximized) {
		return;
	}

	int w = p_size.width;
	int h = p_size.height;
	RECT rect;
	GetWindowRect(wd.hWnd, &rect);

	if (!wd.borderless) {
		RECT crect;
		GetClientRect(wd.hWnd, &crect);

		w += (rect.right - rect.left) - (crect.right - crect.left);
		h += (rect.bottom - rect.top) - (crect.bottom - crect.top);
	}

	MoveWindow(wd.hWnd, rect.left, rect.top, w, h, TRUE);
}

Size2i DisplayServerWindows::window_get_size(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Size2i());
	const WindowData &wd = windows[p_window];

	// GetClientRect() returns a zero rect for a minimized window, so we need to get the size in another way.
	if (wd.minimized) {
		return Size2(wd.width, wd.height);
	}

	RECT r;
	if (GetClientRect(wd.hWnd, &r)) { // Retrieves area inside of window border, including decoration.
		Vector2i off = (wd.multiwindow_fs || (!wd.fullscreen && wd.borderless && wd.maximized)) ? _get_screen_expand_offset(window_get_current_screen(p_window)) : Vector2i();
		return Size2(r.right - r.left - off.x, r.bottom - r.top - off.y);
	}
	return Size2();
}

Size2i DisplayServerWindows::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Size2i());
	const WindowData &wd = windows[p_window];

	// GetWindowRect() returns a zero rect for a minimized window, so we need to get the size in another way.
	if (wd.minimized) {
		return Size2(wd.width_with_decorations, wd.height_with_decorations);
	}

	RECT r;
	if (GetWindowRect(wd.hWnd, &r)) { // Retrieves area inside of window border, including decoration.
		Vector2i off = (wd.multiwindow_fs || (!wd.fullscreen && wd.borderless && wd.maximized)) ? _get_screen_expand_offset(window_get_current_screen(p_window)) : Vector2i();
		return Size2(r.right - r.left - off.x, r.bottom - r.top - off.y);
	}
	return Size2();
}

void DisplayServerWindows::_get_window_style(bool p_main_window, bool p_initialized, bool p_fullscreen, bool p_multiwindow_fs, bool p_borderless, bool p_resizable, bool p_no_min_btn, bool p_no_max_btn, bool p_minimized, bool p_maximized, bool p_maximized_fs, bool p_no_activate_focus, bool p_embed_child, bool p_no_redirection_bitmap, DWORD &r_style, DWORD &r_style_ex) {
	// Windows docs for window styles:
	// https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
	// https://docs.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles

	r_style = 0;
	r_style_ex = WS_EX_WINDOWEDGE;
	if (p_main_window) {
		// When embedded, we don't want the window to have WS_EX_APPWINDOW because it will
		// show the embedded process in the taskbar and Alt-Tab.
		if (!p_embed_child) {
			r_style_ex |= WS_EX_APPWINDOW;
		}
		if (p_initialized) {
			r_style |= WS_VISIBLE;
		}
	}

	if (p_embed_child) {
		r_style |= WS_POPUP;
	} else if (p_fullscreen || p_borderless) {
		r_style |= WS_POPUP; // p_borderless was WS_EX_TOOLWINDOW in the past.
		if (p_minimized) {
			r_style |= WS_MINIMIZE;
		} else if (p_maximized) {
			r_style |= WS_MAXIMIZE;
		}
		if (!p_fullscreen) {
			r_style |= WS_SYSMENU;
			if (!p_no_min_btn) {
				r_style |= WS_MINIMIZEBOX;
			}
			if (!p_no_max_btn) {
				r_style |= WS_MAXIMIZEBOX;
			}
		}
	} else {
		if (p_resizable) {
			if (p_minimized) {
				r_style = WS_OVERLAPPEDWINDOW | WS_MINIMIZE;
			} else if (p_maximized) {
				r_style = WS_OVERLAPPEDWINDOW | WS_MAXIMIZE;
			} else {
				r_style = WS_OVERLAPPEDWINDOW;
			}
			if (p_no_min_btn) {
				r_style &= ~WS_MINIMIZEBOX;
			}
			if (p_no_max_btn) {
				r_style &= ~WS_MAXIMIZEBOX;
			}
		} else {
			if (p_minimized) {
				r_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZE;
			} else {
				r_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
			}
			if (!p_no_min_btn) {
				r_style |= WS_MINIMIZEBOX;
			}
			if (!p_no_max_btn) {
				r_style |= WS_MAXIMIZEBOX;
			}
		}
	}

	if (p_no_activate_focus && !p_embed_child) {
		r_style_ex |= WS_EX_TOPMOST | WS_EX_NOACTIVATE;
	}

	if (!p_borderless && !p_no_activate_focus && p_initialized) {
		r_style |= WS_VISIBLE;
	}

	r_style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	r_style_ex |= WS_EX_ACCEPTFILES;

	if (p_no_redirection_bitmap) {
		r_style_ex |= WS_EX_NOREDIRECTIONBITMAP;
	}
}

void DisplayServerWindows::_update_window_style(DisplayServerEnums::WindowID p_window, bool p_repaint) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	DWORD style = 0;
	DWORD style_ex = 0;

	_get_window_style(p_window == DisplayServerEnums::MAIN_WINDOW_ID, wd.initialized, wd.fullscreen, wd.multiwindow_fs, wd.borderless, wd.resizable, wd.no_min_btn, wd.no_max_btn, wd.minimized, wd.maximized, wd.maximized_fs, wd.no_focus || wd.is_popup, wd.parent_hwnd, wd.no_redirection_bitmap, style, style_ex);

	SetWindowLongPtr(wd.hWnd, GWL_STYLE, style);
	SetWindowLongPtr(wd.hWnd, GWL_EXSTYLE, style_ex);

	if (wd.icon_set) {
		if (wd.icon_big && !wd.icon_small) {
			SendMessage(wd.hWnd, WM_SETICON, ICON_BIG, (LPARAM)wd.icon_big);
			SendMessage(wd.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wd.icon_big);
		} else {
			if (wd.icon_big) {
				SendMessage(wd.hWnd, WM_SETICON, ICON_BIG, (LPARAM)wd.icon_big);
			}
			if (wd.icon_small) {
				SendMessage(wd.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wd.icon_small);
			}
		}
	} else if (p_window != DisplayServerEnums::MAIN_WINDOW_ID) {
		HICON mainwindow_icon = (HICON)SendMessage(windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd, WM_GETICON, ICON_SMALL, 0);
		if (mainwindow_icon) {
			SendMessage(wd.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)mainwindow_icon);
		}
		mainwindow_icon = (HICON)SendMessage(windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd, WM_GETICON, ICON_BIG, 0);
		if (mainwindow_icon) {
			SendMessage(wd.hWnd, WM_SETICON, ICON_BIG, (LPARAM)mainwindow_icon);
		}
	}

	SetWindowPos(wd.hWnd, _is_always_on_top_recursive(p_window) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | ((wd.no_focus || wd.is_popup) ? SWP_NOACTIVATE : 0));

	if (p_repaint) {
		RECT rect;
		GetWindowRect(wd.hWnd, &rect);
		Vector2i off = (wd.multiwindow_fs || (!wd.fullscreen && wd.borderless && wd.maximized)) ? _get_screen_expand_offset(window_get_current_screen(p_window)) : Vector2i();
		MoveWindow(wd.hWnd, rect.left, rect.top, rect.right - rect.left + off.x, rect.bottom - rect.top - off.y, TRUE);
	}
}

void DisplayServerWindows::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (p_mode != DisplayServerEnums::WINDOW_MODE_WINDOWED && wd.parent_hwnd) {
		print_line("Embedded window only supports Windowed mode.");
		return;
	}

	bool was_fullscreen = wd.fullscreen;
	wd.was_fullscreen_pre_min = false;

	if (p_mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED && wd.borderless) {
		int cs = window_get_current_screen(p_window);
		Rect2i full = Rect2i(screen_get_position(cs), screen_get_size(cs));
		Rect2i usable = screen_get_usable_rect(cs);
		if (full == usable) {
			p_mode = DisplayServerEnums::WINDOW_MODE_FULLSCREEN;
		}
	}

	if (wd.fullscreen && p_mode != DisplayServerEnums::WINDOW_MODE_FULLSCREEN && p_mode != DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
		RECT rect;

		wd.fullscreen = false;
		wd.multiwindow_fs = false;

		// Restore previous maximized state.
		wd.maximized = wd.was_maximized_pre_fs;

		_update_window_style(p_window, false);

		// Restore window rect after exiting fullscreen.
		if (wd.pre_fs_valid) {
			rect = wd.pre_fs_rect;
		} else {
			rect.left = 0;
			rect.right = wd.width;
			rect.top = 0;
			rect.bottom = wd.height;
		}

		ShowWindow(wd.hWnd, SW_RESTORE);
		MoveWindow(wd.hWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);

		if (restore_mouse_trails > 1) {
			SystemParametersInfoA(SPI_SETMOUSETRAILS, restore_mouse_trails, nullptr, 0);
			restore_mouse_trails = 0;
		}
	}

	if ((wd.maximized || wd.was_maximized_pre_fs) && wd.borderless && p_mode != DisplayServerEnums::WINDOW_MODE_MINIMIZED && p_mode != DisplayServerEnums::WINDOW_MODE_FULLSCREEN && p_mode != DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
		RECT rect;
		if (wd.pre_fs_valid) {
			rect = wd.pre_fs_rect;
		} else {
			rect.left = 0;
			rect.right = wd.width;
			rect.top = 0;
			rect.bottom = wd.height;
		}

		ShowWindow(wd.hWnd, SW_RESTORE);
		MoveWindow(wd.hWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
	}

	if (p_mode == DisplayServerEnums::WINDOW_MODE_WINDOWED) {
		ShowWindow(wd.hWnd, SW_NORMAL);
		wd.maximized = false;
		wd.minimized = false;
	}

	if (p_mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED && !wd.borderless) {
		ShowWindow(wd.hWnd, SW_MAXIMIZE);
		wd.maximized = true;
		wd.minimized = false;
	}

	if (p_mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED && wd.borderless) {
		if (!was_fullscreen && !(wd.maximized && wd.borderless)) {
			// Save non-fullscreen rect before entering fullscreen.
			GetWindowRect(wd.hWnd, &wd.pre_fs_rect);
			wd.pre_fs_valid = true;
		}
		ShowWindow(wd.hWnd, SW_NORMAL);
		wd.maximized = true;
		wd.minimized = false;

		int cs = window_get_current_screen(p_window);
		Rect2i usable = screen_get_usable_rect(cs);
		Point2 pos = usable.position + _get_screens_origin();
		Size2 size = usable.size;
		MoveWindow(wd.hWnd, pos.x, pos.y, size.width, size.height, TRUE);
	}

	if (p_mode == DisplayServerEnums::WINDOW_MODE_MINIMIZED) {
		ShowWindow(wd.hWnd, SW_MINIMIZE);
		wd.maximized = false;
		wd.minimized = true;
		wd.was_fullscreen_pre_min = was_fullscreen;
	}

	if (p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
		wd.multiwindow_fs = false;
	} else if (p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN) {
		wd.multiwindow_fs = true;
	}
	_update_window_style(p_window, false);

	if ((p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) && !wd.fullscreen) {
		if (wd.minimized || wd.maximized) {
			ShowWindow(wd.hWnd, SW_RESTORE);
		}

		// Save previous maximized stare.
		wd.was_maximized_pre_fs = wd.maximized;

		if (!was_fullscreen && !(wd.maximized && wd.borderless)) {
			// Save non-fullscreen rect before entering fullscreen.
			GetWindowRect(wd.hWnd, &wd.pre_fs_rect);
			wd.pre_fs_valid = true;
		}

		int cs = window_get_current_screen(p_window);
		Point2 pos = screen_get_position(cs) + _get_screens_origin();
		Size2 size = screen_get_size(cs);

		wd.fullscreen = true;
		wd.maximized = false;
		wd.minimized = false;

		_update_window_style(p_window, false);

		Vector2i off = (wd.multiwindow_fs || (!wd.fullscreen && wd.borderless && wd.maximized)) ? _get_screen_expand_offset(cs) : Vector2i();
		MoveWindow(wd.hWnd, pos.x, pos.y, size.width + off.x, size.height + off.y, TRUE);

		// If the user has mouse trails enabled in windows, then sometimes the cursor disappears in fullscreen mode.
		// Save number of trails so we can restore when exiting, then turn off mouse trails
		SystemParametersInfoA(SPI_GETMOUSETRAILS, 0, &restore_mouse_trails, 0);
		if (restore_mouse_trails > 1) {
			SystemParametersInfoA(SPI_SETMOUSETRAILS, 0, nullptr, 0);
		}
	}
	_update_window_mouse_passthrough(p_window);
}

DisplayServerEnums::WindowMode DisplayServerWindows::window_get_mode(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), DisplayServerEnums::WINDOW_MODE_WINDOWED);
	const WindowData &wd = windows[p_window];

	if (wd.fullscreen) {
		if (wd.multiwindow_fs) {
			return DisplayServerEnums::WINDOW_MODE_FULLSCREEN;
		} else {
			return DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
		}
	} else if (wd.minimized) {
		return DisplayServerEnums::WINDOW_MODE_MINIMIZED;
	} else if (wd.maximized) {
		return DisplayServerEnums::WINDOW_MODE_MAXIMIZED;
	} else {
		return DisplayServerEnums::WINDOW_MODE_WINDOWED;
	}
}

bool DisplayServerWindows::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), false);
	const WindowData &wd = windows[p_window];

	const DWORD style = GetWindowLongPtr(wd.hWnd, GWL_STYLE);
	return (style & WS_MAXIMIZEBOX) == WS_MAXIMIZEBOX;
}

void DisplayServerWindows::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	switch (p_flag) {
		case DisplayServerEnums::WINDOW_FLAG_MINIMIZE_DISABLED: {
			wd.no_min_btn = p_enabled;
			_update_window_style(p_window);
		} break;
		case DisplayServerEnums::WINDOW_FLAG_MAXIMIZE_DISABLED: {
			wd.no_max_btn = p_enabled;
			_update_window_style(p_window);
		} break;
		case DisplayServerEnums::WINDOW_FLAG_RESIZE_DISABLED: {
			if (p_enabled && wd.parent_hwnd) {
				print_line("Embedded window resize can't be disabled.");
				return;
			}
			wd.resizable = !p_enabled;
			_update_window_style(p_window);
		} break;
		case DisplayServerEnums::WINDOW_FLAG_BORDERLESS: {
			wd.borderless = p_enabled;
			if (wd.fullscreen) {
				return;
			}
			_update_window_mouse_passthrough(p_window);
			_update_window_style(p_window);
			ShowWindow(wd.hWnd, (wd.no_focus || wd.is_popup) ? SW_SHOWNOACTIVATE : SW_SHOW); // Show the window.
		} break;
		case DisplayServerEnums::WINDOW_FLAG_ALWAYS_ON_TOP: {
			ERR_FAIL_COND_MSG(wd.transient_parent != DisplayServerEnums::INVALID_WINDOW_ID && p_enabled, "Transient windows can't become on top.");
			if (p_enabled && wd.parent_hwnd) {
				print_line("Embedded window can't become on top.");
				return;
			}
			wd.always_on_top = p_enabled;
			_update_window_style(p_window);
		} break;
		case DisplayServerEnums::WINDOW_FLAG_SHARP_CORNERS: {
			wd.sharp_corners = p_enabled;
			DWORD value = wd.sharp_corners ? DWMWCP_DONOTROUND : DWMWCP_DEFAULT;
			::DwmSetWindowAttribute(wd.hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value));
			_update_window_style(p_window);
		} break;
		case DisplayServerEnums::WINDOW_FLAG_TRANSPARENT: {
			if (p_enabled) {
				// Enable per-pixel alpha.
				if (OS::is_layered_allowed()) {
					DWM_BLURBEHIND bb;
					ZeroMemory(&bb, sizeof(bb));
					HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
					bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
					bb.hRgnBlur = hRgn;
					bb.fEnable = TRUE;
					DwmEnableBlurBehindWindow(wd.hWnd, &bb);
				}
				wd.layered_window = true;
			} else {
				// Disable per-pixel alpha.
				wd.layered_window = false;
				if (OS::is_layered_allowed()) {
					DWM_BLURBEHIND bb;
					ZeroMemory(&bb, sizeof(bb));
					HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
					bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
					bb.hRgnBlur = hRgn;
					bb.fEnable = FALSE;
					DwmEnableBlurBehindWindow(wd.hWnd, &bb);
				}
			}
		} break;
		case DisplayServerEnums::WINDOW_FLAG_NO_FOCUS: {
			wd.no_focus = p_enabled;
			_update_window_style(p_window);
		} break;
		case DisplayServerEnums::WINDOW_FLAG_MOUSE_PASSTHROUGH: {
			wd.mpass = p_enabled;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_EXCLUDE_FROM_CAPTURE: {
			wd.hide_from_capture = p_enabled;
			if (p_enabled) {
				if (os_ver.dwBuildNumber >= 19041) {
					SetWindowDisplayAffinity(wd.hWnd, WDA_EXCLUDEFROMCAPTURE);
				} else {
					SetWindowDisplayAffinity(wd.hWnd, WDA_MONITOR);
				}
			} else {
				SetWindowDisplayAffinity(wd.hWnd, WDA_NONE);
			}
		} break;
		case DisplayServerEnums::WINDOW_FLAG_POPUP: {
			ERR_FAIL_COND_MSG(p_window == DisplayServerEnums::MAIN_WINDOW_ID, "Main window can't be popup.");
			ERR_FAIL_COND_MSG(IsWindowVisible(wd.hWnd) && (wd.is_popup != p_enabled), "Popup flag can't changed while window is opened.");
			if (p_enabled && wd.parent_hwnd) {
				print_line("Embedded window can't be popup.");
				return;
			}
			wd.is_popup = p_enabled;
		} break;
		default:
			break;
	}
}

bool DisplayServerWindows::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window) const {
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
			return !wd.resizable;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_BORDERLESS: {
			return wd.borderless;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_ALWAYS_ON_TOP: {
			return wd.always_on_top;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_SHARP_CORNERS: {
			return wd.sharp_corners;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_TRANSPARENT: {
			return wd.layered_window;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_NO_FOCUS: {
			return wd.no_focus;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_MOUSE_PASSTHROUGH: {
			return wd.mpass;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_EXCLUDE_FROM_CAPTURE: {
			return wd.hide_from_capture;
		} break;
		case DisplayServerEnums::WINDOW_FLAG_POPUP: {
			return wd.is_popup;
		} break;
		default:
			break;
	}

	return false;
}

void DisplayServerWindows::window_request_attention(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	const WindowData &wd = windows[p_window];

	FLASHWINFO info;
	info.cbSize = sizeof(FLASHWINFO);
	info.hwnd = wd.hWnd;
	info.dwFlags = FLASHW_ALL;
	info.dwTimeout = 0;
	info.uCount = 2;
	FlashWindowEx(&info);
}

void DisplayServerWindows::window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.progress_value = p_value;
	if (wd.progress_state == DisplayServerEnums::PROGRESS_STATE_NOPROGRESS) {
		return;
	}
	if (taskbar == nullptr) {
		if (CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void **)&taskbar) != S_OK) {
			taskbar = nullptr;
			return;
		} else {
			taskbar->HrInit();
		}
	}

	taskbar->SetProgressValue(wd.hWnd, Math::round(p_value * 100000), 100000);
}

void DisplayServerWindows::window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.progress_state = p_state;
	if (taskbar == nullptr) {
		if (CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void **)&taskbar) != S_OK) {
			taskbar = nullptr;
			return;
		} else {
			taskbar->HrInit();
		}
	}

	TBPFLAG tbpf = TBPF_NOPROGRESS;
	switch (p_state) {
		case DisplayServerEnums::PROGRESS_STATE_NOPROGRESS:
			tbpf = TBPF_NOPROGRESS;
			break;
		case DisplayServerEnums::PROGRESS_STATE_INDETERMINATE:
			tbpf = TBPF_INDETERMINATE;
			break;
		case DisplayServerEnums::PROGRESS_STATE_ERROR:
			tbpf = TBPF_ERROR;
			break;
		case DisplayServerEnums::PROGRESS_STATE_PAUSED:
			tbpf = TBPF_PAUSED;
			break;
		case DisplayServerEnums::PROGRESS_STATE_NORMAL:
			tbpf = TBPF_NORMAL;
			break;
		default:
			break;
	}

	taskbar->SetProgressState(wd.hWnd, tbpf);
	if (p_state != DisplayServerEnums::PROGRESS_STATE_INDETERMINATE) {
		taskbar->SetProgressValue(wd.hWnd, Math::round(wd.progress_value * 100000), 100000);
	}
}

void DisplayServerWindows::window_move_to_foreground(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (!wd.no_focus && !wd.is_popup) {
		SetForegroundWindow(wd.hWnd);
	}
}

bool DisplayServerWindows::window_is_focused(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), false);
	const WindowData &wd = windows[p_window];

	return wd.window_focused;
}

DisplayServerEnums::WindowID DisplayServerWindows::get_focused_window() const {
	return last_focused_window;
}

bool DisplayServerWindows::window_can_draw(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), false);
	const WindowData &wd = windows[p_window];
	return !wd.minimized;
}

bool DisplayServerWindows::can_any_window_draw() const {
	_THREAD_SAFE_METHOD_

	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (!E.value.minimized) {
			return true;
		}
	}

	return false;
}

int DisplayServerWindows::accessibility_should_increase_contrast() const {
	HIGHCONTRASTA hc;
	hc.cbSize = sizeof(HIGHCONTRAST);
	if (!SystemParametersInfoA(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0)) {
		return -1;
	}
	return (hc.dwFlags & HCF_HIGHCONTRASTON);
}

int DisplayServerWindows::accessibility_should_reduce_animation() const {
	BOOL anim_enabled = false; // Note: this should be BOOL (WinAPI), not bool (C++), since SystemParametersInfoA expect variable with specific size.
	if (!SystemParametersInfoA(SPI_GETCLIENTAREAANIMATION, 0, &anim_enabled, 0)) {
		return -1;
	}
	return (!anim_enabled);
}

int DisplayServerWindows::accessibility_should_reduce_transparency() const {
	BOOL tr_enabled = false; // Note: this should be BOOL (WinAPI), not bool (C++), since SystemParametersInfoA expect variable with specific size.
	if (!SystemParametersInfoA(SPI_GETDISABLEOVERLAPPEDCONTENT, 0, &tr_enabled, 0)) {
		return -1;
	}
	return tr_enabled;
}

int DisplayServerWindows::accessibility_screen_reader_active() const {
	BOOL sr_enabled = false; // Note: this should be BOOL (WinAPI), not bool (C++), since SystemParametersInfoA expect variable with specific size.
	if (SystemParametersInfoA(SPI_GETSCREENREADER, 0, &sr_enabled, 0) && sr_enabled) {
		return true;
	}

	static const WCHAR *narrator_mutex_name = L"NarratorRunning";
	HANDLE narrator_mutex = OpenMutexW(MUTEX_ALL_ACCESS, false, narrator_mutex_name);
	if (narrator_mutex) {
		CloseHandle(narrator_mutex);
		return true;
	}
	return false;
}

Vector2i DisplayServerWindows::ime_get_selection() const {
	_THREAD_SAFE_METHOD_

	DisplayServerEnums::WindowID window_id = _get_focused_window_or_popup();
	const WindowData &wd = windows[window_id];
	if (!wd.ime_active) {
		return Vector2i();
	}

	int cursor = ImmGetCompositionStringW(wd.im_himc, GCS_CURSORPOS, nullptr, 0);

	int32_t length = ImmGetCompositionStringW(wd.im_himc, GCS_COMPSTR, nullptr, 0);
	wchar_t *string = reinterpret_cast<wchar_t *>(memalloc(length));
	ImmGetCompositionStringW(wd.im_himc, GCS_COMPSTR, string, length);

	int32_t utf32_cursor = 0;
	for (int32_t i = 0; i < length / int32_t(sizeof(wchar_t)); i++) {
		if ((string[i] & 0xfffffc00) == 0xd800) {
			i++;
		}
		if (i < cursor) {
			utf32_cursor++;
		} else {
			break;
		}
	}

	memdelete(string);

	return Vector2i(utf32_cursor, 0);
}

String DisplayServerWindows::ime_get_text() const {
	_THREAD_SAFE_METHOD_

	DisplayServerEnums::WindowID window_id = _get_focused_window_or_popup();
	const WindowData &wd = windows[window_id];
	if (!wd.ime_active) {
		return String();
	}

	String ret;
	int32_t length = ImmGetCompositionStringW(wd.im_himc, GCS_COMPSTR, nullptr, 0);
	wchar_t *string = reinterpret_cast<wchar_t *>(memalloc(length));
	ImmGetCompositionStringW(wd.im_himc, GCS_COMPSTR, string, length);
	ret.append_utf16((char16_t *)string, length / sizeof(wchar_t));

	memdelete(string);

	return ret;
}

void DisplayServerWindows::window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (p_active) {
		wd.ime_active = true;
		ImmAssociateContext(wd.hWnd, wd.im_himc);
		CreateCaret(wd.hWnd, nullptr, 1, 1);
		window_set_ime_position(wd.im_position, p_window);
	} else {
		ImmAssociateContext(wd.hWnd, (HIMC) nullptr);
		DestroyCaret();
		wd.ime_active = false;
	}
}

void DisplayServerWindows::window_set_ime_position(const Point2i &p_pos, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	wd.im_position = p_pos;

	HIMC himc = ImmGetContext(wd.hWnd);
	if (himc == (HIMC) nullptr) {
		return;
	}

	update_ime_form_positions(himc, wd.im_position);
	ImmReleaseContext(wd.hWnd, himc);
}

void DisplayServerWindows::cursor_set_shape(DisplayServerEnums::CursorShape p_shape) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);

	if (cursor_shape == p_shape) {
		return;
	}

	if (mouse_mode != DisplayServerEnums::MOUSE_MODE_VISIBLE && mouse_mode != DisplayServerEnums::MOUSE_MODE_CONFINED) {
		cursor_shape = p_shape;
		return;
	}

	static const LPCTSTR win_cursors[DisplayServerEnums::CURSOR_MAX] = {
		IDC_ARROW,
		IDC_IBEAM,
		IDC_HAND, // Finger.
		IDC_CROSS,
		IDC_WAIT,
		IDC_APPSTARTING,
		IDC_SIZEALL,
		IDC_ARROW,
		IDC_NO,
		IDC_SIZENS,
		IDC_SIZEWE,
		IDC_SIZENESW,
		IDC_SIZENWSE,
		IDC_SIZEALL,
		IDC_SIZENS,
		IDC_SIZEWE,
		IDC_HELP
	};

	if (cursors_cache.has(p_shape)) {
		SetCursor(cursors[p_shape]);
	} else {
		SetCursor(LoadCursor(hInstance, win_cursors[p_shape]));
	}

	cursor_shape = p_shape;
}

DisplayServerEnums::CursorShape DisplayServerWindows::cursor_get_shape() const {
	return cursor_shape;
}

void DisplayServerWindows::cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape, const Vector2 &p_hotspot) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);

	if (p_cursor.is_valid()) {
		RBMap<DisplayServerEnums::CursorShape, Vector<Variant>>::Element *cursor_c = cursors_cache.find(p_shape);

		if (cursor_c) {
			if (cursor_c->get()[0] == p_cursor && cursor_c->get()[1] == p_hotspot) {
				cursor_set_shape(p_shape);
				return;
			}

			cursors_cache.erase(p_shape);
		}

		Ref<Image> image = _get_cursor_image_from_resource(p_cursor, p_hotspot);
		ERR_FAIL_COND(image.is_null());
		Vector2i texture_size = image->get_size();

		UINT image_size = texture_size.width * texture_size.height;

		// Create the BITMAP with alpha channel.
		COLORREF *buffer = nullptr;

		BITMAPV5HEADER bi;
		ZeroMemory(&bi, sizeof(bi));
		bi.bV5Size = sizeof(bi);
		bi.bV5Width = texture_size.width;
		bi.bV5Height = -texture_size.height;
		bi.bV5Planes = 1;
		bi.bV5BitCount = 32;
		bi.bV5Compression = BI_BITFIELDS;
		bi.bV5RedMask = 0x00ff0000;
		bi.bV5GreenMask = 0x0000ff00;
		bi.bV5BlueMask = 0x000000ff;
		bi.bV5AlphaMask = 0xff000000;

		HDC dc = GetDC(nullptr);
		HBITMAP bitmap = CreateDIBSection(dc, reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS, reinterpret_cast<void **>(&buffer), nullptr, 0);
		HBITMAP mask = CreateBitmap(texture_size.width, texture_size.height, 1, 1, nullptr);

		bool fully_transparent = true;
		for (UINT index = 0; index < image_size; index++) {
			int row_index = std::floor(index / texture_size.width);
			int column_index = index % int(texture_size.width);

			const Color &c = image->get_pixel(column_index, row_index);
			fully_transparent = fully_transparent && (c.a == 0.f);

			*(buffer + index) = c.to_argb32();
		}

		// Finally, create the icon.
		if (cursors[p_shape]) {
			DestroyIcon(cursors[p_shape]);
		}

		if (fully_transparent) {
			cursors[p_shape] = nullptr;
		} else {
			ICONINFO iconinfo;
			iconinfo.fIcon = FALSE;
			iconinfo.xHotspot = p_hotspot.x;
			iconinfo.yHotspot = p_hotspot.y;
			iconinfo.hbmMask = mask;
			iconinfo.hbmColor = bitmap;
			cursors[p_shape] = CreateIconIndirect(&iconinfo);
		}

		Vector<Variant> params;
		params.push_back(p_cursor);
		params.push_back(p_hotspot);
		cursors_cache.insert(p_shape, params);

		if (p_shape == cursor_shape) {
			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_VISIBLE || mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED) {
				SetCursor(cursors[p_shape]);
			}
		}

		DeleteObject(mask);
		DeleteObject(bitmap);
		ReleaseDC(nullptr, dc);
	} else {
		// Reset to default system cursor.
		if (cursors[p_shape]) {
			DestroyIcon(cursors[p_shape]);
		}
		cursors[p_shape] = nullptr;

		cursors_cache.erase(p_shape);

		DisplayServerEnums::CursorShape c = cursor_shape;
		cursor_shape = DisplayServerEnums::CURSOR_MAX;
		cursor_set_shape(c);
	}
}

bool DisplayServerWindows::get_swap_cancel_ok() {
	return true;
}

void DisplayServerWindows::enable_for_stealing_focus(ProcessID pid) {
	_THREAD_SAFE_METHOD_

	AllowSetForegroundWindow(pid);
}

struct WindowEnumData {
	DWORD process_id;
	HWND parent_hWnd;
	HWND hWnd;
};

static BOOL CALLBACK _enum_proc_find_window_from_process_id_callback(HWND hWnd, LPARAM lParam) {
	WindowEnumData &ed = *(WindowEnumData *)lParam;
	DWORD process_id = 0x0;

	GetWindowThreadProcessId(hWnd, &process_id);
	if (ed.process_id == process_id) {
		if (GetParent(hWnd) != ed.parent_hWnd) {
			return TRUE;
		}

		// Found it.
		ed.hWnd = hWnd;
		SetLastError(ERROR_SUCCESS);
		return FALSE;
	}
	// Continue enumeration.
	return TRUE;
}

HWND DisplayServerWindows::_find_window_from_process_id(ProcessID p_pid, HWND p_current_hwnd) {
	DWORD pid = p_pid;
	WindowEnumData ed = { pid, p_current_hwnd, nullptr };

	// First, check our own child, maybe it's already embedded.
	if (!EnumChildWindows(p_current_hwnd, _enum_proc_find_window_from_process_id_callback, (LPARAM)&ed) && (GetLastError() == ERROR_SUCCESS)) {
		if (ed.hWnd) {
			return ed.hWnd;
		}
	}

	// Then check all the opened windows on the computer.
	if (!EnumWindows(_enum_proc_find_window_from_process_id_callback, (LPARAM)&ed) && (GetLastError() == ERROR_SUCCESS)) {
		return ed.hWnd;
	}

	return nullptr;
}

// Get screen HDR capabilities for internal use only.
DisplayServerWindows::ScreenHdrData DisplayServerWindows::_get_screen_hdr_data(DisplayServerEnums::WindowID p_window, bool p_include_sdr_white_level) const {
	ScreenHdrData data;
	const WindowData &wd = windows[p_window];
	if (WinRTUtils::window_has_display_info(wd.wrt_wd)) {
		WinRTUtils::window_get_advanced_color_info(wd.wrt_wd, data.hdr_supported, data.min_luminance, data.max_luminance, data.max_average_luminance, data.sdr_white_level);
	} else {
		int screen = window_get_current_screen(p_window);
		HMONITOR monitor = _get_hmonitor_of_screen(screen);
		if (!monitor) {
			return data;
		}

#ifdef D3D12_ENABLED
		// A dynamic cast is used here because the rendering context is not an Object and Object:cast is not supported.
		RenderingContextDriverD3D12 *rendering_context_d3d12 = dynamic_cast<RenderingContextDriverD3D12 *>(rendering_context);
		if (rendering_context_d3d12) {
			IDXGIFactory2 *dxgi_factory = rendering_context_d3d12->dxgi_factory_get();

			DXGI_OUTPUT_DESC1 desc;
			if (_get_monitor_desc(monitor, dxgi_factory, desc)) {
				data.hdr_supported = desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
				data.min_luminance = desc.MinLuminance;
				data.max_luminance = desc.MaxLuminance;
				data.max_average_luminance = desc.MaxFullFrameLuminance;
			}
		}
#endif // D3D12_ENABLED

		if (p_include_sdr_white_level) {
			uint32_t path_count = 0;
			uint32_t mode_count = 0;

			if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count, &mode_count) == ERROR_SUCCESS) {
				LocalVector<DISPLAYCONFIG_PATH_INFO> paths;
				LocalVector<DISPLAYCONFIG_MODE_INFO> modes;
				paths.resize(path_count);
				modes.resize(mode_count);

				if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_count, paths.ptr(), &mode_count, modes.ptr(), nullptr) == ERROR_SUCCESS) {
					data.sdr_white_level = _get_sdr_white_level_for_hmonitor(monitor, paths);
				}
			}
		}
	}

	return data;
}

void DisplayServerWindows::_winrt_adv_color_info_cb(DisplayServerEnums::WindowID p_id) {
	if (!windows.has(p_id)) {
		return;
	}
	WindowData &wd = windows[p_id];

	DisplayServerWindows::ScreenHdrData data = _get_screen_hdr_data(p_id, true);
	_update_hdr_output_for_window(p_id, wd, data);
}

void DisplayServerWindows::_update_hdr_output_for_window(DisplayServerEnums::WindowID p_window, const WindowData &p_window_data, ScreenHdrData p_screen_data) {
#ifdef RD_ENABLED
	if (rendering_context) {
		bool current_hdr_enabled = rendering_context->window_get_hdr_output_enabled(p_window);
		bool desired_hdr_enabled = p_window_data.hdr_output_requested && p_screen_data.hdr_supported;
		bool hdr_state_changed = false;

		if (current_hdr_enabled != desired_hdr_enabled) {
			rendering_context->window_set_hdr_output_enabled(p_window, desired_hdr_enabled);
			rendering_context->window_set_hdr_output_linear_luminance_scale(p_window, 80.0f);
			hdr_state_changed = true;
		}

		// If auto reference luminance is enabled, update it based on the current SDR white level.
		if (p_window_data.hdr_output_reference_luminance < 0.0f) {
			if (p_screen_data.sdr_white_level > 0.0f) {
				float current_ref_luminance = rendering_context->window_get_hdr_output_reference_luminance(p_window);
				if (!Math::is_equal_approx(current_ref_luminance, p_screen_data.sdr_white_level)) {
					rendering_context->window_set_hdr_output_reference_luminance(p_window, p_screen_data.sdr_white_level);
					hdr_state_changed = true;
				}
			}
			// If we cannot get the SDR white level, leave the previous value unchanged.
		}

		// If auto max luminance is enabled, update it based on the screen's max luminance.
		if (p_window_data.hdr_output_max_luminance < 0.0f) {
			if (p_screen_data.max_luminance > 0.0f) {
				float current_max_luminance = rendering_context->window_get_hdr_output_max_luminance(p_window);
				if (!Math::is_equal_approx(current_max_luminance, p_screen_data.max_luminance)) {
					rendering_context->window_set_hdr_output_max_luminance(p_window, p_screen_data.max_luminance);
					hdr_state_changed = true;
				}
			}
			// If we cannot get the screen's max luminance, leave the previous value unchanged.
		}

		// Trigger HDR output changed event if any HDR parameter was modified.
		if (hdr_state_changed) {
			_send_window_event(p_window_data, DisplayServerEnums::WINDOW_EVENT_OUTPUT_MAX_LINEAR_VALUE_CHANGED);
		}
	}
#endif // RD_ENABLED
}

void DisplayServerWindows::_legacy_update_hdr_output_for_tracked_windows(bool p_include_sdr_white_level) {
	hdr_output_cache.clear();
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (WinRTUtils::window_has_display_info(E.value.wrt_wd)) {
			continue; // Updated by "_winrt_adv_color_info_cb" callback.
		}
		if (E.value.hdr_output_requested) {
			int screen = window_get_current_screen(E.key);

			ScreenHdrData data;
			if (!hdr_output_cache.has(screen)) {
				data = _get_screen_hdr_data(E.key, p_include_sdr_white_level);
				hdr_output_cache.insert(screen, data);
			} else {
				data = hdr_output_cache[screen];
			}

			_update_hdr_output_for_window(E.key, E.value, data);
		}
	}
}

Error DisplayServerWindows::embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid, const Rect2i &p_rect, bool p_visible, bool p_grab_focus) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), FAILED);

	const WindowData &wd = windows[p_window];

	EmbeddedProcessData *ep = nullptr;
	if (embedded_processes.has(p_pid)) {
		ep = embedded_processes.get(p_pid);
	} else {
		// New process, trying to find the window.
		HWND handle_to_embed = _find_window_from_process_id(p_pid, wd.hWnd);
		if (!handle_to_embed) {
			return ERR_DOES_NOT_EXIST;
		}

		const DWORD style = GetWindowLongPtr(handle_to_embed, GWL_STYLE);

		ep = memnew(EmbeddedProcessData);
		ep->window_handle = handle_to_embed;
		ep->parent_window_handle = wd.hWnd;
		ep->is_visible = (style & WS_VISIBLE) == WS_VISIBLE;

		embedded_processes.insert(p_pid, ep);
	}

	if (p_rect.size.x <= 100 || p_rect.size.y <= 100) {
		p_visible = false;
	}

	// In Godot, the window position is offset by the screen's origin coordinates.
	// We need to adjust for this when a screen is positioned in the negative space
	// (e.g., a screen to the left of the main screen).
	const Rect2i adjusted_rect = Rect2i(p_rect.position + _get_screens_origin(), p_rect.size);

	// Use HWND_BOTTOM to prevent reordering of the embedded window over another popup.
	SetWindowPos(ep->window_handle, HWND_BOTTOM, adjusted_rect.position.x, adjusted_rect.position.y, adjusted_rect.size.x, adjusted_rect.size.y, SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);

	if (ep->is_visible != p_visible) {
		if (p_visible) {
			ShowWindow(ep->window_handle, SW_SHOWNA);
		} else {
			ShowWindow(ep->window_handle, SW_HIDE);
		}
		ep->is_visible = p_visible;
	}

	if (p_grab_focus) {
		SetForegroundWindow(ep->window_handle);
		SetFocus(ep->window_handle);
	}

	return OK;
}

Error DisplayServerWindows::request_close_embedded_process(ProcessID p_pid) {
	_THREAD_SAFE_METHOD_

	if (!embedded_processes.has(p_pid)) {
		return ERR_DOES_NOT_EXIST;
	}

	EmbeddedProcessData *ep = embedded_processes.get(p_pid);

	// Send a close message to gracefully close the process.
	PostMessage(ep->window_handle, WM_CLOSE, 0, 0);

	return OK;
}

Error DisplayServerWindows::remove_embedded_process(ProcessID p_pid) {
	_THREAD_SAFE_METHOD_

	if (!embedded_processes.has(p_pid)) {
		return ERR_DOES_NOT_EXIST;
	}

	EmbeddedProcessData *ep = embedded_processes.get(p_pid);

	request_close_embedded_process(p_pid);

	// This is a workaround to ensure the parent window correctly regains focus after the
	// embedded window is closed. When the embedded window is closed while it has focus,
	// the parent window (the editor) does not become active. It appears focused but is not truly activated.
	// Opening a new window and closing it forces Windows to set the focus and activation correctly.
	DWORD style = WS_POPUP | WS_VISIBLE;
	DWORD style_ex = WS_EX_TOPMOST;

	WNDCLASSW wcTemp = {};
	wcTemp.lpfnWndProc = DefWindowProcW;
	wcTemp.hInstance = GetModuleHandle(nullptr);
	wcTemp.lpszClassName = L"Engine temp window";
	RegisterClassW(&wcTemp);

	HWND hWnd = CreateWindowExW(
			style_ex,
			L"Engine temp window", L"",
			style,
			0,
			0,
			1,
			1,
			ep->parent_window_handle,
			nullptr,
			GetModuleHandle(nullptr),
			nullptr);

	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);

	DestroyWindow(hWnd);
	UnregisterClassW(L"Engine temp window", GetModuleHandle(nullptr));

	SetForegroundWindow(ep->parent_window_handle);

	embedded_processes.erase(p_pid);
	memdelete(ep);

	return OK;
}

ProcessID DisplayServerWindows::get_focused_process_id() {
	HWND hwnd = GetForegroundWindow();
	if (!hwnd) {
		return 0;
	}

	// Get the process ID of the window.
	DWORD processID;
	GetWindowThreadProcessId(hwnd, &processID);

	return processID;
}

static HRESULT CALLBACK win32_task_dialog_callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData) {
	if (msg == TDN_CREATED) {
		// To match the input text dialog.
		SendMessageW(hwnd, WM_SETICON, ICON_BIG, 0);
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, 0);
	}

	return 0;
}

Error DisplayServerWindows::dialog_show(String p_title, String p_description, Vector<String> p_buttons, const Callable &p_callback) {
	_THREAD_SAFE_METHOD_

	TASKDIALOGCONFIG config;
	ZeroMemory(&config, sizeof(TASKDIALOGCONFIG));
	config.cbSize = sizeof(TASKDIALOGCONFIG);

	Char16String title = p_title.utf16();
	Char16String message = p_description.utf16();
	LocalVector<Char16String> buttons;
	for (String s : p_buttons) {
		buttons.push_back(s.utf16());
	}

	DisplayServerEnums::WindowID window_id = _get_focused_window_or_popup();
	if (!windows.has(window_id)) {
		window_id = DisplayServerEnums::MAIN_WINDOW_ID;
	}

	config.pszWindowTitle = (LPCWSTR)(title.get_data());
	config.pszContent = (LPCWSTR)(message.get_data());
	config.hwndParent = windows[window_id].hWnd;

	const int button_count = buttons.size();
	config.cButtons = button_count;

	// No dynamic stack array size :(
	TASKDIALOG_BUTTON *tbuttons = button_count != 0 ? (TASKDIALOG_BUTTON *)alloca(sizeof(TASKDIALOG_BUTTON) * button_count) : nullptr;
	if (tbuttons) {
		for (int i = 0; i < button_count; i++) {
			tbuttons[i].nButtonID = i + 100;
			tbuttons[i].pszButtonText = (LPCWSTR)(buttons[i].get_data());
		}
	}
	config.pButtons = tbuttons;
	config.pfCallback = win32_task_dialog_callback;

	Error result = FAILED;
	HMODULE comctl = LoadLibraryW(L"comctl32.dll");
	if (comctl) {
		typedef HRESULT(WINAPI * TaskDialogIndirectPtr)(const TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, BOOL *pfVerificationFlagChecked);

		TaskDialogIndirectPtr task_dialog_indirect = (TaskDialogIndirectPtr)(void *)GetProcAddress(comctl, "TaskDialogIndirect");
		int button_pressed;

		if (task_dialog_indirect && SUCCEEDED(task_dialog_indirect(&config, &button_pressed, nullptr, nullptr))) {
			if (p_callback.is_valid()) {
				Variant button = button_pressed - 100;
				const Variant *args[1] = { &button };
				Variant ret;
				Callable::CallError ce;
				p_callback.callp(args, 1, ret, ce);
				if (ce.error != Callable::CallError::CALL_OK) {
					ERR_PRINT(vformat("Failed to execute dialog callback: %s.", Variant::get_callable_error_text(p_callback, args, 1, ce)));
				}
			}

			result = OK;
		}
		FreeLibrary(comctl);
	} else {
		ERR_PRINT("Unable to create native dialog.");
	}

	return result;
}

struct Win32InputTextDialogInit {
	const char16_t *title;
	const char16_t *description;
	const char16_t *partial;
	const Callable &callback;
};

static int scale_with_dpi(int p_pos, int p_dpi) {
	return IsProcessDPIAware() ? (p_pos * p_dpi / 96) : p_pos;
}

static INT_PTR input_text_dialog_init(HWND hWnd, UINT code, WPARAM wParam, LPARAM lParam) {
	Win32InputTextDialogInit init = *(Win32InputTextDialogInit *)lParam;
	SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)&init.callback); // Set dialog callback.

	SetWindowTextW(hWnd, (LPCWSTR)init.title);

	const int dpi = DisplayServer::screen_get_dpi();

	const int margin = scale_with_dpi(7, dpi);
	const SIZE dlg_size = { scale_with_dpi(300, dpi), scale_with_dpi(50, dpi) };

	int str_len = lstrlenW((LPCWSTR)init.description);
	SIZE str_size = { dlg_size.cx, 0 };
	if (str_len > 0) {
		HDC hdc = GetDC(nullptr);
		RECT trect = { margin, margin, margin + dlg_size.cx, margin + dlg_size.cy };
		SelectObject(hdc, (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0));

		// `+ margin` adds some space between the static text and the edit field.
		// Don't scale this with DPI because DPI is already handled by DrawText.
		str_size.cy = DrawTextW(hdc, (LPCWSTR)init.description, str_len, &trect, DT_LEFT | DT_WORDBREAK | DT_CALCRECT) + margin;

		ReleaseDC(nullptr, hdc);
	}

	RECT crect, wrect;
	GetClientRect(hWnd, &crect);
	GetWindowRect(hWnd, &wrect);
	int sw = GetSystemMetrics(SM_CXSCREEN);
	int sh = GetSystemMetrics(SM_CYSCREEN);
	int new_width = dlg_size.cx + margin * 2 + wrect.right - wrect.left - crect.right;
	int new_height = dlg_size.cy + margin * 2 + wrect.bottom - wrect.top - crect.bottom + str_size.cy;

	MoveWindow(hWnd, (sw - new_width) / 2, (sh - new_height) / 2, new_width, new_height, true);

	HWND ok_button = GetDlgItem(hWnd, 1);
	MoveWindow(ok_button,
			dlg_size.cx + margin - scale_with_dpi(65, dpi),
			dlg_size.cy + str_size.cy + margin - scale_with_dpi(20, dpi),
			scale_with_dpi(65, dpi), scale_with_dpi(20, dpi), true);

	HWND description = GetDlgItem(hWnd, 3);
	MoveWindow(description, margin, margin, dlg_size.cx, str_size.cy, true);
	SetWindowTextW(description, (LPCWSTR)init.description);

	HWND text_edit = GetDlgItem(hWnd, 2);
	MoveWindow(text_edit, margin, str_size.cy + margin, dlg_size.cx, scale_with_dpi(20, dpi), true);
	SetWindowTextW(text_edit, (LPCWSTR)init.partial);

	return TRUE;
}

static INT_PTR input_text_dialog_cmd_proc(HWND hWnd, UINT code, WPARAM wParam, LPARAM lParam) {
	if (LOWORD(wParam) == 1) {
		HWND text_edit = GetDlgItem(hWnd, 2);
		ERR_FAIL_NULL_V(text_edit, false);

		Char16String text;
		text.resize_uninitialized(GetWindowTextLengthW(text_edit) + 1);
		GetWindowTextW(text_edit, (LPWSTR)text.get_data(), text.size());

		const Callable *callback = (const Callable *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
		if (callback && callback->is_valid()) {
			Variant v_result = String((const wchar_t *)text.get_data());
			Variant ret;
			Callable::CallError ce;
			const Variant *args[1] = { &v_result };

			callback->callp(args, 1, ret, ce);
			if (ce.error != Callable::CallError::CALL_OK) {
				ERR_PRINT(vformat("Failed to execute input dialog callback: %s.", Variant::get_callable_error_text(*callback, args, 1, ce)));
			}
		}

		return EndDialog(hWnd, 0);
	}

	return false;
}

static INT_PTR CALLBACK input_text_dialog_proc(HWND hWnd, UINT code, WPARAM wParam, LPARAM lParam) {
	switch (code) {
		case WM_INITDIALOG:
			return input_text_dialog_init(hWnd, code, wParam, lParam);

		case WM_COMMAND:
			return input_text_dialog_cmd_proc(hWnd, code, wParam, lParam);

		default:
			return FALSE;
	}
}

Error DisplayServerWindows::dialog_input_text(String p_title, String p_description, String p_partial, const Callable &p_callback) {
#pragma pack(push, 1)

	// NOTE: Use default/placeholder coordinates here. Windows uses its own coordinate system
	//       specifically for dialogs which relies on font sizes instead of pixels.
	const struct {
		WORD dlgVer; // must be 1
		WORD signature; // must be 0xFFFF
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		WORD cDlgItems;
		short x;
		short y;
		short cx;
		short cy;
		WCHAR menu[1]; // must be 0
		WCHAR windowClass[7]; // must be "#32770" -- the default window class for dialogs
		WCHAR title[1]; // must be 0
		WORD pointsize;
		WORD weight;
		BYTE italic;
		BYTE charset;
		WCHAR font[13]; // must be "MS Shell Dlg"
	} template_base = {
		1, 0xFFFF, 0, 0,
		DS_SYSMODAL | DS_SETFONT | DS_MODALFRAME | DS_3DLOOK | DS_FIXEDSYS | DS_CENTER | WS_POPUP | WS_CAPTION,
		3, 0, 0, 20, 20, L"", L"#32770", L"", 8, FW_NORMAL, 0, DEFAULT_CHARSET, L"MS Shell Dlg"
	};

	const struct {
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		short x;
		short y;
		short cx;
		short cy;
		DWORD id;
		WCHAR windowClass[7]; // must be "Button"
		WCHAR title[3]; // must be "OK"
		WORD extraCount;
	} ok_button = {
		0, 0, WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 50, 14, 1, WC_BUTTONW, L"OK", 0
	};
	const struct {
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		short x;
		short y;
		short cx;
		short cy;
		DWORD id;
		WCHAR windowClass[5]; // must be "Edit"
		WCHAR title[1]; // must be 0
		WORD extraCount;
	} text_field = {
		0, 0, WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 0, 0, 250, 14, 2, WC_EDITW, L"", 0
	};
	const struct {
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		short x;
		short y;
		short cx;
		short cy;
		DWORD id;
		WCHAR windowClass[7]; // must be "Static"
		WCHAR title[1]; // must be 0
		WORD extraCount;
	} static_text = {
		0, 0, WS_VISIBLE, 0, 0, 250, 14, 3, WC_STATICW, L"", 0
	};

#pragma pack(pop)

	// Dialog template
	const size_t data_size = sizeof(template_base) + (sizeof(template_base) % 4) +
			sizeof(ok_button) + (sizeof(ok_button) % 4) +
			sizeof(text_field) + (sizeof(text_field) % 4) +
			sizeof(static_text) + (sizeof(static_text) % 4);

	void *data_template = memalloc(data_size);
	ERR_FAIL_NULL_V_MSG(data_template, FAILED, "Unable to allocate memory for the dialog template.");
	ZeroMemory(data_template, data_size);

	char *current_block = (char *)data_template;
	CopyMemory(current_block, &template_base, sizeof(template_base));
	current_block += sizeof(template_base) + (sizeof(template_base) % 4);
	CopyMemory(current_block, &ok_button, sizeof(ok_button));
	current_block += sizeof(ok_button) + (sizeof(ok_button) % 4);
	CopyMemory(current_block, &text_field, sizeof(text_field));
	current_block += sizeof(text_field) + (sizeof(text_field) % 4);
	CopyMemory(current_block, &static_text, sizeof(static_text));

	Char16String title16 = p_title.utf16();
	Char16String description16 = p_description.utf16();
	Char16String partial16 = p_partial.utf16();

	Win32InputTextDialogInit init = {
		title16.get_data(), description16.get_data(), partial16.get_data(), p_callback
	};

	// No modal dialogs for specific windows? Assume main window here.
	INT_PTR ret = DialogBoxIndirectParamW(hInstance, (LPDLGTEMPLATEW)data_template, nullptr, (DLGPROC)input_text_dialog_proc, (LPARAM)(&init));

	Error result = ret != -1 ? OK : FAILED;
	memfree(data_template);

	if (result == FAILED) {
		ERR_PRINT("Unable to create native dialog.");
	}
	return result;
}

int DisplayServerWindows::keyboard_get_layout_count() const {
	return GetKeyboardLayoutList(0, nullptr);
}

int DisplayServerWindows::keyboard_get_current_layout() const {
	HKL cur_layout = GetKeyboardLayout(0);

	int layout_count = GetKeyboardLayoutList(0, nullptr);
	HKL *layouts = (HKL *)memalloc(layout_count * sizeof(HKL));
	GetKeyboardLayoutList(layout_count, layouts);

	for (int i = 0; i < layout_count; i++) {
		if (cur_layout == layouts[i]) {
			memfree(layouts);
			return i;
		}
	}
	memfree(layouts);
	return -1;
}

void DisplayServerWindows::keyboard_set_current_layout(int p_index) {
	int layout_count = GetKeyboardLayoutList(0, nullptr);

	ERR_FAIL_INDEX(p_index, layout_count);

	HKL *layouts = (HKL *)memalloc(layout_count * sizeof(HKL));
	GetKeyboardLayoutList(layout_count, layouts);
	ActivateKeyboardLayout(layouts[p_index], KLF_SETFORPROCESS);
	memfree(layouts);
}

String DisplayServerWindows::keyboard_get_layout_language(int p_index) const {
	int layout_count = GetKeyboardLayoutList(0, nullptr);

	ERR_FAIL_INDEX_V(p_index, layout_count, "");

	HKL *layouts = (HKL *)memalloc(layout_count * sizeof(HKL));
	GetKeyboardLayoutList(layout_count, layouts);

	WCHAR buf[LOCALE_NAME_MAX_LENGTH];
	memset(buf, 0, LOCALE_NAME_MAX_LENGTH * sizeof(WCHAR));
	LCIDToLocaleName(MAKELCID(LOWORD(layouts[p_index]), SORT_DEFAULT), buf, LOCALE_NAME_MAX_LENGTH, 0);

	memfree(layouts);

	return String::utf16((const char16_t *)buf).substr(0, 2);
}

Key DisplayServerWindows::keyboard_get_keycode_from_physical(Key p_keycode) const {
	Key modifiers = p_keycode & KeyModifierMask::MODIFIER_MASK;
	Key keycode_no_mod = (Key)(p_keycode & KeyModifierMask::CODE_MASK);

	if (keycode_no_mod == Key::PRINT ||
			keycode_no_mod == Key::KP_ADD ||
			keycode_no_mod == Key::KP_5 ||
			(keycode_no_mod >= Key::KEY_0 && keycode_no_mod <= Key::KEY_9)) {
		return p_keycode;
	}

	unsigned int scancode = KeyMappingWindows::get_scancode(keycode_no_mod);
	if (scancode == 0) {
		return p_keycode;
	}

	HKL current_layout = GetKeyboardLayout(0);
	UINT vk = MapVirtualKeyEx(scancode, MAPVK_VSC_TO_VK, current_layout);
	if (vk == 0) {
		return p_keycode;
	}

	UINT char_code = MapVirtualKeyEx(vk, MAPVK_VK_TO_CHAR, current_layout) & 0x7FFF;
	// Unlike a similar Linux/BSD check which matches full Latin-1 range,
	// we limit these to ASCII to fix some layouts, including Arabic ones
	if (char_code >= 32 && char_code <= 127) {
		// Godot uses 'braces' instead of 'brackets'
		if (char_code == (unsigned int)Key::BRACKETLEFT || char_code == (unsigned int)Key::BRACKETRIGHT) {
			char_code += 32;
		}
		return (Key)(char_code | (unsigned int)modifiers);
	}

	return (Key)(KeyMappingWindows::get_keysym(vk) | modifiers);
}

Key DisplayServerWindows::keyboard_get_label_from_physical(Key p_keycode) const {
	Key modifiers = p_keycode & KeyModifierMask::MODIFIER_MASK;
	Key keycode_no_mod = (Key)(p_keycode & KeyModifierMask::CODE_MASK);

	if (keycode_no_mod == Key::PRINT ||
			keycode_no_mod == Key::KP_ADD ||
			keycode_no_mod == Key::KP_5 ||
			(keycode_no_mod >= Key::KEY_0 && keycode_no_mod <= Key::KEY_9)) {
		return p_keycode;
	}

	unsigned int scancode = KeyMappingWindows::get_scancode(keycode_no_mod);
	if (scancode == 0) {
		return p_keycode;
	}

	Key keycode = KeyMappingWindows::get_keysym(MapVirtualKey(scancode, MAPVK_VSC_TO_VK));

	HKL current_layout = GetKeyboardLayout(0);
	static BYTE keyboard_state[256];
	memset(keyboard_state, 0, 256);
	wchar_t chars[256] = {};
	UINT extended_code = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
	if (ToUnicodeEx(extended_code, scancode, keyboard_state, chars, 255, 4, current_layout) > 0) {
		String keysym = String::utf16((char16_t *)chars, 255);
		if (!keysym.is_empty()) {
			return fix_key_label(keysym[0], keycode) | modifiers;
		}
	}
	return p_keycode;
}

void DisplayServerWindows::show_emoji_and_symbol_picker() const {
	if (!WinRTUtils::try_show_onecore_emoji_picker()) {
		// Send Win + Period shortcut.

		INPUT input[4] = {};
		input[0].type = INPUT_KEYBOARD; // Win down.
		input[0].ki.wVk = VK_LWIN;

		input[1].type = INPUT_KEYBOARD; // Period down.
		input[1].ki.wVk = VK_OEM_PERIOD;

		input[2].type = INPUT_KEYBOARD; // Win up.
		input[2].ki.wVk = VK_LWIN;
		input[2].ki.dwFlags = KEYEVENTF_KEYUP;

		input[3].type = INPUT_KEYBOARD; // Period up.
		input[3].ki.wVk = VK_OEM_PERIOD;
		input[3].ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(4, input, sizeof(INPUT));
	}
}

String DisplayServerWindows::_get_keyboard_layout_display_name(const String &p_klid) const {
	String ret;
	HKEY key;
	if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts", &key) != ERROR_SUCCESS) {
		return String();
	}

	WCHAR buffer[MAX_PATH] = {};
	DWORD buffer_size = MAX_PATH;
	if (RegGetValueW(key, (LPCWSTR)p_klid.utf16().get_data(), L"Layout Display Name", RRF_RT_REG_SZ, nullptr, buffer, &buffer_size) == ERROR_SUCCESS) {
		if (SHLoadIndirectString(buffer, buffer, buffer_size, nullptr) == S_OK) {
			ret = String::utf16((const char16_t *)buffer, buffer_size);
		}
	} else {
		if (RegGetValueW(key, (LPCWSTR)p_klid.utf16().get_data(), L"Layout Text", RRF_RT_REG_SZ, nullptr, buffer, &buffer_size) == ERROR_SUCCESS) {
			ret = String::utf16((const char16_t *)buffer, buffer_size);
		}
	}

	RegCloseKey(key);
	return ret;
}

String DisplayServerWindows::_get_klid(HKL p_hkl) const {
	String ret;

	WORD device = HIWORD(p_hkl);
	if ((device & 0xf000) == 0xf000) {
		WORD layout_id = device & 0x0fff;

		HKEY key;
		if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts", &key) != ERROR_SUCCESS) {
			return String();
		}

		DWORD index = 0;
		wchar_t klid_buffer[KL_NAMELENGTH];
		DWORD klid_buffer_size = KL_NAMELENGTH;
		while (RegEnumKeyExW(key, index, klid_buffer, &klid_buffer_size, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
			wchar_t layout_id_buf[MAX_PATH] = {};
			DWORD layout_id_size = MAX_PATH;
			if (RegGetValueW(key, klid_buffer, L"Layout Id", RRF_RT_REG_SZ, nullptr, layout_id_buf, &layout_id_size) == ERROR_SUCCESS) {
				if (layout_id == String::utf16((char16_t *)layout_id_buf, layout_id_size).hex_to_int()) {
					ret = String::utf16((const char16_t *)klid_buffer, klid_buffer_size).lpad(8, "0");
					break;
				}
			}
			klid_buffer_size = KL_NAMELENGTH;
			++index;
		}

		RegCloseKey(key);
	} else {
		if (device == 0) {
			device = LOWORD(p_hkl);
		}
		ret = (String::num_uint64((uint64_t)device, 16, false)).lpad(8, "0");
	}

	return ret;
}

String DisplayServerWindows::keyboard_get_layout_name(int p_index) const {
	int layout_count = GetKeyboardLayoutList(0, nullptr);

	ERR_FAIL_INDEX_V(p_index, layout_count, "");

	HKL *layouts = (HKL *)memalloc(layout_count * sizeof(HKL));
	GetKeyboardLayoutList(layout_count, layouts);

	String ret = _get_keyboard_layout_display_name(_get_klid(layouts[p_index])); // Try reading full name from Windows registry, fallback to locale name if failed (e.g. on Wine).
	if (ret.is_empty()) {
		WCHAR buf[LOCALE_NAME_MAX_LENGTH];
		memset(buf, 0, LOCALE_NAME_MAX_LENGTH * sizeof(WCHAR));
		LCIDToLocaleName(MAKELCID(LOWORD(layouts[p_index]), SORT_DEFAULT), buf, LOCALE_NAME_MAX_LENGTH, 0);

		WCHAR name[1024];
		memset(name, 0, 1024 * sizeof(WCHAR));
		GetLocaleInfoEx(buf, LOCALE_SLOCALIZEDDISPLAYNAME, (LPWSTR)&name, 1024);

		ret = String::utf16((const char16_t *)name);
	}
	memfree(layouts);

	return ret;
}

void DisplayServerWindows::process_events() {
	ERR_FAIL_COND(!Thread::is_main_thread());

	if (!drop_events) {
#ifdef SDL_ENABLED
		if (joypad_sdl) {
			joypad_sdl->process_events();
		}
#endif
	}

	_THREAD_SAFE_LOCK_
	MSG msg = {};
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	_THREAD_SAFE_UNLOCK_

	if (TTS_Windows::is_initialized()) {
		TTS_Windows::process_events();
	}

	if (!drop_events) {
		_process_key_events();
		Input::flush_buffered_events();
	}

	// Poll HDR output state for windows that have requested it.
	// Needed to detect changes in luminance values due to a lack of Windows events for such changes.
	// System-reported max luminance changes when the user adjust the screen brightness of a laptop
	// with a built-in HDR screen. Additionally, some computers may continue to report a
	// DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709 color space for a period of time after the
	// WM_DISPLAYCHANGE event is received by Godot which means we must poll this regularly to
	// capture this change in HDR capabilities of the screen triggered by the Win + Alt + B shortcut.
	// The SDR white level (reference white luminance) does not need to be polled every frame
	// because the only way to adjust this is to leave the Godot Window and adjust the SDR/HDR
	// Content Brightness Windows display setting. This means the user must return to the Godot
	// window, which triggers a WM_WINDOWPOSCHANGED event.
	_legacy_update_hdr_output_for_tracked_windows(false);

	LocalVector<List<FileDialogData *>::Element *> to_remove;
	for (List<FileDialogData *>::Element *E = file_dialogs.front(); E; E = E->next()) {
		FileDialogData *fd = E->get();
		if (fd->finished.is_set()) {
			if (fd->listener_thread.is_started()) {
				fd->listener_thread.wait_to_finish();
			}
			to_remove.push_back(E);
		}
	}
	for (List<FileDialogData *>::Element *E : to_remove) {
		memdelete(E->get());
		E->erase();
	}
	process_file_dialog_callbacks();
}

void DisplayServerWindows::force_process_and_drop_events() {
	ERR_FAIL_COND(!Thread::is_main_thread());

	drop_events = true;
	process_events();
	drop_events = false;
}

void DisplayServerWindows::release_rendering_thread() {
#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->release_current();
	}
#endif
	if (gl_manager_native) {
		gl_manager_native->release_current();
	}
#endif
}

void DisplayServerWindows::swap_buffers() {
#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->swap_buffers();
	}
#endif
	if (gl_manager_native) {
		gl_manager_native->swap_buffers();
	}
#endif
}

void DisplayServerWindows::set_native_icon(const String &p_filename) {
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (E.value.icon_set && E.key != DisplayServerEnums::MAIN_WINDOW_ID) {
			continue;
		}
		_window_set_native_icon(p_filename, E.key);
	}
}

void DisplayServerWindows::_window_set_native_icon(const String &p_filename, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.icon_big) {
		DestroyIcon(wd.icon_big);
		wd.icon_buffer_big.clear();
		wd.icon_big = nullptr;
	}
	if (wd.icon_small) {
		DestroyIcon(wd.icon_small);
		wd.icon_buffer_small.clear();
		wd.icon_small = nullptr;
	}

	Ref<FileAccess> f = FileAccess::open(p_filename, FileAccess::READ);
	ERR_FAIL_COND_MSG(f.is_null(), "Cannot open file with icon '" + p_filename + "'.");

	ICONDIR *icon_dir = (ICONDIR *)memalloc(sizeof(ICONDIR));
	int pos = 0;

	icon_dir->idReserved = f->get_32();
	pos += sizeof(WORD);
	f->seek(pos);

	icon_dir->idType = f->get_32();
	pos += sizeof(WORD);
	f->seek(pos);

	ERR_FAIL_COND_MSG(icon_dir->idType != 1, "Invalid icon file format!");

	icon_dir->idCount = f->get_32();
	pos += sizeof(WORD);
	f->seek(pos);

	icon_dir = (ICONDIR *)memrealloc(icon_dir, sizeof(ICONDIR) - sizeof(ICONDIRENTRY) + icon_dir->idCount * sizeof(ICONDIRENTRY));
	f->get_buffer((uint8_t *)&icon_dir->idEntries[0], icon_dir->idCount * sizeof(ICONDIRENTRY));

	int small_icon_index = -1; // Select 16x16 with largest color count.
	int small_icon_cc = 0;
	int big_icon_index = -1; // Select largest.
	int big_icon_width = 16;
	int big_icon_cc = 0;

	for (int i = 0; i < icon_dir->idCount; i++) {
		int colors = (icon_dir->idEntries[i].bColorCount == 0) ? 32768 : icon_dir->idEntries[i].bColorCount;
		int width = (icon_dir->idEntries[i].bWidth == 0) ? 256 : icon_dir->idEntries[i].bWidth;
		if (width == 16) {
			if (colors >= small_icon_cc) {
				small_icon_index = i;
				small_icon_cc = colors;
			}
		}
		if (width >= big_icon_width) {
			if (colors >= big_icon_cc) {
				big_icon_index = i;
				big_icon_width = width;
				big_icon_cc = colors;
			}
		}
	}

	ERR_FAIL_COND_MSG(big_icon_index == -1, "No valid icons found!");

	if (small_icon_index == -1) {
		WARN_PRINT("No small icon found, reusing " + itos(big_icon_width) + "x" + itos(big_icon_width) + " @" + itos(big_icon_cc) + " icon!");
		small_icon_index = big_icon_index;
		small_icon_cc = big_icon_cc;
	}

	// Read the big icon.
	DWORD bytecount_big = icon_dir->idEntries[big_icon_index].dwBytesInRes;
	wd.icon_buffer_big.resize(bytecount_big);
	pos = icon_dir->idEntries[big_icon_index].dwImageOffset;
	f->seek(pos);
	f->get_buffer((uint8_t *)&wd.icon_buffer_big.write[0], bytecount_big);
	wd.icon_big = CreateIconFromResourceEx((PBYTE)&wd.icon_buffer_big.write[0], bytecount_big, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE);
	ERR_FAIL_NULL_MSG(wd.icon_big, "Could not create " + itos(big_icon_width) + "x" + itos(big_icon_width) + " @" + itos(big_icon_cc) + " icon, error: " + format_ds_error_message(GetLastError()) + ".");

	// Read the small icon.
	DWORD bytecount_small = icon_dir->idEntries[small_icon_index].dwBytesInRes;
	wd.icon_buffer_small.resize(bytecount_small);
	pos = icon_dir->idEntries[small_icon_index].dwImageOffset;
	f->seek(pos);
	f->get_buffer((uint8_t *)&wd.icon_buffer_small.write[0], bytecount_small);
	wd.icon_small = CreateIconFromResourceEx((PBYTE)&wd.icon_buffer_small.write[0], bytecount_small, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE);
	ERR_FAIL_NULL_MSG(wd.icon_small, "Could not create 16x16 @" + itos(small_icon_cc) + " icon, error: " + format_ds_error_message(GetLastError()) + ".");

	// Online tradition says to be sure last error is cleared and set the small icon first.
	int err = 0;
	SetLastError(err);

	wd.icon_set = true;
	SendMessage(wd.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wd.icon_small);
	SendMessage(wd.hWnd, WM_SETICON, ICON_BIG, (LPARAM)wd.icon_big);
	if (p_window == DisplayServerEnums::MAIN_WINDOW_ID) {
		for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
			if (!E.value.icon_set && E.key != DisplayServerEnums::MAIN_WINDOW_ID) {
				SendMessage(E.value.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wd.icon_small);
				SendMessage(E.value.hWnd, WM_SETICON, ICON_BIG, (LPARAM)wd.icon_big);
			}
		}
	}

	memdelete(icon_dir);
}

void DisplayServerWindows::set_icon(const Ref<Image> &p_icon) {
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (E.value.icon_set && E.key != DisplayServerEnums::MAIN_WINDOW_ID) {
			continue;
		}
		window_set_icon(p_icon, E.key);
	}
}

void DisplayServerWindows::window_set_icon(const Ref<Image> &p_icon, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.icon_big) {
		DestroyIcon(wd.icon_big);
		wd.icon_buffer_big.clear();
		wd.icon_big = nullptr;
	}
	if (wd.icon_small) {
		DestroyIcon(wd.icon_small);
		wd.icon_buffer_small.clear();
		wd.icon_small = nullptr;
	}

	if (p_icon.is_valid()) {
		ERR_FAIL_COND(p_icon->get_width() <= 0 || p_icon->get_height() <= 0);

		Ref<Image> img = p_icon->duplicate();
		img->convert(Image::FORMAT_RGBA8);

		int w = img->get_width();
		int h = img->get_height();

		// Create temporary bitmap buffer.
		int icon_len = 40 + h * w * 4;
		wd.icon_buffer_big.resize(icon_len);
		BYTE *icon_bmp = wd.icon_buffer_big.ptrw();

		encode_uint32(40, &icon_bmp[0]);
		encode_uint32(w, &icon_bmp[4]);
		encode_uint32(h * 2, &icon_bmp[8]);
		encode_uint16(1, &icon_bmp[12]);
		encode_uint16(32, &icon_bmp[14]);
		encode_uint32(BI_RGB, &icon_bmp[16]);
		encode_uint32(w * h * 4, &icon_bmp[20]);
		encode_uint32(0, &icon_bmp[24]);
		encode_uint32(0, &icon_bmp[28]);
		encode_uint32(0, &icon_bmp[32]);
		encode_uint32(0, &icon_bmp[36]);

		uint8_t *wr = &icon_bmp[40];
		const uint8_t *r = img->get_data().ptr();

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				const uint8_t *rpx = &r[((h - i - 1) * w + j) * 4];
				uint8_t *wpx = &wr[(i * w + j) * 4];
				wpx[0] = rpx[2];
				wpx[1] = rpx[1];
				wpx[2] = rpx[0];
				wpx[3] = rpx[3];
			}
		}
		wd.icon_big = CreateIconFromResourceEx(icon_bmp, icon_len, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE);
		ERR_FAIL_NULL(wd.icon_big);

		wd.icon_set = true;
		SendMessage(wd.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wd.icon_big);
		SendMessage(wd.hWnd, WM_SETICON, ICON_BIG, (LPARAM)wd.icon_big);

		if (p_window == DisplayServerEnums::MAIN_WINDOW_ID) {
			for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
				if (!E.value.icon_set && E.key != DisplayServerEnums::MAIN_WINDOW_ID) {
					SendMessage(E.value.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wd.icon_big);
					SendMessage(E.value.hWnd, WM_SETICON, ICON_BIG, (LPARAM)wd.icon_big);
				}
			}
		}
	} else {
		wd.icon_set = false;
		SendMessage(wd.hWnd, WM_SETICON, ICON_SMALL, 0);
		SendMessage(wd.hWnd, WM_SETICON, ICON_BIG, 0);
		if (p_window == DisplayServerEnums::MAIN_WINDOW_ID) {
			for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
				if (!E.value.icon_set && E.key != DisplayServerEnums::MAIN_WINDOW_ID) {
					SendMessage(E.value.hWnd, WM_SETICON, ICON_SMALL, 0);
					SendMessage(E.value.hWnd, WM_SETICON, ICON_BIG, 0);
				}
			}
		}
	}
}

DisplayServerEnums::IndicatorID DisplayServerWindows::create_status_indicator(const Ref<Texture2D> &p_icon, const String &p_tooltip, const Callable &p_callback) {
	IndicatorData idat;
	if (p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
		Ref<Image> img = p_icon->get_image();
		img = img->duplicate();
		if (img->is_compressed()) {
			img->decompress();
		}
		img->convert(Image::FORMAT_RGBA8);

		int w = img->get_width();
		int h = img->get_height();

		// Create temporary bitmap buffer.
		int icon_len = 40 + h * w * 4;
		idat.icon_buffer.resize(icon_len);
		BYTE *icon_bmp = idat.icon_buffer.ptrw();

		encode_uint32(40, &icon_bmp[0]);
		encode_uint32(w, &icon_bmp[4]);
		encode_uint32(h * 2, &icon_bmp[8]);
		encode_uint16(1, &icon_bmp[12]);
		encode_uint16(32, &icon_bmp[14]);
		encode_uint32(BI_RGB, &icon_bmp[16]);
		encode_uint32(w * h * 4, &icon_bmp[20]);
		encode_uint32(0, &icon_bmp[24]);
		encode_uint32(0, &icon_bmp[28]);
		encode_uint32(0, &icon_bmp[32]);
		encode_uint32(0, &icon_bmp[36]);

		uint8_t *wr = &icon_bmp[40];
		const uint8_t *r = img->get_data().ptr();

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				const uint8_t *rpx = &r[((h - i - 1) * w + j) * 4];
				uint8_t *wpx = &wr[(i * w + j) * 4];
				wpx[0] = rpx[2];
				wpx[1] = rpx[1];
				wpx[2] = rpx[0];
				wpx[3] = rpx[3];
			}
		}

		idat.icon = CreateIconFromResourceEx(icon_bmp, icon_len, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE);
	}

	idat.callback = p_callback;

	NOTIFYICONDATAW ndat;
	ZeroMemory(&ndat, sizeof(NOTIFYICONDATAW));
	ndat.cbSize = sizeof(NOTIFYICONDATAW);
	ndat.hWnd = windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd;
	ndat.uID = indicator_id_counter;
	ndat.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	ndat.uCallbackMessage = WM_INDICATOR_CALLBACK_MESSAGE;
	ndat.hIcon = idat.icon;
	memcpy(ndat.szTip, p_tooltip.utf16().get_data(), MIN(p_tooltip.utf16().length(), 127) * sizeof(WCHAR));
	ndat.uVersion = NOTIFYICON_VERSION;

	Shell_NotifyIconW(NIM_ADD, &ndat);
	Shell_NotifyIconW(NIM_SETVERSION, &ndat);

	DisplayServerEnums::IndicatorID iid = indicator_id_counter++;
	indicators[iid] = idat;

	return iid;
}

void DisplayServerWindows::status_indicator_set_icon(DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_COND(!indicators.has(p_id));

	IndicatorData &idat = indicators[p_id];

	if (idat.icon) {
		DestroyIcon(idat.icon);
		idat.icon_buffer.clear();
		idat.icon = nullptr;
	}

	if (p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
		Ref<Image> img = p_icon->get_image();
		img = img->duplicate();
		if (img->is_compressed()) {
			img->decompress();
		}
		img->convert(Image::FORMAT_RGBA8);

		int w = img->get_width();
		int h = img->get_height();

		// Create temporary bitmap buffer.
		int icon_len = 40 + h * w * 4;
		idat.icon_buffer.resize(icon_len);
		BYTE *icon_bmp = idat.icon_buffer.ptrw();

		encode_uint32(40, &icon_bmp[0]);
		encode_uint32(w, &icon_bmp[4]);
		encode_uint32(h * 2, &icon_bmp[8]);
		encode_uint16(1, &icon_bmp[12]);
		encode_uint16(32, &icon_bmp[14]);
		encode_uint32(BI_RGB, &icon_bmp[16]);
		encode_uint32(w * h * 4, &icon_bmp[20]);
		encode_uint32(0, &icon_bmp[24]);
		encode_uint32(0, &icon_bmp[28]);
		encode_uint32(0, &icon_bmp[32]);
		encode_uint32(0, &icon_bmp[36]);

		uint8_t *wr = &icon_bmp[40];
		const uint8_t *r = img->get_data().ptr();

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				const uint8_t *rpx = &r[((h - i - 1) * w + j) * 4];
				uint8_t *wpx = &wr[(i * w + j) * 4];
				wpx[0] = rpx[2];
				wpx[1] = rpx[1];
				wpx[2] = rpx[0];
				wpx[3] = rpx[3];
			}
		}

		idat.icon = CreateIconFromResourceEx(icon_bmp, icon_len, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE);
	}

	NOTIFYICONDATAW ndat;
	ZeroMemory(&ndat, sizeof(NOTIFYICONDATAW));
	ndat.cbSize = sizeof(NOTIFYICONDATAW);
	ndat.hWnd = windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd;
	ndat.uID = p_id;
	ndat.uFlags = NIF_ICON;
	ndat.hIcon = idat.icon;
	ndat.uVersion = NOTIFYICON_VERSION;

	Shell_NotifyIconW(NIM_MODIFY, &ndat);
}

void DisplayServerWindows::status_indicator_set_tooltip(DisplayServerEnums::IndicatorID p_id, const String &p_tooltip) {
	ERR_FAIL_COND(!indicators.has(p_id));

	NOTIFYICONDATAW ndat;
	ZeroMemory(&ndat, sizeof(NOTIFYICONDATAW));
	ndat.cbSize = sizeof(NOTIFYICONDATAW);
	ndat.hWnd = windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd;
	ndat.uID = p_id;
	ndat.uFlags = NIF_TIP;
	memcpy(ndat.szTip, p_tooltip.utf16().get_data(), MIN(p_tooltip.utf16().length(), 127) * sizeof(WCHAR));
	ndat.uVersion = NOTIFYICON_VERSION;

	Shell_NotifyIconW(NIM_MODIFY, &ndat);
}

void DisplayServerWindows::status_indicator_set_menu(DisplayServerEnums::IndicatorID p_id, const RID &p_menu_rid) {
	ERR_FAIL_COND(!indicators.has(p_id));

	indicators[p_id].menu_rid = p_menu_rid;
}

void DisplayServerWindows::status_indicator_set_callback(DisplayServerEnums::IndicatorID p_id, const Callable &p_callback) {
	ERR_FAIL_COND(!indicators.has(p_id));

	indicators[p_id].callback = p_callback;
}

Rect2 DisplayServerWindows::status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const {
	ERR_FAIL_COND_V(!indicators.has(p_id), Rect2());

	NOTIFYICONIDENTIFIER nid;
	ZeroMemory(&nid, sizeof(NOTIFYICONIDENTIFIER));
	nid.cbSize = sizeof(NOTIFYICONIDENTIFIER);
	nid.hWnd = windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd;
	nid.uID = p_id;
	nid.guidItem = GUID_NULL;

	RECT rect;
	if (Shell_NotifyIconGetRect(&nid, &rect) != S_OK) {
		return Rect2();
	}
	Rect2 ind_rect = Rect2(Point2(rect.left, rect.top) - _get_screens_origin(), Size2(rect.right - rect.left, rect.bottom - rect.top));
	for (int i = 0; i < get_screen_count(); i++) {
		Rect2 screen_rect = Rect2(screen_get_position(i), screen_get_size(i));
		if (screen_rect.encloses(ind_rect)) {
			return ind_rect;
		}
	}
	return Rect2();
}

void DisplayServerWindows::delete_status_indicator(DisplayServerEnums::IndicatorID p_id) {
	ERR_FAIL_COND(!indicators.has(p_id));

	IndicatorData &idat = indicators[p_id];
	if (idat.icon) {
		DestroyIcon(idat.icon);
		idat.icon_buffer.clear();
		idat.icon = nullptr;
	}

	NOTIFYICONDATAW ndat;
	ZeroMemory(&ndat, sizeof(NOTIFYICONDATAW));
	ndat.cbSize = sizeof(NOTIFYICONDATAW);
	ndat.hWnd = windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd;
	ndat.uID = p_id;
	ndat.uVersion = NOTIFYICON_VERSION;

	Shell_NotifyIconW(NIM_DELETE, &ndat);
	indicators.erase(p_id);
}

void DisplayServerWindows::window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_
#if defined(RD_ENABLED)
	if (rendering_context) {
		rendering_context->window_set_vsync_mode(p_window, p_vsync_mode);
	}
#endif

#if defined(GLES3_ENABLED)
	if (gl_manager_native) {
		gl_manager_native->set_use_vsync(p_window, p_vsync_mode != DisplayServerEnums::VSYNC_DISABLED);
	}
#if defined(ANGLE_ENABLED)
	if (gl_manager_angle) {
		gl_manager_angle->set_use_vsync(p_vsync_mode != DisplayServerEnums::VSYNC_DISABLED);
	}
#endif
#endif
}

DisplayServerEnums::VSyncMode DisplayServerWindows::window_get_vsync_mode(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_vsync_mode(p_window);
	}
#endif

#if defined(GLES3_ENABLED)
	if (gl_manager_native) {
		return gl_manager_native->is_using_vsync(p_window) ? DisplayServerEnums::VSYNC_ENABLED : DisplayServerEnums::VSYNC_DISABLED;
	}
#ifdef ANGLE_ENABLED
	if (gl_manager_angle) {
		return gl_manager_angle->is_using_vsync() ? DisplayServerEnums::VSYNC_ENABLED : DisplayServerEnums::VSYNC_DISABLED;
	}
#endif
#endif
	return DisplayServerEnums::VSYNC_ENABLED;
}

bool DisplayServerWindows::window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), false);
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

	// The window supports HDR if the screen it is on supports HDR.
	DisplayServerWindows::ScreenHdrData data = _get_screen_hdr_data(p_window, false);
	return data.hdr_supported;
}

void DisplayServerWindows::window_request_hdr_output(const bool p_enable, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	if (p_enable) {
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

	WindowData &wd = windows[p_window];
	wd.hdr_output_requested = p_enable;

	DisplayServerWindows::ScreenHdrData data = _get_screen_hdr_data(p_window, true);
	_update_hdr_output_for_window(p_window, wd, data);
}

bool DisplayServerWindows::window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), false);
	const WindowData &wd = windows[p_window];
	return wd.hdr_output_requested;
}

bool DisplayServerWindows::window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), false);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_enabled(p_window);
	}
#endif

	return false;
}

void DisplayServerWindows::window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (Math::is_equal_approx(wd.hdr_output_reference_luminance, p_reference_luminance)) {
		return;
	}

	wd.hdr_output_reference_luminance = p_reference_luminance;

	// Negative luminance means auto-adjust
	if (wd.hdr_output_reference_luminance < 0.0f) {
		DisplayServerWindows::ScreenHdrData data = _get_screen_hdr_data(p_window, true);
		_update_hdr_output_for_window(p_window, wd, data);
	} else {
		// Otherwise, apply the requested luminance
#if defined(RD_ENABLED)
		if (rendering_context) {
			rendering_context->window_set_hdr_output_reference_luminance(p_window, p_reference_luminance);
			_send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_OUTPUT_MAX_LINEAR_VALUE_CHANGED);
		}
#endif
	}
}

float DisplayServerWindows::window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), 0.0);
	const WindowData &wd = windows[p_window];
	return wd.hdr_output_reference_luminance;
}

float DisplayServerWindows::window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), 0.0);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_reference_luminance(p_window);
	}
#endif

	return 0.0f;
}

void DisplayServerWindows::window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (Math::is_equal_approx(wd.hdr_output_max_luminance, p_max_luminance)) {
		return;
	}

	wd.hdr_output_max_luminance = p_max_luminance;

	// Negative luminance means auto-adjust
	if (wd.hdr_output_max_luminance < 0.0f) {
		DisplayServerWindows::ScreenHdrData data = _get_screen_hdr_data(p_window, true);
		_update_hdr_output_for_window(p_window, wd, data);
	} else {
		// Otherwise, apply the requested luminance
#if defined(RD_ENABLED)
		if (rendering_context) {
			rendering_context->window_set_hdr_output_max_luminance(p_window, p_max_luminance);
			_send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_OUTPUT_MAX_LINEAR_VALUE_CHANGED);
		}
#endif
	}
}

float DisplayServerWindows::window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), 0.0);
	const WindowData &wd = windows[p_window];
	return wd.hdr_output_max_luminance;
}

float DisplayServerWindows::window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), 0.0);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_max_luminance(p_window);
	}
#endif

	return 0.0f;
}

float DisplayServerWindows::window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), 1.0);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_output_max_linear_value(p_window);
	}
#endif

	return 1.0f; // SDR
}

void DisplayServerWindows::window_start_drag(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.parent_hwnd) {
		return; // Embedded window.
	}

	ReleaseCapture();

	POINT coords;
	GetCursorPos(&coords);
	ScreenToClient(wd.hWnd, &coords);

	SendMessage(wd.hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(coords.x, coords.y));

	for (int btn = (int)MouseButton::LEFT; btn <= (int)MouseButton::MB_XBUTTON2; btn++) {
		if (Input::is_mouse_button_pressed(MouseButton(btn))) {
			Ref<InputEventMouseButton> mb;
			mb.instantiate();
			mb->set_window_id(p_window);
			mb->set_pressed(false);
			mb->set_button_index(MouseButton::LEFT);
			mb->set_position(Vector2(coords.x, coords.y));
			mb->set_global_position(mb->get_position());
			Input::parse_input_event(mb);
		}
	}
}

void DisplayServerWindows::window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_INDEX(int(p_edge), DisplayServerEnums::WINDOW_EDGE_MAX);
	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];

	if (wd.parent_hwnd) {
		return; // Embedded window.
	}

	ReleaseCapture();

	POINT coords;
	GetCursorPos(&coords);
	ScreenToClient(wd.hWnd, &coords);

	DWORD op = 0;
	switch (p_edge) {
		case DisplayServerEnums::WINDOW_EDGE_TOP_LEFT: {
			op = WMSZ_TOPLEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP: {
			op = WMSZ_TOP;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP_RIGHT: {
			op = WMSZ_TOPRIGHT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_LEFT: {
			op = WMSZ_LEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_RIGHT: {
			op = WMSZ_RIGHT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_LEFT: {
			op = WMSZ_BOTTOMLEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM: {
			op = WMSZ_BOTTOM;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_RIGHT: {
			op = WMSZ_BOTTOMRIGHT;
		} break;
		default:
			break;
	}

	SendMessage(wd.hWnd, WM_SYSCOMMAND, SC_SIZE | op, MAKELPARAM(coords.x, coords.y));

	for (int btn = (int)MouseButton::LEFT; btn <= (int)MouseButton::MB_XBUTTON2; btn++) {
		if (Input::is_mouse_button_pressed(MouseButton(btn))) {
			Ref<InputEventMouseButton> mb;
			mb.instantiate();
			mb->set_window_id(p_window);
			mb->set_pressed(false);
			mb->set_button_index(MouseButton::LEFT);
			mb->set_position(Vector2(coords.x, coords.y));
			mb->set_global_position(mb->get_position());
			Input::parse_input_event(mb);
		}
	}
}

void DisplayServerWindows::set_context(DisplayServerEnums::Context p_context) {
}

bool DisplayServerWindows::is_window_transparency_available() const {
#if defined(RD_ENABLED)
	if (rendering_device && !rendering_device->is_composite_alpha_supported()) {
		return false;
	}
#endif
	return OS::is_layered_allowed();
}

#define MI_WP_SIGNATURE 0xFF515700
#define SIGNATURE_MASK 0xFFFFFF00
// Keeping the name suggested by Microsoft, but this macro really answers:
// Is this mouse event emulated from touch or pen input?
#define IsPenEvent(dw) (((dw) & SIGNATURE_MASK) == MI_WP_SIGNATURE)
// This one tells whether the event comes from touchscreen (and not from pen).
#define IsTouchEvent(dw) (IsPenEvent(dw) && ((dw) & 0x80))

void DisplayServerWindows::_touch_event(DisplayServerEnums::WindowID p_window, bool p_pressed, float p_x, float p_y, int idx) {
	if (touch_state.has(idx) == p_pressed) {
		return;
	}

	if (p_pressed) {
		touch_state.insert(idx, Vector2(p_x, p_y));
	} else {
		touch_state.erase(idx);
	}

	Ref<InputEventScreenTouch> event;
	event.instantiate();
	event->set_index(idx);
	event->set_window_id(p_window);
	event->set_pressed(p_pressed);
	event->set_position(Vector2(p_x, p_y));

	Input::parse_input_event(event);
}

void DisplayServerWindows::_drag_event(DisplayServerEnums::WindowID p_window, float p_x, float p_y, int idx) {
	RBMap<int, Vector2>::Element *curr = touch_state.find(idx);
	if (!curr) {
		return;
	}

	if (curr->get() == Vector2(p_x, p_y)) {
		return;
	}

	Ref<InputEventScreenDrag> event;
	event.instantiate();
	event->set_window_id(p_window);
	event->set_index(idx);
	event->set_position(Vector2(p_x, p_y));
	event->set_relative(Vector2(p_x, p_y) - curr->get());
	event->set_relative_screen_position(event->get_relative());

	Input::parse_input_event(event);

	curr->get() = Vector2(p_x, p_y);
}

void DisplayServerWindows::_send_window_event(const WindowData &wd, DisplayServerEnums::WindowEvent p_event) {
	if (wd.event_callback.is_valid()) {
		Variant event = int(p_event);
		wd.event_callback.call(event);
	}
}

void DisplayServerWindows::_dispatch_input_events(const Ref<InputEvent> &p_event) {
	if (DisplayServerWindows *ds = DisplayServer::driver_as<DisplayServerWindows>()) {
		ds->_dispatch_input_event(p_event);
	}
}

void DisplayServerWindows::_dispatch_input_event(const Ref<InputEvent> &p_event) {
	if (in_dispatch_input_event) {
		return;
	}
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
		// Send to a single window.
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

LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam) {
	DisplayServerWindows *ds_win = DisplayServer::driver_as<DisplayServerWindows>();
	if (ds_win) {
		return ds_win->MouseProc(code, wParam, lParam);
	} else {
		return ::CallNextHookEx(nullptr, code, wParam, lParam);
	}
}

DisplayServerEnums::WindowID DisplayServerWindows::window_get_active_popup() const {
	const List<DisplayServerEnums::WindowID>::Element *E = popup_list.back();
	if (E) {
		return E->get();
	} else {
		return DisplayServerEnums::INVALID_WINDOW_ID;
	}
}

void DisplayServerWindows::window_set_popup_safe_rect(DisplayServerEnums::WindowID p_window, const Rect2i &p_rect) {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND(!windows.has(p_window));
	WindowData &wd = windows[p_window];
	wd.parent_safe_rect = p_rect;
}

Rect2i DisplayServerWindows::window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window) const {
	_THREAD_SAFE_METHOD_

	ERR_FAIL_COND_V(!windows.has(p_window), Rect2i());
	const WindowData &wd = windows[p_window];
	return wd.parent_safe_rect;
}

void DisplayServerWindows::popup_open(DisplayServerEnums::WindowID p_window) {
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
			_send_window_event(windows[C->get()], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
		}

		time_since_popup = OS::get_ticks_msec();
		popup_list.push_back(p_window);
	}
}

void DisplayServerWindows::popup_close(DisplayServerEnums::WindowID p_window) {
	_THREAD_SAFE_METHOD_

	List<DisplayServerEnums::WindowID>::Element *E = popup_list.find(p_window);
	while (E) {
		List<DisplayServerEnums::WindowID>::Element *F = E->next();
		DisplayServerEnums::WindowID win_id = E->get();
		popup_list.erase(E);

		if (win_id != p_window) {
			// Only request close on related windows, not this window.  We are already processing it.
			_send_window_event(windows[win_id], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
		}
		E = F;
	}
}

uint32_t DisplayServerWindows::_get_mods() const {
	uint32_t mask = {};
	static unsigned char keyboard_state[256];
	if (GetKeyboardState((PBYTE)&keyboard_state)) {
		if ((keyboard_state[VK_LSHIFT] & 0x80) || (keyboard_state[VK_RSHIFT] & 0x80)) {
			mask.set_flag(WinKeyModifierMask::SHIFT);
		}
		if ((keyboard_state[VK_LCONTROL] & 0x80) || (keyboard_state[VK_RCONTROL] & 0x80)) {
			mask.set_flag(WinKeyModifierMask::CTRL);
		}
		if ((keyboard_state[VK_LMENU] & 0x80) || (keyboard_state[VK_RMENU] & 0x80)) {
			mask.set_flag(WinKeyModifierMask::ALT);
		}
		if ((keyboard_state[VK_RMENU] & 0x80)) {
			mask.set_flag(WinKeyModifierMask::ALT_GR);
		}
		if ((keyboard_state[VK_LWIN] & 0x80) || (keyboard_state[VK_RWIN] & 0x80)) {
			mask.set_flag(WinKeyModifierMask::META);
		}
	}

	return mask;
}

LRESULT DisplayServerWindows::MouseProc(int code, WPARAM wParam, LPARAM lParam) {
	_THREAD_SAFE_METHOD_

	uint64_t delta = OS::get_ticks_msec() - time_since_popup;
	if (delta > 250) {
		switch (wParam) {
			case WM_NCLBUTTONDOWN:
			case WM_NCRBUTTONDOWN:
			case WM_NCMBUTTONDOWN:
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN: {
				MOUSEHOOKSTRUCT *ms = (MOUSEHOOKSTRUCT *)lParam;
				Point2i pos = Point2i(ms->pt.x, ms->pt.y) - _get_screens_origin();
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
					_send_window_event(windows[C->get()], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
					return 1;
				}
			} break;
		}
	}
	return ::CallNextHookEx(mouse_monitor, code, wParam, lParam);
}

// Handle a single window message received while CreateWindowEx is still on the stack and our data
// structures are not fully initialized.
LRESULT DisplayServerWindows::_handle_early_window_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_GETMINMAXINFO: {
			// We receive this during CreateWindowEx and we haven't initialized the window
			// struct, so let Windows figure out the maximized size.
			// Silently forward to user/default.
		} break;
		case WM_NCCREATE: {
			// We tunnel an unowned pointer to our window context (WindowData) through the
			// first possible message (WM_NCCREATE) to fix up our window context collection.
			CREATESTRUCTW *pCreate = (CREATESTRUCTW *)lParam;
			WindowData *pWindowData = reinterpret_cast<WindowData *>(pCreate->lpCreateParams);

			// Fix this up so we can recognize the remaining messages.
			pWindowData->hWnd = hWnd;

			if (!AccessibilityServer::window_create(pWindowData->id, (void *)hWnd)) {
				if (OS::is_stdout_verbose()) {
					ERR_PRINT("Can't create an accessibility adapter for window, accessibility support disabled!");
				}
			}
		} break;
		default: {
			// Additional messages during window creation should happen after we fixed
			// up the data structures on WM_NCCREATE, but this might change in the future,
			// so report an error here and then we can implement them.
			ERR_PRINT_ONCE(vformat("Unexpected window message 0x%x received for window we cannot recognize in our collection; sequence error.", uMsg));
		} break;
	}

	if (user_proc) {
		return CallWindowProcW(user_proc, hWnd, uMsg, wParam, lParam);
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// The window procedure for our window class "Engine", used to handle processing of window-related system messages/events.
// See: https://docs.microsoft.com/en-us/windows/win32/winmsg/window-procedures
LRESULT DisplayServerWindows::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (drop_events) {
		if (user_proc) {
			return CallWindowProcW(user_proc, hWnd, uMsg, wParam, lParam);
		} else {
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}
	}

	DisplayServerEnums::WindowID window_id = DisplayServerEnums::INVALID_WINDOW_ID;
	bool window_created = false;

	// Check whether window exists
	// FIXME this is O(n), where n is the set of currently open windows and subwindows
	// we should have a secondary map from HWND to DisplayServerEnums::WindowID or even WindowData* alias, if we want to eliminate all the map lookups below
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		if (E.value.hWnd == hWnd) {
			window_id = E.key;
			window_created = true;
			break;
		}
	}

	// WARNING: We get called with events before the window is registered in our collection
	// specifically, even the call to CreateWindowEx already calls here while still on the stack,
	// so there is no way to store the window handle in our collection before we get here.
	if (!window_created) {
		// don't let code below operate on incompletely initialized window objects or missing window_id
		return _handle_early_window_message(hWnd, uMsg, wParam, lParam);
	}

	// Process window messages.
	switch (uMsg) {
		case WM_GETOBJECT: {
			get_object_received = true;
		} break;
		case WM_MENUCOMMAND: {
			native_menu->_menu_activate(HMENU(lParam), (int)wParam);
		} break;
		case WM_CREATE: {
			{
				DWORD value = windows[window_id].sharp_corners ? DWMWCP_DONOTROUND : DWMWCP_DEFAULT;
				::DwmSetWindowAttribute(windows[window_id].hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value));
			}
			if (is_dark_mode_supported() && dark_title_available) {
				BOOL value = is_dark_mode();

				::DwmSetWindowAttribute(windows[window_id].hWnd, use_legacy_dark_mode_before_20H1 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
				SendMessageW(windows[window_id].hWnd, WM_PAINT, 0, 0);
			}
		} break;
		case WM_NCHITTEST: {
			if (windows[window_id].mpass) {
				return HTTRANSPARENT;
			}
		} break;
		case WM_MOUSEACTIVATE: {
			if (windows[window_id].no_focus || windows[window_id].is_popup) {
				return MA_NOACTIVATE; // Do not activate, but process mouse messages.
			}
			// When embedded, the window is a child of the parent and is not activated
			// by default because it lacks native controls.
			if (windows[window_id].parent_hwnd) {
				SetFocus(windows[window_id].hWnd);
				return MA_ACTIVATE;
			}
		} break;
		case WM_ACTIVATEAPP: {
			bool new_app_focused = (bool)wParam;
			if (new_app_focused == app_focused) {
				break;
			}
			app_focused = new_app_focused;
			if (MainLoop::is_active()) {
				MainLoop::notification(app_focused ? MainLoop::NOTIFICATION_APPLICATION_FOCUS_IN : MainLoop::NOTIFICATION_APPLICATION_FOCUS_OUT);
			}
		} break;
		case WM_ACTIVATE: {
			// Activation can happen just after the window has been created, even before the callbacks are set.
			// Therefore, it's safer to defer the delivery of the event.
			// It's important to set an nIDEvent different from the SetTimer for move_timer_id because
			// if the same nIDEvent is passed, the timer is replaced and the same timer_id is returned.
			// The problem with the timer is that the window cannot be resized or the buttons cannot be used correctly
			// if the window is not activated first. This happens because the code in the activation process runs
			// after the mouse click is handled. To address this, the timer is now used only during the window creation,
			// and only as part of the activation process. We don't want 'Input::release_pressed_events()'
			// to be called immediately in '_process_activate_event' when the window is not yet activated,
			// as it would reset the currently pressed keys when hiding a window, which is incorrect behavior.
			windows[window_id].activate_state = GET_WM_ACTIVATE_STATE(wParam, lParam);
			if (windows[window_id].first_activation_done && (windows[window_id].activate_state == WA_ACTIVE || windows[window_id].activate_state == WA_CLICKACTIVE)) {
				_process_activate_event(window_id);
			} else {
				windows[window_id].activate_timer_id = SetTimer(windows[window_id].hWnd, DisplayServerWindows::TIMER_ID_WINDOW_ACTIVATION, USER_TIMER_MINIMUM, (TIMERPROC) nullptr);
			}
			return 0;
		} break;
		case WM_GETMINMAXINFO: {
			if (windows[window_id].resizable && !windows[window_id].fullscreen) {
				// Size of window decorations.
				Size2 decor = window_get_size_with_decorations(window_id) - window_get_size(window_id);

				MINMAXINFO *min_max_info = (MINMAXINFO *)lParam;
				if (windows[window_id].min_size != Size2()) {
					min_max_info->ptMinTrackSize.x = windows[window_id].min_size.x + decor.x;
					min_max_info->ptMinTrackSize.y = windows[window_id].min_size.y + decor.y;
				}
				if (windows[window_id].max_size != Size2()) {
					min_max_info->ptMaxTrackSize.x = windows[window_id].max_size.x + decor.x;
					min_max_info->ptMaxTrackSize.y = windows[window_id].max_size.y + decor.y;
				}
				if (windows[window_id].borderless) {
					Rect2i screen_rect = screen_get_usable_rect(window_get_current_screen(window_id));

					// Set the size of (borderless) maximized mode to exclude taskbar (or any other panel) if present.
					min_max_info->ptMaxPosition.x = screen_rect.position.x;
					min_max_info->ptMaxPosition.y = screen_rect.position.y;
					min_max_info->ptMaxSize.x = screen_rect.size.x;
					min_max_info->ptMaxSize.y = screen_rect.size.y;
				}
				return 0;
			}
		} break;
		case WM_ERASEBKGND: {
			Color early_color;
			if (!_get_window_early_clear_override(early_color)) {
				break;
			}
			bool must_recreate_brush = !window_bkg_brush || window_bkg_brush_color != early_color.to_argb32();
			if (must_recreate_brush) {
				if (window_bkg_brush) {
					DeleteObject(window_bkg_brush);
				}
				window_bkg_brush = CreateSolidBrush(RGB(early_color.get_r8(), early_color.get_g8(), early_color.get_b8()));
			}
			HDC hdc = (HDC)wParam;
			RECT rect = {};
			if (GetUpdateRect(hWnd, &rect, true)) {
				FillRect(hdc, &rect, window_bkg_brush);
			}
			return 1;
		} break;
		case WM_PAINT: {
			Main::force_redraw();
		} break;
		case WM_SETTINGCHANGE:
		case WM_SYSCOLORCHANGE: {
			if (lParam && CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, true) == CSTR_EQUAL) {
				if (is_dark_mode_supported() && dark_title_available) {
					BOOL value = is_dark_mode();
					::DwmSetWindowAttribute(windows[window_id].hWnd, use_legacy_dark_mode_before_20H1 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
				}
			}
			if (system_theme_changed.is_valid()) {
				Variant ret;
				Callable::CallError ce;
				system_theme_changed.callp(nullptr, 0, ret, ce);
				if (ce.error != Callable::CallError::CALL_OK) {
					ERR_PRINT(vformat("Failed to execute system theme changed callback: %s.", Variant::get_callable_error_text(system_theme_changed, nullptr, 0, ce)));
				}
			}
		} break;
		case WM_THEMECHANGED: {
			if (is_dark_mode_supported() && dark_title_available) {
				BOOL value = is_dark_mode();
				::DwmSetWindowAttribute(windows[window_id].hWnd, use_legacy_dark_mode_before_20H1 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
			}
		} break;
		case WM_SYSCOMMAND: // Intercept system commands.
		{
			switch (wParam) // Check system calls.
			{
				case SC_SCREENSAVE: // Screensaver trying to start?
				case SC_MONITORPOWER: // Monitor trying to enter powersave?
					return 0; // Prevent from happening.
				case SC_KEYMENU:
					if (((lParam >> 16) <= 0) && !Engine::is_project_manager_hint() && !Engine::is_editor_hint() && !GLOBAL_GET_CACHED(bool, "application/run/enable_alt_space_menu")) {
						return 0;
					}
					if (!_get_mods().has_flag(WinKeyModifierMask::ALT) || !(GetAsyncKeyState(VK_SPACE) & (1 << 15))) {
						return 0;
					}
					SendMessage(windows[window_id].hWnd, WM_SYSKEYUP, VK_SPACE, 0);
					SendMessage(windows[window_id].hWnd, WM_SYSKEYUP, VK_MENU, 0);
			}
		} break;
		case WM_INDICATOR_CALLBACK_MESSAGE: {
			if (lParam == WM_LBUTTONDOWN || lParam == WM_RBUTTONDOWN || lParam == WM_MBUTTONDOWN || lParam == WM_XBUTTONDOWN) {
				DisplayServerEnums::IndicatorID iid = (DisplayServerEnums::IndicatorID)wParam;
				MouseButton mb = MouseButton::LEFT;
				if (lParam == WM_RBUTTONDOWN) {
					mb = MouseButton::RIGHT;
				} else if (lParam == WM_MBUTTONDOWN) {
					mb = MouseButton::MIDDLE;
				} else if (lParam == WM_XBUTTONDOWN) {
					mb = MouseButton::MB_XBUTTON1;
				}
				if (indicators.has(iid)) {
					if (lParam == WM_RBUTTONDOWN && indicators[iid].menu_rid.is_valid() && native_menu->has_menu(indicators[iid].menu_rid)) {
						NOTIFYICONIDENTIFIER nid;
						ZeroMemory(&nid, sizeof(NOTIFYICONIDENTIFIER));
						nid.cbSize = sizeof(NOTIFYICONIDENTIFIER);
						nid.hWnd = windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd;
						nid.uID = iid;
						nid.guidItem = GUID_NULL;

						RECT rect;
						if (Shell_NotifyIconGetRect(&nid, &rect) == S_OK) {
							native_menu->popup(indicators[iid].menu_rid, Vector2i((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2));
						}
					} else if (indicators[iid].callback.is_valid()) {
						Variant v_button = mb;
						Variant v_pos = mouse_get_position();
						const Variant *v_args[2] = { &v_button, &v_pos };
						Variant ret;
						Callable::CallError ce;
						indicators[iid].callback.callp((const Variant **)&v_args, 2, ret, ce);
						if (ce.error != Callable::CallError::CALL_OK) {
							ERR_PRINT(vformat("Failed to execute status indicator callback: %s.", Variant::get_callable_error_text(indicators[iid].callback, v_args, 2, ce)));
						}
					}
				}
				return 0;
			}
		} break;
		case WM_CLOSE: {
			if (windows[window_id].activate_timer_id) {
				KillTimer(windows[window_id].hWnd, windows[window_id].activate_timer_id);
				windows[window_id].activate_timer_id = 0;
			}
			_send_window_event(windows[window_id], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
			return 0;
		}
		case WM_MOUSELEAVE: {
			if (window_mouseover_id == window_id) {
				old_invalid = true;
				window_mouseover_id = DisplayServerEnums::INVALID_WINDOW_ID;

				_send_window_event(windows[window_id], DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT);
			} else if (window_mouseover_id != DisplayServerEnums::INVALID_WINDOW_ID && windows.has(window_mouseover_id)) {
				// This is reached during drag and drop, after dropping in a different window.
				// Once-off notification, must call again.
				track_mouse_leave_event(windows[window_mouseover_id].hWnd);
			}

		} break;
		case WM_INPUT: {
			if (!use_raw_input) {
				break;
			}

			UINT dwSize;

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
			LPBYTE lpb = new BYTE[dwSize];
			if (lpb == nullptr) {
				return 0;
			}

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
				OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));
			}

			RAWINPUT *raw = (RAWINPUT *)lpb;

			const uint32_t &mods = _get_mods();
			if (raw->header.dwType == RIM_TYPEKEYBOARD) {
				if (raw->data.keyboard.VKey == VK_SHIFT) {
					// If multiple Shifts are held down at the same time,
					// Windows natively only sends a KEYUP for the last one to be released.
					// Handle all shift key up here for  consistency
					if (raw->data.keyboard.Flags & RI_KEY_BREAK) {
						ERR_BREAK(key_event_pos >= KEY_EVENT_BUFFER_SIZE);

						KeyEvent ke;
						ke.shift = false;
						ke.altgr = mods.has_flag(WinKeyModifierMask::ALT_GR);
						ke.alt = mods.has_flag(WinKeyModifierMask::ALT);
						ke.control = mods.has_flag(WinKeyModifierMask::CTRL);
						ke.meta = mods.has_flag(WinKeyModifierMask::META);
						ke.uMsg = WM_KEYUP;
						ke.window_id = window_id;

						ke.wParam = VK_SHIFT;
						// data.keyboard.MakeCode -> 0x2A - left shift, 0x36 - right shift.
						// Bit 30 -> key was previously down, bit 31 -> key is being released.
						ke.lParam = raw->data.keyboard.MakeCode << 16 | 1 << 30 | 1 << 31;
						key_event_buffer[key_event_pos++] = ke;
					}
				}
			} else if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED && raw->header.dwType == RIM_TYPEMOUSE) {
				Ref<InputEventMouseMotion> mm;
				mm.instantiate();

				mm->set_window_id(window_id);
				mm->set_ctrl_pressed(mods.has_flag(WinKeyModifierMask::CTRL));
				mm->set_shift_pressed(mods.has_flag(WinKeyModifierMask::SHIFT));
				mm->set_alt_pressed(mods.has_flag(WinKeyModifierMask::ALT));
				mm->set_meta_pressed(mods.has_flag(WinKeyModifierMask::META));

				mm->set_pressure((raw->data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_DOWN) ? 1.0f : 0.0f);

				mm->set_button_mask(mouse_get_button_state());

				Point2i c(windows[window_id].width / 2, windows[window_id].height / 2);

				// Centering just so it works as before.
				POINT pos = { (int)c.x, (int)c.y };
				ClientToScreen(windows[window_id].hWnd, &pos);
				SetCursorPos(pos.x, pos.y);

				mm->set_position(c);
				mm->set_global_position(c);
				mm->set_velocity(Vector2(0, 0));
				mm->set_screen_velocity(Vector2(0, 0));

				if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
					mm->set_relative(Vector2(raw->data.mouse.lLastX, raw->data.mouse.lLastY));

				} else if (raw->data.mouse.usFlags == MOUSE_MOVE_ABSOLUTE) {
					int nScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
					int nScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
					int nScreenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
					int nScreenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);

					Vector2 abs_pos(
							(double(raw->data.mouse.lLastX) - 65536.0 / (nScreenWidth)) * nScreenWidth / 65536.0 + nScreenLeft,
							(double(raw->data.mouse.lLastY) - 65536.0 / (nScreenHeight)) * nScreenHeight / 65536.0 + nScreenTop);

					POINT coords; // Client coords.
					coords.x = abs_pos.x;
					coords.y = abs_pos.y;

					ScreenToClient(hWnd, &coords);

					mm->set_relative(Vector2(coords.x - old_x, coords.y - old_y));
					old_x = coords.x;
					old_y = coords.y;
				}
				mm->set_relative_screen_position(mm->get_relative());

				if ((windows[window_id].window_focused || windows[window_id].is_popup) && mm->get_relative() != Vector2()) {
					Input::parse_input_event(mm);
				}
			}
			delete[] lpb;
		} break;
		case WT_CSRCHANGE:
		case WT_PROXIMITY: {
			if ((tablet_get_current_driver() == "wintab") && wintab_available && windows[window_id].wtctx) {
				AXIS pressure;
				if (wintab_WTInfo(WTI_DEVICES + windows[window_id].wtlc.lcDevice, DVC_NPRESSURE, &pressure)) {
					windows[window_id].min_pressure = int(pressure.axMin);
					windows[window_id].max_pressure = int(pressure.axMax);
				}
				AXIS orientation[3];
				if (wintab_WTInfo(WTI_DEVICES + windows[window_id].wtlc.lcDevice, DVC_ORIENTATION, &orientation)) {
					windows[window_id].tilt_supported = orientation[0].axResolution && orientation[1].axResolution;
				}
				return 0;
			}
		} break;
		case WT_PACKET: {
			if ((tablet_get_current_driver() == "wintab") && wintab_available && windows[window_id].wtctx) {
				PACKET packet;
				if (wintab_WTPacket(windows[window_id].wtctx, wParam, &packet)) {
					POINT coords;
					GetCursorPos(&coords);
					ScreenToClient(windows[window_id].hWnd, &coords);

					windows[window_id].last_pressure_update = 0;

					float pressure = float(packet.pkNormalPressure - windows[window_id].min_pressure) / float(windows[window_id].max_pressure - windows[window_id].min_pressure);
					double azim = (packet.pkOrientation.orAzimuth / 10.0f) * (Math::PI / 180);
					double alt = Math::tan((Math::abs(packet.pkOrientation.orAltitude / 10.0f)) * (Math::PI / 180));
					bool inverted = packet.pkStatus & TPS_INVERT;

					Vector2 tilt = (windows[window_id].tilt_supported) ? Vector2(Math::atan(Math::sin(azim) / alt), Math::atan(Math::cos(azim) / alt)) : Vector2();

					// Nothing changed, ignore event.
					if (!old_invalid && coords.x == old_x && coords.y == old_y && windows[window_id].last_pressure == pressure && windows[window_id].last_tilt == tilt && windows[window_id].last_pen_inverted == inverted) {
						break;
					}

					windows[window_id].last_pressure = pressure;
					windows[window_id].last_tilt = tilt;
					windows[window_id].last_pen_inverted = inverted;

					// Don't calculate relative mouse movement if we don't have focus in CAPTURED mode.
					if (!windows[window_id].window_focused && mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
						break;
					}

					const uint32_t &mods = _get_mods();
					Ref<InputEventMouseMotion> mm;
					mm.instantiate();
					mm->set_window_id(window_id);
					mm->set_ctrl_pressed(mods.has_flag(WinKeyModifierMask::CTRL));
					mm->set_shift_pressed(mods.has_flag(WinKeyModifierMask::SHIFT));
					mm->set_alt_pressed(mods.has_flag(WinKeyModifierMask::ALT));
					mm->set_meta_pressed(mods.has_flag(WinKeyModifierMask::META));

					mm->set_pressure(windows[window_id].last_pressure);
					mm->set_tilt(windows[window_id].last_tilt);
					mm->set_pen_inverted(windows[window_id].last_pen_inverted);

					mm->set_button_mask(mouse_get_button_state());

					mm->set_position(Vector2(coords.x, coords.y));
					mm->set_global_position(Vector2(coords.x, coords.y));

					if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
						Point2i c(windows[window_id].width / 2, windows[window_id].height / 2);
						old_x = c.x;
						old_y = c.y;

						if (mm->get_position() == c) {
							center = c;
							return 0;
						}

						Point2i ncenter = mm->get_position();
						center = ncenter;
						POINT pos = { (int)c.x, (int)c.y };
						ClientToScreen(windows[window_id].hWnd, &pos);
						SetCursorPos(pos.x, pos.y);
					}

					mm->set_velocity(Input::get_last_mouse_velocity());
					mm->set_screen_velocity(mm->get_velocity());

					if (old_invalid) {
						old_x = mm->get_position().x;
						old_y = mm->get_position().y;
						old_invalid = false;
					}

					mm->set_relative(Vector2(mm->get_position() - Vector2(old_x, old_y)));
					mm->set_relative_screen_position(mm->get_relative());
					old_x = mm->get_position().x;
					old_y = mm->get_position().y;

					if (windows[window_id].window_focused || window_get_active_popup() == window_id) {
						Input::parse_input_event(mm);
					}
				}
				return 0;
			}
		} break;
		case WM_POINTERENTER: {
			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED && use_raw_input) {
				break;
			}

			if (tablet_get_current_driver() != "winink") {
				break;
			}

			uint32_t pointer_id = LOWORD(wParam);
			POINTER_INPUT_TYPE pointer_type = PT_POINTER;
			if (!GetPointerType(pointer_id, &pointer_type)) {
				break;
			}

			if (pointer_type != PT_PEN) {
				break;
			}

			pointer_button[GET_POINTERID_WPARAM(wParam)] = MouseButton::NONE;
			windows[window_id].block_mm = true;
			return 0;
		} break;
		case WM_POINTERLEAVE: {
			pointer_button[GET_POINTERID_WPARAM(wParam)] = MouseButton::NONE;
			windows[window_id].block_mm = false;
			return 0;
		} break;
		case WM_POINTERDOWN:
		case WM_POINTERUP: {
			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED && use_raw_input) {
				break;
			}

			if (tablet_get_current_driver() != "winink") {
				break;
			}

			uint32_t pointer_id = LOWORD(wParam);
			POINTER_INPUT_TYPE pointer_type = PT_POINTER;
			if (!GetPointerType(pointer_id, &pointer_type)) {
				break;
			}

			if (pointer_type != PT_PEN) {
				break;
			}

			Ref<InputEventMouseButton> mb;
			mb.instantiate();
			mb->set_window_id(window_id);

			uint32_t last_button_state = MouseButtonMask::NONE;
			if (IS_POINTER_FIRSTBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::LEFT);
				mb->set_button_index(MouseButton::LEFT);
			}
			if (IS_POINTER_SECONDBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::RIGHT);
				mb->set_button_index(MouseButton::RIGHT);
			}
			if (IS_POINTER_THIRDBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::MIDDLE);
				mb->set_button_index(MouseButton::MIDDLE);
			}
			if (IS_POINTER_FOURTHBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::MB_XBUTTON1);
				mb->set_button_index(MouseButton::MB_XBUTTON1);
			}
			if (IS_POINTER_FIFTHBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::MB_XBUTTON2);
				mb->set_button_index(MouseButton::MB_XBUTTON2);
			}
			mb->set_button_mask(last_button_state);

			const uint32_t &mods = _get_mods();
			mb->set_ctrl_pressed(mods.has_flag(WinKeyModifierMask::CTRL));
			mb->set_shift_pressed(mods.has_flag(WinKeyModifierMask::SHIFT));
			mb->set_alt_pressed(mods.has_flag(WinKeyModifierMask::ALT));
			mb->set_meta_pressed(mods.has_flag(WinKeyModifierMask::META));

			POINT coords; // Client coords.
			coords.x = GET_X_LPARAM(lParam);
			coords.y = GET_Y_LPARAM(lParam);

			// Note: Handle popup closing here, since mouse event is not emulated and hook will not be called.
			uint64_t delta = OS::get_ticks_msec() - time_since_popup;
			if (delta > 250) {
				Point2i pos = Point2i(coords.x, coords.y) - _get_screens_origin();
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
					_send_window_event(windows[C->get()], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
				}
			}

			int64_t pen_id = GET_POINTERID_WPARAM(wParam);
			if (uMsg == WM_POINTERDOWN) {
				mb->set_pressed(true);
				if (pointer_down_time.has(pen_id) && (pointer_prev_button[pen_id] == mb->get_button_index()) && (Math::abs(coords.y - pointer_last_pos[pen_id].y) < GetSystemMetrics(SM_CYDOUBLECLK)) && GetMessageTime() - pointer_down_time[pen_id] < (LONG)GetDoubleClickTime()) {
					mb->set_double_click(true);
					pointer_down_time[pen_id] = 0;
				} else {
					pointer_down_time[pen_id] = GetMessageTime();
					pointer_prev_button[pen_id] = mb->get_button_index();
					pointer_last_pos[pen_id] = Vector2(coords.x, coords.y);
				}
				pointer_button[pen_id] = mb->get_button_index();
			} else {
				if (!pointer_button.has(pen_id)) {
					return 0;
				}
				mb->set_pressed(false);
				mb->set_button_index(pointer_button[pen_id]);
				pointer_button[pen_id] = MouseButton::NONE;
			}

			ScreenToClient(windows[window_id].hWnd, &coords);

			mb->set_position(Vector2(coords.x, coords.y));
			mb->set_global_position(Vector2(coords.x, coords.y));

			Input::parse_input_event(mb);

			return 0;
		} break;
		case WM_POINTERUPDATE: {
			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED && use_raw_input) {
				break;
			}

			if (tablet_get_current_driver() != "winink") {
				break;
			}

			uint32_t pointer_id = LOWORD(wParam);
			POINTER_INPUT_TYPE pointer_type = PT_POINTER;
			if (!GetPointerType(pointer_id, &pointer_type)) {
				break;
			}

			if (pointer_type != PT_PEN) {
				break;
			}

			POINTER_PEN_INFO pen_info;
			if (!GetPointerPenInfo(pointer_id, &pen_info)) {
				break;
			}

			if (Input::is_emulating_mouse_from_touch()) {
				// Universal translation enabled; ignore OS translation.
				LPARAM extra = GetMessageExtraInfo();
				if (IsTouchEvent(extra)) {
					break;
				}
			}

			if (window_mouseover_id != window_id) {
				// Mouse enter.

				if (mouse_mode != DisplayServerEnums::MOUSE_MODE_CAPTURED) {
					if (window_mouseover_id != DisplayServerEnums::INVALID_WINDOW_ID && windows.has(window_mouseover_id)) {
						// Leave previous window.
						_send_window_event(windows[window_mouseover_id], DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT);
					}
					_send_window_event(windows[window_id], DisplayServerEnums::WINDOW_EVENT_MOUSE_ENTER);
				}

				DisplayServerEnums::CursorShape c = cursor_shape;
				cursor_shape = DisplayServerEnums::CURSOR_MAX;
				cursor_set_shape(c);
				window_mouseover_id = window_id;

				// Once-off notification, must call again.
				track_mouse_leave_event(hWnd);
			}

			// Don't calculate relative mouse movement if we don't have focus in CAPTURED mode.
			if (!windows[window_id].window_focused && mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
				break;
			}

			Ref<InputEventMouseMotion> mm;
			mm.instantiate();

			mm->set_window_id(window_id);
			if (pen_info.penMask & PEN_MASK_PRESSURE) {
				mm->set_pressure((float)pen_info.pressure / 1024);
			} else {
				mm->set_pressure((HIWORD(wParam) & POINTER_MESSAGE_FLAG_FIRSTBUTTON) ? 1.0f : 0.0f);
			}
			if ((pen_info.penMask & PEN_MASK_TILT_X) && (pen_info.penMask & PEN_MASK_TILT_Y)) {
				mm->set_tilt(Vector2((float)pen_info.tiltX / 90, (float)pen_info.tiltY / 90));
			}
			mm->set_pen_inverted(pen_info.penFlags & (PEN_FLAG_INVERTED | PEN_FLAG_ERASER));

			const uint32_t &mods = _get_mods();
			mm->set_ctrl_pressed(mods.has_flag(WinKeyModifierMask::CTRL));
			mm->set_shift_pressed(mods.has_flag(WinKeyModifierMask::SHIFT));
			mm->set_alt_pressed(mods.has_flag(WinKeyModifierMask::ALT));
			mm->set_meta_pressed(mods.has_flag(WinKeyModifierMask::META));

			uint32_t last_button_state = MouseButtonMask::NONE;
			if (IS_POINTER_FIRSTBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::LEFT);
			}
			if (IS_POINTER_SECONDBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::RIGHT);
			}
			if (IS_POINTER_THIRDBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::MIDDLE);
			}
			if (IS_POINTER_FOURTHBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::MB_XBUTTON1);
			}
			if (IS_POINTER_FIFTHBUTTON_WPARAM(wParam)) {
				last_button_state.set_flag(MouseButtonMask::MB_XBUTTON2);
			}
			mm->set_button_mask(last_button_state);

			POINT coords; // Client coords.
			coords.x = GET_X_LPARAM(lParam);
			coords.y = GET_Y_LPARAM(lParam);

			ScreenToClient(windows[window_id].hWnd, &coords);

			mm->set_position(Vector2(coords.x, coords.y));
			mm->set_global_position(Vector2(coords.x, coords.y));

			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
				Point2i c(windows[window_id].width / 2, windows[window_id].height / 2);
				old_x = c.x;
				old_y = c.y;

				if (mm->get_position() == c) {
					center = c;
					return 0;
				}

				Point2i ncenter = mm->get_position();
				center = ncenter;
				POINT pos = { (int)c.x, (int)c.y };
				ClientToScreen(hWnd, &pos);
				SetCursorPos(pos.x, pos.y);
			}

			mm->set_velocity(Input::get_last_mouse_velocity());
			mm->set_screen_velocity(mm->get_velocity());

			if (old_invalid) {
				old_x = mm->get_position().x;
				old_y = mm->get_position().y;
				old_invalid = false;
			}

			mm->set_relative(Vector2(mm->get_position() - Vector2(old_x, old_y)));
			mm->set_relative_screen_position(mm->get_relative());
			old_x = mm->get_position().x;
			old_y = mm->get_position().y;
			if (windows[window_id].window_focused || window_get_active_popup() == window_id) {
				Input::parse_input_event(mm);
			}

			return 0; // Pointer event handled return 0 to avoid duplicate WM_MOUSEMOVE event.
		} break;
		case WM_MOUSEMOVE: {
			if (windows[window_id].block_mm) {
				break;
			}

			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED && use_raw_input) {
				break;
			}

			if (Input::is_emulating_mouse_from_touch()) {
				// Universal translation enabled; ignore OS translation.
				LPARAM extra = GetMessageExtraInfo();
				if (IsTouchEvent(extra)) {
					break;
				}
			}

			DisplayServerEnums::WindowID over_id = get_window_at_screen_position(mouse_get_position());
			if (windows.has(over_id) && !Rect2(window_get_position(over_id), Point2(windows[over_id].width, windows[over_id].height)).has_point(mouse_get_position())) {
				// Don't consider the windowborder as part of the window.
				over_id = DisplayServerEnums::INVALID_WINDOW_ID;
			}
			if (window_mouseover_id != over_id) {
				// Mouse enter.

				if (mouse_mode != DisplayServerEnums::MOUSE_MODE_CAPTURED) {
					if (window_mouseover_id != DisplayServerEnums::INVALID_WINDOW_ID && windows.has(window_mouseover_id)) {
						// Leave previous window.
						_send_window_event(windows[window_mouseover_id], DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT);
					}

					if (over_id != DisplayServerEnums::INVALID_WINDOW_ID && windows.has(over_id)) {
						_send_window_event(windows[over_id], DisplayServerEnums::WINDOW_EVENT_MOUSE_ENTER);
					}
				}

				DisplayServerEnums::CursorShape c = cursor_shape;
				cursor_shape = DisplayServerEnums::CURSOR_MAX;
				cursor_set_shape(c);
				window_mouseover_id = over_id;

				// Once-off notification, must call again.
				track_mouse_leave_event(hWnd);
			}

			// Don't calculate relative mouse movement if we don't have focus in CAPTURED mode.
			if (!windows[window_id].window_focused && mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
				break;
			}

			DisplayServerEnums::WindowID receiving_window_id = window_id;
			if (!windows[window_id].no_focus) {
				receiving_window_id = _get_focused_window_or_popup();
				if (receiving_window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
					receiving_window_id = window_id;
				}
			}

			const uint32_t &mods = _get_mods();
			Ref<InputEventMouseMotion> mm;
			mm.instantiate();
			mm->set_window_id(receiving_window_id);
			mm->set_ctrl_pressed(mods.has_flag(WinKeyModifierMask::CTRL));
			mm->set_shift_pressed(mods.has_flag(WinKeyModifierMask::SHIFT));
			mm->set_alt_pressed(mods.has_flag(WinKeyModifierMask::ALT));
			mm->set_meta_pressed(mods.has_flag(WinKeyModifierMask::META));

			if ((tablet_get_current_driver() == "wintab") && wintab_available && windows[window_id].wtctx) {
				// Note: WinTab sends both WT_PACKET and WM_xBUTTONDOWN/UP/MOUSEMOVE events, use mouse 1/0 pressure only when last_pressure was not updated recently.
				if (windows[window_id].last_pressure_update < 10) {
					windows[window_id].last_pressure_update++;
				} else {
					windows[window_id].last_tilt = Vector2();
					windows[window_id].last_pressure = (wParam & MK_LBUTTON) ? 1.0f : 0.0f;
					windows[window_id].last_pen_inverted = false;
				}
			} else {
				windows[window_id].last_tilt = Vector2();
				windows[window_id].last_pressure = (wParam & MK_LBUTTON) ? 1.0f : 0.0f;
				windows[window_id].last_pen_inverted = false;
			}

			mm->set_pressure(windows[window_id].last_pressure);
			mm->set_tilt(windows[window_id].last_tilt);
			mm->set_pen_inverted(windows[window_id].last_pen_inverted);

			mm->set_button_mask(mouse_get_button_state());

			mm->set_position(Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
			mm->set_global_position(Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));

			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
				Point2i c(windows[window_id].width / 2, windows[window_id].height / 2);
				old_x = c.x;
				old_y = c.y;

				if (mm->get_position() == c) {
					center = c;
					return 0;
				}

				Point2i ncenter = mm->get_position();
				center = ncenter;
				POINT pos = { (int)c.x, (int)c.y };
				ClientToScreen(windows[window_id].hWnd, &pos);
				SetCursorPos(pos.x, pos.y);
			}

			mm->set_velocity(Input::get_last_mouse_velocity());
			mm->set_screen_velocity(mm->get_velocity());

			if (old_invalid) {
				old_x = mm->get_position().x;
				old_y = mm->get_position().y;
				old_invalid = false;
			}

			mm->set_relative(Vector2(mm->get_position() - Vector2(old_x, old_y)));
			mm->set_relative_screen_position(mm->get_relative());
			old_x = mm->get_position().x;
			old_y = mm->get_position().y;

			if (receiving_window_id != window_id) {
				// Adjust event position relative to window distance when event is sent to a different window.
				mm->set_position(mm->get_position() - window_get_position(receiving_window_id) + window_get_position(window_id));
				mm->set_global_position(mm->get_position());
			}

			Input::parse_input_event(mm);

		} break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			if (Input::is_emulating_mouse_from_touch()) {
				// Universal translation enabled; ignore OS translations for left button.
				LPARAM extra = GetMessageExtraInfo();
				if (IsTouchEvent(extra)) {
					break;
				}
			}
			[[fallthrough]];
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP: {
			Ref<InputEventMouseButton> mb;
			mb.instantiate();
			mb->set_window_id(window_id);

			switch (uMsg) {
				case WM_LBUTTONDOWN: {
					mb->set_pressed(true);
					mb->set_button_index(MouseButton::LEFT);
				} break;
				case WM_LBUTTONUP: {
					mb->set_pressed(false);
					mb->set_button_index(MouseButton::LEFT);
				} break;
				case WM_MBUTTONDOWN: {
					mb->set_pressed(true);
					mb->set_button_index(MouseButton::MIDDLE);
				} break;
				case WM_MBUTTONUP: {
					mb->set_pressed(false);
					mb->set_button_index(MouseButton::MIDDLE);
				} break;
				case WM_RBUTTONDOWN: {
					mb->set_pressed(true);
					mb->set_button_index(MouseButton::RIGHT);
				} break;
				case WM_RBUTTONUP: {
					mb->set_pressed(false);
					mb->set_button_index(MouseButton::RIGHT);
				} break;
				case WM_LBUTTONDBLCLK: {
					mb->set_pressed(true);
					mb->set_button_index(MouseButton::LEFT);
					mb->set_double_click(true);
				} break;
				case WM_RBUTTONDBLCLK: {
					mb->set_pressed(true);
					mb->set_button_index(MouseButton::RIGHT);
					mb->set_double_click(true);
				} break;
				case WM_MBUTTONDBLCLK: {
					mb->set_pressed(true);
					mb->set_button_index(MouseButton::MIDDLE);
					mb->set_double_click(true);
				} break;
				case WM_MOUSEWHEEL: {
					mb->set_pressed(true);
					int motion = (short)HIWORD(wParam);
					if (!motion) {
						return 0;
					}

					if (motion > 0) {
						mb->set_button_index(MouseButton::WHEEL_UP);
					} else {
						mb->set_button_index(MouseButton::WHEEL_DOWN);
					}
					mb->set_factor(std::fabs((double)motion / (double)WHEEL_DELTA));
				} break;
				case WM_MOUSEHWHEEL: {
					mb->set_pressed(true);
					int motion = (short)HIWORD(wParam);
					if (!motion) {
						return 0;
					}

					if (motion < 0) {
						mb->set_button_index(MouseButton::WHEEL_LEFT);
					} else {
						mb->set_button_index(MouseButton::WHEEL_RIGHT);
					}
					mb->set_factor(std::fabs((double)motion / (double)WHEEL_DELTA));
				} break;
				case WM_XBUTTONDOWN: {
					mb->set_pressed(true);
					if (HIWORD(wParam) == XBUTTON1) {
						mb->set_button_index(MouseButton::MB_XBUTTON1);
					} else {
						mb->set_button_index(MouseButton::MB_XBUTTON2);
					}
				} break;
				case WM_XBUTTONUP: {
					mb->set_pressed(false);
					if (HIWORD(wParam) == XBUTTON1) {
						mb->set_button_index(MouseButton::MB_XBUTTON1);
					} else {
						mb->set_button_index(MouseButton::MB_XBUTTON2);
					}
				} break;
				case WM_XBUTTONDBLCLK: {
					mb->set_pressed(true);
					if (HIWORD(wParam) == XBUTTON1) {
						mb->set_button_index(MouseButton::MB_XBUTTON1);
					} else {
						mb->set_button_index(MouseButton::MB_XBUTTON2);
					}
					mb->set_double_click(true);
				} break;
				default: {
					return 0;
				}
			}

			const uint32_t &mods = _get_mods();
			mb->set_ctrl_pressed(mods.has_flag(WinKeyModifierMask::CTRL));
			mb->set_shift_pressed(mods.has_flag(WinKeyModifierMask::SHIFT));
			mb->set_alt_pressed(mods.has_flag(WinKeyModifierMask::ALT));
			mb->set_meta_pressed(mods.has_flag(WinKeyModifierMask::META));

			if (mb->is_pressed() && mb->get_button_index() >= MouseButton::WHEEL_UP && mb->get_button_index() <= MouseButton::WHEEL_RIGHT) {
				MouseButtonMask mask = mouse_button_to_mask(mb->get_button_index());
				uint32_t scroll_mask = mouse_get_button_state();
				scroll_mask.set_flag(mask);
				mb->set_button_mask(scroll_mask);
			} else {
				mb->set_button_mask(mouse_get_button_state());
			}

			mb->set_position(Vector2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));

			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED && !use_raw_input) {
				mb->set_position(Vector2(old_x, old_y));
			}

			if (uMsg != WM_MOUSEWHEEL && uMsg != WM_MOUSEHWHEEL) {
				if (mb->is_pressed()) {
					if (++pressrc > 0 && mouse_mode != DisplayServerEnums::MOUSE_MODE_CAPTURED) {
						SetCapture(hWnd);
					}
				} else {
					if (--pressrc <= 0 || mouse_get_button_state().is_empty()) {
						if (mouse_mode != DisplayServerEnums::MOUSE_MODE_CAPTURED) {
							ReleaseCapture();
						}
						pressrc = 0;
					}
				}
			} else {
				// For reasons unknown to humanity, wheel comes in screen coordinates.
				POINT coords;
				coords.x = mb->get_position().x;
				coords.y = mb->get_position().y;

				ScreenToClient(hWnd, &coords);

				mb->set_position(Vector2(coords.x, coords.y));
			}

			mb->set_global_position(mb->get_position());

			Input::parse_input_event(mb);
			if (mb->is_pressed() && mb->get_button_index() >= MouseButton::WHEEL_UP && mb->get_button_index() <= MouseButton::WHEEL_RIGHT) {
				// Send release for mouse wheel.
				Ref<InputEventMouseButton> mbd = mb->duplicate();
				mbd->set_window_id(window_id);
				mbd->set_button_mask(mouse_get_button_state());
				mbd->set_pressed(false);
				Input::parse_input_event(mbd);
			}

			// Propagate the button up event to the window on which the button down
			// event was triggered. This is needed for drag & drop to work between windows,
			// because the engine expects events to keep being processed
			// on the same window dragging started.
			if (mb->is_pressed()) {
				last_mouse_button_down_window = window_id;
			} else if (last_mouse_button_down_window != DisplayServerEnums::INVALID_WINDOW_ID) {
				mb->set_window_id(last_mouse_button_down_window);
				last_mouse_button_down_window = DisplayServerEnums::INVALID_WINDOW_ID;
			}
		} break;

		case WM_DISPLAYCHANGE: {
			// Update HDR capabilities and reference luminance when display changes.
			_legacy_update_hdr_output_for_tracked_windows(true);
		} break;

		case WM_WINDOWPOSCHANGED: {
			WindowData &window = windows[window_id];

			Vector2i off = (window.multiwindow_fs || (!window.fullscreen && window.borderless && window.maximized)) ? _get_screen_expand_offset(window_get_current_screen(window_id)) : Vector2i();
			Rect2i window_client_rect;
			Rect2i window_rect;
			{
				RECT rect;
				GetClientRect(hWnd, &rect);
				ClientToScreen(hWnd, (POINT *)&rect.left);
				ClientToScreen(hWnd, (POINT *)&rect.right);
				window_client_rect = Rect2i(rect.left, rect.top, rect.right - rect.left - off.x, rect.bottom - rect.top - off.y);
				window_client_rect.position -= _get_screens_origin();

				RECT wrect;
				GetWindowRect(hWnd, &wrect);
				window_rect = Rect2i(wrect.left, wrect.top, wrect.right - wrect.left - off.x, wrect.bottom - wrect.top - off.y);
				window_rect.position -= _get_screens_origin();
			}

			WINDOWPOS *window_pos_params = (WINDOWPOS *)lParam;

			bool rect_changed = false;
			if (!(window_pos_params->flags & SWP_NOSIZE) || window_pos_params->flags & SWP_FRAMECHANGED) {
				int screen_id = window_get_current_screen(window_id);
				Size2i screen_size = screen_get_size(screen_id);
				Point2i screen_position = screen_get_position(screen_id);
				Rect2i usable = screen_get_usable_rect(screen_id);

				window.maximized = false;
				window.minimized = false;
				window.fullscreen = false;

				if (IsIconic(hWnd)) {
					window.minimized = true;
				} else if (IsZoomed(hWnd)) {
					window.maximized = true;

					// If maximized_window_size == screen_size add 1px border to prevent switching to exclusive_fs.
					if (!window.maximized_fs && window.borderless && window_rect.position == screen_position && window_rect.size == screen_size) {
						// Window (borderless) was just maximized and the covers the entire screen.
						window.maximized_fs = true;
						_update_window_style(window_id, false);
					}
					if (window.borderless && (screen_size != usable.size || screen_position != usable.position)) {
						Point2 pos = usable.position + _get_screens_origin();
						Size2 size = usable.size;
						MoveWindow(window.hWnd, pos.x, pos.y, size.width, size.height, TRUE);
					}
				} else if (window_rect.position == screen_position && window_rect.size == screen_size) {
					window.fullscreen = true;
				} else if (window.borderless && usable.position == window_rect.position && usable.size == window_rect.size) {
					window.maximized = true;
				}

				if (window.maximized_fs && !window.maximized) {
					// Window (maximized and covering fullscreen) was just non-maximized.
					window.maximized_fs = false;
					_update_window_style(window_id, false);
				}

				if (!window.minimized) {
					window.width = window_client_rect.size.width;
					window.height = window_client_rect.size.height;
					window.width_with_decorations = window_rect.size.width;
					window.height_with_decorations = window_rect.size.height;

					rect_changed = true;
				}
#if defined(RD_ENABLED)
				if (window.create_completed && rendering_context && window.rendering_context_window_created) {
					// Note: Trigger resize event to update swapchains when window is minimized/restored, even if size is not changed.
					rendering_context->window_set_size(window_id, window.width + off.x, window.height + off.y);
				}
#endif
#if defined(GLES3_ENABLED)
				if (window.create_completed && gl_manager_native && window.gl_native_window_created) {
					gl_manager_native->window_resize(window_id, window.width + off.x, window.height + off.y);
				}
#if defined(ANGLE_ENABLED)
				if (window.create_completed && gl_manager_angle && window.gl_angle_window_created) {
					gl_manager_angle->window_resize(window_id, window.width + off.x, window.height + off.y);
				}
#endif
#endif
			}

			if (!window.minimized && (!(window_pos_params->flags & SWP_NOMOVE) || window_pos_params->flags & SWP_FRAMECHANGED)) {
				window.last_pos = window_client_rect.position;
				rect_changed = true;
			}

			if (rect_changed) {
				if (window.rect_changed_callback.is_valid()) {
					window.rect_changed_callback.call(Rect2i(window.last_pos.x, window.last_pos.y, window.width, window.height));
				}

				// Update cursor clip region after window rect has changed.
				if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED || mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED || mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN) {
					RECT crect;
					GetClientRect(window.hWnd, &crect);
					crect.right -= off.x;
					crect.bottom -= off.y;
					ClientToScreen(window.hWnd, (POINT *)&crect.left);
					ClientToScreen(window.hWnd, (POINT *)&crect.right);
					ClipCursor(&crect);
				}

				if (!window.minimized && window.was_fullscreen_pre_min) {
					// Restore fullscreen mode if window was in fullscreen before it was minimized.
					int cs = window_get_current_screen(window_id);
					Point2 pos = screen_get_position(cs) + _get_screens_origin();
					Size2 size = screen_get_size(cs);

					window.was_fullscreen_pre_min = false;
					window.fullscreen = true;
					window.maximized = false;
					window.minimized = false;

					_update_window_style(window_id, false);

					MoveWindow(window.hWnd, pos.x, pos.y, size.width, size.height, TRUE);
				}
			} else {
				if (window.parent_hwnd) {
					// WM_WINDOWPOSCHANGED is sent when the parent changes.
					// If we are supposed to have a parent and now we don't, it's likely
					// because the parent was closed. We will close our window as well.
					// This prevents an embedded game from staying alive when the editor is closed or crashes.
					if (!GetParent(window.hWnd)) {
						SendMessage(window.hWnd, WM_CLOSE, 0, 0);
					}
				}
			}

			// Update HDR capabilities and reference luminance when window moves to different screen.
			// Also update when Godot has regained focus because the user may have adjusted their SDR white
			// level while Godot was not in focus.
			_legacy_update_hdr_output_for_tracked_windows(true);

			// Return here to prevent WM_MOVE and WM_SIZE from being sent
			// See: https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-windowposchanged#remarks
			return 0;
		} break;

		case WM_ENTERSIZEMOVE: {
			Input::release_pressed_events();
			windows[window_id].move_timer_id = SetTimer(windows[window_id].hWnd, DisplayServerWindows::TIMER_ID_MOVE_REDRAW, USER_TIMER_MINIMUM, (TIMERPROC) nullptr);
		} break;
		case WM_EXITSIZEMOVE: {
			KillTimer(windows[window_id].hWnd, windows[window_id].move_timer_id);
			windows[window_id].move_timer_id = 0;
			// Reset the correct mouse mode because we couldn't call ReleaseCapture in
			// _set_mouse_mode_impl while in _process_activate_event (because the user was moving a window).
			_set_mouse_mode_impl(mouse_mode);
		} break;
		case WM_TIMER: {
			if (wParam == windows[window_id].move_timer_id) {
				_THREAD_SAFE_UNLOCK_
				_process_key_events();
				if (!Main::is_iterating()) {
					Main::iteration();
				}
				_THREAD_SAFE_LOCK_
			} else if (wParam == windows[window_id].activate_timer_id) {
				_process_activate_event(window_id);
				KillTimer(windows[window_id].hWnd, windows[window_id].activate_timer_id);
				windows[window_id].activate_timer_id = 0;
				windows[window_id].first_activation_done = true;
			}
		} break;
		case WM_SYSKEYUP:
		case WM_KEYUP:
			// Windows handles shift KEYUP inconsistently, handle with WM_INPUT
			if (wParam == VK_SHIFT) {
				break;
			}
			[[fallthrough]];
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
			if (windows[window_id].ime_suppress_next_keyup && (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP)) {
				windows[window_id].ime_suppress_next_keyup = false;
				break;
			}
			if (windows[window_id].ime_in_progress) {
				break;
			}

			if (mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED) {
				// When SetCapture is used, ALT+F4 hotkey is ignored by Windows, so handle it ourselves
				if (wParam == VK_F4 && _get_mods().has_flag(WinKeyModifierMask::ALT) && (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)) {
					_send_window_event(windows[window_id], DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
				}
			}
			[[fallthrough]];
		}
		case WM_CHAR: {
			ERR_BREAK(key_event_pos >= KEY_EVENT_BUFFER_SIZE);
			const uint32_t &mods = _get_mods();

			KeyEvent ke;
			ke.shift = mods.has_flag(WinKeyModifierMask::SHIFT);
			ke.alt = mods.has_flag(WinKeyModifierMask::ALT);
			ke.altgr = mods.has_flag(WinKeyModifierMask::ALT_GR);
			ke.control = mods.has_flag(WinKeyModifierMask::CTRL);
			ke.meta = mods.has_flag(WinKeyModifierMask::META);
			ke.uMsg = uMsg;
			ke.window_id = window_id;

			if (ke.uMsg == WM_SYSKEYDOWN) {
				ke.uMsg = WM_KEYDOWN;
			}
			if (ke.uMsg == WM_SYSKEYUP) {
				ke.uMsg = WM_KEYUP;
			}

			ke.wParam = wParam;
			ke.lParam = lParam;
			key_event_buffer[key_event_pos++] = ke;

		} break;
		case WM_IME_COMPOSITION: {
			update_ime_form_positions(windows[window_id].im_himc, windows[window_id].im_position);

			if (windows[window_id].ime_active) {
				SetCaretPos(windows[window_id].im_position.x, windows[window_id].im_position.y);
				if (MainLoop::is_active()) {
					MainLoop::notification(MainLoop::NOTIFICATION_OS_IME_UPDATE);
				}
			}
		} break;
		case WM_INPUTLANGCHANGEREQUEST: {
			// FIXME: Do something?
		} break;
		case WM_IME_STARTCOMPOSITION: {
			if (windows[window_id].ime_active) {
				windows[window_id].ime_in_progress = true;
				if (key_event_pos > 0) {
					key_event_pos--;
				}
			}
			return 0;
		} break;
		case WM_IME_ENDCOMPOSITION: {
			if (windows[window_id].ime_active) {
				windows[window_id].ime_in_progress = false;
				windows[window_id].ime_suppress_next_keyup = true;
			}
			return 0;
		} break;
		case WM_IME_NOTIFY: {
			return 0;
		} break;
		case WM_TOUCH: {
			BOOL bHandled = FALSE;
			UINT cInputs = LOWORD(wParam);
			PTOUCHINPUT pInputs = memnew_arr(TOUCHINPUT, cInputs);
			if (pInputs) {
				if (GetTouchInputInfo((HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT))) {
					for (UINT i = 0; i < cInputs; i++) {
						TOUCHINPUT ti = pInputs[i];
						POINT touch_pos = {
							TOUCH_COORD_TO_PIXEL(ti.x),
							TOUCH_COORD_TO_PIXEL(ti.y),
						};
						ScreenToClient(hWnd, &touch_pos);
						// Do something with each touch input entry.
						if (ti.dwFlags & TOUCHEVENTF_MOVE) {
							_drag_event(window_id, touch_pos.x, touch_pos.y, ti.dwID);
						} else if (ti.dwFlags & (TOUCHEVENTF_UP | TOUCHEVENTF_DOWN)) {
							_touch_event(window_id, ti.dwFlags & TOUCHEVENTF_DOWN, touch_pos.x, touch_pos.y, ti.dwID);
						}
					}
					bHandled = TRUE;
				} else {
					// TODO: Handle the error here.
				}
				memdelete_arr(pInputs);
			} else {
				// TODO: Handle the error here, probably out of memory.
			}
			if (bHandled) {
				CloseTouchInputHandle((HTOUCHINPUT)lParam);
				return 0;
			}

		} break;
		case WM_DESTROY: {
			AccessibilityServer::window_destroy(window_id);

			Input::flush_buffered_events();
			if (window_mouseover_id == window_id) {
				window_mouseover_id = DisplayServerEnums::INVALID_WINDOW_ID;
				_send_window_event(windows[window_id], DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT);
			}
		} break;
		case WM_SETCURSOR: {
			if (LOWORD(lParam) == HTCLIENT) {
				if (windows[window_id].window_focused && (mouse_mode == DisplayServerEnums::MOUSE_MODE_HIDDEN || mouse_mode == DisplayServerEnums::MOUSE_MODE_CAPTURED || mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN)) {
					// Hide the cursor.
					if (hCursor == nullptr) {
						hCursor = SetCursor(nullptr);
					} else {
						SetCursor(nullptr);
					}
				} else {
					if (hCursor != nullptr) {
						DisplayServerEnums::CursorShape c = cursor_shape;
						cursor_shape = DisplayServerEnums::CURSOR_MAX;
						cursor_set_shape(c);
						hCursor = nullptr;
					}
				}
			}
		} break;
		default: {
			if (user_proc) {
				return CallWindowProcW(user_proc, hWnd, uMsg, wParam, lParam);
			}
		}
	}

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	DisplayServerWindows *ds_win = DisplayServer::driver_as<DisplayServerWindows>();
	if (ds_win) {
		return ds_win->WndProc(hWnd, uMsg, wParam, lParam);
	} else {
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

void DisplayServerWindows::_process_activate_event(DisplayServerEnums::WindowID p_window_id) {
	WindowData &wd = windows[p_window_id];
	if (wd.activate_state == WA_ACTIVE || wd.activate_state == WA_CLICKACTIVE) {
		last_focused_window = p_window_id;
		_set_mouse_mode_impl(mouse_mode);
		if (!IsIconic(wd.hWnd)) {
			SetFocus(wd.hWnd);
		}
		wd.window_focused = true;
		AccessibilityServer::set_window_focused(p_window_id, true);

		_send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_FOCUS_IN);
	} else { // WM_INACTIVE.
		Input::release_pressed_events();
		track_mouse_leave_event(wd.hWnd);
		// Release capture unconditionally because it can be set due to dragging, in addition to captured mode.
		// When the user is moving a window, it's important to not ReleaseCapture because it will cause
		// the window movement to stop and if the user tries to move the Windows when it's not activated,
		// it will prevent the window movement. If we are here and a window is moving, it's because we had multiple
		// opened windows in the editor and we are definitively not in a middle of dragging.
		if (!_has_moving_window()) {
			ReleaseCapture();
		}
		wd.window_focused = false;
		AccessibilityServer::set_window_focused(p_window_id, false);

		_send_window_event(wd, DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT);
	}

	if ((tablet_get_current_driver() == "wintab") && wintab_available && wd.wtctx) {
		wintab_WTEnable(wd.wtctx, wd.activate_state);
	}
}

void DisplayServerWindows::_process_key_events() {
	for (int i = 0; i < key_event_pos; i++) {
		KeyEvent &ke = key_event_buffer[i];
		switch (ke.uMsg) {
			case WM_CHAR: {
				// Extended keys should only be processed as WM_KEYDOWN message.
				if (!KeyMappingWindows::is_extended_key(ke.wParam) && ((i == 0 && ke.uMsg == WM_CHAR) || (i > 0 && key_event_buffer[i - 1].uMsg == WM_CHAR))) {
					static char32_t prev_wc = 0;
					char32_t unicode = ke.wParam;
					if ((unicode & 0xfffffc00) == 0xd800) {
						if (prev_wc != 0) {
							ERR_PRINT("invalid utf16 surrogate input");
						}
						prev_wc = unicode;
						break; // Skip surrogate.
					} else if ((unicode & 0xfffffc00) == 0xdc00) {
						if (prev_wc == 0) {
							ERR_PRINT("invalid utf16 surrogate input");
							break; // Skip invalid surrogate.
						}
						unicode = (prev_wc << 10UL) + unicode - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
						prev_wc = 0;
					} else {
						prev_wc = 0;
					}
					Ref<InputEventKey> k;
					k.instantiate();

					UINT vk = MapVirtualKey((ke.lParam >> 16) & 0xFF, MAPVK_VSC_TO_VK);
					bool is_oem = (vk >= 0xB8) && (vk <= 0xE6);
					Key keycode = KeyMappingWindows::get_keysym(vk);
					Key key_label = keycode;
					Key physical_keycode = KeyMappingWindows::get_scansym((ke.lParam >> 16) & 0xFF, ke.lParam & (1 << 24));

					static BYTE keyboard_state[256];
					memset(keyboard_state, 0, 256);
					wchar_t chars[256] = {};
					UINT extended_code = MapVirtualKey((ke.lParam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX);
					if (!(ke.lParam & (1 << 24)) && ToUnicodeEx(extended_code, (ke.lParam >> 16) & 0xFF, keyboard_state, chars, 255, 4, GetKeyboardLayout(0)) > 0) {
						String keysym = String::utf16((char16_t *)chars, 255);
						if (!keysym.is_empty()) {
							char32_t unicode_value = keysym[0];
							// For printable ASCII characters (0x20-0x7E), override the original keycode with the character value.
							if (is_oem && Key::SPACE <= (Key)unicode_value && (Key)unicode_value <= Key::ASCIITILDE) {
								keycode = fix_keycode(unicode_value, (Key)unicode_value);
							}
							key_label = fix_key_label(unicode_value, keycode);
						}
					}

					k->set_window_id(ke.window_id);
					if (keycode != Key::SHIFT) {
						k->set_shift_pressed(ke.shift);
					}
					if (keycode != Key::ALT) {
						k->set_alt_pressed(ke.alt);
					}
					if (keycode != Key::CTRL) {
						k->set_ctrl_pressed(ke.control);
					}
					if (keycode != Key::META) {
						k->set_meta_pressed(ke.meta);
					}
					k->set_pressed(true);
					k->set_keycode(keycode);
					k->set_physical_keycode(physical_keycode);
					k->set_key_label(key_label);
					k->set_unicode(fix_unicode(unicode));
					if (k->get_unicode() && ke.altgr && windows[ke.window_id].ime_active) {
						k->set_alt_pressed(false);
						k->set_ctrl_pressed(false);
					}

					Input::parse_input_event(k);
				} else {
					// Do nothing.
				}
			} break;
			case WM_KEYUP:
			case WM_KEYDOWN: {
				Ref<InputEventKey> k;
				k.instantiate();

				k->set_window_id(ke.window_id);
				k->set_pressed(ke.uMsg == WM_KEYDOWN);

				bool is_oem = (ke.wParam >= 0xB8) && (ke.wParam <= 0xE6);
				Key keycode = KeyMappingWindows::get_keysym(ke.wParam);
				if ((ke.lParam & (1 << 24)) && (ke.wParam == VK_RETURN)) {
					// Special case for Numpad Enter key.
					keycode = Key::KP_ENTER;
				}
				Key key_label = keycode;
				Key physical_keycode = KeyMappingWindows::get_scansym((ke.lParam >> 16) & 0xFF, ke.lParam & (1 << 24));
				KeyLocation location = KeyMappingWindows::get_location((ke.lParam >> 16) & 0xFF, ke.lParam & (1 << 24));

				static BYTE keyboard_state[256];
				memset(keyboard_state, 0, 256);
				wchar_t chars[256] = {};
				UINT extended_code = MapVirtualKey((ke.lParam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX);
				if (!(ke.lParam & (1 << 24)) && ToUnicodeEx(extended_code, (ke.lParam >> 16) & 0xFF, keyboard_state, chars, 255, 4, GetKeyboardLayout(0)) > 0) {
					String keysym = String::utf16((char16_t *)chars, 255);
					if (!keysym.is_empty()) {
						char32_t unicode_value = keysym[0];
						// For printable ASCII characters (0x20-0x7E), override the original keycode with the character value.
						if (is_oem && Key::SPACE <= (Key)unicode_value && (Key)unicode_value <= Key::ASCIITILDE) {
							keycode = fix_keycode(unicode_value, (Key)unicode_value);
						}
						key_label = fix_key_label(unicode_value, keycode);
					}
				}

				if (keycode != Key::SHIFT) {
					k->set_shift_pressed(ke.shift);
				}
				if (keycode != Key::ALT) {
					k->set_alt_pressed(ke.alt);
				}
				if (keycode != Key::CTRL) {
					k->set_ctrl_pressed(ke.control);
				}
				if (keycode != Key::META) {
					k->set_meta_pressed(ke.meta);
				}
				k->set_keycode(keycode);
				k->set_physical_keycode(physical_keycode);
				k->set_location(location);
				k->set_key_label(key_label);

				if (i + 1 < key_event_pos && key_event_buffer[i + 1].uMsg == WM_CHAR) {
					char32_t unicode = key_event_buffer[i + 1].wParam;
					static char32_t prev_wck = 0;
					if ((unicode & 0xfffffc00) == 0xd800) {
						if (prev_wck != 0) {
							ERR_PRINT("invalid utf16 surrogate input");
						}
						prev_wck = unicode;
						break; // Skip surrogate.
					} else if ((unicode & 0xfffffc00) == 0xdc00) {
						if (prev_wck == 0) {
							ERR_PRINT("invalid utf16 surrogate input");
							break; // Skip invalid surrogate.
						}
						unicode = (prev_wck << 10UL) + unicode - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
						prev_wck = 0;
					} else {
						prev_wck = 0;
					}
					k->set_unicode(fix_unicode(unicode));
				}
				if (k->get_unicode() && ke.altgr && windows[ke.window_id].ime_active) {
					k->set_alt_pressed(false);
					k->set_ctrl_pressed(false);
				}

				k->set_echo((ke.uMsg == WM_KEYDOWN && (ke.lParam & (1 << 30))));

				Input::parse_input_event(k);

			} break;
		}
	}

	key_event_pos = 0;
}

void DisplayServerWindows::_update_tablet_ctx(const String &p_old_driver, const String &p_new_driver) {
	for (KeyValue<DisplayServerEnums::WindowID, WindowData> &E : windows) {
		WindowData &wd = E.value;
		wd.block_mm = false;
		if ((p_old_driver == "wintab") && wintab_available && wd.wtctx) {
			wintab_WTEnable(wd.wtctx, false);
			wintab_WTClose(wd.wtctx);
			wd.wtctx = nullptr;
		}
		if ((p_new_driver == "wintab") && wintab_available) {
			wintab_WTInfo(WTI_DEFSYSCTX, 0, &wd.wtlc);
			wd.wtlc.lcOptions |= CXO_MESSAGES;
			wd.wtlc.lcPktData = PK_STATUS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE | PK_ORIENTATION;
			wd.wtlc.lcMoveMask = PK_STATUS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE;
			wd.wtlc.lcPktMode = 0;
			wd.wtlc.lcOutOrgX = 0;
			wd.wtlc.lcOutExtX = wd.wtlc.lcInExtX;
			wd.wtlc.lcOutOrgY = 0;
			wd.wtlc.lcOutExtY = -wd.wtlc.lcInExtY;
			wd.wtctx = wintab_WTOpen(wd.hWnd, &wd.wtlc, false);
			if (wd.wtctx) {
				wintab_WTEnable(wd.wtctx, true);
				AXIS pressure;
				if (wintab_WTInfo(WTI_DEVICES + wd.wtlc.lcDevice, DVC_NPRESSURE, &pressure)) {
					wd.min_pressure = int(pressure.axMin);
					wd.max_pressure = int(pressure.axMax);
				}
				AXIS orientation[3];
				if (wintab_WTInfo(WTI_DEVICES + wd.wtlc.lcDevice, DVC_ORIENTATION, &orientation)) {
					wd.tilt_supported = orientation[0].axResolution && orientation[1].axResolution;
				}
				wintab_WTEnable(wd.wtctx, true);
			} else {
				print_verbose("WinTab context creation failed.");
			}
		}
	}
}

Error DisplayServerWindows::_create_window(DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::WindowMode p_mode, uint32_t p_flags, const Rect2i &p_rect, bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent, HWND p_parent_hwnd, bool p_no_redirection_bitmap) {
	DWORD dwExStyle;
	DWORD dwStyle;

	_get_window_style(p_window_id == DisplayServerEnums::MAIN_WINDOW_ID, false, (p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN), p_mode != DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN, p_flags & DisplayServerEnums::WINDOW_FLAG_BORDERLESS_BIT, !(p_flags & DisplayServerEnums::WINDOW_FLAG_RESIZE_DISABLED_BIT), p_flags & DisplayServerEnums::WINDOW_FLAG_MINIMIZE_DISABLED_BIT, p_flags & DisplayServerEnums::WINDOW_FLAG_MAXIMIZE_DISABLED_BIT, p_mode == DisplayServerEnums::WINDOW_MODE_MINIMIZED, p_mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED, false, (p_flags & DisplayServerEnums::WINDOW_FLAG_NO_FOCUS_BIT) | (p_flags & DisplayServerEnums::WINDOW_FLAG_POPUP_BIT), p_parent_hwnd, p_no_redirection_bitmap, dwStyle, dwExStyle);

	int rq_screen = get_screen_from_rect(p_rect);
	if (rq_screen < 0) {
		rq_screen = get_primary_screen(); // Requested window rect is outside any screen bounds.
	}
	Rect2i usable_rect = screen_get_usable_rect(rq_screen);

	Point2i offset = _get_screens_origin();

	RECT WindowRect;

	Vector2i off = (p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || ((p_flags & DisplayServerEnums::WINDOW_FLAG_BORDERLESS_BIT) && p_mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED)) ? _get_screen_expand_offset(rq_screen) : Vector2i();

	WindowRect.left = p_rect.position.x;
	WindowRect.right = p_rect.position.x + p_rect.size.x + off.x;
	WindowRect.top = p_rect.position.y;
	WindowRect.bottom = p_rect.position.y + p_rect.size.y + off.y;

	if (!p_parent_hwnd) {
		if (p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
			Rect2i screen_rect = Rect2i(screen_get_position(rq_screen), screen_get_size(rq_screen));

			WindowRect.left = screen_rect.position.x;
			WindowRect.right = screen_rect.position.x + screen_rect.size.x + off.x;
			WindowRect.top = screen_rect.position.y;
			WindowRect.bottom = screen_rect.position.y + screen_rect.size.y + off.y;
		} else {
			Rect2i srect = screen_get_usable_rect(rq_screen);
			Point2i wpos = p_rect.position;
			if (srect != Rect2i()) {
				wpos = wpos.clamp(srect.position, srect.position + srect.size - p_rect.size / 3);
			}

			WindowRect.left = wpos.x;
			WindowRect.right = wpos.x + p_rect.size.x + off.x;
			WindowRect.top = wpos.y;
			WindowRect.bottom = wpos.y + p_rect.size.y + off.y;
		}
	}

	WindowRect.left += offset.x;
	WindowRect.right += offset.x;
	WindowRect.top += offset.y;
	WindowRect.bottom += offset.y;

	if (p_mode != DisplayServerEnums::WINDOW_MODE_FULLSCREEN && p_mode != DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
		AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);
	}

	DisplayServerEnums::WindowID id = p_window_id;
	{
		WindowData *wd_transient_parent = nullptr;
		HWND owner_hwnd = nullptr;
		if (p_parent_hwnd) {
			owner_hwnd = p_parent_hwnd;
		} else if (p_transient_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
			if (!windows.has(p_transient_parent)) {
				ERR_PRINT("Condition \"!windows.has(p_transient_parent)\" is true.");
				p_transient_parent = DisplayServerEnums::INVALID_WINDOW_ID;
			} else {
				wd_transient_parent = &windows[p_transient_parent];
				if (p_exclusive) {
					owner_hwnd = wd_transient_parent->hWnd;
				}
			}
		}

		WindowData &wd = windows[id];

		wd.id = id;
		wd.hWnd = CreateWindowExW(
				dwExStyle,
				L"Engine", L"",
				dwStyle,
				WindowRect.left,
				WindowRect.top,
				WindowRect.right - WindowRect.left,
				WindowRect.bottom - WindowRect.top,
				owner_hwnd,
				nullptr,
				hInstance,
				// tunnel the WindowData we need to handle creation message
				// lifetime is ensured because we are still on the stack when this is
				// processed in the window proc
				reinterpret_cast<void *>(&wd));
		if (!wd.hWnd) {
			MessageBoxW(nullptr, L"Window Creation Error.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
			windows.erase(id);
			ERR_FAIL_V_MSG(ERR_CANT_CREATE, "Failed to create Windows OS window.");
		}

		wd.parent_hwnd = p_parent_hwnd;

		if (has_winrt_queue) {
			wd.wrt_wd = WinRTUtils::create_wd(wd.hWnd, callable_mp(this, &DisplayServerWindows::_winrt_adv_color_info_cb), wd.id);
		}

		// Detach the input queue from the parent window.
		// This prevents the embedded window from waiting on the main window's input queue,
		// causing lags input lags when resizing or moving the main window.
		if (p_parent_hwnd) {
			DWORD mainThreadId = GetWindowThreadProcessId(owner_hwnd, nullptr);
			DWORD embeddedThreadId = GetCurrentThreadId();
			AttachThreadInput(embeddedThreadId, mainThreadId, FALSE);
		}

		if (p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
			wd.fullscreen = true;
			if (p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN) {
				wd.multiwindow_fs = true;
			}
		}

		if (p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
			// Save initial non-fullscreen rect.
			Rect2i srect = screen_get_usable_rect(rq_screen);
			Point2i wpos = p_rect.position;
			if (srect != Rect2i()) {
				wpos = wpos.clamp(srect.position, srect.position + srect.size - p_rect.size / 3);
			}

			wd.pre_fs_rect.left = wpos.x + offset.x;
			wd.pre_fs_rect.right = wpos.x + p_rect.size.x + offset.x;
			wd.pre_fs_rect.top = wpos.y + offset.y;
			wd.pre_fs_rect.bottom = wpos.y + p_rect.size.y + offset.y;
			wd.pre_fs_valid = true;
		}

		wd.exclusive = p_exclusive;
		bool on_top = (p_flags & DisplayServerEnums::WINDOW_FLAG_ALWAYS_ON_TOP_BIT && p_mode != DisplayServerEnums::WINDOW_MODE_FULLSCREEN && p_mode != DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN);
		if (wd_transient_parent) {
			if (on_top) {
				ERR_PRINT("Windows with the 'on top' can't become transient.");
			} else {
				wd.transient_parent = p_transient_parent;
				wd_transient_parent->transient_children.insert(id);
			}
		}

		wd.sharp_corners = p_flags & DisplayServerEnums::WINDOW_FLAG_SHARP_CORNERS_BIT;
		{
			DWORD value = wd.sharp_corners ? DWMWCP_DONOTROUND : DWMWCP_DEFAULT;
			::DwmSetWindowAttribute(wd.hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value));
		}

		if (is_dark_mode_supported() && dark_title_available) {
			BOOL value = is_dark_mode();
			::DwmSetWindowAttribute(wd.hWnd, use_legacy_dark_mode_before_20H1 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
		}

		RegisterTouchWindow(wd.hWnd, 0);
		DragAcceptFiles(wd.hWnd, true);

		if ((tablet_get_current_driver() == "wintab") && wintab_available) {
			wintab_WTInfo(WTI_DEFSYSCTX, 0, &wd.wtlc);
			wd.wtlc.lcOptions |= CXO_MESSAGES;
			wd.wtlc.lcPktData = PK_STATUS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE | PK_ORIENTATION;
			wd.wtlc.lcMoveMask = PK_STATUS | PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE;
			wd.wtlc.lcPktMode = 0;
			wd.wtlc.lcOutOrgX = 0;
			wd.wtlc.lcOutExtX = wd.wtlc.lcInExtX;
			wd.wtlc.lcOutOrgY = 0;
			wd.wtlc.lcOutExtY = -wd.wtlc.lcInExtY;
			wd.wtctx = wintab_WTOpen(wd.hWnd, &wd.wtlc, false);
			if (wd.wtctx) {
				wintab_WTEnable(wd.wtctx, true);
				AXIS pressure;
				if (wintab_WTInfo(WTI_DEVICES + wd.wtlc.lcDevice, DVC_NPRESSURE, &pressure)) {
					wd.min_pressure = int(pressure.axMin);
					wd.max_pressure = int(pressure.axMax);
				}
				AXIS orientation[3];
				if (wintab_WTInfo(WTI_DEVICES + wd.wtlc.lcDevice, DVC_ORIENTATION, &orientation)) {
					wd.tilt_supported = orientation[0].axResolution && orientation[1].axResolution;
				}
			} else {
				print_verbose("WinTab context creation failed.");
			}
		} else {
			wd.wtctx = nullptr;
		}

		if (p_mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED) {
			wd.maximized = true;
			wd.minimized = false;
		}

		if (p_mode == DisplayServerEnums::WINDOW_MODE_MINIMIZED) {
			wd.maximized = false;
			wd.minimized = true;
		}

		wd.last_pressure = 0;
		wd.last_pressure_update = 0;
		wd.last_tilt = Vector2();

		IPropertyStore *prop_store;
		HRESULT hr = SHGetPropertyStoreForWindow(wd.hWnd, IID_IPropertyStore, (void **)&prop_store);
		if (hr == S_OK) {
			PROPVARIANT val;
			String appname;
			if (Engine::is_editor_hint()) {
				appname = "Godot.GodotEditor." + String(VLTR_VERSION_FULL_CONFIG);
			} else {
				String name = GLOBAL_GET("application/config/name");
				String version = GLOBAL_GET("application/config/version");
				if (version.is_empty()) {
					version = "0";
				}
				String clean_app_name = name.to_pascal_case();
				for (int i = 0; i < clean_app_name.length(); i++) {
					if (!is_ascii_alphanumeric_char(clean_app_name[i]) && clean_app_name[i] != '_' && clean_app_name[i] != '.') {
						clean_app_name[i] = '_';
					}
				}
				clean_app_name = clean_app_name.substr(0, 120 - version.length()).trim_suffix(".");
				appname = "Godot." + clean_app_name + "." + version;
			}
			InitPropVariantFromString((PCWSTR)appname.utf16().get_data(), &val);
			prop_store->SetValue(PKEY_AppUserModel_ID, val);
			prop_store->Release();
		}

		// IME.
		wd.im_himc = ImmGetContext(wd.hWnd);
		ImmAssociateContext(wd.hWnd, (HIMC) nullptr);

		wd.im_position = Vector2();

		if (p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED) {
			RECT r;
			GetClientRect(wd.hWnd, &r);
			ClientToScreen(wd.hWnd, (POINT *)&r.left);
			ClientToScreen(wd.hWnd, (POINT *)&r.right);
			wd.last_pos = Point2i(r.left, r.top) - _get_screens_origin();
			wd.width = r.right - r.left - off.x;
			wd.height = r.bottom - r.top - off.y;
		} else {
			wd.last_pos = p_rect.position;
			wd.width = p_rect.size.width;
			wd.height = p_rect.size.height;
		}

		wd.no_redirection_bitmap = p_no_redirection_bitmap;

		wd.create_completed = true;
		// Set size of maximized borderless window (by default it covers the entire screen).
		if (!p_parent_hwnd && p_mode == DisplayServerEnums::WINDOW_MODE_MAXIMIZED && (p_flags & DisplayServerEnums::WINDOW_FLAG_BORDERLESS_BIT)) {
			SetWindowPos(wd.hWnd, HWND_TOP, usable_rect.position.x - off.x, usable_rect.position.y - off.y, usable_rect.size.width + off.x, usable_rect.size.height + off.y, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		_update_window_mouse_passthrough(id);
	}

	return OK;
}

void DisplayServerWindows::_destroy_window(DisplayServerEnums::WindowID p_window_id) {
	WindowData &wd = windows[p_window_id];

	IPropertyStore *prop_store;
	HRESULT hr = SHGetPropertyStoreForWindow(wd.hWnd, IID_IPropertyStore, (void **)&prop_store);
	if (hr == S_OK) {
		PROPVARIANT val;
		PropVariantInit(&val);
		prop_store->SetValue(PKEY_AppUserModel_ID, val);
		prop_store->Release();
	}

	if ((tablet_get_current_driver() == "wintab") && wintab_available && wd.wtctx) {
		wintab_WTClose(wd.wtctx);
		wd.wtctx = nullptr;
	}

	if (wd.drop_target != nullptr) {
		RevokeDragDrop(wd.hWnd);
		wd.drop_target->Release();
	}

	if (wd.icon_big) {
		DestroyIcon(wd.icon_big);
		wd.icon_buffer_big.clear();
		wd.icon_big = nullptr;
	}
	if (wd.icon_small) {
		DestroyIcon(wd.icon_small);
		wd.icon_buffer_small.clear();
		wd.icon_small = nullptr;
	}

	if (has_winrt_queue) {
		WinRTUtils::destroy_wd(wd.wrt_wd);
	}
	DestroyWindow(wd.hWnd);
	windows.erase(p_window_id);
}

#ifdef RD_ENABLED
Error DisplayServerWindows::_create_rendering_context_window(DisplayServerEnums::WindowID p_window_id, const String &p_rendering_driver) {
	DEV_ASSERT(rendering_context != nullptr);

	WindowData &wd = windows[p_window_id];

	union {
#ifdef VULKAN_ENABLED
		RenderingContextDriverVulkan::WindowPlatformData vulkan;
#endif
#ifdef D3D12_ENABLED
		RenderingContextDriverD3D12::WindowPlatformData d3d12;
#endif
	} wpd;
#ifdef VULKAN_ENABLED
	if (p_rendering_driver == "vulkan") {
		wpd.vulkan.window = wd.hWnd;
		wpd.vulkan.instance = hInstance;
	}
#endif
#ifdef D3D12_ENABLED
	if (p_rendering_driver == "d3d12") {
		wpd.d3d12.window = wd.hWnd;
	}
#endif

	Error err = rendering_context->window_create(p_window_id, &wpd);
	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Failed to create %s window.", p_rendering_driver));

	Vector2i off = (wd.multiwindow_fs || (!wd.fullscreen && wd.borderless && wd.maximized)) ? _get_screen_expand_offset(window_get_current_screen(p_window_id)) : Vector2i();
	rendering_context->window_set_size(p_window_id, wd.width + off.x, wd.height + off.y);
	wd.rendering_context_window_created = true;

	return OK;
}

void DisplayServerWindows::_destroy_rendering_context_window(DisplayServerEnums::WindowID p_window_id) {
	DEV_ASSERT(rendering_context != nullptr);

	WindowData &wd = windows[p_window_id];
	DEV_ASSERT(wd.rendering_context_window_created);

	rendering_context->window_destroy(p_window_id);
	wd.rendering_context_window_created = false;
}
#endif

#ifdef GLES3_ENABLED
Error DisplayServerWindows::_create_gl_window(DisplayServerEnums::WindowID p_window_id) {
	if (gl_manager_native) {
		WindowData &wd = windows[p_window_id];

		Error err = gl_manager_native->window_create(p_window_id, wd.hWnd, hInstance, wd.width, wd.height);
		ERR_FAIL_COND_V_MSG(err != OK, err, "Failed to create native OpenGL window.");

		wd.gl_native_window_created = true;
	}
#ifdef ANGLE_ENABLED
	if (gl_manager_angle) {
		WindowData &wd = windows[p_window_id];

		Error err = gl_manager_angle->window_create(p_window_id, nullptr, wd.hWnd, wd.width, wd.height);
		ERR_FAIL_COND_V_MSG(err != OK, err, "Failed to create ANGLE OpenGL window.");

		wd.gl_angle_window_created = true;
	}
#endif

	return OK;
}
#endif

uint32_t DisplayServerWindows::tested_drivers = 0;

// WinTab API.
bool DisplayServerWindows::wintab_available = false;
WTOpenPtr DisplayServerWindows::wintab_WTOpen = nullptr;
WTClosePtr DisplayServerWindows::wintab_WTClose = nullptr;
WTInfoPtr DisplayServerWindows::wintab_WTInfo = nullptr;
WTPacketPtr DisplayServerWindows::wintab_WTPacket = nullptr;
WTEnablePtr DisplayServerWindows::wintab_WTEnable = nullptr;

// UXTheme API.
bool DisplayServerWindows::dark_title_available = false;
bool DisplayServerWindows::use_legacy_dark_mode_before_20H1 = false;
bool DisplayServerWindows::ux_theme_available = false;
ShouldAppsUseDarkModePtr DisplayServerWindows::ShouldAppsUseDarkMode = nullptr;
GetImmersiveColorFromColorSetExPtr DisplayServerWindows::GetImmersiveColorFromColorSetEx = nullptr;
GetImmersiveColorTypeFromNamePtr DisplayServerWindows::GetImmersiveColorTypeFromName = nullptr;
GetImmersiveUserColorSetPreferencePtr DisplayServerWindows::GetImmersiveUserColorSetPreference = nullptr;

Vector2i _get_device_ids_reg(const String &p_device_name) {
	Vector2i out;

	String subkey = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}";
	HKEY hkey = nullptr;
	LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, (LPCWSTR)subkey.utf16().get_data(), 0, KEY_READ, &hkey);
	if (result != ERROR_SUCCESS) {
		return Vector2i();
	}

	DWORD subkeys = 0;
	result = RegQueryInfoKeyW(hkey, nullptr, nullptr, nullptr, &subkeys, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	if (result != ERROR_SUCCESS) {
		RegCloseKey(hkey);
		return Vector2i();
	}
	for (DWORD i = 0; i < subkeys; i++) {
		WCHAR key_name[MAX_PATH] = L"";
		DWORD key_name_size = MAX_PATH;
		result = RegEnumKeyExW(hkey, i, key_name, &key_name_size, nullptr, nullptr, nullptr, nullptr);
		if (result != ERROR_SUCCESS) {
			continue;
		}
		String id = String::utf16((const char16_t *)key_name, key_name_size);
		if (!id.is_empty()) {
			HKEY sub_hkey = nullptr;
			result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, (LPCWSTR)(subkey + "\\" + id).utf16().get_data(), 0, KEY_QUERY_VALUE, &sub_hkey);
			if (result != ERROR_SUCCESS) {
				continue;
			}

			WCHAR buffer[4096];
			DWORD buffer_len = 4096;
			DWORD vtype = REG_SZ;
			if (RegQueryValueExW(sub_hkey, L"DriverDesc", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) != ERROR_SUCCESS || buffer_len == 0) {
				buffer_len = 4096;
				if (RegQueryValueExW(sub_hkey, L"HardwareInformation.AdapterString", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) != ERROR_SUCCESS || buffer_len == 0) {
					RegCloseKey(sub_hkey);
					continue;
				}
			}

			String driver_name = String::utf16((const char16_t *)buffer, buffer_len).strip_edges();
			if (driver_name == p_device_name) {
				String driver_id;

				buffer_len = 4096;
				if (RegQueryValueExW(sub_hkey, L"MatchingDeviceId", nullptr, &vtype, (LPBYTE)buffer, &buffer_len) == ERROR_SUCCESS && buffer_len != 0) {
					driver_id = String::utf16((const char16_t *)buffer, buffer_len).strip_edges();

					Vector<String> id_parts = driver_id.to_lower().split("&");
					for (const String &id_part : id_parts) {
						int ven_off = id_part.find("ven_");
						if (ven_off >= 0) {
							out.x = id_part.substr(ven_off + 4).hex_to_int();
						}
						int dev_off = id_part.find("dev_");
						if (dev_off >= 0) {
							out.y = id_part.substr(dev_off + 4).hex_to_int();
						}
					}

					RegCloseKey(sub_hkey);
					break;
				}
			}
			RegCloseKey(sub_hkey);
		}
	}
	RegCloseKey(hkey);
	return out;
}

Vector2i _get_device_ids_wmi(const String &p_device_name) {
	if (p_device_name.is_empty()) {
		return Vector2i();
	}

	REFCLSID clsid = CLSID_WbemLocator; // Unmarshaler CLSID
	REFIID uuid = IID_IWbemLocator; // Interface UUID
	IWbemLocator *wbemLocator = nullptr; // to get the services
	IWbemServices *wbemServices = nullptr; // to get the class
	IEnumWbemClassObject *iter = nullptr;
	IWbemClassObject *pnpSDriverObject[1]; // contains driver name, version, etc.

	HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, uuid, (LPVOID *)&wbemLocator);
	if (hr != S_OK) {
		return Vector2i();
	}
	BSTR resource_name = SysAllocString(L"root\\CIMV2");
	hr = wbemLocator->ConnectServer(resource_name, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &wbemServices);
	SysFreeString(resource_name);

	SAFE_RELEASE(wbemLocator) // from now on, use `wbemServices`
	if (hr != S_OK) {
		SAFE_RELEASE(wbemServices)
		return Vector2i();
	}

	Vector2i ids;

	const String gpu_device_class_query = vformat("SELECT * FROM Win32_PnPSignedDriver WHERE DeviceName = \"%s\"", p_device_name);
	BSTR query = SysAllocString((const WCHAR *)gpu_device_class_query.utf16().get_data());
	BSTR query_lang = SysAllocString(L"WQL");
	hr = wbemServices->ExecQuery(query_lang, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, nullptr, &iter);
	SysFreeString(query_lang);
	SysFreeString(query);
	if (hr == S_OK) {
		ULONG resultCount;
		hr = iter->Next(5000, 1, pnpSDriverObject, &resultCount); // Get exactly 1. Wait max 5 seconds.

		if (hr == S_OK && resultCount > 0) {
			VARIANT did;
			VariantInit(&did);
			BSTR object_name = SysAllocString(L"DeviceID");
			hr = pnpSDriverObject[0]->Get(object_name, 0, &did, nullptr, nullptr);
			SysFreeString(object_name);
			if (hr == S_OK) {
				String device_id = String(V_BSTR(&did));
				ids.x = device_id.get_slicec('&', 0).lstrip("PCI\\VEN_").hex_to_int();
				ids.y = device_id.get_slicec('&', 1).lstrip("DEV_").hex_to_int();
			}

			for (ULONG i = 0; i < resultCount; i++) {
				SAFE_RELEASE(pnpSDriverObject[i])
			}
		}
	}

	SAFE_RELEASE(wbemServices)
	SAFE_RELEASE(iter)

	return ids;
}

Vector2i _get_device_ids(const String &p_device_name) {
	Vector2i out = _get_device_ids_reg(p_device_name);
	if (out == Vector2i()) {
		out = _get_device_ids_wmi(p_device_name);
	}
	return out;
}

bool DisplayServerWindows::is_dark_mode_supported() const {
	return ux_theme_available;
}

bool DisplayServerWindows::is_dark_mode() const {
	return ux_theme_available && ShouldAppsUseDarkMode();
}

Color DisplayServerWindows::get_accent_color() const {
	if (!ux_theme_available) {
		return Color(0, 0, 0, 0);
	}

	int argb = GetImmersiveColorFromColorSetEx((UINT)GetImmersiveUserColorSetPreference(false, false), GetImmersiveColorTypeFromName(L"ImmersiveSystemAccent"), false, 0);
	return Color((argb & 0xFF) / 255.f, ((argb & 0xFF00) >> 8) / 255.f, ((argb & 0xFF0000) >> 16) / 255.f, ((argb & 0xFF000000) >> 24) / 255.f);
}

Color DisplayServerWindows::get_base_color() const {
	if (!ux_theme_available) {
		return Color(0, 0, 0, 0);
	}

	int argb = GetImmersiveColorFromColorSetEx((UINT)GetImmersiveUserColorSetPreference(false, false), GetImmersiveColorTypeFromName(ShouldAppsUseDarkMode() ? L"ImmersiveDarkChromeMediumLow" : L"ImmersiveLightChromeMediumLow"), false, 0);
	return Color((argb & 0xFF) / 255.f, ((argb & 0xFF00) >> 8) / 255.f, ((argb & 0xFF0000) >> 16) / 255.f, ((argb & 0xFF000000) >> 24) / 255.f);
}

void DisplayServerWindows::set_system_theme_change_callback(const Callable &p_callable) {
	system_theme_changed = p_callable;
}

int DisplayServerWindows::tablet_get_driver_count() const {
	return tablet_drivers.size();
}

String DisplayServerWindows::tablet_get_driver_name(int p_driver) const {
	if (p_driver < 0 || p_driver >= tablet_drivers.size()) {
		return "";
	} else {
		return tablet_drivers[p_driver];
	}
}

String DisplayServerWindows::tablet_get_current_driver() const {
	return tablet_driver;
}

void DisplayServerWindows::tablet_set_current_driver(const String &p_driver) {
	if (tablet_get_driver_count() == 0) {
		return;
	}

	String driver = p_driver;
	if (driver == "auto") {
		if (!winink_disabled) {
			driver = "winink";
		} else if (wintab_available) {
			driver = "wintab";
		} else {
			driver = "dummy";
		}
	}

	bool found = false;
	for (int i = 0; i < tablet_get_driver_count(); i++) {
		if (driver == tablet_get_driver_name(i)) {
			found = true;
		}
	}
	if (found) {
		_update_tablet_ctx(tablet_driver, driver);
		tablet_driver = driver;
	} else {
		ERR_PRINT("Unknown tablet driver " + p_driver + ".");
	}
}

DisplayServerWindows::DisplayServerWindows(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	KeyMappingWindows::initialize();

	tested_drivers.clear();

	drop_events = false;
	key_event_pos = 0;

	hInstance = OS_Windows::get_hinstance();

	pressrc = 0;
	old_invalid = true;
	mouse_mode = DisplayServerEnums::MOUSE_MODE_VISIBLE;

	rendering_driver = p_rendering_driver;

	// Init TTS
	bool tts_enabled = GLOBAL_GET("audio/general/text_to_speech");
	if (tts_enabled) {
		initialize_tts();
	}
	native_menu = memnew(NativeMenuWindows);

	has_winrt_queue = WinRTUtils::create_queue();

	// Enforce default keep screen on value.
	screen_set_keep_on(GLOBAL_GET("display/window/energy_saving/keep_screen_on"));

	// Load Windows version info.
	ZeroMemory(&os_ver, sizeof(OSVERSIONINFOW));
	os_ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);

	HMODULE nt_lib = LoadLibraryW(L"ntdll.dll");
#if defined(ANGLE_ENABLED)
	bool is_wine = false;
#endif
	if (nt_lib) {
		WineGetVersionPtr wine_get_version = (WineGetVersionPtr)(void *)GetProcAddress(nt_lib, "wine_get_version"); // Do not read Windows build number under Wine, it can be set to arbitrary value.
		if (wine_get_version) {
#if defined(ANGLE_ENABLED)
			is_wine = true;
#endif
		} else {
			RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)(void *)GetProcAddress(nt_lib, "RtlGetVersion");
			if (RtlGetVersion) {
				RtlGetVersion(&os_ver);
			}
		}
		FreeLibrary(nt_lib);
	}

	// Load UXTheme.
	if (os_ver.dwBuildNumber >= 10240) { // Not available on Wine, use only if real Windows 10/11 detected.
		HMODULE ux_theme_lib = LoadLibraryW(L"uxtheme.dll");
		if (ux_theme_lib) {
			ShouldAppsUseDarkMode = (ShouldAppsUseDarkModePtr)(void *)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(132));
			GetImmersiveColorFromColorSetEx = (GetImmersiveColorFromColorSetExPtr)(void *)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(95));
			GetImmersiveColorTypeFromName = (GetImmersiveColorTypeFromNamePtr)(void *)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(96));
			GetImmersiveUserColorSetPreference = (GetImmersiveUserColorSetPreferencePtr)(void *)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(98));
			if (os_ver.dwBuildNumber >= 17763) { // Windows 10 Redstone 5 (1809)+ only.
				AllowDarkModeForAppPtr AllowDarkModeForApp = nullptr;
				SetPreferredAppModePtr SetPreferredAppMode = nullptr;
				FlushMenuThemesPtr FlushMenuThemes = nullptr;
				if (os_ver.dwBuildNumber < 18362) { // Windows 10 Redstone 5 (1809) and 19H1 (1903) only.
					AllowDarkModeForApp = (AllowDarkModeForAppPtr)(void *)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(135));
				} else { // Windows 10 19H2 (1909)+ only.
					SetPreferredAppMode = (SetPreferredAppModePtr)(void *)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(135));
					FlushMenuThemes = (FlushMenuThemesPtr)(void *)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(136));
				}
				RefreshImmersiveColorPolicyStatePtr RefreshImmersiveColorPolicyState = (RefreshImmersiveColorPolicyStatePtr)(void *)GetProcAddress(ux_theme_lib, MAKEINTRESOURCEA(104));
				if (ShouldAppsUseDarkMode) {
					bool dark_mode = ShouldAppsUseDarkMode();
					if (SetPreferredAppMode) {
						SetPreferredAppMode(dark_mode ? APPMODE_ALLOWDARK : APPMODE_DEFAULT);
					} else if (AllowDarkModeForApp) {
						AllowDarkModeForApp(dark_mode);
					}
					if (RefreshImmersiveColorPolicyState) {
						RefreshImmersiveColorPolicyState();
					}
					if (FlushMenuThemes) {
						FlushMenuThemes();
					}
				}
			}

			ux_theme_available = ShouldAppsUseDarkMode && GetImmersiveColorFromColorSetEx && GetImmersiveColorTypeFromName && GetImmersiveUserColorSetPreference;
			if (os_ver.dwBuildNumber >= 18363) {
				dark_title_available = true;
				if (os_ver.dwBuildNumber < 19041) {
					use_legacy_dark_mode_before_20H1 = true;
				}
			}
		}
	}

	tablet_drivers.push_back("auto");
	tablet_drivers.push_back("winink");

	// Note: Wacom WinTab driver API for pen input, for devices incompatible with Windows Ink.
	HMODULE wintab_lib = LoadLibraryW(L"wintab32.dll");
	if (wintab_lib) {
		wintab_WTOpen = (WTOpenPtr)(void *)GetProcAddress(wintab_lib, "WTOpenW");
		wintab_WTClose = (WTClosePtr)(void *)GetProcAddress(wintab_lib, "WTClose");
		wintab_WTInfo = (WTInfoPtr)(void *)GetProcAddress(wintab_lib, "WTInfoW");
		wintab_WTPacket = (WTPacketPtr)(void *)GetProcAddress(wintab_lib, "WTPacket");
		wintab_WTEnable = (WTEnablePtr)(void *)GetProcAddress(wintab_lib, "WTEnable");

		wintab_available = wintab_WTOpen && wintab_WTClose && wintab_WTInfo && wintab_WTPacket && wintab_WTEnable;
	}

	if (wintab_available) {
		tablet_drivers.push_back("wintab");
	}

	tablet_drivers.push_back("dummy");

	String wacom_cfg = OS::get_config_path().path_join("WTablet").path_join("Wacom_Tablet.dat");
	if (FileAccess::exists(wacom_cfg)) {
		Ref<XMLParser> parser;
		parser.instantiate();
		if (parser->open(wacom_cfg) == OK) {
			while (parser->read() == OK) {
				if (parser->get_node_type() != XMLParser::NODE_ELEMENT) {
					continue;
				}
				if (parser->get_node_name() == "WinUseInk") {
					parser->read();
					if (parser->get_node_type() == XMLParser::NODE_TEXT) {
						winink_disabled = (parser->get_node_data().to_lower().strip_edges() != "true");
						print_verbose(vformat("Wacom tablet config found at \"%s\", Windows Ink support is %s.", wacom_cfg, winink_disabled ? "disabled" : "enabled"));
						break;
					}
				}
			}
		}
	}

	if (OS::is_hidpi_allowed()) {
		SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
	}

	HMODULE comctl32 = LoadLibraryW(L"comctl32.dll");
	if (comctl32) {
		typedef BOOL(WINAPI * InitCommonControlsExPtr)(const _In_ INITCOMMONCONTROLSEX *picce);
		InitCommonControlsExPtr init_common_controls_ex = (InitCommonControlsExPtr)(void *)GetProcAddress(comctl32, "InitCommonControlsEx");

		// Fails if the incorrect version was loaded. Probably not a big enough deal to print an error about.
		if (init_common_controls_ex) {
			INITCOMMONCONTROLSEX icc = {};
			icc.dwICC = ICC_STANDARD_CLASSES;
			icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
			if (!init_common_controls_ex(&icc)) {
				WARN_PRINT("Unable to initialize Windows common controls. Native dialogs may not work properly.");
			}
		}
		FreeLibrary(comctl32);
	}

	OleInitialize(nullptr);

	HICON default_icon = LoadIconW(GetModuleHandle(nullptr), L"GODOT_ICON");
	if (default_icon == nullptr) {
		default_icon = LoadIcon(nullptr, IDI_WINLOGO);
	}

	memset(&wc, 0, sizeof(WNDCLASSEXW));
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC)::WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance ? hInstance : GetModuleHandle(nullptr);
	wc.hIcon = default_icon;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"Engine";

	if (!RegisterClassExW(&wc)) {
		r_error = ERR_UNAVAILABLE;
		return;
	}

	_register_raw_input_devices(DisplayServerEnums::INVALID_WINDOW_ID);

	String appname;
	if (Engine::is_editor_hint()) {
		appname = "Godot.GodotEditor." + String(VLTR_VERSION_FULL_CONFIG);
	} else {
		String name = GLOBAL_GET("application/config/name");
		String version = GLOBAL_GET("application/config/version");
		if (version.is_empty()) {
			version = "0";
		}
		String clean_app_name = name.to_pascal_case();
		for (int i = 0; i < clean_app_name.length(); i++) {
			if (!is_ascii_alphanumeric_char(clean_app_name[i]) && clean_app_name[i] != '_' && clean_app_name[i] != '.') {
				clean_app_name[i] = '_';
			}
		}
		clean_app_name = clean_app_name.substr(0, 120 - version.length()).trim_suffix(".");
		appname = "Godot." + clean_app_name + "." + version;

#ifndef TOOLS_ENABLED
		// Set for exported projects only.
		HKEY key;
		if (RegOpenKeyW(HKEY_CURRENT_USER_LOCAL_SETTINGS, L"Software\\Microsoft\\Windows\\Shell\\MuiCache", &key) == ERROR_SUCCESS) {
			Char16String cs_name = name.utf16();
			String value_name = OS::get_executable_path().replace_char('/', '\\') + ".FriendlyAppName";
			RegSetValueExW(key, (LPCWSTR)value_name.utf16().get_data(), 0, REG_SZ, (const BYTE *)cs_name.get_data(), cs_name.size() * sizeof(WCHAR));
			RegCloseKey(key);
		}
#endif
	}
	SetCurrentProcessExplicitAppUserModelID((PCWSTR)appname.utf16().get_data());

	mouse_monitor = SetWindowsHookEx(WH_MOUSE, ::MouseProc, nullptr, GetCurrentThreadId());

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

	HWND parent_hwnd = nullptr;
	if (p_parent_window) {
		// Parented window.
		parent_hwnd = (HWND)p_parent_window;
	}

	// Init context and rendering device.
	if (rendering_driver == "dummy") {
		RasterizerDummy::make_current();
	}

#ifdef RD_ENABLED
	bool fallback_to_vulkan = GLOBAL_GET("rendering/rendering_device/fallback_to_vulkan");
	bool fallback_to_d3d12 = GLOBAL_GET("rendering/rendering_device/fallback_to_d3d12");

#ifndef VULKAN_ENABLED
	fallback_to_d3d12 = true; // Always enable fallback if engine was built w/o other driver support.
#endif
#ifndef D3D12_ENABLED
	fallback_to_vulkan = true; // Always enable fallback if engine was built w/o other driver support.
#endif

	String rendering_drivers[2];
	uint32_t rendering_driver_count = 0;

	if (rendering_driver == "d3d12") {
		rendering_drivers[rendering_driver_count++] = rendering_driver;
		if (fallback_to_vulkan) {
			rendering_drivers[rendering_driver_count++] = "vulkan";
		}
	} else if (rendering_driver == "vulkan") {
		rendering_drivers[rendering_driver_count++] = rendering_driver;
		if (fallback_to_d3d12) {
			rendering_drivers[rendering_driver_count++] = "d3d12";
		}
	}

	bool main_window_created = false;
	bool cur_no_redirection_bitmap_value = false;

	for (uint32_t i = 0; i < rendering_driver_count; i++) {
		const String &tested_rendering_driver = rendering_drivers[i];

#ifdef VULKAN_ENABLED
		if (tested_rendering_driver == "vulkan") {
			rendering_context = memnew(RenderingContextDriverVulkan(RenderingContextDriverVulkan::SURFACE_PLATFORM));
			tested_drivers.set_flag(DRIVER_ID_RD_VULKAN);
		}
#endif
#ifdef D3D12_ENABLED
		if (tested_rendering_driver == "d3d12") {
			rendering_context = memnew(RenderingContextDriverD3D12);
			tested_drivers.set_flag(DRIVER_ID_RD_D3D12);
		}
#endif
		if (rendering_context != nullptr) {
			if (rendering_context->initialize() == OK) {
				// The window needs to be recreated when this value differs, because it cannot be added or removed after creation.
#ifdef DCOMP_ENABLED
				bool new_no_redirection_bitmap_value = OS::is_layered_allowed() && tested_rendering_driver == "d3d12";
#else
				bool new_no_redirection_bitmap_value = false;
#endif
				if (cur_no_redirection_bitmap_value != new_no_redirection_bitmap_value) {
					if (main_window_created) {
						_destroy_window(DisplayServerEnums::MAIN_WINDOW_ID);
						main_window_created = false;
					}
					cur_no_redirection_bitmap_value = new_no_redirection_bitmap_value;
				}

				if (!main_window_created) {
					if (_create_window(DisplayServerEnums::MAIN_WINDOW_ID, p_mode, p_flags, Rect2i(window_position, p_resolution), false, DisplayServerEnums::INVALID_WINDOW_ID, parent_hwnd, cur_no_redirection_bitmap_value) != OK) {
						r_error = ERR_UNAVAILABLE;
						ERR_FAIL_MSG("Failed to create main window.");
					}
					main_window_created = true;
				}

				if (_create_rendering_context_window(DisplayServerEnums::MAIN_WINDOW_ID, tested_rendering_driver) == OK) {
					rendering_device = memnew(RenderingDevice);
					if (rendering_device->initialize(rendering_context, DisplayServerEnums::MAIN_WINDOW_ID) == OK) {
#ifdef VULKAN_ENABLED
						if (rendering_driver == "vulkan" && tested_rendering_driver == "d3d12") {
							WARN_PRINT("Your video card drivers seem not to support Vulkan, switching to Direct3D 12.");
						}
#endif
#ifdef D3D12_ENABLED
						if (rendering_driver == "d3d12" && tested_rendering_driver == "vulkan") {
							WARN_PRINT("Your video card drivers seem not to support Direct3D 12, switching to Vulkan.");
						}
#endif
						rendering_driver = tested_rendering_driver;
						OS::set_current_rendering_driver_name(rendering_driver, i > 0 ? OS::RENDERING_SOURCE_FALLBACK : OS::get_current_rendering_driver_name_source());

						break;
					}

					memdelete(rendering_device);
					rendering_device = nullptr;

					_destroy_rendering_context_window(DisplayServerEnums::MAIN_WINDOW_ID);
				}
			}

			memdelete(rendering_context);
			rendering_context = nullptr;
		}
	}

	bool rendering_driver_failed = rendering_driver_count != 0 && rendering_context == nullptr;

#ifdef GLES3_ENABLED
	if (rendering_driver_failed) {
		bool fallback_to_opengl3 = GLOBAL_GET("rendering/rendering_device/fallback_to_opengl3");
		if (fallback_to_opengl3) {
			tested_drivers.set_flag(DRIVER_ID_COMPAT_OPENGL3);
			WARN_PRINT("Your video card drivers seem not to support Direct3D 12 or Vulkan, switching to OpenGL 3.");
			rendering_driver = "opengl3";
			OS::set_current_rendering_method("gl_compatibility", OS::RENDERING_SOURCE_FALLBACK);
			OS::set_current_rendering_driver_name(rendering_driver, OS::RENDERING_SOURCE_FALLBACK);
			rendering_driver_failed = false;
		}
	}
#endif

	if (rendering_driver_failed) {
		r_error = ERR_UNAVAILABLE;
		return;
	}
#endif

#if defined(GLES3_ENABLED)
#if defined(ANGLE_ENABLED)
	bool fallback = GLOBAL_GET("rendering/gl_compatibility/fallback_to_angle");
	bool show_warning = true;
#endif

	if (rendering_driver == "opengl3") {
		// There's no native OpenGL drivers on Windows for ARM, always enable fallback.
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#if defined(ANGLE_ENABLED)
		fallback = true;
		show_warning = false;
#else
		r_error = ERR_UNAVAILABLE;
		ERR_FAIL_MSG("Could not initialize OpenGL.");
#endif
#else
		typedef BOOL(WINAPI * IsWow64Process2Ptr)(HANDLE, USHORT *, USHORT *);

		IsWow64Process2Ptr IsWow64Process2 = (IsWow64Process2Ptr)(void *)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process2");
		if (IsWow64Process2) {
			USHORT process_arch = 0;
			USHORT machine_arch = 0;
			if (!IsWow64Process2(GetCurrentProcess(), &process_arch, &machine_arch)) {
				machine_arch = 0;
			}
			if (machine_arch == 0xAA64) {
#if defined(ANGLE_ENABLED)
				fallback = true;
				show_warning = false;
#else
				r_error = ERR_UNAVAILABLE;
				ERR_FAIL_MSG("Could not initialize OpenGL.");
#endif
			}
		}
#endif
	}

#if defined(ANGLE_ENABLED)
	bool gl_supported = true;
	if (fallback && !is_wine && (rendering_driver == "opengl3")) {
		Dictionary gl_info = detect_wgl();

		bool force_angle = false;
		gl_supported = gl_info["version"].operator int() >= 30003;

		Vector2i device_id = Vector2i(-1, -1);
		Array device_list = GLOBAL_GET("rendering/gl_compatibility/force_angle_on_devices");
		for (int i = 0; i < device_list.size(); i++) {
			const Dictionary &device = device_list[i];
			if (device.has("vendor") && device.has("name")) {
				const String &vendor = device["vendor"];
				const String &name = device["name"];
				if (gl_info["vendor"].operator String().containsn(vendor) && (name == "*" || gl_info["name"].operator String().containsn(name))) {
					// Check vendor/device names.
					force_angle = true;
					break;
				} else if (vendor.begins_with("0x") && name.begins_with("0x")) {
					if (device_id == Vector2i(-1, -1)) {
						// Load device IDs.
						device_id = _get_device_ids(gl_info["name"]);
					}
					if (device_id.x == vendor.lstrip("0x").hex_to_int() && device_id.y == name.lstrip("0x").hex_to_int()) {
						// Check vendor/device IDs.
						force_angle = true;
						break;
					}
				}
			}
		}

		if (force_angle || (gl_info["version"].operator int() < 30003)) {
			tested_drivers.set_flag(DRIVER_ID_COMPAT_OPENGL3);
			if (show_warning) {
				if (gl_info["version"].operator int() < 30003) {
					WARN_PRINT("Your video card drivers seem not to support the required OpenGL 3.3 version, switching to ANGLE.");
				} else {
					WARN_PRINT("Your video card drivers are known to have low quality OpenGL 3.3 support, switching to ANGLE.");
				}
			}
			rendering_driver = "opengl3_angle";
			OS::set_current_rendering_driver_name(rendering_driver, OS::RENDERING_SOURCE_FALLBACK);
		}
	}

	if (rendering_driver == "opengl3_angle") {
		gl_manager_angle = memnew(GLManagerANGLE_Windows);
		tested_drivers.set_flag(DRIVER_ID_COMPAT_ANGLE_D3D11);

		if (gl_manager_angle->initialize() != OK) {
			memdelete(gl_manager_angle);
			gl_manager_angle = nullptr;
			bool fallback_to_native = GLOBAL_GET("rendering/gl_compatibility/fallback_to_native");
			if (fallback_to_native && gl_supported) {
				WARN_PRINT("Your video card drivers seem not to support GLES3 / ANGLE, switching to native OpenGL.");
				rendering_driver = "opengl3";
			} else {
				r_error = ERR_UNAVAILABLE;
				ERR_FAIL_MSG("Could not initialize ANGLE OpenGL.");
			}
		}
	}
#endif // ANGLE_ENABLED
	if (rendering_driver == "opengl3") {
		gl_manager_native = memnew(GLManagerNative_Windows);
		tested_drivers.set_flag(DRIVER_ID_COMPAT_OPENGL3);

		if (gl_manager_native->initialize() != OK) {
			memdelete(gl_manager_native);
			gl_manager_native = nullptr;
			r_error = ERR_UNAVAILABLE;
			ERR_FAIL_MSG("Could not initialize native OpenGL.");
		}
	}
#endif

	bool should_create_main_window = true;
	bool no_redirection_bitmap = false;

#ifdef RD_ENABLED
#ifdef DCOMP_ENABLED
	no_redirection_bitmap = OS::is_layered_allowed() && rendering_driver == "d3d12";
#endif

	// The window may still need to be recreated when all RD backends fail and it falls back to OpenGL.
	if (main_window_created) {
		if (no_redirection_bitmap != cur_no_redirection_bitmap_value) {
			DEV_ASSERT(rendering_context == nullptr);
			_destroy_window(DisplayServerEnums::MAIN_WINDOW_ID);
		} else {
			should_create_main_window = false;
		}
	}
#endif

	if (should_create_main_window) {
		if (_create_window(DisplayServerEnums::MAIN_WINDOW_ID, p_mode, p_flags, Rect2i(window_position, p_resolution), false, DisplayServerEnums::INVALID_WINDOW_ID, parent_hwnd, no_redirection_bitmap) != OK) {
			r_error = ERR_UNAVAILABLE;
			ERR_FAIL_MSG("Failed to create main window.");
		}
	}
	++window_id_counter;

#ifdef GLES3_ENABLED
	if (rendering_driver == "opengl3") {
		if (_create_gl_window(DisplayServerEnums::MAIN_WINDOW_ID) != OK) {
			memdelete(gl_manager_native);
			gl_manager_native = nullptr;
			windows.erase(DisplayServerEnums::MAIN_WINDOW_ID);
			r_error = ERR_UNAVAILABLE;
			return;
		}
		RasterizerGLES3::make_current(true);
	}
#ifdef ANGLE_ENABLED
	if (rendering_driver == "opengl3_angle") {
		if (_create_gl_window(DisplayServerEnums::MAIN_WINDOW_ID) != OK) {
			memdelete(gl_manager_angle);
			gl_manager_angle = nullptr;
			windows.erase(DisplayServerEnums::MAIN_WINDOW_ID);
			r_error = ERR_UNAVAILABLE;
			return;
		}
		RasterizerGLES3::make_current(false);
	}
#endif
#endif

	window_set_vsync_mode(p_vsync_mode, DisplayServerEnums::MAIN_WINDOW_ID);

#ifdef SDL_ENABLED
	joypad_sdl = memnew(JoypadSDL);
	if (joypad_sdl->initialize() != OK) {
		ERR_PRINT("Couldn't initialize SDL joypad input driver.");
		memdelete(joypad_sdl);
		joypad_sdl = nullptr;
	}
#endif

	for (int i = 0; i < DisplayServerEnums::WINDOW_FLAG_MAX; i++) {
		if (p_flags & (1 << i)) {
			window_set_flag(DisplayServerEnums::WindowFlags(i), true, DisplayServerEnums::MAIN_WINDOW_ID);
		}
	}

	windows[DisplayServerEnums::MAIN_WINDOW_ID].initialized = true;

#ifdef ACCESSKIT_ENABLED
	if (accessibility_screen_reader_active()) {
		_THREAD_SAFE_LOCK_
		uint64_t time_wait = OS::get_ticks_msec();
		while (true) {
			MSG msg = {};
			while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}

			uint64_t delta = OS::get_ticks_msec() - time_wait;
			if (delta > 500 || get_object_received) {
				break;
			}
		}
		_THREAD_SAFE_UNLOCK_
	}
#endif

#if defined(RD_ENABLED)
	if (rendering_context) {
		DEV_ASSERT(rendering_device != nullptr);

		rendering_device->screen_create(DisplayServerEnums::MAIN_WINDOW_ID);

		RendererCompositorRD::make_current();
	}
#endif

	if (!Engine::is_editor_hint() && !OS::is_in_low_processor_usage_mode()) {
		// Increase priority for projects that are not in low-processor mode (typically games)
		// to reduce the risk of frame stuttering.
		// This is not done for the editor to prevent importers or resource bakers
		// from making the system unresponsive.
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
		DWORD index = 0;
		HANDLE handle = AvSetMmThreadCharacteristicsW(L"Games", &index);
		if (handle) {
			AvSetMmThreadPriority(handle, AVRT_PRIORITY_CRITICAL);
		}

		// This is needed to make sure that background work does not starve the main thread.
		// This is only setting the priority of this thread, not the whole process.
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	}

	cursor_shape = DisplayServerEnums::CURSOR_ARROW;

	_update_real_mouse_position(DisplayServerEnums::MAIN_WINDOW_ID);

	r_error = OK;

	OS_Windows::set_main_window(windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd);
	Input::set_event_dispatch_function(_dispatch_input_events);
}

Vector<String> DisplayServerWindows::get_rendering_drivers_func() {
	Vector<String> drivers;

#ifdef VULKAN_ENABLED
	drivers.push_back("vulkan");
#endif
#ifdef D3D12_ENABLED
	drivers.push_back("d3d12");
#endif
#ifdef GLES3_ENABLED
	drivers.push_back("opengl3");
#ifdef ANGLE_ENABLED
	drivers.push_back("opengl3_angle");
#endif
#endif
	drivers.push_back("dummy");

	return drivers;
}

DisplayServerDriver *DisplayServerWindows::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	DisplayServerDriver *ds = memnew(DisplayServerWindows(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error));
	if (r_error != OK) {
		if (tested_drivers == 0) {
			OS::alert("Failed to register the window class.", "Unable to initialize DisplayServer");
		} else if (tested_drivers.has_flag(DRIVER_ID_RD_VULKAN) || tested_drivers.has_flag(DRIVER_ID_RD_D3D12)) {
			Vector<String> drivers;
			if (tested_drivers.has_flag(DRIVER_ID_RD_VULKAN)) {
				drivers.push_back("Vulkan");
			}
			if (tested_drivers.has_flag(DRIVER_ID_RD_D3D12)) {
				drivers.push_back("Direct3D 12");
			}
			String executable_name = OS::get_executable_path().get_file();
			OS::alert(
					vformat("Your video card drivers seem not to support the required %s version.\n\n"
							"If possible, consider updating your video card drivers or using the OpenGL 3 driver.\n\n"
							"You can enable the OpenGL 3 driver by starting the engine from the\n"
							"command line with the command:\n\n    \"%s\" --rendering-driver opengl3\n\n"
							"If you have recently updated your video card drivers, try rebooting.",
							String(" or ").join(drivers),
							executable_name),
					"Unable to initialize video driver");
		} else {
			Vector<String> drivers;
			if (tested_drivers.has_flag(DRIVER_ID_COMPAT_OPENGL3)) {
				drivers.push_back("OpenGL 3.3");
			}
			if (tested_drivers.has_flag(DRIVER_ID_COMPAT_ANGLE_D3D11)) {
				drivers.push_back("Direct3D 11");
			}
			OS::alert(
					vformat(
							"Your video card drivers seem not to support the required %s version.\n\n"
							"If possible, consider updating your video card drivers.\n\n"
							"If you have recently updated your video card drivers, try rebooting.",
							String(" or ").join(drivers)),
					"Unable to initialize video driver");
		}
	}
	return ds;
}

void DisplayServerWindows::register_windows_driver() {
	DisplayServer::register_create_function("windows", create_func, get_rendering_drivers_func);
}

DisplayServerWindows::~DisplayServerWindows() {
	LocalVector<List<FileDialogData *>::Element *> to_remove;
	for (List<FileDialogData *>::Element *E = file_dialogs.front(); E; E = E->next()) {
		FileDialogData *fd = E->get();
		if (fd->listener_thread.is_started()) {
			fd->close_requested.set();
			fd->listener_thread.wait_to_finish();
		}
		to_remove.push_back(E);
	}
	for (List<FileDialogData *>::Element *E : to_remove) {
		memdelete(E->get());
		E->erase();
	}

#ifdef SDL_ENABLED
	memdelete(joypad_sdl);
#endif
	touch_state.clear();

	cursors_cache.clear();

	if (has_winrt_queue) {
		WinRTUtils::destroy_queue();
	}

	// Destroy all status indicators.
	for (HashMap<DisplayServerEnums::IndicatorID, IndicatorData>::Iterator E = indicators.begin(); E; ++E) {
		NOTIFYICONDATAW ndat;
		ZeroMemory(&ndat, sizeof(NOTIFYICONDATAW));
		ndat.cbSize = sizeof(NOTIFYICONDATAW);
		ndat.hWnd = windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd;
		ndat.uID = E->key;
		ndat.uVersion = NOTIFYICON_VERSION;

		if (E->value.icon) {
			DestroyIcon(E->value.icon);
			E->value.icon_buffer.clear();
			E->value.icon = nullptr;
		}

		Shell_NotifyIconW(NIM_DELETE, &ndat);
	}

	if (mouse_monitor) {
		UnhookWindowsHookEx(mouse_monitor);
	}

	if (user_proc) {
		SetWindowLongPtr(windows[DisplayServerEnums::MAIN_WINDOW_ID].hWnd, GWLP_WNDPROC, (LONG_PTR)user_proc);
	}

	// Close power request handle.
	screen_set_keep_on(false);

	if (native_menu) {
		NativeMenu::finalize();
		native_menu = nullptr;
	}

#ifdef GLES3_ENABLED
	// destroy windows .. NYI?
	// FIXME wglDeleteContext is never called
#endif

	if (windows.has(DisplayServerEnums::MAIN_WINDOW_ID)) {
#ifdef RD_ENABLED
		if (rendering_device) {
			rendering_device->screen_free(DisplayServerEnums::MAIN_WINDOW_ID);
		}

		if (rendering_context) {
			rendering_context->window_destroy(DisplayServerEnums::MAIN_WINDOW_ID);
		}
#endif
		_destroy_window(DisplayServerEnums::MAIN_WINDOW_ID);
	}

#ifdef RD_ENABLED
	if (rendering_device) {
		memdelete(rendering_device);
		rendering_device = nullptr;
	}

	if (rendering_context) {
		memdelete(rendering_context);
		rendering_context = nullptr;
	}
#endif

	if (restore_mouse_trails > 1) {
		SystemParametersInfoA(SPI_SETMOUSETRAILS, restore_mouse_trails, nullptr, 0);
	}
#ifdef GLES3_ENABLED
#ifdef ANGLE_ENABLED
	if (gl_manager_angle) {
		memdelete(gl_manager_angle);
		gl_manager_angle = nullptr;
	}
#endif
	if (gl_manager_native) {
		memdelete(gl_manager_native);
		gl_manager_native = nullptr;
	}
#endif
	TTS_Windows::finalize();

	OleUninitialize();
}


// --- Merged from platform/windows/key_mapping_windows.cpp ---
#include "servers/display/display_server.h"

#include "core/io/ip.h"

// This provides translation from Windows virtual key codes to Godot and back.
// See WinUser.h and the below for documentation:
// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

HashMap<unsigned int, Key> vk_map;
HashMap<unsigned int, Key> scansym_map;
HashMap<Key, unsigned int> scansym_map_inv;
HashMap<unsigned int, Key> scansym_map_ext;
HashMap<unsigned int, KeyLocation> location_map;

void KeyMappingWindows::initialize() {
	// VK_LBUTTON (0x01)
	// VK_RBUTTON (0x02)
	// VK_CANCEL (0x03)
	// VK_MBUTTON (0x04)
	// VK_XBUTTON1 (0x05)
	// VK_XBUTTON2 (0x06), We have no mappings for the above;as we only map keyboard buttons here.
	// 0x07 is undefined.
	vk_map[VK_BACK] = Key::BACKSPACE; // (0x08)
	vk_map[VK_TAB] = Key::TAB; // (0x09)
	// 0x0A-0B are reserved.
	vk_map[VK_CLEAR] = Key::CLEAR; // (0x0C)
	vk_map[VK_RETURN] = Key::ENTER; // (0x0D)
	// 0x0E-0F are undefined.
	vk_map[VK_SHIFT] = Key::SHIFT; // (0x10)
	vk_map[VK_CONTROL] = Key::CTRL; // (0x11)
	vk_map[VK_MENU] = Key::ALT; // (0x12)
	vk_map[VK_PAUSE] = Key::PAUSE; // (0x13)
	vk_map[VK_CAPITAL] = Key::CAPSLOCK; // (0x14)
	// 0x15-1A are IME keys.
	vk_map[VK_ESCAPE] = Key::ESCAPE; // (0x1B)
	// 0x1C-1F are IME keys.
	vk_map[VK_SPACE] = Key::SPACE; // (0x20)
	vk_map[VK_PRIOR] = Key::PAGEUP; // (0x21)
	vk_map[VK_NEXT] = Key::PAGEDOWN; // (0x22)
	vk_map[VK_END] = Key::END; // (0x23)
	vk_map[VK_HOME] = Key::HOME; // (0x24)
	vk_map[VK_LEFT] = Key::LEFT; // (0x25)
	vk_map[VK_UP] = Key::UP; // (0x26)
	vk_map[VK_RIGHT] = Key::RIGHT; // (0x27)
	vk_map[VK_DOWN] = Key::DOWN; // (0x28)
	// VK_SELECT (0x29), Old select key; e.g. on Digital Equipment Corporation keyboards.
	vk_map[VK_PRINT] = Key::PRINT; // (0x2A), Old IBM key; modern keyboards use VK_SNAPSHOT.
	// VK_EXECUTE (0x2B), Old and uncommon.
	vk_map[VK_SNAPSHOT] = Key::PRINT; // (0x2C)
	vk_map[VK_INSERT] = Key::INSERT; // (0x2D)
	vk_map[VK_DELETE] = Key::KEY_DELETE; // (0x2E)
	vk_map[VK_HELP] = Key::HELP; // (0x2F)
	vk_map[0x30] = Key::KEY_0; // 0 key.
	vk_map[0x31] = Key::KEY_1; // 1 key.
	vk_map[0x32] = Key::KEY_2; // 2 key.
	vk_map[0x33] = Key::KEY_3; // 3 key.
	vk_map[0x34] = Key::KEY_4; // 4 key.
	vk_map[0x35] = Key::KEY_5; // 5 key.
	vk_map[0x36] = Key::KEY_6; // 6 key.
	vk_map[0x37] = Key::KEY_7; // 7 key.
	vk_map[0x38] = Key::KEY_8; // 8 key.
	vk_map[0x39] = Key::KEY_9; // 9 key.
	// 0x3A-40 are undefined.
	vk_map[0x41] = Key::A; // A key.
	vk_map[0x42] = Key::B; // B key.
	vk_map[0x43] = Key::C; // C key.
	vk_map[0x44] = Key::D; // D key.
	vk_map[0x45] = Key::E; // E key.
	vk_map[0x46] = Key::F; // F key.
	vk_map[0x47] = Key::G; // G key.
	vk_map[0x48] = Key::H; // H key.
	vk_map[0x49] = Key::I; // I key
	vk_map[0x4A] = Key::J; // J key.
	vk_map[0x4B] = Key::K; // K key.
	vk_map[0x4C] = Key::L; // L key.
	vk_map[0x4D] = Key::M; // M key.
	vk_map[0x4E] = Key::N; // N key.
	vk_map[0x4F] = Key::O; // O key.
	vk_map[0x50] = Key::P; // P key.
	vk_map[0x51] = Key::Q; // Q key.
	vk_map[0x52] = Key::R; // R key.
	vk_map[0x53] = Key::S; // S key.
	vk_map[0x54] = Key::T; // T key.
	vk_map[0x55] = Key::U; // U key.
	vk_map[0x56] = Key::V; // V key.
	vk_map[0x57] = Key::W; // W key.
	vk_map[0x58] = Key::X; // X key.
	vk_map[0x59] = Key::Y; // Y key.
	vk_map[0x5A] = Key::Z; // Z key.
	vk_map[VK_LWIN] = (Key)Key::META; // (0x5B)
	vk_map[VK_RWIN] = (Key)Key::META; // (0x5C)
	vk_map[VK_APPS] = Key::MENU; // (0x5D)
	// 0x5E is reserved.
	vk_map[VK_SLEEP] = Key::STANDBY; // (0x5F)
	vk_map[VK_NUMPAD0] = Key::KP_0; // (0x60)
	vk_map[VK_NUMPAD1] = Key::KP_1; // (0x61)
	vk_map[VK_NUMPAD2] = Key::KP_2; // (0x62)
	vk_map[VK_NUMPAD3] = Key::KP_3; // (0x63)
	vk_map[VK_NUMPAD4] = Key::KP_4; // (0x64)
	vk_map[VK_NUMPAD5] = Key::KP_5; // (0x65)
	vk_map[VK_NUMPAD6] = Key::KP_6; // (0x66)
	vk_map[VK_NUMPAD7] = Key::KP_7; // (0x67)
	vk_map[VK_NUMPAD8] = Key::KP_8; // (0x68)
	vk_map[VK_NUMPAD9] = Key::KP_9; // (0x69)
	vk_map[VK_MULTIPLY] = Key::KP_MULTIPLY; // (0x6A)
	vk_map[VK_ADD] = Key::KP_ADD; // (0x6B)
	vk_map[VK_SEPARATOR] = Key::KP_PERIOD; // (0x6C)
	vk_map[VK_SUBTRACT] = Key::KP_SUBTRACT; // (0x6D)
	vk_map[VK_DECIMAL] = Key::KP_PERIOD; // (0x6E)
	vk_map[VK_DIVIDE] = Key::KP_DIVIDE; // (0x6F)
	vk_map[VK_F1] = Key::F1; // (0x70)
	vk_map[VK_F2] = Key::F2; // (0x71)
	vk_map[VK_F3] = Key::F3; // (0x72)
	vk_map[VK_F4] = Key::F4; // (0x73)
	vk_map[VK_F5] = Key::F5; // (0x74)
	vk_map[VK_F6] = Key::F6; // (0x75)
	vk_map[VK_F7] = Key::F7; // (0x76)
	vk_map[VK_F8] = Key::F8; // (0x77)
	vk_map[VK_F9] = Key::F9; // (0x78)
	vk_map[VK_F10] = Key::F10; // (0x79)
	vk_map[VK_F11] = Key::F11; // (0x7A)
	vk_map[VK_F12] = Key::F12; // (0x7B)
	vk_map[VK_F13] = Key::F13; // (0x7C)
	vk_map[VK_F14] = Key::F14; // (0x7D)
	vk_map[VK_F15] = Key::F15; // (0x7E)
	vk_map[VK_F16] = Key::F16; // (0x7F)
	vk_map[VK_F17] = Key::F17; // (0x80)
	vk_map[VK_F18] = Key::F18; // (0x81)
	vk_map[VK_F19] = Key::F19; // (0x82)
	vk_map[VK_F20] = Key::F20; // (0x83)
	vk_map[VK_F21] = Key::F21; // (0x84)
	vk_map[VK_F22] = Key::F22; // (0x85)
	vk_map[VK_F23] = Key::F23; // (0x86)
	vk_map[VK_F24] = Key::F24; // (0x87)
	// 0x88-8F are reserved for UI navigation.
	vk_map[VK_NUMLOCK] = Key::NUMLOCK; // (0x90)
	vk_map[VK_SCROLL] = Key::SCROLLLOCK; // (0x91)
	vk_map[VK_OEM_NEC_EQUAL] = Key::EQUAL; // (0x92), OEM NEC PC-9800 numpad '=' key.
	// 0x93-96 are OEM specific (e.g. used by Fujitsu/OASYS);
	// 0x97-9F are unassigned.
	vk_map[VK_LSHIFT] = Key::SHIFT; // (0xA0)
	vk_map[VK_RSHIFT] = Key::SHIFT; // (0xA1)
	vk_map[VK_LCONTROL] = Key::CTRL; // (0xA2)
	vk_map[VK_RCONTROL] = Key::CTRL; // (0xA3)
	vk_map[VK_LMENU] = Key::MENU; // (0xA4)
	vk_map[VK_RMENU] = Key::MENU; // (0xA5)
	vk_map[VK_BROWSER_BACK] = Key::BACK; // (0xA6)
	vk_map[VK_BROWSER_FORWARD] = Key::FORWARD; // (0xA7)
	vk_map[VK_BROWSER_REFRESH] = Key::REFRESH; // (0xA8)
	vk_map[VK_BROWSER_STOP] = Key::STOP; // (0xA9)
	vk_map[VK_BROWSER_SEARCH] = Key::SEARCH; // (0xAA)
	vk_map[VK_BROWSER_FAVORITES] = Key::FAVORITES; // (0xAB)
	vk_map[VK_BROWSER_HOME] = Key::HOMEPAGE; // (0xAC)
	vk_map[VK_VOLUME_MUTE] = Key::VOLUMEMUTE; // (0xAD)
	vk_map[VK_VOLUME_DOWN] = Key::VOLUMEDOWN; // (0xAE)
	vk_map[VK_VOLUME_UP] = Key::VOLUMEUP; // (0xAF)
	vk_map[VK_MEDIA_NEXT_TRACK] = Key::MEDIANEXT; // (0xB0)
	vk_map[VK_MEDIA_PREV_TRACK] = Key::MEDIAPREVIOUS; // (0xB1)
	vk_map[VK_MEDIA_STOP] = Key::MEDIASTOP; // (0xB2)
	vk_map[VK_MEDIA_PLAY_PAUSE] = Key::MEDIAPLAY; // (0xB3), Media button play/pause toggle.
	vk_map[VK_LAUNCH_MAIL] = Key::LAUNCHMAIL; // (0xB4)
	vk_map[VK_LAUNCH_MEDIA_SELECT] = Key::LAUNCHMEDIA; // (0xB5)
	vk_map[VK_LAUNCH_APP1] = Key::LAUNCH0; // (0xB6)
	vk_map[VK_LAUNCH_APP2] = Key::LAUNCH1; // (0xB7)
	// 0xB8-B9 are reserved.
	vk_map[VK_OEM_1] = Key::SEMICOLON; // (0xBA), Misc. character;can vary by keyboard/region. For US standard keyboards;the ';:' key.
	vk_map[VK_OEM_PLUS] = Key::EQUAL; // (0xBB)
	vk_map[VK_OEM_COMMA] = Key::COMMA; // (0xBC)
	vk_map[VK_OEM_MINUS] = Key::MINUS; // (0xBD)
	vk_map[VK_OEM_PERIOD] = Key::PERIOD; // (0xBE)
	vk_map[VK_OEM_2] = Key::SLASH; // (0xBF), For US standard keyboards;the '/?' key.
	vk_map[VK_OEM_3] = Key::QUOTELEFT; // (0xC0), For US standard keyboards;the '`~' key.
	// 0xC1-D7 are reserved. 0xD8-DA are unassigned.
	// 0xC3-DA may be used for old gamepads? Maybe we want to support this? See WinUser.h.
	vk_map[VK_OEM_4] = Key::BRACKETLEFT; // (0xDB),  For US standard keyboards;the '[{' key.
	vk_map[VK_OEM_5] = Key::BACKSLASH; // (0xDC), For US standard keyboards;the '\|' key.
	vk_map[VK_OEM_6] = Key::BRACKETRIGHT; // (0xDD), For US standard keyboards;the ']}' key.
	vk_map[VK_OEM_7] = Key::APOSTROPHE; // (0xDE), For US standard keyboards;single quote/double quote.
	// VK_OEM_8 (0xDF)
	// 0xE0 is reserved. 0xE1 is OEM specific.
	vk_map[VK_OEM_102] = Key::BAR; // (0xE2), Either angle bracket or backslash key on the RT 102-key keyboard.
	vk_map[VK_ICO_HELP] = Key::HELP; // (0xE3)
	// 0xE4 is OEM (e.g. ICO) specific.
	// VK_PROCESSKEY (0xE5), For IME.
	vk_map[VK_ICO_CLEAR] = Key::CLEAR; // (0xE6)
	// VK_PACKET (0xE7), Used to pass Unicode characters as if they were keystrokes.
	// 0xE8 is unassigned.
	// 0xE9-F5 are OEM (Nokia/Ericsson) specific.
	vk_map[VK_ATTN] = Key::ESCAPE; // (0xF6), Old IBM 'ATTN' key used on midrange computers ;e.g. AS/400.
	vk_map[VK_CRSEL] = Key::TAB; // (0xF7), Old IBM 3270 'CrSel' (cursor select) key; used to select data fields.
	// VK_EXSEL (0xF7), Old IBM 3270 extended selection key.
	// VK_EREOF (0xF8), Old IBM 3270 erase to end of field key.
	vk_map[VK_PLAY] = Key::MEDIAPLAY; // (0xFA), Old IBM 3270 'Play' key.
	// VK_ZOOM (0xFB), Old IBM 3290 'Zoom' key.
	// VK_NONAME (0xFC), Reserved.
	// VK_PA1 (0xFD), Old IBM 3270 PA1 key.
	vk_map[VK_OEM_CLEAR] = Key::CLEAR; // (0xFE), OEM specific clear key. Unclear how it differs from normal clear.

	scansym_map[0x00] = Key::PAUSE;
	scansym_map[0x01] = Key::ESCAPE;
	scansym_map[0x02] = Key::KEY_1;
	scansym_map[0x03] = Key::KEY_2;
	scansym_map[0x04] = Key::KEY_3;
	scansym_map[0x05] = Key::KEY_4;
	scansym_map[0x06] = Key::KEY_5;
	scansym_map[0x07] = Key::KEY_6;
	scansym_map[0x08] = Key::KEY_7;
	scansym_map[0x09] = Key::KEY_8;
	scansym_map[0x0A] = Key::KEY_9;
	scansym_map[0x0B] = Key::KEY_0;
	scansym_map[0x0C] = Key::MINUS;
	scansym_map[0x0D] = Key::EQUAL;
	scansym_map[0x0E] = Key::BACKSPACE;
	scansym_map[0x0F] = Key::TAB;
	scansym_map[0x10] = Key::Q;
	scansym_map[0x11] = Key::W;
	scansym_map[0x12] = Key::E;
	scansym_map[0x13] = Key::R;
	scansym_map[0x14] = Key::T;
	scansym_map[0x15] = Key::Y;
	scansym_map[0x16] = Key::U;
	scansym_map[0x17] = Key::I;
	scansym_map[0x18] = Key::O;
	scansym_map[0x19] = Key::P;
	scansym_map[0x1A] = Key::BRACKETLEFT;
	scansym_map[0x1B] = Key::BRACKETRIGHT;
	scansym_map[0x1C] = Key::ENTER;
	scansym_map[0x1D] = Key::CTRL;
	scansym_map[0x1E] = Key::A;
	scansym_map[0x1F] = Key::S;
	scansym_map[0x20] = Key::D;
	scansym_map[0x21] = Key::F;
	scansym_map[0x22] = Key::G;
	scansym_map[0x23] = Key::H;
	scansym_map[0x24] = Key::J;
	scansym_map[0x25] = Key::K;
	scansym_map[0x26] = Key::L;
	scansym_map[0x27] = Key::SEMICOLON;
	scansym_map[0x28] = Key::APOSTROPHE;
	scansym_map[0x29] = Key::QUOTELEFT;
	scansym_map[0x2A] = Key::SHIFT;
	scansym_map[0x2B] = Key::BACKSLASH;
	scansym_map[0x2C] = Key::Z;
	scansym_map[0x2D] = Key::X;
	scansym_map[0x2E] = Key::C;
	scansym_map[0x2F] = Key::V;
	scansym_map[0x30] = Key::B;
	scansym_map[0x31] = Key::N;
	scansym_map[0x32] = Key::M;
	scansym_map[0x33] = Key::COMMA;
	scansym_map[0x34] = Key::PERIOD;
	scansym_map[0x35] = Key::SLASH;
	scansym_map[0x36] = Key::SHIFT;
	scansym_map[0x37] = Key::KP_MULTIPLY;
	scansym_map[0x38] = Key::ALT;
	scansym_map[0x39] = Key::SPACE;
	scansym_map[0x3A] = Key::CAPSLOCK;
	scansym_map[0x3B] = Key::F1;
	scansym_map[0x3C] = Key::F2;
	scansym_map[0x3D] = Key::F3;
	scansym_map[0x3E] = Key::F4;
	scansym_map[0x3F] = Key::F5;
	scansym_map[0x40] = Key::F6;
	scansym_map[0x41] = Key::F7;
	scansym_map[0x42] = Key::F8;
	scansym_map[0x43] = Key::F9;
	scansym_map[0x44] = Key::F10;
	scansym_map[0x45] = Key::NUMLOCK;
	scansym_map[0x46] = Key::SCROLLLOCK;
	scansym_map[0x47] = Key::KP_7;
	scansym_map[0x48] = Key::KP_8;
	scansym_map[0x49] = Key::KP_9;
	scansym_map[0x4A] = Key::KP_SUBTRACT;
	scansym_map[0x4B] = Key::KP_4;
	scansym_map[0x4C] = Key::KP_5;
	scansym_map[0x4D] = Key::KP_6;
	scansym_map[0x4E] = Key::KP_ADD;
	scansym_map[0x4F] = Key::KP_1;
	scansym_map[0x50] = Key::KP_2;
	scansym_map[0x51] = Key::KP_3;
	scansym_map[0x52] = Key::KP_0;
	scansym_map[0x53] = Key::KP_PERIOD;
	scansym_map[0x56] = Key::SECTION;
	scansym_map[0x57] = Key::F11;
	scansym_map[0x58] = Key::F12;
	scansym_map[0x5B] = Key::META;
	scansym_map[0x5C] = Key::META;
	scansym_map[0x5D] = Key::MENU;
	scansym_map[0x64] = Key::F13;
	scansym_map[0x65] = Key::F14;
	scansym_map[0x66] = Key::F15;
	scansym_map[0x67] = Key::F16;
	scansym_map[0x68] = Key::F17;
	scansym_map[0x69] = Key::F18;
	scansym_map[0x6A] = Key::F19;
	scansym_map[0x6B] = Key::F20;
	scansym_map[0x6C] = Key::F21;
	scansym_map[0x6D] = Key::F22;
	scansym_map[0x6E] = Key::F23;
	//	scansym_map[0x71] = Key::JIS_KANA;
	//	scansym_map[0x72] = Key::JIS_EISU;
	scansym_map[0x76] = Key::F24;

	for (const KeyValue<unsigned int, Key> &E : scansym_map) {
		scansym_map_inv[E.value] = E.key;
	}

	scansym_map_ext[0x09] = Key::MENU;
	scansym_map_ext[0x10] = Key::MEDIAPREVIOUS;
	scansym_map_ext[0x19] = Key::MEDIANEXT;
	scansym_map_ext[0x1C] = Key::KP_ENTER;
	scansym_map_ext[0x20] = Key::VOLUMEMUTE;
	scansym_map_ext[0x21] = Key::LAUNCH1;
	scansym_map_ext[0x22] = Key::MEDIAPLAY;
	scansym_map_ext[0x24] = Key::MEDIASTOP;
	scansym_map_ext[0x2E] = Key::VOLUMEDOWN;
	scansym_map_ext[0x30] = Key::VOLUMEUP;
	scansym_map_ext[0x32] = Key::HOMEPAGE;
	scansym_map_ext[0x35] = Key::KP_DIVIDE;
	scansym_map_ext[0x37] = Key::PRINT;
	scansym_map_ext[0x3A] = Key::KP_ADD;
	scansym_map_ext[0x45] = Key::NUMLOCK;
	scansym_map_ext[0x47] = Key::HOME;
	scansym_map_ext[0x48] = Key::UP;
	scansym_map_ext[0x49] = Key::PAGEUP;
	scansym_map_ext[0x4A] = Key::KP_SUBTRACT;
	scansym_map_ext[0x4B] = Key::LEFT;
	scansym_map_ext[0x4C] = Key::KP_5;
	scansym_map_ext[0x4D] = Key::RIGHT;
	scansym_map_ext[0x4E] = Key::KP_ADD;
	scansym_map_ext[0x4F] = Key::END;
	scansym_map_ext[0x50] = Key::DOWN;
	scansym_map_ext[0x51] = Key::PAGEDOWN;
	scansym_map_ext[0x52] = Key::INSERT;
	scansym_map_ext[0x53] = Key::KEY_DELETE;
	scansym_map_ext[0x5D] = Key::MENU;
	scansym_map_ext[0x5F] = Key::STANDBY;
	scansym_map_ext[0x65] = Key::SEARCH;
	scansym_map_ext[0x66] = Key::FAVORITES;
	scansym_map_ext[0x67] = Key::REFRESH;
	scansym_map_ext[0x68] = Key::STOP;
	scansym_map_ext[0x69] = Key::FORWARD;
	scansym_map_ext[0x6A] = Key::BACK;
	scansym_map_ext[0x6B] = Key::LAUNCH0;
	scansym_map_ext[0x6C] = Key::LAUNCHMAIL;
	scansym_map_ext[0x6D] = Key::LAUNCHMEDIA;
	scansym_map_ext[0x78] = Key::MEDIARECORD;

	// Scancode to physical location map.
	// Shift.
	location_map[0x2A] = KeyLocation::LEFT;
	location_map[0x36] = KeyLocation::RIGHT;
	// Meta.
	location_map[0x5B] = KeyLocation::LEFT;
	location_map[0x5C] = KeyLocation::RIGHT;
	// Ctrl and Alt must be handled differently.
}

Key KeyMappingWindows::get_keysym(unsigned int p_code) {
	const Key *key = vk_map.getptr(p_code);
	if (key) {
		return *key;
	}
	return Key::UNKNOWN;
}

unsigned int KeyMappingWindows::get_scancode(Key p_keycode) {
	const unsigned int *key = scansym_map_inv.getptr(p_keycode);
	if (key) {
		return *key;
	}
	return 0;
}

Key KeyMappingWindows::get_scansym(unsigned int p_code, bool p_extended) {
	if (p_extended) {
		const Key *key = scansym_map_ext.getptr(p_code);
		if (key) {
			return *key;
		}
	}
	const Key *key = scansym_map.getptr(p_code);
	if (key) {
		return *key;
	}
	return Key::NONE;
}

bool KeyMappingWindows::is_extended_key(unsigned int p_code) {
	return p_code == VK_INSERT ||
			p_code == VK_DELETE ||
			p_code == VK_HOME ||
			p_code == VK_END ||
			p_code == VK_PRIOR ||
			p_code == VK_NEXT ||
			p_code == VK_LEFT ||
			p_code == VK_UP ||
			p_code == VK_RIGHT ||
			p_code == VK_DOWN;
}

KeyLocation KeyMappingWindows::get_location(unsigned int p_code, bool p_extended) {
	// Right- ctrl and alt have the same scancode as left, but are in the extended keys.
	const Key *key = scansym_map.getptr(p_code);
	if (key && (*key == Key::CTRL || *key == Key::ALT)) {
		return p_extended ? KeyLocation::RIGHT : KeyLocation::LEFT;
	}
	const KeyLocation *location = location_map.getptr(p_code);
	if (location) {
		return *location;
	}
	return KeyLocation::UNSPECIFIED;
}


// --- Merged from platform/windows/native_menu_windows.cpp ---
#include "servers/display/display_server.h"



HBITMAP NativeMenuWindows::_make_bitmap(const Ref<Image> &p_img) const {
	p_img->convert(Image::FORMAT_RGBA8);

	Vector2i texture_size = p_img->get_size();
	UINT image_size = texture_size.width * texture_size.height;

	COLORREF *buffer = nullptr;

	BITMAPV5HEADER bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.bV5Size = sizeof(bi);
	bi.bV5Width = texture_size.width;
	bi.bV5Height = -texture_size.height;
	bi.bV5Planes = 1;
	bi.bV5BitCount = 32;
	bi.bV5Compression = BI_BITFIELDS;
	bi.bV5RedMask = 0x00ff0000;
	bi.bV5GreenMask = 0x0000ff00;
	bi.bV5BlueMask = 0x000000ff;
	bi.bV5AlphaMask = 0xff000000;

	HDC dc = GetDC(nullptr);
	HBITMAP bitmap = CreateDIBSection(dc, reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS, reinterpret_cast<void **>(&buffer), nullptr, 0);
	for (UINT index = 0; index < image_size; index++) {
		int row_index = std::floor(index / texture_size.width);
		int column_index = (index % int(texture_size.width));
		const Color &c = p_img->get_pixel(column_index, row_index);
		*(buffer + index) = c.to_argb32();
	}
	ReleaseDC(nullptr, dc);

	return bitmap;
}

void NativeMenuWindows::_menu_activate(HMENU p_menu, int p_index) const {
	if (menu_lookup.has(p_menu)) {
		MenuData *md = menus.get_or_null(menu_lookup[p_menu]);
		if (md) {
			int count = GetMenuItemCount(md->menu);
			if (p_index >= 0 && p_index < count) {
				MENUITEMINFOW item;
				ZeroMemory(&item, sizeof(item));
				item.cbSize = sizeof(item);
				item.fMask = MIIM_STATE | MIIM_DATA;
				if (GetMenuItemInfoW(md->menu, p_index, true, &item)) {
					MenuItemData *item_data = (MenuItemData *)item.dwItemData;
					if (item_data) {
						if (item_data->callback.is_valid()) {
							Variant ret;
							Callable::CallError ce;
							const Variant *args[1] = { &item_data->meta };

							item_data->callback.callp(args, 1, ret, ce);
							if (ce.error != Callable::CallError::CALL_OK) {
								ERR_PRINT(vformat("Failed to execute menu callback: %s.", Variant::get_callable_error_text(item_data->callback, args, 1, ce)));
							}
						}
					}
				}
			}
		}
	}
}

bool NativeMenuWindows::has_feature(Feature p_feature) const {
	switch (p_feature) {
		// case FEATURE_GLOBAL_MENU:
		// case FEATURE_OPEN_CLOSE_CALLBACK:
		// case FEATURE_HOVER_CALLBACK:
		// case FEATURE_KEY_CALLBACK:
		case FEATURE_POPUP_MENU:
			return true;
		default:
			return false;
	}
}

bool NativeMenuWindows::has_system_menu(SystemMenus p_menu_id) const {
	return false;
}

RID NativeMenuWindows::get_system_menu(SystemMenus p_menu_id) const {
	return RID();
}

RID NativeMenuWindows::create_menu() {
	MenuData *md = memnew(MenuData);
	md->menu = CreatePopupMenu();

	MENUINFO menu_info;
	ZeroMemory(&menu_info, sizeof(menu_info));
	menu_info.cbSize = sizeof(menu_info);
	menu_info.fMask = MIM_STYLE;
	menu_info.dwStyle = MNS_NOTIFYBYPOS;
	SetMenuInfo(md->menu, &menu_info);

	RID rid = menus.make_rid(md);
	menu_lookup[md->menu] = rid;
	return rid;
}

bool NativeMenuWindows::has_menu(const RID &p_rid) const {
	return menus.owns(p_rid);
}

void NativeMenuWindows::free_menu(const RID &p_rid) {
	MenuData *md = menus.get_or_null(p_rid);
	if (md) {
		clear(p_rid);
		DestroyMenu(md->menu);
		menus.free(p_rid);
		menu_lookup.erase(md->menu);
		memdelete(md);
	}
}

Size2 NativeMenuWindows::get_size(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Size2());

	Size2 size;
	int count = GetMenuItemCount(md->menu);
	for (int i = 0; i < count; i++) {
		RECT rect;
		if (GetMenuItemRect(nullptr, md->menu, i, &rect)) {
			size.x = MAX(size.x, rect.right - rect.left);
			size.y += rect.bottom - rect.top;
		}
	}
	return size;
}

void NativeMenuWindows::popup(const RID &p_rid, const Vector2i &p_position) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	HWND hwnd = (HWND)DisplayServer::window_get_native_handle(DisplayServerEnums::WINDOW_HANDLE, DisplayServerEnums::MAIN_WINDOW_ID);
	UINT flags = TPM_HORIZONTAL | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_VERPOSANIMATION;
	if (md->is_rtl) {
		flags |= TPM_LAYOUTRTL;
	}
	SetForegroundWindow(hwnd);
	TrackPopupMenuEx(md->menu, flags, p_position.x, p_position.y, hwnd, nullptr);

	if (md->close_cb.is_valid()) {
		Variant ret;
		Callable::CallError ce;
		md->close_cb.callp(nullptr, 0, ret, ce);
		if (ce.error != Callable::CallError::CALL_OK) {
			ERR_PRINT(vformat("Failed to execute popup close callback: %s.", Variant::get_callable_error_text(md->close_cb, nullptr, 0, ce)));
		}
	}

	PostMessage(hwnd, WM_NULL, 0, 0);
}

void NativeMenuWindows::set_interface_direction(const RID &p_rid, bool p_is_rtl) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	if (md->is_rtl == p_is_rtl) {
		return;
	}
	md->is_rtl = p_is_rtl;
}

void NativeMenuWindows::set_popup_open_callback(const RID &p_rid, const Callable &p_callback) {
	// Not supported.
}

Callable NativeMenuWindows::get_popup_open_callback(const RID &p_rid) const {
	// Not supported.
	return Callable();
}

void NativeMenuWindows::set_popup_close_callback(const RID &p_rid, const Callable &p_callback) {
	MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	md->close_cb = p_callback;
}

Callable NativeMenuWindows::get_popup_close_callback(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Callable());

	return md->close_cb;
}

void NativeMenuWindows::set_minimum_width(const RID &p_rid, float p_width) {
	// Not supported.
}

float NativeMenuWindows::get_minimum_width(const RID &p_rid) const {
	// Not supported.
	return 0.f;
}

bool NativeMenuWindows::is_opened(const RID &p_rid) const {
	// Not supported.
	return false;
}

int NativeMenuWindows::add_submenu_item(const RID &p_rid, const String &p_label, const RID &p_submenu_rid, const Variant &p_tag, int p_index) {
	MenuData *md = menus.get_or_null(p_rid);
	MenuData *md_sub = menus.get_or_null(p_submenu_rid);
	ERR_FAIL_NULL_V(md, -1);
	ERR_FAIL_NULL_V(md_sub, -1);
	ERR_FAIL_COND_V_MSG(md->menu == md_sub->menu, -1, "Can't set submenu to self!");

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->meta = p_tag;
	item_data->checkable_type = CHECKABLE_TYPE_NONE;
	item_data->max_states = 0;
	item_data->state = 0;

	Char16String label = p_label.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA | MIIM_SUBMENU;
	item.fType = MFT_STRING;
	item.hSubMenu = md_sub->menu;
	item.dwItemData = (ULONG_PTR)item_data;
	item.dwTypeData = (LPWSTR)label.get_data();

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::add_item(const RID &p_rid, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->callback = p_callback;
	item_data->meta = p_tag;
	item_data->checkable_type = CHECKABLE_TYPE_NONE;
	item_data->max_states = 0;
	item_data->state = 0;

	Char16String label = p_label.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA;
	item.fType = MFT_STRING;
	item.dwItemData = (ULONG_PTR)item_data;
	item.dwTypeData = (LPWSTR)label.get_data();

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::add_check_item(const RID &p_rid, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->callback = p_callback;
	item_data->meta = p_tag;
	item_data->checkable_type = CHECKABLE_TYPE_CHECK_BOX;
	item_data->max_states = 0;
	item_data->state = 0;

	Char16String label = p_label.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA;
	item.fType = MFT_STRING;
	item.dwItemData = (ULONG_PTR)item_data;
	item.dwTypeData = (LPWSTR)label.get_data();

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::add_icon_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->callback = p_callback;
	item_data->meta = p_tag;
	item_data->checkable_type = CHECKABLE_TYPE_NONE;
	item_data->max_states = 0;
	item_data->state = 0;
	if (p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
		item_data->img = p_icon->get_image();
		item_data->img = item_data->img->duplicate();
		if (item_data->img->is_compressed()) {
			item_data->img->decompress();
		}
		item_data->bmp = _make_bitmap(item_data->img);
	}

	Char16String label = p_label.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA | MIIM_BITMAP;
	item.fType = MFT_STRING;
	item.dwItemData = (ULONG_PTR)item_data;
	item.dwTypeData = (LPWSTR)label.get_data();
	item.hbmpItem = item_data->bmp;

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::add_icon_check_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->callback = p_callback;
	item_data->meta = p_tag;
	item_data->checkable_type = CHECKABLE_TYPE_CHECK_BOX;
	item_data->max_states = 0;
	item_data->state = 0;
	if (p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
		item_data->img = p_icon->get_image();
		item_data->img = item_data->img->duplicate();
		if (item_data->img->is_compressed()) {
			item_data->img->decompress();
		}
		item_data->bmp = _make_bitmap(item_data->img);
	}

	Char16String label = p_label.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA | MIIM_BITMAP;
	item.fType = MFT_STRING;
	item.dwItemData = (ULONG_PTR)item_data;
	item.dwTypeData = (LPWSTR)label.get_data();
	item.hbmpItem = item_data->bmp;

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::add_radio_check_item(const RID &p_rid, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->callback = p_callback;
	item_data->meta = p_tag;
	item_data->checkable_type = CHECKABLE_TYPE_RADIO_BUTTON;
	item_data->max_states = 0;
	item_data->state = 0;

	Char16String label = p_label.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA;
	item.fType = MFT_STRING | MFT_RADIOCHECK;
	item.dwItemData = (ULONG_PTR)item_data;
	item.dwTypeData = (LPWSTR)label.get_data();

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::add_icon_radio_check_item(const RID &p_rid, const Ref<Texture2D> &p_icon, const String &p_label, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->callback = p_callback;
	item_data->meta = p_tag;
	item_data->checkable_type = CHECKABLE_TYPE_RADIO_BUTTON;
	item_data->max_states = 0;
	item_data->state = 0;
	if (p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
		item_data->img = p_icon->get_image();
		item_data->img = item_data->img->duplicate();
		if (item_data->img->is_compressed()) {
			item_data->img->decompress();
		}
		item_data->bmp = _make_bitmap(item_data->img);
	}

	Char16String label = p_label.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA | MIIM_BITMAP;
	item.fType = MFT_STRING | MFT_RADIOCHECK;
	item.dwItemData = (ULONG_PTR)item_data;
	item.dwTypeData = (LPWSTR)label.get_data();
	item.hbmpItem = item_data->bmp;

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::add_multistate_item(const RID &p_rid, const String &p_label, int p_max_states, int p_default_state, const Callable &p_callback, const Callable &p_key_callback, const Variant &p_tag, Key p_accel, int p_index) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->callback = p_callback;
	item_data->meta = p_tag;
	item_data->checkable_type = CHECKABLE_TYPE_NONE;
	item_data->max_states = p_max_states;
	item_data->state = p_default_state;

	Char16String label = p_label.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA;
	item.fType = MFT_STRING;
	item.dwItemData = (ULONG_PTR)item_data;
	item.dwTypeData = (LPWSTR)label.get_data();

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::add_separator(const RID &p_rid, int p_index) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	if (p_index == -1) {
		p_index = GetMenuItemCount(md->menu);
	} else {
		p_index = CLAMP(p_index, 0, GetMenuItemCount(md->menu));
	}

	MenuItemData *item_data = memnew(MenuItemData);
	item_data->checkable_type = CHECKABLE_TYPE_NONE;
	item_data->max_states = 0;
	item_data->state = 0;

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_DATA;
	item.fType = MFT_SEPARATOR;
	item.dwItemData = (ULONG_PTR)item_data;

	if (!InsertMenuItemW(md->menu, p_index, true, &item)) {
		memdelete(item_data);
		return -1;
	}
	return p_index;
}

int NativeMenuWindows::find_item_index_with_text(const RID &p_rid, const String &p_text) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	MENUITEMINFOW item;
	int count = GetMenuItemCount(md->menu);
	for (int i = 0; i < count; i++) {
		ZeroMemory(&item, sizeof(item));
		item.cbSize = sizeof(item);
		item.fMask = MIIM_STRING;
		item.dwTypeData = nullptr;
		if (GetMenuItemInfoW(md->menu, i, true, &item)) {
			item.cch++;
			Char16String str;
			str.resize_uninitialized(item.cch);
			item.dwTypeData = (LPWSTR)str.ptrw();
			if (GetMenuItemInfoW(md->menu, i, true, &item)) {
				if (String::utf16((const char16_t *)str.get_data()) == p_text) {
					return i;
				}
			}
		}
	}
	return -1;
}

int NativeMenuWindows::find_item_index_with_tag(const RID &p_rid, const Variant &p_tag) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);

	MENUITEMINFOW item;
	int count = GetMenuItemCount(md->menu);
	for (int i = 0; i < count; i++) {
		ZeroMemory(&item, sizeof(item));
		item.cbSize = sizeof(item);
		item.fMask = MIIM_DATA;
		if (GetMenuItemInfoW(md->menu, i, true, &item)) {
			MenuItemData *item_data = (MenuItemData *)item.dwItemData;
			if (item_data) {
				if (item_data->meta == p_tag) {
					return i;
				}
			}
		}
	}
	return -1;
}

bool NativeMenuWindows::is_item_checked(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, false);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_STATE | MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			return item_data->checked;
		}
	}
	return false;
}

bool NativeMenuWindows::is_item_checkable(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, false);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			return item_data->checkable_type == CHECKABLE_TYPE_CHECK_BOX;
		}
	}
	return false;
}

bool NativeMenuWindows::is_item_radio_checkable(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, false);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			return item_data->checkable_type == CHECKABLE_TYPE_RADIO_BUTTON;
		}
	}
	return false;
}

Callable NativeMenuWindows::get_item_callback(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Callable());
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Callable());
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, Callable());

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			return item_data->callback;
		}
	}
	return Callable();
}

Callable NativeMenuWindows::get_item_key_callback(const RID &p_rid, int p_idx) const {
	// Not supported.
	return Callable();
}

Variant NativeMenuWindows::get_item_tag(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Variant());
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Variant());
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, Variant());

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			return item_data->meta;
		}
	}
	return Variant();
}

String NativeMenuWindows::get_item_text(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, String());
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, String());
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, String());

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_STRING;
	item.dwTypeData = nullptr;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		item.cch++;
		Char16String str;
		str.resize_uninitialized(item.cch);
		item.dwTypeData = (LPWSTR)str.ptrw();
		if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
			return String::utf16((const char16_t *)str.get_data());
		}
	}
	return String();
}

RID NativeMenuWindows::get_item_submenu(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, RID());
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, RID());
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, RID());

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_SUBMENU;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		if (menu_lookup.has(item.hSubMenu)) {
			return menu_lookup[item.hSubMenu];
		}
	}
	return RID();
}

Key NativeMenuWindows::get_item_accelerator(const RID &p_rid, int p_idx) const {
	// Not supported.
	return Key::NONE;
}

bool NativeMenuWindows::is_item_disabled(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, false);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, false);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, false);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_STATE;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		return (item.fState & MFS_DISABLED) == MFS_DISABLED;
	}
	return false;
}

bool NativeMenuWindows::is_item_hidden(const RID &p_rid, int p_idx) const {
	// Not supported.
	return false;
}

String NativeMenuWindows::get_item_tooltip(const RID &p_rid, int p_idx) const {
	// Not supported.
	return String();
}

int NativeMenuWindows::get_item_state(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, -1);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, -1);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			return item_data->state;
		}
	}
	return -1;
}

int NativeMenuWindows::get_item_max_states(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, -1);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, -1);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			return item_data->max_states;
		}
	}
	return -1;
}

Ref<Texture2D> NativeMenuWindows::get_item_icon(const RID &p_rid, int p_idx) const {
	ERR_FAIL_COND_V(p_idx < 0, Ref<Texture2D>());
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, Ref<Texture2D>());
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, Ref<Texture2D>());

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			return ImageTexture::create_from_image(item_data->img);
		}
	}
	return Ref<Texture2D>();
}

int NativeMenuWindows::get_item_indentation_level(const RID &p_rid, int p_idx) const {
	// Not supported.
	return 0;
}

void NativeMenuWindows::set_item_checked(const RID &p_rid, int p_idx, bool p_checked) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_STATE | MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			item_data->checked = p_checked;
			if (p_checked) {
				item.fState |= MFS_CHECKED;
			} else {
				item.fState &= ~MFS_CHECKED;
			}
		}
		SetMenuItemInfoW(md->menu, p_idx, true, &item);
	}
}

void NativeMenuWindows::set_item_checkable(const RID &p_rid, int p_idx, bool p_checkable) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			item.fType &= ~MFT_RADIOCHECK;
			item_data->checkable_type = (p_checkable) ? CHECKABLE_TYPE_CHECK_BOX : CHECKABLE_TYPE_NONE;
			SetMenuItemInfoW(md->menu, p_idx, true, &item);
		}
	}
}

void NativeMenuWindows::set_item_radio_checkable(const RID &p_rid, int p_idx, bool p_checkable) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			if (p_checkable) {
				item.fType |= MFT_RADIOCHECK;
				item_data->checkable_type = CHECKABLE_TYPE_CHECK_BOX;
			} else {
				item.fType &= ~MFT_RADIOCHECK;
				item_data->checkable_type = CHECKABLE_TYPE_NONE;
			}
			SetMenuItemInfoW(md->menu, p_idx, true, &item);
		}
	}
}

void NativeMenuWindows::set_item_callback(const RID &p_rid, int p_idx, const Callable &p_callback) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			item_data->callback = p_callback;
		}
	}
}

void NativeMenuWindows::set_item_key_callback(const RID &p_rid, int p_idx, const Callable &p_key_callback) {
	// Not supported.
}

void NativeMenuWindows::set_item_hover_callbacks(const RID &p_rid, int p_idx, const Callable &p_callback) {
	// Not supported.
}

void NativeMenuWindows::set_item_tag(const RID &p_rid, int p_idx, const Variant &p_tag) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			item_data->meta = p_tag;
		}
	}
}

void NativeMenuWindows::set_item_text(const RID &p_rid, int p_idx, const String &p_text) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	Char16String label = p_text.utf16();
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		item.dwTypeData = (LPWSTR)label.get_data();
		SetMenuItemInfoW(md->menu, p_idx, true, &item);
	}
}

void NativeMenuWindows::set_item_submenu(const RID &p_rid, int p_idx, const RID &p_submenu_rid) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_SUBMENU;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		if (p_submenu_rid.is_valid()) {
			MenuData *md_sub = menus.get_or_null(p_submenu_rid);
			ERR_FAIL_NULL(md_sub);
			ERR_FAIL_COND_MSG(md->menu == md_sub->menu, "Can't set submenu to self!");
			item.hSubMenu = md_sub->menu;
		} else {
			item.hSubMenu = nullptr;
		}
		SetMenuItemInfoW(md->menu, p_idx, true, &item);
	}
}

void NativeMenuWindows::set_item_accelerator(const RID &p_rid, int p_idx, Key p_keycode) {
	// Not supported.
}

void NativeMenuWindows::set_item_disabled(const RID &p_rid, int p_idx, bool p_disabled) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_STATE;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		if (p_disabled) {
			item.fState |= MFS_DISABLED;
		} else {
			item.fState &= ~MFS_DISABLED;
		}
		SetMenuItemInfoW(md->menu, p_idx, true, &item);
	}
}

void NativeMenuWindows::set_item_hidden(const RID &p_rid, int p_idx, bool p_hidden) {
	// Not supported.
}

void NativeMenuWindows::set_item_tooltip(const RID &p_rid, int p_idx, const String &p_tooltip) {
	// Not supported.
}

void NativeMenuWindows::set_item_state(const RID &p_rid, int p_idx, int p_state) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			item_data->state = p_state;
		}
	}
}

void NativeMenuWindows::set_item_max_states(const RID &p_rid, int p_idx, int p_max_states) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			item_data->max_states = p_max_states;
		}
	}
}

void NativeMenuWindows::set_item_icon(const RID &p_rid, int p_idx, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA | MIIM_BITMAP;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			if (item_data->bmp) {
				DeleteObject(item_data->bmp);
			}
			if (p_icon.is_valid() && p_icon->get_width() > 0 && p_icon->get_height() > 0 && p_icon->get_image().is_valid()) {
				item_data->img = p_icon->get_image();
				item_data->img = item_data->img->duplicate();
				if (item_data->img->is_compressed()) {
					item_data->img->decompress();
				}
				item_data->bmp = _make_bitmap(item_data->img);
			} else {
				item_data->img = Ref<Image>();
				item_data->bmp = nullptr;
			}
			item.hbmpItem = item_data->bmp;
			SetMenuItemInfoW(md->menu, p_idx, true, &item);
		}
	}
}

void NativeMenuWindows::set_item_indentation_level(const RID &p_rid, int p_idx, int p_level) {
	// Not supported.
}

int NativeMenuWindows::set_item_index(const RID &p_rid, int p_idx, int p_target_idx) {
	ERR_FAIL_COND_V(p_idx < 0, -1);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, -1);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND_V(p_idx >= count, -1);
	ERR_FAIL_INDEX_V(p_target_idx, count, -1);

	// Get item text separately.
	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_STRING;
	item.dwTypeData = nullptr;
	Char16String str;
	if (!GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		return -1;
	}
	item.cch++;
	str.resize_uninitialized(item.cch);
	item.dwTypeData = (LPWSTR)str.ptrw();
	if (!GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		return -1;
	}

	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_STRING | MIIM_STATE | MIIM_SUBMENU | MIIM_BITMAP;
	if (!GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		return -1;
	}

	item.dwTypeData = (LPWSTR)str.get_data();

	if (!RemoveMenu(md->menu, p_idx, MF_BYPOSITION)) {
		return -1;
	}
	if (!InsertMenuItemW(md->menu, p_target_idx, true, &item)) {
		// Delete item if failed to insert.
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			if (item_data->bmp) {
				DeleteObject(item_data->bmp);
			}
			memdelete(item_data);
		}
		return -1;
	}

	return p_target_idx;
}

int NativeMenuWindows::get_item_count(const RID &p_rid) const {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL_V(md, 0);

	return GetMenuItemCount(md->menu);
}

bool NativeMenuWindows::is_system_menu(const RID &p_rid) const {
	return false;
}

void NativeMenuWindows::remove_item(const RID &p_rid, int p_idx) {
	ERR_FAIL_COND(p_idx < 0);
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);
	int count = GetMenuItemCount(md->menu);
	ERR_FAIL_COND(p_idx >= count);

	MENUITEMINFOW item;
	ZeroMemory(&item, sizeof(item));
	item.cbSize = sizeof(item);
	item.fMask = MIIM_DATA;
	if (GetMenuItemInfoW(md->menu, p_idx, true, &item)) {
		MenuItemData *item_data = (MenuItemData *)item.dwItemData;
		if (item_data) {
			if (item_data->bmp) {
				DeleteObject(item_data->bmp);
			}
			memdelete(item_data);
		}
	}
	RemoveMenu(md->menu, p_idx, MF_BYPOSITION);
}

void NativeMenuWindows::clear(const RID &p_rid) {
	const MenuData *md = menus.get_or_null(p_rid);
	ERR_FAIL_NULL(md);

	MENUITEMINFOW item;
	int count = GetMenuItemCount(md->menu);
	for (int i = 0; i < count; i++) {
		ZeroMemory(&item, sizeof(item));
		item.cbSize = sizeof(item);
		item.fMask = MIIM_DATA;
		if (GetMenuItemInfoW(md->menu, 0, true, &item)) {
			MenuItemData *item_data = (MenuItemData *)item.dwItemData;
			if (item_data) {
				if (item_data->bmp) {
					DeleteObject(item_data->bmp);
				}
				memdelete(item_data);
			}
		}
		RemoveMenu(md->menu, 0, MF_BYPOSITION);
	}
}

NativeMenuWindows::NativeMenuWindows() {}

NativeMenuWindows::~NativeMenuWindows() {}


// --- Merged from platform/windows/gl_manager_windows_native.cpp ---
#include "servers/display/display_server.h"

#if defined(WINDOWS_ENABLED) && defined(GLES3_ENABLED)

#include "core/os/os.h"
#include "core/typedefs.h"

#include <thirdparty/misc/nvapi_minimal.h>

#include <dwmapi.h>

#include <cstdio>
#include <cstdlib>

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

#define _WGL_CONTEXT_DEBUG_BIT_ARB 0x0001

typedef HGLRC(APIENTRY *PFNWGLCREATECONTEXT)(HDC);
typedef BOOL(APIENTRY *PFNWGLDELETECONTEXT)(HGLRC);
typedef BOOL(APIENTRY *PFNWGLMAKECURRENT)(HDC, HGLRC);
typedef HGLRC(APIENTRY *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int *);
typedef void *(APIENTRY *PFNWGLGETPROCADDRESS)(LPCSTR);

static String gl_format_error_message(DWORD id) {
	LPWSTR messageBuffer = nullptr;
	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);

	String msg = "Error " + itos(id) + ": " + String::utf16((const char16_t *)messageBuffer, size);

	LocalFree(messageBuffer);

	return msg;
}

const int OGL_THREAD_CONTROL_ID = 0x20C1221E;
const int OGL_THREAD_CONTROL_DISABLE = 0x00000002;
const int OGL_THREAD_CONTROL_ENABLE = 0x00000001;
const int VRR_MODE_ID = 0x1194F158;
const int VRR_MODE_FULLSCREEN_ONLY = 0x1;

typedef int(__cdecl *NvAPI_Initialize_t)();
typedef int(__cdecl *NvAPI_Unload_t)();
typedef int(__cdecl *NvAPI_GetErrorMessage_t)(unsigned int, NvAPI_ShortString);
typedef int(__cdecl *NvAPI_DRS_CreateSession_t)(NvDRSSessionHandle *);
typedef int(__cdecl *NvAPI_DRS_DestroySession_t)(NvDRSSessionHandle);
typedef int(__cdecl *NvAPI_DRS_LoadSettings_t)(NvDRSSessionHandle);
typedef int(__cdecl *NvAPI_DRS_CreateProfile_t)(NvDRSSessionHandle, NVDRS_PROFILE *, NvDRSProfileHandle *);
typedef int(__cdecl *NvAPI_DRS_CreateApplication_t)(NvDRSSessionHandle, NvDRSProfileHandle, NVDRS_APPLICATION *);
typedef int(__cdecl *NvAPI_DRS_SaveSettings_t)(NvDRSSessionHandle);
typedef int(__cdecl *NvAPI_DRS_SetSetting_t)(NvDRSSessionHandle, NvDRSProfileHandle, NVDRS_SETTING *);
typedef int(__cdecl *NvAPI_DRS_FindProfileByName_t)(NvDRSSessionHandle, NvAPI_UnicodeString, NvDRSProfileHandle *);
typedef int(__cdecl *NvAPI_DRS_GetApplicationInfo_t)(NvDRSSessionHandle, NvDRSProfileHandle, NvAPI_UnicodeString, NVDRS_APPLICATION *);
typedef int(__cdecl *NvAPI_DRS_DeleteProfile_t)(NvDRSSessionHandle, NvDRSProfileHandle);
NvAPI_GetErrorMessage_t NvAPI_GetErrorMessage__;

static bool nvapi_err_check(const char *msg, int status) {
	if (status != 0) {
		if (OS::is_stdout_verbose()) {
			NvAPI_ShortString err_desc = { 0 };
			NvAPI_GetErrorMessage__(status, err_desc);
			print_verbose(vformat("%s: %s(code %d)", msg, err_desc, status));
		}
		return false;
	}
	return true;
}

// On windows we have to customize the NVIDIA application profile:
// * disable threaded optimization when using NVIDIA cards to avoid stuttering, see
//   https://stackoverflow.com/questions/36959508/nvidia-graphics-driver-causing-noticeable-frame-stuttering/37632948
//   https://github.com/Ryujinx/Ryujinx/blob/master/src/Ryujinx.Common/GraphicsDriver/NVThreadedOptimization.cs
// * disable G-SYNC in windowed mode, as it results in unstable editor refresh rates
void GLManagerNative_Windows::_nvapi_setup_profile() {
	HMODULE nvapi = nullptr;
#ifdef _WIN64
	nvapi = LoadLibraryA("nvapi64.dll");
#else
	nvapi = LoadLibraryA("nvapi.dll");
#endif

	if (nvapi == nullptr) {
		return;
	}

	void *(__cdecl * NvAPI_QueryInterface)(unsigned int interface_id) = nullptr;

	NvAPI_QueryInterface = (void *(__cdecl *)(unsigned int))(void *)GetProcAddress(nvapi, "nvapi_QueryInterface");

	if (NvAPI_QueryInterface == nullptr) {
		print_verbose("Error getting NVAPI NvAPI_QueryInterface");
		return;
	}

	// Setup NVAPI function pointers
	NvAPI_Initialize_t NvAPI_Initialize = (NvAPI_Initialize_t)NvAPI_QueryInterface(0x0150E828);
	NvAPI_GetErrorMessage__ = (NvAPI_GetErrorMessage_t)NvAPI_QueryInterface(0x6C2D048C);
	NvAPI_DRS_CreateSession_t NvAPI_DRS_CreateSession = (NvAPI_DRS_CreateSession_t)NvAPI_QueryInterface(0x0694D52E);
	NvAPI_DRS_DestroySession_t NvAPI_DRS_DestroySession = (NvAPI_DRS_DestroySession_t)NvAPI_QueryInterface(0xDAD9CFF8);
	NvAPI_Unload_t NvAPI_Unload = (NvAPI_Unload_t)NvAPI_QueryInterface(0xD22BDD7E);
	NvAPI_DRS_LoadSettings_t NvAPI_DRS_LoadSettings = (NvAPI_DRS_LoadSettings_t)NvAPI_QueryInterface(0x375DBD6B);
	NvAPI_DRS_CreateProfile_t NvAPI_DRS_CreateProfile = (NvAPI_DRS_CreateProfile_t)NvAPI_QueryInterface(0xCC176068);
	NvAPI_DRS_CreateApplication_t NvAPI_DRS_CreateApplication = (NvAPI_DRS_CreateApplication_t)NvAPI_QueryInterface(0x4347A9DE);
	NvAPI_DRS_SaveSettings_t NvAPI_DRS_SaveSettings = (NvAPI_DRS_SaveSettings_t)NvAPI_QueryInterface(0xFCBC7E14);
	NvAPI_DRS_SetSetting_t NvAPI_DRS_SetSetting = (NvAPI_DRS_SetSetting_t)NvAPI_QueryInterface(0x577DD202);
	NvAPI_DRS_FindProfileByName_t NvAPI_DRS_FindProfileByName = (NvAPI_DRS_FindProfileByName_t)NvAPI_QueryInterface(0x7E4A9A0B);
	NvAPI_DRS_GetApplicationInfo_t NvAPI_DRS_GetApplicationInfo = (NvAPI_DRS_GetApplicationInfo_t)NvAPI_QueryInterface(0xED1F8C69);
	NvAPI_DRS_DeleteProfile_t NvAPI_DRS_DeleteProfile = (NvAPI_DRS_DeleteProfile_t)NvAPI_QueryInterface(0x17093206);

	if (!nvapi_err_check("NVAPI: Init failed", NvAPI_Initialize())) {
		return;
	}

	print_verbose("NVAPI: Init OK!");

	NvDRSSessionHandle session_handle;

	if (NvAPI_DRS_CreateSession == nullptr) {
		return;
	}

	if (!nvapi_err_check("NVAPI: Error creating DRS session", NvAPI_DRS_CreateSession(&session_handle))) {
		NvAPI_Unload();
		return;
	}

	if (!nvapi_err_check("NVAPI: Error loading DRS settings", NvAPI_DRS_LoadSettings(session_handle))) {
		NvAPI_DRS_DestroySession(session_handle);
		NvAPI_Unload();
		return;
	}

	String app_executable_name = OS::get_executable_path().get_file();
	String app_profile_name = GLOBAL_GET("application/config/name");
	// We need a name anyways, so let's use the engine name if an application name is not available
	// (this is used mostly by the Project Manager)
	if (app_profile_name.is_empty()) {
		app_profile_name = VLTR_VERSION_NAME;
	}
	String old_profile_name = app_profile_name + " Nvidia Profile";
	Char16String app_profile_name_u16 = app_profile_name.utf16();
	Char16String old_profile_name_u16 = old_profile_name.utf16();
	Char16String app_executable_name_u16 = app_executable_name.utf16();

	// A previous error in app creation logic could result in invalid profiles,
	// clean these if they exist before proceeding.
	NvDRSProfileHandle old_profile_handle;

	int old_status = NvAPI_DRS_FindProfileByName(session_handle, (NvU16 *)(old_profile_name_u16.ptrw()), &old_profile_handle);

	if (old_status == 0) {
		print_verbose("NVAPI: Deleting old profile...");

		if (!nvapi_err_check("NVAPI: Error deleting old profile", NvAPI_DRS_DeleteProfile(session_handle, old_profile_handle))) {
			NvAPI_DRS_DestroySession(session_handle);
			NvAPI_Unload();
			return;
		}

		if (!nvapi_err_check("NVAPI: Error deleting old profile", NvAPI_DRS_SaveSettings(session_handle))) {
			NvAPI_DRS_DestroySession(session_handle);
			NvAPI_Unload();
			return;
		}
	}

	NvDRSProfileHandle profile_handle = nullptr;

	int profile_status = NvAPI_DRS_FindProfileByName(session_handle, (NvU16 *)(app_profile_name_u16.ptrw()), &profile_handle);

	if (profile_status != 0) {
		print_verbose("NVAPI: Profile not found, creating...");

		NVDRS_PROFILE profile_info;
		profile_info.version = NVDRS_PROFILE_VER;
		profile_info.isPredefined = 0;
		memcpy(profile_info.profileName, app_profile_name_u16.get_data(), sizeof(char16_t) * app_profile_name_u16.size());

		if (!nvapi_err_check("NVAPI: Error creating profile", NvAPI_DRS_CreateProfile(session_handle, &profile_info, &profile_handle))) {
			NvAPI_DRS_DestroySession(session_handle);
			NvAPI_Unload();
			return;
		}
	}

	NVDRS_APPLICATION_V4 app;
	app.version = NVDRS_APPLICATION_VER_V4;

	int app_status = NvAPI_DRS_GetApplicationInfo(session_handle, profile_handle, (NvU16 *)(app_executable_name_u16.ptrw()), &app);

	if (app_status != 0) {
		print_verbose("NVAPI: Application not found in profile, creating...");

		app.isPredefined = 0;
		memcpy(app.appName, app_executable_name_u16.get_data(), sizeof(char16_t) * app_executable_name_u16.size());
		memcpy(app.launcher, L"", sizeof(wchar_t));
		memcpy(app.fileInFolder, L"", sizeof(wchar_t));

		if (!nvapi_err_check("NVAPI: Error creating application", NvAPI_DRS_CreateApplication(session_handle, profile_handle, &app))) {
			NvAPI_DRS_DestroySession(session_handle);
			NvAPI_Unload();
			return;
		}
	}

	NVDRS_SETTING ogl_thread_control_setting = {};
	ogl_thread_control_setting.version = NVDRS_SETTING_VER;
	ogl_thread_control_setting.settingId = OGL_THREAD_CONTROL_ID;
	ogl_thread_control_setting.settingType = NVDRS_DWORD_TYPE;
	int thread_control_val = OGL_THREAD_CONTROL_DISABLE;
	if (!GLOBAL_GET("rendering/gl_compatibility/nvidia_disable_threaded_optimization")) {
		thread_control_val = OGL_THREAD_CONTROL_ENABLE;
	}
	ogl_thread_control_setting.u32CurrentValue = thread_control_val;

	if (!nvapi_err_check("NVAPI: Error calling NvAPI_DRS_SetSetting", NvAPI_DRS_SetSetting(session_handle, profile_handle, &ogl_thread_control_setting))) {
		NvAPI_DRS_DestroySession(session_handle);
		NvAPI_Unload();
		return;
	}

	NVDRS_SETTING vrr_mode_setting = {};
	vrr_mode_setting.version = NVDRS_SETTING_VER;
	vrr_mode_setting.settingId = VRR_MODE_ID;
	vrr_mode_setting.settingType = NVDRS_DWORD_TYPE;
	vrr_mode_setting.u32CurrentValue = VRR_MODE_FULLSCREEN_ONLY;

	if (!nvapi_err_check("NVAPI: Error calling NvAPI_DRS_SetSetting", NvAPI_DRS_SetSetting(session_handle, profile_handle, &vrr_mode_setting))) {
		NvAPI_DRS_DestroySession(session_handle);
		NvAPI_Unload();
		return;
	}

	if (!nvapi_err_check("NVAPI: Error saving settings", NvAPI_DRS_SaveSettings(session_handle))) {
		NvAPI_DRS_DestroySession(session_handle);
		NvAPI_Unload();
		return;
	}

	if (thread_control_val == OGL_THREAD_CONTROL_DISABLE) {
		print_verbose("NVAPI: Disabled OpenGL threaded optimization successfully");
	} else {
		print_verbose("NVAPI: Enabled OpenGL threaded optimization successfully");
	}
	print_verbose("NVAPI: Disabled G-SYNC for windowed mode successfully");

	NvAPI_DRS_DestroySession(session_handle);
}

int GLManagerNative_Windows::_find_or_create_display(GLWindow &win) {
	// find display NYI, only 1 supported so far
	if (_displays.size()) {
		return 0;
	}

	// create
	GLDisplay d_temp = {};
	_displays.push_back(d_temp);
	int new_display_id = _displays.size() - 1;

	// create context
	GLDisplay &d = _displays[new_display_id];
	Error err = _create_context(win, d);

	if (err != OK) {
		// not good
		// delete the _display?
		_displays.remove_at(new_display_id);
		return -1;
	}

	return new_display_id;
}

static Error _configure_pixel_format(HDC hDC) {
	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
		1,
		PFD_DRAW_TO_WINDOW | // Format Must Support Window
				PFD_SUPPORT_OPENGL | // Format Must Support OpenGL
				PFD_DOUBLEBUFFER,
		(BYTE)PFD_TYPE_RGBA,
		(BYTE)(OS::is_layered_allowed() ? 32 : 24),
		(BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, // Color Bits Ignored
		(BYTE)(OS::is_layered_allowed() ? 8 : 0), // Alpha Buffer
		(BYTE)0, // Shift Bit Ignored
		(BYTE)0, // No Accumulation Buffer
		(BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, // Accumulation Bits Ignored
		(BYTE)24, // 24Bit Z-Buffer (Depth Buffer)
		(BYTE)0, // No Stencil Buffer
		(BYTE)0, // No Auxiliary Buffer
		(BYTE)PFD_MAIN_PLANE, // Main Drawing Layer
		(BYTE)0, // Reserved
		0, 0, 0 // Layer Masks Ignored
	};

	int pixel_format = ChoosePixelFormat(hDC, &pfd);
	if (!pixel_format) // Did Windows Find A Matching Pixel Format?
	{
		return ERR_CANT_CREATE; // Return FALSE
	}

	BOOL ret = SetPixelFormat(hDC, pixel_format, &pfd);
	if (!ret) // Are We Able To Set The Pixel Format?
	{
		return ERR_CANT_CREATE; // Return FALSE
	}

	return OK;
}

PFNWGLCREATECONTEXT gd_wglCreateContext;
PFNWGLMAKECURRENT gd_wglMakeCurrent;
PFNWGLDELETECONTEXT gd_wglDeleteContext;
PFNWGLGETPROCADDRESS gd_wglGetProcAddress;

Error GLManagerNative_Windows::_create_context(GLWindow &win, GLDisplay &gl_display) {
	Error err = _configure_pixel_format(win.hDC);
	if (err != OK) {
		return err;
	}

	HMODULE module = LoadLibraryW(L"opengl32.dll");
	if (!module) {
		return ERR_CANT_CREATE;
	}
	gd_wglCreateContext = (PFNWGLCREATECONTEXT)(void *)GetProcAddress(module, "wglCreateContext");
	gd_wglMakeCurrent = (PFNWGLMAKECURRENT)(void *)GetProcAddress(module, "wglMakeCurrent");
	gd_wglDeleteContext = (PFNWGLDELETECONTEXT)(void *)GetProcAddress(module, "wglDeleteContext");
	gd_wglGetProcAddress = (PFNWGLGETPROCADDRESS)(void *)GetProcAddress(module, "wglGetProcAddress");
	if (!gd_wglCreateContext || !gd_wglMakeCurrent || !gd_wglDeleteContext || !gd_wglGetProcAddress) {
		return ERR_CANT_CREATE;
	}

	gl_display.hRC = gd_wglCreateContext(win.hDC);
	if (!gl_display.hRC) // Are We Able To Get A Rendering Context?
	{
		return ERR_CANT_CREATE; // Return FALSE
	}

	if (!gd_wglMakeCurrent(win.hDC, gl_display.hRC)) {
		ERR_PRINT("Could not attach OpenGL context to newly created window: " + gl_format_error_message(GetLastError()));
	}

	int attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3, //we want a 3.3 context
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		//and it shall be forward compatible so that we can only use up to date functionality
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB /*| _WGL_CONTEXT_DEBUG_BIT_ARB*/,
		0
	}; //zero indicates the end of the array

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr; //pointer to the method
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)gd_wglGetProcAddress("wglCreateContextAttribsARB");

	if (wglCreateContextAttribsARB == nullptr) //OpenGL 3.0 is not supported
	{
		gd_wglDeleteContext(gl_display.hRC);
		gl_display.hRC = nullptr;
		return ERR_CANT_CREATE;
	}

	HGLRC new_hRC = wglCreateContextAttribsARB(win.hDC, nullptr, attribs);
	if (!new_hRC) {
		gd_wglDeleteContext(gl_display.hRC);
		gl_display.hRC = nullptr;
		return ERR_CANT_CREATE;
	}

	if (!gd_wglMakeCurrent(win.hDC, nullptr)) {
		ERR_PRINT("Could not detach OpenGL context from newly created window: " + gl_format_error_message(GetLastError()));
	}

	gd_wglDeleteContext(gl_display.hRC);
	gl_display.hRC = new_hRC;

	if (!gd_wglMakeCurrent(win.hDC, gl_display.hRC)) // Try to activate the rendering context.
	{
		ERR_PRINT("Could not attach OpenGL context to newly created window with replaced OpenGL context: " + gl_format_error_message(GetLastError()));
		gd_wglDeleteContext(gl_display.hRC);
		gl_display.hRC = nullptr;
		return ERR_CANT_CREATE;
	}

	if (!wglSwapIntervalEXT) {
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)gd_wglGetProcAddress("wglSwapIntervalEXT");
	}

	return OK;
}

Error GLManagerNative_Windows::window_create(DisplayServerEnums::WindowID p_window_id, HWND p_hwnd, HINSTANCE p_hinstance, int p_width, int p_height) {
	HDC hDC = GetDC(p_hwnd);
	if (!hDC) {
		return ERR_CANT_CREATE;
	}

	// configure the HDC to use a compatible pixel format
	Error result = _configure_pixel_format(hDC);
	if (result != OK) {
		return result;
	}

	GLWindow win;
	win.hwnd = p_hwnd;
	win.hDC = hDC;

	win.gldisplay_id = _find_or_create_display(win);

	if (win.gldisplay_id == -1) {
		return FAILED;
	}

	// WARNING: `p_window_id` is an eternally growing integer since popup windows keep coming and going
	// and each of them has a higher id than the previous, so it must be used in a map not a vector.
	_windows[p_window_id] = win;

	// make current
	window_make_current(p_window_id);

	return OK;
}

void GLManagerNative_Windows::window_destroy(DisplayServerEnums::WindowID p_window_id) {
	GLWindow &win = get_window(p_window_id);
	if (_current_window == &win) {
		_current_window = nullptr;
	}
	_windows.erase(p_window_id);
}

void GLManagerNative_Windows::release_current() {
	if (!_current_window) {
		return;
	}

	if (!gd_wglMakeCurrent(_current_window->hDC, nullptr)) {
		ERR_PRINT("Could not detach OpenGL context from window marked current: " + gl_format_error_message(GetLastError()));
	}

	_current_window = nullptr;
}

void GLManagerNative_Windows::window_make_current(DisplayServerEnums::WindowID p_window_id) {
	if (p_window_id == -1) {
		return;
	}

	// crash if our data structures are out of sync, i.e. not found
	GLWindow &win = _windows[p_window_id];

	// noop
	if (&win == _current_window) {
		return;
	}

	const GLDisplay &disp = get_display(win.gldisplay_id);
	if (!gd_wglMakeCurrent(win.hDC, disp.hRC)) {
		ERR_PRINT("Could not switch OpenGL context to other window: " + gl_format_error_message(GetLastError()));
	}

	_current_window = &win;
}

void GLManagerNative_Windows::swap_buffers() {
	SwapBuffers(_current_window->hDC);
}

Error GLManagerNative_Windows::initialize() {
	_nvapi_setup_profile();
	return OK;
}

void GLManagerNative_Windows::set_use_vsync(DisplayServerEnums::WindowID p_window_id, bool p_use) {
	GLWindow &win = get_window(p_window_id);

	if (&win != _current_window) {
		window_make_current(p_window_id);
	}

	if (wglSwapIntervalEXT) {
		win.use_vsync = p_use;

		if (!wglSwapIntervalEXT(p_use ? 1 : 0)) {
			WARN_PRINT_ONCE("Could not set V-Sync mode, as changing V-Sync mode is not supported by the graphics driver.");
		}
	} else {
		WARN_PRINT_ONCE("Could not set V-Sync mode, as changing V-Sync mode is not supported by the graphics driver.");
	}
}

bool GLManagerNative_Windows::is_using_vsync(DisplayServerEnums::WindowID p_window_id) const {
	return get_window(p_window_id).use_vsync;
}

HDC GLManagerNative_Windows::get_hdc(DisplayServerEnums::WindowID p_window_id) {
	return get_window(p_window_id).hDC;
}

HGLRC GLManagerNative_Windows::get_hglrc(DisplayServerEnums::WindowID p_window_id) {
	const GLWindow &win = get_window(p_window_id);
	const GLDisplay &disp = get_display(win.gldisplay_id);
	return disp.hRC;
}

GLManagerNative_Windows::GLManagerNative_Windows() {
	direct_render = false;
	glx_minor = glx_major = 0;
	_current_window = nullptr;
}

GLManagerNative_Windows::~GLManagerNative_Windows() {
	release_current();
}

#endif // WINDOWS_ENABLED && GLES3_ENABLED

#ifdef WGL_CONTEXT_MAJOR_VERSION_ARB
#undef WGL_CONTEXT_MAJOR_VERSION_ARB
#endif
#ifdef WGL_CONTEXT_MINOR_VERSION_ARB
#undef WGL_CONTEXT_MINOR_VERSION_ARB
#endif
#ifdef WGL_CONTEXT_FLAGS_ARB
#undef WGL_CONTEXT_FLAGS_ARB
#endif
#ifdef WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#undef WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#endif
#ifdef WGL_CONTEXT_PROFILE_MASK_ARB
#undef WGL_CONTEXT_PROFILE_MASK_ARB
#endif
#ifdef WGL_CONTEXT_CORE_PROFILE_BIT_ARB
#undef WGL_CONTEXT_CORE_PROFILE_BIT_ARB
#endif


// --- Merged from platform/windows/wgl_detect_version.cpp ---
#if defined(WINDOWS_ENABLED) && defined(GLES3_ENABLED)

#include "servers/display/display_server.h"


#include "core/os/os.h"
#include "core/string/ustring.h"

#include <windows.h>

#include <dwmapi.h>

#include <cstdio>
#include <cstdlib>

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_VENDOR 0x1F00
#define WGL_RENDERER 0x1F01
#define WGL_VERSION 0x1F02

typedef HGLRC(APIENTRY *PFNWGLCREATECONTEXT)(HDC);
typedef BOOL(APIENTRY *PFNWGLDELETECONTEXT)(HGLRC);
typedef BOOL(APIENTRY *PFNWGLMAKECURRENT)(HDC, HGLRC);
typedef HGLRC(APIENTRY *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int *);
typedef void *(APIENTRY *PFNWGLGETPROCADDRESS)(LPCSTR);
typedef const char *(APIENTRY *PFNWGLGETSTRINGPROC)(unsigned int);

Dictionary detect_wgl() {
	Dictionary gl_info;
	gl_info["version"] = 0;
	gl_info["vendor"] = String();
	gl_info["name"] = String();

	PFNWGLCREATECONTEXT gd_wglCreateContext;
	PFNWGLMAKECURRENT gd_wglMakeCurrent;
	PFNWGLDELETECONTEXT gd_wglDeleteContext;
	PFNWGLGETPROCADDRESS gd_wglGetProcAddress;

	HMODULE module = LoadLibraryW(L"opengl32.dll");
	if (!module) {
		return gl_info;
	}
	gd_wglCreateContext = (PFNWGLCREATECONTEXT)(void *)GetProcAddress(module, "wglCreateContext");
	gd_wglMakeCurrent = (PFNWGLMAKECURRENT)(void *)GetProcAddress(module, "wglMakeCurrent");
	gd_wglDeleteContext = (PFNWGLDELETECONTEXT)(void *)GetProcAddress(module, "wglDeleteContext");
	gd_wglGetProcAddress = (PFNWGLGETPROCADDRESS)(void *)GetProcAddress(module, "wglGetProcAddress");
	if (!gd_wglCreateContext || !gd_wglMakeCurrent || !gd_wglDeleteContext || !gd_wglGetProcAddress) {
		return gl_info;
	}

	LPCWSTR class_name = L"EngineWGLDetect";
	HINSTANCE hInstance = OS_Windows::get_hinstance();
	WNDCLASSW wc = {};

	wc.lpfnWndProc = DefWindowProcW;
	wc.hInstance = hInstance;
	wc.lpszClassName = class_name;

	RegisterClassW(&wc);

	HWND hWnd = CreateWindowExW(WS_EX_APPWINDOW, class_name, L"", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);
	if (hWnd) {
		HDC hDC = GetDC(hWnd);
		if (hDC) {
			static PIXELFORMATDESCRIPTOR pfd = {
				sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
				1,
				PFD_DRAW_TO_WINDOW | // Format Must Support Window
						PFD_SUPPORT_OPENGL | // Format Must Support OpenGL
						PFD_DOUBLEBUFFER,
				(BYTE)PFD_TYPE_RGBA,
				(BYTE)(OS::is_layered_allowed() ? 32 : 24),
				(BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, // Color Bits Ignored
				(BYTE)(OS::is_layered_allowed() ? 8 : 0), // Alpha Buffer
				(BYTE)0, // Shift Bit Ignored
				(BYTE)0, // No Accumulation Buffer
				(BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, // Accumulation Bits Ignored
				(BYTE)24, // 24Bit Z-Buffer (Depth Buffer)
				(BYTE)0, // No Stencil Buffer
				(BYTE)0, // No Auxiliary Buffer
				(BYTE)PFD_MAIN_PLANE, // Main Drawing Layer
				(BYTE)0, // Reserved
				0, 0, 0 // Layer Masks Ignored
			};

			int pixel_format = ChoosePixelFormat(hDC, &pfd);
			SetPixelFormat(hDC, pixel_format, &pfd);

			HGLRC hRC = gd_wglCreateContext(hDC);
			if (hRC) {
				if (gd_wglMakeCurrent(hDC, hRC)) {
					int attribs[] = {
						WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
						WGL_CONTEXT_MINOR_VERSION_ARB, 3,
						WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
						WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
						0
					};

					PFNWGLCREATECONTEXTATTRIBSARBPROC gd_wglCreateContextAttribsARB = nullptr;
					gd_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)gd_wglGetProcAddress("wglCreateContextAttribsARB");
					if (gd_wglCreateContextAttribsARB) {
						HGLRC new_hRC = gd_wglCreateContextAttribsARB(hDC, nullptr, attribs);
						if (new_hRC) {
							if (gd_wglMakeCurrent(hDC, new_hRC)) {
								PFNWGLGETSTRINGPROC gd_wglGetString = (PFNWGLGETSTRINGPROC)(void *)GetProcAddress(module, "glGetString");
								if (gd_wglGetString) {
									const char *prefixes[] = {
										"OpenGL ES-CM ",
										"OpenGL ES-CL ",
										"OpenGL ES ",
										"OpenGL SC ",
										nullptr
									};
									const char *version = (const char *)gd_wglGetString(WGL_VERSION);
									if (version) {
										const String device_vendor = String::utf8((const char *)gd_wglGetString(WGL_VENDOR)).strip_edges().trim_suffix(" Corporation");
										const String device_name = String::utf8((const char *)gd_wglGetString(WGL_RENDERER)).strip_edges().trim_suffix("/PCIe/SSE2");
										for (int i = 0; prefixes[i]; i++) {
											size_t length = strlen(prefixes[i]);
											if (strncmp(version, prefixes[i], length) == 0) {
												version += length;
												break;
											}
										}
										int major = 0;
										int minor = 0;
#ifdef _MSC_VER
										sscanf_s(version, "%d.%d", &major, &minor);
#else
										sscanf(version, "%d.%d", &major, &minor);
#endif
										print_verbose(vformat("Native OpenGL API detected: %d.%d: %s - %s", major, minor, device_vendor, device_name));
										gl_info["vendor"] = device_vendor;
										gl_info["name"] = device_name;
										gl_info["version"] = major * 10000 + minor;
									}
								}
							}
							gd_wglMakeCurrent(nullptr, nullptr);
							gd_wglDeleteContext(new_hRC);
						}
					}
				}
				gd_wglMakeCurrent(nullptr, nullptr);
				gd_wglDeleteContext(hRC);
			}
			ReleaseDC(hWnd, hDC);
		}
		DestroyWindow(hWnd);
	}
	UnregisterClassW(class_name, hInstance);

	return gl_info;
}

#endif // WINDOWS_ENABLED && GLES3_ENABLED


// --- Merged from platform/windows/rendering_context_driver_vulkan_windows.cpp ---
#if defined(WINDOWS_ENABLED) && defined(VULKAN_ENABLED)

#include "servers/display/display_server.h"

#include "core/os/os.h"

#include <drivers/vulkan/godot_vulkan.h>

const char *RenderingContextDriverVulkan::_get_platform_surface_extension() const {
	return surface_mode == SURFACE_NONE ? nullptr : VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

RenderingContextDriver::SurfaceID RenderingContextDriverVulkan::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	VkWin32SurfaceCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.hinstance = (HINSTANCE)wpd->instance;
	create_info.hwnd = (HWND)wpd->window;

	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkResult err = vkCreateWin32SurfaceKHR(instance_get(), &create_info, get_allocation_callbacks(VK_OBJECT_TYPE_SURFACE_KHR), &vk_surface);
	ERR_FAIL_COND_V(err != VK_SUCCESS, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->vk_surface = vk_surface;
	return SurfaceID(surface);
}

#endif // WINDOWS_ENABLED && VULKAN_ENABLED


// --- Merged from platform/windows/drop_target_windows.cpp ---
#include "servers/display/display_server.h"

#include "core/io/net_socket.h"
#include "core/io/ip.h"
#include "core/os/os.h"

#include <fileapi.h>
#include <shellapi.h>

// Helpers

static String create_temp_dir() {
	Char16String buf;
	int bufsize = GetTempPathW(0, nullptr) + 1;
	buf.resize_uninitialized(bufsize);
	if (GetTempPathW(bufsize, (LPWSTR)buf.ptrw()) == 0) {
		return "";
	}

	String tmp_dir = String::utf16((const char16_t *)buf.ptr());
	RandomPCG gen(Time::get_ticks_usec());

	const int attempts = 4;

	for (int i = 0; i < attempts; ++i) {
		uint32_t rnd = gen.rand();
		String dirname = "godot_tmp_" + String::num_uint64(rnd);
		String res_dir = tmp_dir.path_join(dirname);
		Char16String res_dir16 = res_dir.utf16();

		if (CreateDirectoryW((LPCWSTR)res_dir16.ptr(), nullptr)) {
			return res_dir;
		}
	}

	return "";
}

static bool remove_dir_recursive(const String &p_dir) {
	Ref<DirAccess> dir_access = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (dir_access->change_dir(p_dir) != OK) {
		return false;
	}
	return dir_access->erase_contents_recursive() == OK;
}

static bool stream2file(IStream *p_stream, FILEDESCRIPTORW *p_desc, const String &p_path) {
	if (DirAccess::make_dir_recursive_absolute(p_path.get_base_dir()) != OK) {
		return false;
	}

	Char16String path16 = p_path.utf16();
	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

	if (p_desc->dwFlags & FD_ATTRIBUTES) {
		dwFlagsAndAttributes = p_desc->dwFileAttributes;
	}

	HANDLE file = CreateFileW(
			(LPCWSTR)path16.ptr(),
			GENERIC_WRITE,
			0,
			nullptr,
			CREATE_NEW,
			dwFlagsAndAttributes,
			nullptr);

	if (!file) {
		return false;
	}

	const int bufsize = 4096;
	char buf[bufsize];
	ULONG nread = 0;
	DWORD nwritten = 0;
	HRESULT read_result = S_OK;
	bool result = true;

	while (true) {
		read_result = p_stream->Read(buf, bufsize, &nread);
		if (read_result != S_OK && read_result != S_FALSE) {
			result = false;
			goto cleanup;
		}

		if (!nread) {
			break;
		}

		while (nread > 0) {
			if (!WriteFile(file, buf, nread, &nwritten, nullptr) || !nwritten) {
				result = false;
				goto cleanup;
			}
			nread -= nwritten;
		}
	}

cleanup:
	CloseHandle(file);
	return result;
}

// DropTargetWindows

bool DropTargetWindows::is_valid_filedescriptor() {
	return cf_filedescriptor != 0 && cf_filecontents != 0;
}

HRESULT DropTargetWindows::handle_hdrop_format(Vector<String> *p_files, IDataObject *pDataObj) {
	FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stg;
	HRESULT res = S_OK;

	if (pDataObj->GetData(&fmt, &stg) != S_OK) {
		return E_UNEXPECTED;
	}

	HDROP hDropInfo = (HDROP)GlobalLock(stg.hGlobal);

	Char16String buf;

	if (hDropInfo == nullptr) {
		ReleaseStgMedium(&stg);
		return E_UNEXPECTED;
	}

	int fcount = DragQueryFileW(hDropInfo, 0xFFFFFFFF, nullptr, 0);

	for (int i = 0; i < fcount; i++) {
		int buffsize = DragQueryFileW(hDropInfo, i, nullptr, 0);
		buf.resize_uninitialized(buffsize + 1);
		if (DragQueryFileW(hDropInfo, i, (LPWSTR)buf.ptrw(), buffsize + 1) == 0) {
			res = E_UNEXPECTED;
			goto cleanup;
		}
		String file = String::utf16((const char16_t *)buf.ptr());
		p_files->push_back(file);
	}

cleanup:
	GlobalUnlock(stg.hGlobal);
	ReleaseStgMedium(&stg);

	return res;
}

HRESULT DropTargetWindows::handle_filedescriptor_format(Vector<String> *p_files, IDataObject *pDataObj) {
	FORMATETC fmt = { cf_filedescriptor, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stg;
	HRESULT res = S_OK;

	if (pDataObj->GetData(&fmt, &stg) != S_OK) {
		return E_UNEXPECTED;
	}

	FILEGROUPDESCRIPTORW *filegroup_desc = (FILEGROUPDESCRIPTORW *)GlobalLock(stg.hGlobal);

	if (!filegroup_desc) {
		ReleaseStgMedium(&stg);
		return E_UNEXPECTED;
	}

	tmp_path = create_temp_dir();
	Ref<DirAccess> dir_access = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	PackedStringArray copied;

	if (dir_access->change_dir(tmp_path) != OK) {
		res = E_UNEXPECTED;
		goto cleanup;
	}

	for (int i = 0; i < (int)filegroup_desc->cItems; ++i) {
		res = save_as_file(tmp_path, filegroup_desc->fgd + i, pDataObj, i);
		if (res != S_OK) {
			res = E_UNEXPECTED;
			goto cleanup;
		}
	}

	copied = dir_access->get_files();
	for (const String &file : copied) {
		p_files->push_back(tmp_path.path_join(file));
	}

	copied = dir_access->get_directories();
	for (const String &dir : copied) {
		p_files->push_back(tmp_path.path_join(dir));
	}

cleanup:
	GlobalUnlock(filegroup_desc);
	ReleaseStgMedium(&stg);
	if (res != S_OK) {
		remove_dir_recursive(tmp_path);
		tmp_path.clear();
	}
	return res;
}

HRESULT DropTargetWindows::save_as_file(const String &p_out_dir, FILEDESCRIPTORW *p_file_desc, IDataObject *pDataObj, int p_file_idx) {
	String relpath = String::utf16((const char16_t *)p_file_desc->cFileName);
	String fullpath = p_out_dir.path_join(relpath);

	if (p_file_desc->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		if (DirAccess::make_dir_recursive_absolute(fullpath) != OK) {
			return E_UNEXPECTED;
		}
		return S_OK;
	}

	FORMATETC fmt = { cf_filecontents, nullptr, DVASPECT_CONTENT, p_file_idx, TYMED_ISTREAM };
	STGMEDIUM stg;
	HRESULT res = S_OK;

	if (pDataObj->GetData(&fmt, &stg) != S_OK) {
		return E_UNEXPECTED;
	}

	IStream *stream = stg.pstm;
	if (stream == nullptr) {
		res = E_UNEXPECTED;
		goto cleanup;
	}

	if (!stream2file(stream, p_file_desc, fullpath)) {
		res = E_UNEXPECTED;
		goto cleanup;
	}

cleanup:
	ReleaseStgMedium(&stg);
	return res;
}

DropTargetWindows::DropTargetWindows(DisplayServerWindows::WindowData *p_window_data) :
		ref_count(1), window_data(p_window_data) {
	cf_filedescriptor = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
	cf_filecontents = RegisterClipboardFormat(CFSTR_FILECONTENTS);
}

HRESULT STDMETHODCALLTYPE DropTargetWindows::QueryInterface(REFIID riid, void **ppvObject) {
	if (riid == IID_IUnknown || riid == IID_IDropTarget) {
		*ppvObject = static_cast<IDropTarget *>(this);
		AddRef();
		return S_OK;
	}
	*ppvObject = nullptr;
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DropTargetWindows::AddRef() {
	return InterlockedIncrement(&ref_count);
}

ULONG STDMETHODCALLTYPE DropTargetWindows::Release() {
	ULONG count = InterlockedDecrement(&ref_count);
	if (count == 0) {
		memfree(this);
	}
	return count;
}

HRESULT STDMETHODCALLTYPE DropTargetWindows::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
	(void)grfKeyState;
	(void)pt;

	FORMATETC hdrop_fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	FORMATETC filedesc_fmt = { cf_filedescriptor, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	if (!window_data->drop_files_callback.is_valid()) {
		*pdwEffect = DROPEFFECT_NONE;
	} else if (pDataObj->QueryGetData(&hdrop_fmt) == S_OK) {
		*pdwEffect = DROPEFFECT_COPY;
	} else if (is_valid_filedescriptor() && pDataObj->QueryGetData(&filedesc_fmt) == S_OK) {
		*pdwEffect = DROPEFFECT_COPY;
	} else {
		*pdwEffect = DROPEFFECT_NONE;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE DropTargetWindows::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
	(void)grfKeyState;
	(void)pt;

	*pdwEffect = DROPEFFECT_COPY;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DropTargetWindows::DragLeave() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DropTargetWindows::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
	(void)grfKeyState;
	(void)pt;

	*pdwEffect = DROPEFFECT_NONE;

	if (!window_data->drop_files_callback.is_valid()) {
		return S_OK;
	}

	FORMATETC hdrop_fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	FORMATETC filedesc_fmt = { cf_filedescriptor, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	Vector<String> files;

	if (pDataObj->QueryGetData(&hdrop_fmt) == S_OK) {
		HRESULT res = handle_hdrop_format(&files, pDataObj);
		if (res != S_OK) {
			return res;
		}
	} else if (pDataObj->QueryGetData(&filedesc_fmt) == S_OK && is_valid_filedescriptor()) {
		HRESULT res = handle_filedescriptor_format(&files, pDataObj);
		if (res != S_OK) {
			return res;
		}
	} else {
		return E_UNEXPECTED;
	}

	if (!files.size()) {
		return S_OK;
	}

	Variant v_files = files;
	const Variant *v_args[1] = { &v_files };
	Variant ret;
	Callable::CallError ce;
	window_data->drop_files_callback.callp((const Variant **)&v_args, 1, ret, ce);

	if (!tmp_path.is_empty()) {
		remove_dir_recursive(tmp_path);
		tmp_path.clear();
	}

	if (ce.error != Callable::CallError::CALL_OK) {
		ERR_PRINT(vformat("Failed to execute drop files callback: %s.", Variant::get_callable_error_text(window_data->drop_files_callback, v_args, 1, ce)));
		return E_UNEXPECTED;
	}

	*pdwEffect = DROPEFFECT_COPY;
	return S_OK;
}


// --- Merged from platform/windows/gl_manager_windows_angle.cpp ---
#if defined(ANGLE_ENABLED)
#include "servers/display/display_server.h"

#if defined(WINDOWS_ENABLED) && defined(GLES3_ENABLED)

#include <EGL/eglext_angle.h>

const char *GLManagerANGLE_Windows::_get_platform_extension_name() const {
	return "EGL_ANGLE_platform_angle";
}

EGLenum GLManagerANGLE_Windows::_get_platform_extension_enum() const {
	return EGL_PLATFORM_ANGLE_ANGLE;
}

Vector<EGLAttrib> GLManagerANGLE_Windows::_get_platform_display_attributes() const {
	Vector<EGLAttrib> ret;
	ret.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
	ret.push_back(EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE);
	ret.push_back(EGL_NONE);

	return ret;
}

EGLenum GLManagerANGLE_Windows::_get_platform_api_enum() const {
	return EGL_OPENGL_ES_API;
}

Vector<EGLint> GLManagerANGLE_Windows::_get_platform_context_attribs() const {
	Vector<EGLint> ret;
	ret.push_back(EGL_CONTEXT_CLIENT_VERSION);
	ret.push_back(3);
	ret.push_back(EGL_NONE);

	return ret;
}

void GLManagerANGLE_Windows::window_resize(DisplayServerEnums::WindowID p_window_id, int p_width, int p_height) {
	window_make_current(p_window_id);
	eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}

#endif // WINDOWS_ENABLED && GLES3_ENABLED
#endif // defined(ANGLE_ENABLED)

// === Merged from drivers/wasapi/audio_driver_wasapi.cpp ===
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

#ifdef WASAPI_ENABLED

#include "servers/display/display_server.h"

#include "core/os/os.h"

#include <functiondiscoverykeys.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Define IAudioClient3 if not already defined by MinGW headers
#if defined __MINGW32__ || defined __MINGW64__

#ifndef __IAudioClient3_FWD_DEFINED__
#define __IAudioClient3_FWD_DEFINED__

typedef interface IAudioClient3 IAudioClient3;

#endif // __IAudioClient3_FWD_DEFINED__

#ifndef __IAudioClient3_INTERFACE_DEFINED__
#define __IAudioClient3_INTERFACE_DEFINED__

// clang-format off
MIDL_INTERFACE("7ED4EE07-8E67-4CD4-8C1A-2B7A5987AD42")
IAudioClient3 : public IAudioClient2 {
public:
	virtual HRESULT STDMETHODCALLTYPE GetSharedModeEnginePeriod(
			/* [annotation][in] */
			_In_ const WAVEFORMATEX *pFormat,
			/* [annotation][out] */
			_Out_ UINT32 *pDefaultPeriodInFrames,
			/* [annotation][out] */
			_Out_ UINT32 *pFundamentalPeriodInFrames,
			/* [annotation][out] */
			_Out_ UINT32 *pMinPeriodInFrames,
			/* [annotation][out] */
			_Out_ UINT32 *pMaxPeriodInFrames) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentSharedModeEnginePeriod(
			/* [unique][annotation][out] */
			_Out_ WAVEFORMATEX * *ppFormat,
			/* [annotation][out] */
			_Out_ UINT32 * pCurrentPeriodInFrames) = 0;

	virtual HRESULT STDMETHODCALLTYPE InitializeSharedAudioStream(
			/* [annotation][in] */
			_In_ DWORD StreamFlags,
			/* [annotation][in] */
			_In_ UINT32 PeriodInFrames,
			/* [annotation][in] */
			_In_ const WAVEFORMATEX *pFormat,
			/* [annotation][in] */
			_In_opt_ LPCGUID AudioSessionGuid) = 0;
};
// clang-format on
__CRT_UUID_DECL(IAudioClient3, 0x7ED4EE07, 0x8E67, 0x4CD4, 0x8C, 0x1A, 0x2B, 0x7A, 0x59, 0x87, 0xAD, 0x42)

#endif // __IAudioClient3_INTERFACE_DEFINED__

#endif // __MINGW32__ || __MINGW64__

#ifndef PKEY_Device_FriendlyNameGodot

#undef DEFINE_PROPERTYKEY
/* clang-format off */
#define DEFINE_PROPERTYKEY(id, a, b, c, d, e, f, g, h, i, j, k, l) \
	const PROPERTYKEY id = { { a, b, c, { d, e, f, g, h, i, j, k, } }, l };
/* clang-format on */

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyNameGodot, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);
#endif

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioClient3 = __uuidof(IAudioClient3);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

#define SAFE_RELEASE(memory) \
	if ((memory) != nullptr) { \
		(memory)->Release(); \
		(memory) = nullptr; \
	}

#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define CAPTURE_BUFFER_CHANNELS 2

static bool default_output_device_changed = false;
static bool default_input_device_changed = false;
static int output_reinit_countdown = 0;
static int input_reinit_countdown = 0;

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wnon-virtual-dtor") // Silence warning due to a COM API weirdness (GH-35194).

class CMMNotificationClient : public IMMNotificationClient {
	LONG _cRef = 1;

public:
	ComPtr<IMMDeviceEnumerator> enumerator = nullptr;

	CMMNotificationClient() {}
	virtual ~CMMNotificationClient() {}

	ULONG STDMETHODCALLTYPE AddRef() {
		return InterlockedIncrement(&_cRef);
	}

	ULONG STDMETHODCALLTYPE Release() {
		ULONG ulRef = InterlockedDecrement(&_cRef);
		if (0 == ulRef) {
			delete this;
		}
		return ulRef;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) {
		if (IID_IUnknown == riid) {
			AddRef();
			*ppvInterface = (IUnknown *)this;
		} else if (__uuidof(IMMNotificationClient) == riid) {
			AddRef();
			*ppvInterface = (IMMNotificationClient *)this;
		} else {
			*ppvInterface = nullptr;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) {
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) {
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) {
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) {
		if (role == eConsole) {
			if (flow == eRender) {
				default_output_device_changed = true;
			} else if (flow == eCapture) {
				default_input_device_changed = true;
			}
		}

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) {
		return S_OK;
	}
};

VLTR_GCC_WARNING_POP

static CMMNotificationClient notif_client;

Error AudioDriverWASAPI::audio_device_init(AudioDeviceWASAPI *p_device, bool p_input, bool p_reinit, bool p_no_audio_client_3) {
	// This function can be called recursively, so clean up before starting:
	audio_device_finish(p_device);

	WAVEFORMATEX *pwfex;
	ComPtr<IMMDeviceEnumerator> enumerator = nullptr;
	ComPtr<IMMDevice> output_device = nullptr;

	HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void **)&enumerator);
	ERR_FAIL_COND_V(hr != S_OK, ERR_CANT_OPEN);

	if (p_device->device_name == "Default") {
		hr = enumerator->GetDefaultAudioEndpoint(p_input ? eCapture : eRender, eConsole, &output_device);
	} else {
		ComPtr<IMMDeviceCollection> devices = nullptr;

		hr = enumerator->EnumAudioEndpoints(p_input ? eCapture : eRender, DEVICE_STATE_ACTIVE, &devices);
		ERR_FAIL_COND_V(hr != S_OK, ERR_CANT_OPEN);

		LPWSTR strId = nullptr;
		bool found = false;

		UINT count = 0;
		hr = devices->GetCount(&count);
		ERR_FAIL_COND_V(hr != S_OK, ERR_CANT_OPEN);

		for (ULONG i = 0; i < count && !found; i++) {
			ComPtr<IMMDevice> tmp_device = nullptr;

			hr = devices->Item(i, &tmp_device);
			ERR_BREAK_MSG(hr != S_OK, "Cannot get devices item.");

			ComPtr<IPropertyStore> props = nullptr;
			hr = tmp_device->OpenPropertyStore(STGM_READ, &props);
			ERR_BREAK_MSG(hr != S_OK, "Cannot open property store.");

			PROPVARIANT propvar;
			PropVariantInit(&propvar);

			hr = props->GetValue(PKEY_Device_FriendlyNameGodot, &propvar);
			ERR_BREAK_MSG(hr != S_OK, "Cannot get value.");

			if (p_device->device_name == String(propvar.pwszVal)) {
				hr = tmp_device->GetId(&strId);
				if (unlikely(hr != S_OK)) {
					PropVariantClear(&propvar);
					ERR_PRINT("Cannot get device ID string.");
					break;
				}

				found = true;
			}

			PropVariantClear(&propvar);
		}

		if (found) {
			hr = enumerator->GetDevice(strId, &output_device);
		}

		if (strId) {
			CoTaskMemFree(strId);
		}

		if (output_device == nullptr) {
			hr = enumerator->GetDefaultAudioEndpoint(p_input ? eCapture : eRender, eConsole, &output_device);
		}
	}

	if (p_reinit) {
		// In case we're trying to re-initialize the device, prevent throwing this error on the console,
		// otherwise if there is currently no device available this will spam the console.
		if (hr != S_OK) {
			return ERR_CANT_OPEN;
		}
	} else {
		ERR_FAIL_COND_V(hr != S_OK, ERR_CANT_OPEN);
	}

	if (notif_client.enumerator != nullptr) {
		notif_client.enumerator->UnregisterEndpointNotificationCallback(&notif_client);
		notif_client.enumerator = nullptr;
	}
	hr = enumerator->RegisterEndpointNotificationCallback(&notif_client);
	if (hr == S_OK) {
		notif_client.enumerator = enumerator;
	} else {
		ERR_PRINT("WASAPI: RegisterEndpointNotificationCallback error");
	}

	using_audio_client_3 = !p_input; // IID_IAudioClient3 is only used for adjustable output latency (not input)

	if (p_no_audio_client_3) {
		using_audio_client_3 = false;
	}

	if (using_audio_client_3) {
		hr = output_device->Activate(IID_IAudioClient3, CLSCTX_ALL, nullptr, (void **)&p_device->audio_client);
		if (hr != S_OK) {
			// IID_IAudioClient3 will never activate on OS versions before Windows 10.
			// Older Windows versions should fall back gracefully.
			using_audio_client_3 = false;
			print_verbose("WASAPI: Couldn't activate output_device with IAudioClient3 interface, falling back to IAudioClient interface");
		} else {
			print_verbose("WASAPI: Activated output_device using IAudioClient3 interface");
		}
	}
	if (!using_audio_client_3) {
		hr = output_device->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void **)&p_device->audio_client);
	}

	if (p_reinit) {
		if (hr != S_OK) {
			return ERR_CANT_OPEN;
		}
	} else {
		ERR_FAIL_COND_V(hr != S_OK, ERR_CANT_OPEN);
	}

	if (using_audio_client_3) {
		AudioClientProperties audioProps{};
		audioProps.cbSize = sizeof(AudioClientProperties);
		audioProps.bIsOffload = FALSE;
		audioProps.eCategory = AudioCategory_GameEffects;

		hr = ((IAudioClient3 *)p_device->audio_client)->SetClientProperties(&audioProps);
		ERR_FAIL_COND_V_MSG(hr != S_OK, ERR_CANT_OPEN, "WASAPI: SetClientProperties failed with error 0x" + String::num_uint64(hr, 16) + ".");
	}

	hr = p_device->audio_client->GetMixFormat(&pwfex);
	ERR_FAIL_COND_V(hr != S_OK, ERR_CANT_OPEN);
	// From this point onward, CoTaskMemFree(pwfex) must be called before returning or pwfex will leak!

	print_verbose("WASAPI: wFormatTag = " + itos(pwfex->wFormatTag));
	print_verbose("WASAPI: nChannels = " + itos(pwfex->nChannels));
	print_verbose("WASAPI: nSamplesPerSec = " + itos(pwfex->nSamplesPerSec));
	print_verbose("WASAPI: nAvgBytesPerSec = " + itos(pwfex->nAvgBytesPerSec));
	print_verbose("WASAPI: nBlockAlign = " + itos(pwfex->nBlockAlign));
	print_verbose("WASAPI: wBitsPerSample = " + itos(pwfex->wBitsPerSample));
	print_verbose("WASAPI: cbSize = " + itos(pwfex->cbSize));

	WAVEFORMATEX *closest = nullptr;
	hr = p_device->audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pwfex, &closest);
	if (hr == S_FALSE) {
		WARN_PRINT("WASAPI: Mix format is not supported by the output_device");
		if (closest) {
			print_verbose("WASAPI: closest->wFormatTag = " + itos(closest->wFormatTag));
			print_verbose("WASAPI: closest->nChannels = " + itos(closest->nChannels));
			print_verbose("WASAPI: closest->nSamplesPerSec = " + itos(closest->nSamplesPerSec));
			print_verbose("WASAPI: closest->nAvgBytesPerSec = " + itos(closest->nAvgBytesPerSec));
			print_verbose("WASAPI: closest->nBlockAlign = " + itos(closest->nBlockAlign));
			print_verbose("WASAPI: closest->wBitsPerSample = " + itos(closest->wBitsPerSample));
			print_verbose("WASAPI: closest->cbSize = " + itos(closest->cbSize));

			WARN_PRINT("WASAPI: Using closest match instead");
			CoTaskMemFree(pwfex);
			pwfex = closest;
		}
	}

	// Since we're using WASAPI Shared Mode we can't control any of these, we just tag along
	p_device->channels = pwfex->nChannels;
	p_device->format_tag = pwfex->wFormatTag;
	p_device->bits_per_sample = pwfex->wBitsPerSample;
	p_device->frame_size = (p_device->bits_per_sample / 8) * p_device->channels;

	if (p_device->format_tag == WAVE_FORMAT_EXTENSIBLE) {
		WAVEFORMATEXTENSIBLE *wfex = (WAVEFORMATEXTENSIBLE *)pwfex;

		if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
			p_device->format_tag = WAVE_FORMAT_PCM;
		} else if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
			p_device->format_tag = WAVE_FORMAT_IEEE_FLOAT;
		} else {
			ERR_PRINT("WASAPI: Format not supported");
			CoTaskMemFree(pwfex);
			ERR_FAIL_V(ERR_CANT_OPEN);
		}
	} else {
		if (p_device->format_tag != WAVE_FORMAT_PCM && p_device->format_tag != WAVE_FORMAT_IEEE_FLOAT) {
			ERR_PRINT("WASAPI: Format not supported");
			CoTaskMemFree(pwfex);
			ERR_FAIL_V(ERR_CANT_OPEN);
		}
	}

	if (!using_audio_client_3) {
		DWORD streamflags = 0;
		if ((DWORD)mix_rate != pwfex->nSamplesPerSec) {
			streamflags |= AUDCLNT_STREAMFLAGS_RATEADJUST;
			pwfex->nSamplesPerSec = mix_rate;
			pwfex->nAvgBytesPerSec = pwfex->nSamplesPerSec * pwfex->nChannels * (pwfex->wBitsPerSample / 8);
		}
		hr = p_device->audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, streamflags, p_input ? REFTIMES_PER_SEC : 0, 0, pwfex, nullptr);

		if (p_reinit) {
			// In case we're trying to re-initialize the device, prevent throwing this error on the console,
			// otherwise if there is currently no device available this will spam the console.
			if (hr != S_OK) {
				print_verbose("WASAPI: Initialize failed with error 0x" + String::num_uint64(hr, 16) + ".");
				CoTaskMemFree(pwfex);
				return ERR_CANT_OPEN;
			}
		} else {
			if (unlikely(hr != S_OK)) {
				CoTaskMemFree(pwfex);
				ERR_FAIL_V_MSG(ERR_CANT_OPEN, "WASAPI: Initialize failed with error 0x" + String::num_uint64(hr, 16) + ".");
			}
		}

		UINT32 max_frames;
		hr = p_device->audio_client->GetBufferSize(&max_frames);
		if (unlikely(hr != S_OK)) {
			CoTaskMemFree(pwfex);
			ERR_FAIL_V(ERR_CANT_OPEN);
		}

		// Due to WASAPI Shared Mode we have no control of the buffer size
		if (!p_input) {
			buffer_frames = max_frames;

			int64_t latency = 0;
			audio_output.audio_client->GetStreamLatency(&latency);
			// WASAPI REFERENCE_TIME units are 100 nanoseconds per unit
			// https://docs.microsoft.com/en-us/windows/win32/directshow/reference-time
			// Convert REFTIME to seconds as godot uses for latency
			real_latency = (float)latency / (float)REFTIMES_PER_SEC;
		}

	} else {
		IAudioClient3 *device_audio_client_3 = (IAudioClient3 *)p_device->audio_client;

		// AUDCLNT_STREAMFLAGS_RATEADJUST is an invalid flag with IAudioClient3, therefore we have to use
		// the closest supported mix rate supported by the audio driver.
		mix_rate = pwfex->nSamplesPerSec;
		print_verbose("WASAPI: mix_rate = " + itos(mix_rate));

		UINT32 default_period_frames, fundamental_period_frames, min_period_frames, max_period_frames;
		hr = device_audio_client_3->GetSharedModeEnginePeriod(
				pwfex,
				&default_period_frames,
				&fundamental_period_frames,
				&min_period_frames,
				&max_period_frames);
		if (hr != S_OK) {
			print_verbose("WASAPI: GetSharedModeEnginePeriod failed with error 0x" + String::num_uint64(hr, 16) + ", falling back to IAudioClient.");
			CoTaskMemFree(pwfex);
			return audio_device_init(p_device, p_input, p_reinit, true);
		}

		// Period frames must be an integral multiple of fundamental_period_frames or IAudioClient3 initialization will fail,
		// so we need to select the closest multiple to the user-specified latency.
		UINT32 desired_period_frames = target_latency_ms * mix_rate / 1000;
		UINT32 period_frames = (desired_period_frames / fundamental_period_frames) * fundamental_period_frames;
		if (Math::abs((int64_t)period_frames - (int64_t)desired_period_frames) > Math::abs((int64_t)(period_frames + fundamental_period_frames) - (int64_t)desired_period_frames)) {
			period_frames = period_frames + fundamental_period_frames;
		}
		period_frames = CLAMP(period_frames, min_period_frames, max_period_frames);
		print_verbose("WASAPI: fundamental_period_frames = " + itos(fundamental_period_frames));
		print_verbose("WASAPI: min_period_frames = " + itos(min_period_frames));
		print_verbose("WASAPI: max_period_frames = " + itos(max_period_frames));
		print_verbose("WASAPI: selected a period frame size of " + itos(period_frames));
		buffer_frames = period_frames;

		hr = device_audio_client_3->InitializeSharedAudioStream(0, period_frames, pwfex, nullptr);
		if (hr != S_OK) {
			print_verbose("WASAPI: InitializeSharedAudioStream failed with error 0x" + String::num_uint64(hr, 16) + ", falling back to IAudioClient.");
			CoTaskMemFree(pwfex);
			return audio_device_init(p_device, p_input, p_reinit, true);
		} else {
			uint32_t output_latency_in_frames;
			WAVEFORMATEX *current_pwfex;
			hr = device_audio_client_3->GetCurrentSharedModeEnginePeriod(&current_pwfex, &output_latency_in_frames);
			if (hr == OK) {
				real_latency = (float)output_latency_in_frames / (float)current_pwfex->nSamplesPerSec;
				CoTaskMemFree(current_pwfex);
			} else {
				print_verbose("WASAPI: GetCurrentSharedModeEnginePeriod failed with error 0x" + String::num_uint64(hr, 16) + ", falling back to IAudioClient.");
				CoTaskMemFree(pwfex);
				return audio_device_init(p_device, p_input, p_reinit, true);
			}
		}
	}

	if (p_input) {
		hr = p_device->audio_client->GetService(IID_IAudioCaptureClient, (void **)&p_device->capture_client);
	} else {
		hr = p_device->audio_client->GetService(IID_IAudioRenderClient, (void **)&p_device->render_client);
	}
	if (unlikely(hr != S_OK)) {
		CoTaskMemFree(pwfex);
		ERR_FAIL_V(ERR_CANT_OPEN);
	}

	// Free memory
	CoTaskMemFree(pwfex);

	return OK;
}

Error AudioDriverWASAPI::init_output_device(bool p_reinit) {
	Error err = audio_device_init(&audio_output, false, p_reinit);
	if (err != OK) {
		// We've tried to init the device, but have failed. Time to clean up.
		Error finish_err = finish_output_device();
		if (finish_err != OK) {
			ERR_PRINT("WASAPI: finish_output_device error after failed output audio_device_init");
		}
		return err;
	}

	switch (audio_output.channels) {
		case 1: // Mono
		case 3: // Surround 2.1
		case 5: // Surround 5.0
		case 7: // Surround 7.0
			// We will downmix as required.
			channels = audio_output.channels + 1;
			break;

		case 2: // Stereo
		case 4: // Surround 3.1
		case 6: // Surround 5.1
		case 8: // Surround 7.1
			channels = audio_output.channels;
			break;

		default:
			WARN_PRINT("WASAPI: Unsupported number of channels: " + itos(audio_output.channels));
			channels = 2;
			break;
	}

	// Sample rate is independent of channels (ref: https://stackoverflow.com/questions/11048825/audio-sample-frequency-rely-on-channels)
	samples_in.resize(buffer_frames * channels);

	input_position = 0;
	input_size = 0;

	print_verbose("WASAPI: detected " + itos(audio_output.channels) + " channels");
	print_verbose("WASAPI: audio buffer frames: " + itos(buffer_frames) + " calculated latency: " + itos(buffer_frames * 1000 / mix_rate) + "ms");

	return OK;
}

Error AudioDriverWASAPI::init_input_device(bool p_reinit) {
	if (audio_input.active.is_set()) {
		return ERR_ALREADY_IN_USE;
	}

	Error err = audio_device_init(&audio_input, true, p_reinit);
	if (err != OK) {
		// We've tried to init the device, but have failed. Time to clean up.
		Error finish_err = finish_input_device();
		if (finish_err != OK) {
			ERR_PRINT("WASAPI: finish_input_device error after failed input audio_device_init");
		}
		return err;
	}

	// Get the max frames
	UINT32 max_frames;
	HRESULT hr = audio_input.audio_client->GetBufferSize(&max_frames);
	ERR_FAIL_COND_V(hr != S_OK, ERR_CANT_OPEN);

	input_buffer_init(max_frames);

	return OK;
}

Error AudioDriverWASAPI::audio_device_finish(AudioDeviceWASAPI *p_device) {
	if (p_device->active.is_set()) {
		if (p_device->audio_client) {
			p_device->audio_client->Stop();
		}
		p_device->active.clear();
	}

	SAFE_RELEASE(p_device->audio_client)
	SAFE_RELEASE(p_device->render_client)
	SAFE_RELEASE(p_device->capture_client)

	return OK;
}

Error AudioDriverWASAPI::finish_output_device() {
	return audio_device_finish(&audio_output);
}

Error AudioDriverWASAPI::finish_input_device() {
	return audio_device_finish(&audio_input);
}

Error AudioDriverWASAPI::init() {
	mix_rate = _get_configured_mix_rate();

	target_latency_ms = Engine::get_audio_output_latency();

	exit_thread.clear();

	Error err = init_output_device();
	ERR_FAIL_COND_V_MSG(err != OK, err, "WASAPI: init_output_device error.");

	thread.start(thread_func, this);

	return OK;
}

int AudioDriverWASAPI::get_mix_rate() const {
	return mix_rate;
}

float AudioDriverWASAPI::get_latency() {
	return real_latency;
}

AudioDriver::SpeakerMode AudioDriverWASAPI::get_speaker_mode() const {
	return get_speaker_mode_by_total_channels(channels);
}

PackedStringArray AudioDriverWASAPI::audio_device_get_list(bool p_input) {
	PackedStringArray list;
	ComPtr<IMMDeviceCollection> devices = nullptr;
	ComPtr<IMMDeviceEnumerator> enumerator = nullptr;

	list.push_back(String("Default"));

	HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void **)&enumerator);
	ERR_FAIL_COND_V(hr != S_OK, PackedStringArray());

	hr = enumerator->EnumAudioEndpoints(p_input ? eCapture : eRender, DEVICE_STATE_ACTIVE, &devices);
	ERR_FAIL_COND_V(hr != S_OK, PackedStringArray());

	UINT count = 0;
	hr = devices->GetCount(&count);
	ERR_FAIL_COND_V(hr != S_OK, PackedStringArray());

	for (ULONG i = 0; i < count; i++) {
		ComPtr<IMMDevice> output_device = nullptr;

		hr = devices->Item(i, &output_device);
		ERR_BREAK(hr != S_OK);

		ComPtr<IPropertyStore> props = nullptr;
		hr = output_device->OpenPropertyStore(STGM_READ, &props);
		ERR_BREAK(hr != S_OK);

		PROPVARIANT propvar;
		PropVariantInit(&propvar);

		hr = props->GetValue(PKEY_Device_FriendlyNameGodot, &propvar);
		ERR_BREAK(hr != S_OK);

		list.push_back(String(propvar.pwszVal));

		PropVariantClear(&propvar);
	}

	return list;
}

PackedStringArray AudioDriverWASAPI::get_output_device_list() {
	return audio_device_get_list(false);
}

String AudioDriverWASAPI::get_output_device() {
	lock();
	String name = audio_output.device_name;
	unlock();

	return name;
}

void AudioDriverWASAPI::set_output_device(const String &p_name) {
	lock();
	audio_output.new_device = p_name;
	unlock();
}

int32_t AudioDriverWASAPI::read_sample(WORD format_tag, int bits_per_sample, BYTE *buffer, int i) {
	if (format_tag == WAVE_FORMAT_PCM) {
		int32_t sample = 0;
		switch (bits_per_sample) {
			case 8:
				sample = int32_t(((int8_t *)buffer)[i]) << 24;
				break;

			case 16:
				sample = int32_t(((int16_t *)buffer)[i]) << 16;
				break;

			case 24:
				sample |= int32_t(((int8_t *)buffer)[i * 3 + 2]) << 24;
				sample |= int32_t(((int8_t *)buffer)[i * 3 + 1]) << 16;
				sample |= int32_t(((int8_t *)buffer)[i * 3 + 0]) << 8;
				break;

			case 32:
				sample = ((int32_t *)buffer)[i];
				break;
		}

		return sample;
	} else if (format_tag == WAVE_FORMAT_IEEE_FLOAT) {
		return int32_t(((float *)buffer)[i] * 32768.0) << 16;
	} else {
		ERR_PRINT("WASAPI: Unknown format tag");
	}

	return 0;
}

void AudioDriverWASAPI::write_sample(WORD format_tag, int bits_per_sample, BYTE *buffer, int i, int32_t sample) {
	if (format_tag == WAVE_FORMAT_PCM) {
		switch (bits_per_sample) {
			case 8:
				((int8_t *)buffer)[i] = sample >> 24;
				break;

			case 16:
				((int16_t *)buffer)[i] = sample >> 16;
				break;

			case 24:
				((int8_t *)buffer)[i * 3 + 2] = sample >> 24;
				((int8_t *)buffer)[i * 3 + 1] = sample >> 16;
				((int8_t *)buffer)[i * 3 + 0] = sample >> 8;
				break;

			case 32:
				((int32_t *)buffer)[i] = sample;
				break;
		}
	} else if (format_tag == WAVE_FORMAT_IEEE_FLOAT) {
		((float *)buffer)[i] = (sample >> 16) / 32768.f;
	} else {
		ERR_PRINT("WASAPI: Unknown format tag");
	}
}

void AudioDriverWASAPI::thread_func(void *p_udata) {
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	AudioDriverWASAPI *ad = static_cast<AudioDriverWASAPI *>(p_udata);
	uint32_t avail_frames = 0;
	uint32_t write_ofs = 0;

	while (!ad->exit_thread.is_set()) {
		uint32_t read_frames = 0;
		uint32_t written_frames = 0;

		if (avail_frames == 0) {
			ad->lock();
			ad->start_counting_ticks();

			if (ad->audio_output.active.is_set()) {
				ad->audio_server_process(ad->buffer_frames, ad->samples_in.ptrw());
			} else {
				ad->samples_in.fill(0);
			}

			avail_frames = ad->buffer_frames;
			write_ofs = 0;

			ad->stop_counting_ticks();
			ad->unlock();
		}

		ad->lock();
		ad->start_counting_ticks();

		if (avail_frames > 0 && ad->audio_output.audio_client) {
			UINT32 buffer_size;
			UINT32 cur_frames;
			bool invalidated = false;
			HRESULT hr = ad->audio_output.audio_client->GetBufferSize(&buffer_size);
			if (hr != S_OK) {
				ERR_PRINT("WASAPI: GetBufferSize error");
			}
			hr = ad->audio_output.audio_client->GetCurrentPadding(&cur_frames);
			if (hr == S_OK) {
				// Check how much frames are available on the WASAPI buffer
				UINT32 write_frames = MIN(buffer_size - cur_frames, avail_frames);
				if (write_frames > 0) {
					BYTE *buffer = nullptr;
					hr = ad->audio_output.render_client->GetBuffer(write_frames, &buffer);
					if (hr == S_OK) {
						// We're using WASAPI Shared Mode so we must convert the buffer
						if (ad->channels == ad->audio_output.channels) {
							for (unsigned int i = 0; i < write_frames * ad->channels; i++) {
								ad->write_sample(ad->audio_output.format_tag, ad->audio_output.bits_per_sample, buffer, i, ad->samples_in[write_ofs++]);
							}
						} else if (ad->channels == ad->audio_output.channels + 1) {
							// Pass all channels except the last two as-is, and then mix the last two
							// together as one channel. E.g. stereo -> mono, or 3.1 -> 2.1.
							unsigned int last_chan = ad->audio_output.channels - 1;
							for (unsigned int i = 0; i < write_frames; i++) {
								for (unsigned int j = 0; j < last_chan; j++) {
									ad->write_sample(ad->audio_output.format_tag, ad->audio_output.bits_per_sample, buffer, i * ad->audio_output.channels + j, ad->samples_in[write_ofs++]);
								}
								int32_t l = ad->samples_in[write_ofs++];
								int32_t r = ad->samples_in[write_ofs++];
								int32_t c = (int32_t)(((int64_t)l + (int64_t)r) / 2);
								ad->write_sample(ad->audio_output.format_tag, ad->audio_output.bits_per_sample, buffer, i * ad->audio_output.channels + last_chan, c);
							}
						} else {
							for (unsigned int i = 0; i < write_frames; i++) {
								for (unsigned int j = 0; j < MIN(ad->channels, ad->audio_output.channels); j++) {
									ad->write_sample(ad->audio_output.format_tag, ad->audio_output.bits_per_sample, buffer, i * ad->audio_output.channels + j, ad->samples_in[write_ofs++]);
								}
								if (ad->audio_output.channels > ad->channels) {
									for (unsigned int j = ad->channels; j < ad->audio_output.channels; j++) {
										ad->write_sample(ad->audio_output.format_tag, ad->audio_output.bits_per_sample, buffer, i * ad->audio_output.channels + j, 0);
									}
								}
							}
						}

						hr = ad->audio_output.render_client->ReleaseBuffer(write_frames, 0);
						if (hr != S_OK) {
							ERR_PRINT("WASAPI: Release buffer error");
						}

						avail_frames -= write_frames;
						written_frames += write_frames;
					} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
						// output_device is not valid anymore, reopen it

						Error err = ad->finish_output_device();
						if (err != OK) {
							ERR_PRINT("WASAPI: finish_output_device error");
						} else {
							// We reopened the output device and samples_in may have resized, so invalidate the current avail_frames
							avail_frames = 0;
						}
					} else {
						ERR_PRINT("WASAPI: Get buffer error");
						ad->exit_thread.set();
					}
				}
			} else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
				invalidated = true;
			} else {
				ERR_PRINT("WASAPI: GetCurrentPadding error");
			}

			if (invalidated) {
				// output_device is not valid anymore
				WARN_PRINT("WASAPI: Current output_device invalidated, closing output_device");

				Error err = ad->finish_output_device();
				if (err != OK) {
					ERR_PRINT("WASAPI: finish_output_device error");
				}
			}
		}

		// If we're using the Default output device and it changed finish it so we'll re-init the output device
		if (ad->audio_output.device_name == "Default" && default_output_device_changed) {
			Error err = ad->finish_output_device();
			if (err != OK) {
				ERR_PRINT("WASAPI: finish_output_device error");
			}

			default_output_device_changed = false;
		}

		// User selected a new output device, finish the current one so we'll init the new output device
		if (ad->audio_output.device_name != ad->audio_output.new_device) {
			ad->audio_output.device_name = ad->audio_output.new_device;
			Error err = ad->finish_output_device();
			if (err != OK) {
				ERR_PRINT("WASAPI: finish_output_device error");
			}
		}

		if (!ad->audio_output.audio_client) {
			if (output_reinit_countdown < 1) {
				Error err = ad->init_output_device(true);
				if (err == OK) {
					ad->start();
				} else {
					output_reinit_countdown = 1000;
				}
			} else {
				output_reinit_countdown--;
			}

			avail_frames = 0;
			write_ofs = 0;
		}

		if (ad->audio_input.active.is_set()) {
			UINT32 packet_length = 0;
			BYTE *data;
			UINT32 num_frames_available;
			DWORD flags;

			HRESULT hr = ad->audio_input.capture_client->GetNextPacketSize(&packet_length);
			if (hr == S_OK) {
				while (packet_length != 0) {
					hr = ad->audio_input.capture_client->GetBuffer(&data, &num_frames_available, &flags, nullptr, nullptr);
					ERR_BREAK(hr != S_OK);

					// fixme: Only works for floating point atm
					for (UINT32 j = 0; j < num_frames_available; j++) {
						int32_t l, r;

						if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
							l = r = 0;
						} else {
							if (ad->audio_input.channels == 2) {
								l = read_sample(ad->audio_input.format_tag, ad->audio_input.bits_per_sample, data, j * 2);
								r = read_sample(ad->audio_input.format_tag, ad->audio_input.bits_per_sample, data, j * 2 + 1);
							} else if (ad->audio_input.channels == 1) {
								l = r = read_sample(ad->audio_input.format_tag, ad->audio_input.bits_per_sample, data, j);
							} else {
								l = r = 0;
								ERR_PRINT("WASAPI: unsupported channel count in microphone!");
							}
						}

						ad->input_buffer_write(l);
						ad->input_buffer_write(r);
					}

					read_frames += num_frames_available;

					hr = ad->audio_input.capture_client->ReleaseBuffer(num_frames_available);
					ERR_BREAK(hr != S_OK);

					hr = ad->audio_input.capture_client->GetNextPacketSize(&packet_length);
					ERR_BREAK(hr != S_OK);
				}
			}

			// If we're using the Default output device and it changed finish it so we'll re-init the output device
			if (ad->audio_input.device_name == "Default" && default_input_device_changed) {
				Error err = ad->finish_input_device();
				if (err != OK) {
					ERR_PRINT("WASAPI: finish_input_device error");
				}

				default_input_device_changed = false;
			}

			// User selected a new input device, finish the current one so we'll init the new input device
			if (ad->audio_input.device_name != ad->audio_input.new_device) {
				ad->audio_input.device_name = ad->audio_input.new_device;
				Error err = ad->finish_input_device();
				if (err != OK) {
					ERR_PRINT("WASAPI: finish_input_device error");
				}
			}

			if (!ad->audio_input.audio_client) {
				if (input_reinit_countdown < 1) {
					Error err = ad->init_input_device(true);
					if (err == OK) {
						ad->input_start();
					} else {
						input_reinit_countdown = 1000;
					}
				} else {
					input_reinit_countdown--;
				}
			}
		}

		ad->stop_counting_ticks();
		ad->unlock();

		// Let the thread rest a while if we haven't read or write anything
		if (written_frames == 0 && read_frames == 0) {
			OS::delay_usec(1000);
		}
	}
	CoUninitialize();
}

void AudioDriverWASAPI::start() {
	if (audio_output.audio_client) {
		HRESULT hr = audio_output.audio_client->Start();
		if (hr != S_OK) {
			ERR_PRINT("WASAPI: Start failed");
		} else {
			audio_output.active.set();
		}
	}
}

void AudioDriverWASAPI::lock() {
	mutex.lock();
}

void AudioDriverWASAPI::unlock() {
	mutex.unlock();
}

void AudioDriverWASAPI::finish() {
	exit_thread.set();
	if (thread.is_started()) {
		thread.wait_to_finish();
	}

	finish_input_device();
	finish_output_device();
}

Error AudioDriverWASAPI::input_start() {
	Error err = init_input_device();
	if (err != OK) {
		ERR_PRINT("WASAPI: init_input_device error");
		return err;
	}

	if (audio_input.active.is_set()) {
		return FAILED;
	}

	audio_input.audio_client->Start();
	audio_input.active.set();
	return OK;
}

Error AudioDriverWASAPI::input_stop() {
	if (audio_input.active.is_set()) {
		audio_input.audio_client->Stop();
		audio_input.active.clear();
	}

	return OK;
}

PackedStringArray AudioDriverWASAPI::get_input_device_list() {
	return audio_device_get_list(true);
}

String AudioDriverWASAPI::get_input_device() {
	lock();
	String name = audio_input.device_name;
	unlock();

	return name;
}

void AudioDriverWASAPI::set_input_device(const String &p_name) {
	lock();
	audio_input.new_device = p_name;
	unlock();
}

AudioDriverWASAPI::AudioDriverWASAPI() {
	samples_in.clear();
}

#endif // WASAPI_ENABLED

// === Merged from drivers/xaudio2/audio_driver_xaudio2.cpp ===
#ifdef XAUDIO2_ENABLED
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

#include "core/os/os.h"
#include "core/io/ip.h"

Error AudioDriverXAudio2::init() {
	active.clear();
	exit_thread.clear();
	pcm_open = false;
	samples_in = nullptr;

	mix_rate = _get_configured_mix_rate();

	// FIXME: speaker_mode seems unused in the Xaudio2 driver so far
	speaker_mode = SPEAKER_MODE_STEREO;
	channels = 2;

	int latency = Engine::get_audio_output_latency();
	buffer_size = Math::closest_power_of_2(latency * mix_rate / 1000);

	samples_in = memnew_arr(int32_t, size_t(buffer_size) * channels);
	for (int i = 0; i < AUDIO_BUFFERS; i++) {
		samples_out[i] = memnew_arr(int16_t, size_t(buffer_size) * channels);
		xaudio_buffer[i].AudioBytes = buffer_size * channels * sizeof(int16_t);
		xaudio_buffer[i].pAudioData = (const BYTE *)(samples_out[i]);
		xaudio_buffer[i].Flags = 0;
	}

	HRESULT hr;
	hr = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	ERR_FAIL_COND_V_MSG(hr != S_OK, ERR_UNAVAILABLE, "Error creating XAudio2 engine.");

	hr = xaudio->CreateMasteringVoice(&mastering_voice);
	ERR_FAIL_COND_V_MSG(hr != S_OK, ERR_UNAVAILABLE, "Error creating XAudio2 mastering voice.");

	wave_format.nChannels = channels;
	wave_format.cbSize = 0;
	wave_format.nSamplesPerSec = mix_rate;
	wave_format.wFormatTag = WAVE_FORMAT_PCM;
	wave_format.wBitsPerSample = 16;
	wave_format.nBlockAlign = channels * wave_format.wBitsPerSample >> 3;
	wave_format.nAvgBytesPerSec = mix_rate * wave_format.nBlockAlign;

	hr = xaudio->CreateSourceVoice(&source_voice, &wave_format, 0, XAUDIO2_MAX_FREQ_RATIO, &voice_callback);
	ERR_FAIL_COND_V_MSG(hr != S_OK, ERR_UNAVAILABLE, "Error creating XAudio2 source voice. Error code: " + itos(hr) + ".");

	thread.start(AudioDriverXAudio2::thread_func, this);

	return OK;
}

void AudioDriverXAudio2::thread_func(void *p_udata) {
	AudioDriverXAudio2 *ad = static_cast<AudioDriverXAudio2 *>(p_udata);

	while (!ad->exit_thread.is_set()) {
		if (!ad->active.is_set()) {
			for (int i = 0; i < AUDIO_BUFFERS; i++) {
				ad->xaudio_buffer[i].Flags = XAUDIO2_END_OF_STREAM;
			}

		} else {
			ad->lock();
			ad->start_counting_ticks();

			ad->audio_server_process(ad->buffer_size, ad->samples_in);

			ad->stop_counting_ticks();
			ad->unlock();

			for (int i = 0; i < ad->buffer_size * ad->channels; i++) {
				ad->samples_out[ad->current_buffer][i] = ad->samples_in[i] >> 16;
			}

			ad->xaudio_buffer[ad->current_buffer].Flags = 0;
			ad->xaudio_buffer[ad->current_buffer].AudioBytes = ad->buffer_size * ad->channels * sizeof(int16_t);
			ad->xaudio_buffer[ad->current_buffer].pAudioData = (const BYTE *)(ad->samples_out[ad->current_buffer]);
			ad->xaudio_buffer[ad->current_buffer].PlayBegin = 0;
			ad->source_voice->SubmitSourceBuffer(&(ad->xaudio_buffer[ad->current_buffer]));

			ad->current_buffer = (ad->current_buffer + 1) % AUDIO_BUFFERS;

			XAUDIO2_VOICE_STATE state;
			while (ad->source_voice->GetState(&state), state.BuffersQueued > AUDIO_BUFFERS - 1) {
				WaitForSingleObject(ad->voice_callback.buffer_end_event, INFINITE);
			}
		}
	}
}

void AudioDriverXAudio2::start() {
	active.set();
	HRESULT hr = source_voice->Start(0);
	ERR_FAIL_COND_MSG(hr != S_OK, "Error starting XAudio2 driver. Error code: " + itos(hr) + ".");
}

int AudioDriverXAudio2::get_mix_rate() const {
	return mix_rate;
}

AudioDriver::SpeakerMode AudioDriverXAudio2::get_speaker_mode() const {
	return speaker_mode;
}

float AudioDriverXAudio2::get_latency() {
	XAUDIO2_PERFORMANCE_DATA perf_data;
	xaudio->GetPerformanceData(&perf_data);
	if (perf_data.CurrentLatencyInSamples) {
		return (float)(perf_data.CurrentLatencyInSamples / ((float)mix_rate));
	} else {
		return 0;
	}
}

void AudioDriverXAudio2::lock() {
	mutex.lock();
}

void AudioDriverXAudio2::unlock() {
	mutex.unlock();
}

void AudioDriverXAudio2::finish() {
	exit_thread.set();
	if (thread.is_started()) {
		thread.wait_to_finish();
	}

	if (source_voice) {
		source_voice->Stop(0);
		source_voice->DestroyVoice();
	}

	if (samples_in) {
		memdelete_arr(samples_in);
	}
	if (samples_out[0]) {
		for (int i = 0; i < AUDIO_BUFFERS; i++) {
			memdelete_arr(samples_out[i]);
		}
	}

	mastering_voice->DestroyVoice();
}

AudioDriverXAudio2::AudioDriverXAudio2() {
	for (int i = 0; i < AUDIO_BUFFERS; i++) {
		xaudio_buffer[i] = {};
		samples_out[i] = nullptr;
	}
}
#endif // XAUDIO2_ENABLED

// === Merged from platform/windows/crash_handler_windows_seh.cpp & crash_handler_windows_signal.cpp ===
#if defined(_MSC_VER)
#include "servers/display/display_server.h"

#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/typedefs.h"

#ifdef CRASH_HANDLER_EXCEPTION

// Backtrace code based on: https://stackoverflow.com/questions/6205981/windows-c-stack-trace-from-a-running-app

#include <psapi.h>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

// Some versions of imagehlp.dll lack the proper packing directives themselves
// so we need to do it.
#pragma pack(push, before_imagehlp, 8)
#include <imagehlp.h>
#pragma pack(pop, before_imagehlp)

static void *load_all_modules(HANDLE process, std::vector<HMODULE> &module_handles) {
	void *first_base = nullptr;
	for (size_t i = 0; i < module_handles.size(); i++) {
		char image_name[4096];
		char module_name[4096];
		MODULEINFO mi = {};
		GetModuleInformation(process, module_handles[i], &mi, sizeof(mi));
		if (i == 0) {
			first_base = mi.lpBaseOfDll;
		}
		GetModuleFileNameEx(process, module_handles[i], image_name, sizeof(image_name));
		GetModuleBaseName(process, module_handles[i], module_name, sizeof(module_name));
		SymLoadModule64(process, nullptr, image_name, module_name, (DWORD64)mi.lpBaseOfDll, mi.SizeOfImage);
	}
	return first_base;
}

static std::string get_undecorated_symbol_name(HANDLE process, DWORD64 address) {
	static const int max_name_len = 1024;
	std::vector<char> buf(sizeof(IMAGEHLP_SYMBOL64) + max_name_len, 0);
	IMAGEHLP_SYMBOL64 *sym = reinterpret_cast<IMAGEHLP_SYMBOL64 *>(buf.data());
	sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	sym->MaxNameLength = max_name_len;
	DWORD64 displacement = 0;
	SymGetSymFromAddr64(process, address, &displacement, sym);
	if (*sym->Name == '\0') {
		return "<couldn't map PC to fn name>";
	}
	std::vector<char> und_name(max_name_len, 0);
	UnDecorateSymbolName(sym->Name, und_name.data(), max_name_len, UNDNAME_COMPLETE);
	return std::string(und_name.data(), strlen(und_name.data()));
}

DWORD CrashHandlerException(EXCEPTION_POINTERS *ep) {
	HANDLE process = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	DWORD offset_from_symbol = 0;
	IMAGEHLP_LINE64 line = {};
	DWORD cbNeeded;
	std::vector<HMODULE> module_handles(1);

	if (!OS::is_initialized() || OS::is_disable_crash_handler() || IsDebuggerPresent()) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (OS::is_crash_handler_silent()) {
		std::_Exit(0);
	}

	String msg;
	if (ProjectSettings::is_initialized()) {
		msg = GLOBAL_GET("debug/settings/crash_handler/message");
	}

	// Tell MainLoop about the crash. This can be handled by users too in Node.
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_CRASH);
	}

	print_error("\n================================================================");
	print_error(vformat("%s: Program crashed", __FUNCTION__));

	// Print the engine version just before, so that people are reminded to include the version in backtrace reports.
	if (String(VLTR_VERSION_HASH).is_empty()) {
		print_error(vformat("Engine version: %s", VLTR_VERSION_FULL_NAME));
	} else {
		print_error(vformat("Engine version: %s (%s)", VLTR_VERSION_FULL_NAME, VLTR_VERSION_HASH));
	}
	print_error(vformat("Dumping the backtrace. %s", msg));

	// Load the symbols:
	if (!SymInitialize(process, nullptr, false)) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_EXACT_SYMBOLS);
	EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
	module_handles.resize(cbNeeded / sizeof(HMODULE));
	EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
	void *base = load_all_modules(process, module_handles);

	print_error(vformat("Load address: %x\n", (uint64_t)base));

	// Setup stuff:
	CONTEXT *context = ep->ContextRecord;
	STACKFRAME64 frame;
	bool skip_first = false;

	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;

#if defined(_M_X64)
	frame.AddrPC.Offset = context->Rip;
	frame.AddrStack.Offset = context->Rsp;
	frame.AddrFrame.Offset = context->Rbp;
#elif defined(_M_ARM64) || defined(_M_ARM64EC)
	frame.AddrPC.Offset = context->Pc;
	frame.AddrStack.Offset = context->Sp;
	frame.AddrFrame.Offset = context->Fp;
#elif defined(_M_ARM)
	frame.AddrPC.Offset = context->Pc;
	frame.AddrStack.Offset = context->Sp;
	frame.AddrFrame.Offset = context->R11;
#else
	frame.AddrPC.Offset = context->Eip;
	frame.AddrStack.Offset = context->Esp;
	frame.AddrFrame.Offset = context->Ebp;

	// Skip the first one to avoid a duplicate on 32-bit mode
	skip_first = true;
#endif

	line.SizeOfStruct = sizeof(line);
	IMAGE_NT_HEADERS *h = ImageNtHeader(base);
	DWORD image_type = h->FileHeader.Machine;

	int n = 0;
	do {
		if (skip_first) {
			skip_first = false;
		} else {
			if (frame.AddrPC.Offset != 0) {
				std::string fnName = get_undecorated_symbol_name(process, frame.AddrPC.Offset);

				IMAGEHLP_MODULE64 mod_info;
				memset(&mod_info, 0, sizeof(IMAGEHLP_MODULE64));
				mod_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
				uint64_t offset = (uint64_t)base;
				String mod_name = "main";
				if (SymGetModuleInfo64(process, frame.AddrPC.Offset, &mod_info)) {
					offset = mod_info.BaseOfImage;
					if (offset != (uint64_t)base) {
						if (mod_info.ImageName[0] != 0) {
							mod_name = String((const char *)mod_info.ImageName).to_lower().get_file();
						} else if (mod_info.ModuleName[0] != 0) {
							mod_name = String((const char *)mod_info.ModuleName).to_lower();
						} else {
							mod_name = "<unknown module>";
						}
					}
				}
				if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &offset_from_symbol, &line)) {
					print_error(vformat("[%d] %x (%s+%x) - %s (%s:%d)", n, (uint64_t)frame.AddrPC.Offset, mod_name, (uint64_t)frame.AddrPC.Offset - offset, fnName.c_str(), (char *)line.FileName, (int)line.LineNumber));
				} else if (!fnName.empty()) {
					print_error(vformat("[%d] %x (%s+%x) - %s", n, (uint64_t)frame.AddrPC.Offset, mod_name, (uint64_t)frame.AddrPC.Offset - offset, fnName.c_str()));
				} else {
					print_error(vformat("[%d] %x (%s+%x) - ???", n, (uint64_t)frame.AddrPC.Offset, mod_name, (uint64_t)frame.AddrPC.Offset - offset));
				}
			}

			n++;
		}

		if (!StackWalk64(image_type, process, hThread, &frame, context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
			break;
		}
	} while (frame.AddrReturn.Offset != 0 && n < 256);

	print_error("-- END OF C++ BACKTRACE --");
	print_error("================================================================");

	SymCleanup(process);

	for (const Ref<ScriptBacktrace> &backtrace : ScriptServer::capture_script_backtraces(false)) {
		if (!backtrace->is_empty()) {
			print_error(backtrace->format());
			print_error(vformat("-- END OF %s BACKTRACE --", backtrace->get_language_name().to_upper()));
			print_error("================================================================");
		}
	}

	// Pass the exception to the OS
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

CrashHandler::CrashHandler() {
	disabled = false;
}

CrashHandler::~CrashHandler() {
}

void CrashHandler::disable() {
	if (disabled) {
		return;
	}

	disabled = true;
}

void CrashHandler::initialize() {
}
#else
#include "servers/display/display_server.h"

#include "core/os/os.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/typedefs.h"

#ifdef CRASH_HANDLER_EXCEPTION

#include <thirdparty/libbacktrace/backtrace.h>

#include <cxxabi.h>
#include <psapi.h>

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <iterator>
#include <vector>

// Some versions of imagehlp.dll lack the proper packing directives themselves
// so we need to do it.
#pragma pack(push, before_imagehlp, 8)
#include <imagehlp.h>
#pragma pack(pop, before_imagehlp)

struct CrashHandlerData {
	int64_t index = 0;
	backtrace_state *state = nullptr;
	int64_t offset = 0;
	int64_t base = 0;
	uint64_t pc = 0;
	HANDLE process = nullptr;
	bool sym_ok = false;
};

static void *load_all_modules_mingw(HANDLE process, std::vector<HMODULE> &module_handles) {
	void *first_base = nullptr;
	for (size_t i = 0; i < module_handles.size(); i++) {
		char image_name[4096];
		char module_name[4096];
		MODULEINFO mi = {};
		GetModuleInformation(process, module_handles[i], &mi, sizeof(mi));
		if (i == 0) {
			first_base = mi.lpBaseOfDll;
		}
		GetModuleFileNameEx(process, module_handles[i], image_name, sizeof(image_name));
		GetModuleBaseName(process, module_handles[i], module_name, sizeof(module_name));
		SymLoadModule64(process, nullptr, image_name, module_name, (DWORD64)mi.lpBaseOfDll, mi.SizeOfImage);
	}
	return first_base;
}

int symbol_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function) {
	CrashHandlerData *ch_data = reinterpret_cast<CrashHandlerData *>(data);
	uint64_t offset = (uint64_t)ch_data->base;
	String mod_name = "main";
	if (ch_data->sym_ok) {
		IMAGEHLP_MODULE64 mod_info;
		memset(&mod_info, 0, sizeof(IMAGEHLP_MODULE64));
		mod_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
		if (SymGetModuleInfo64(ch_data->process, ch_data->pc, &mod_info)) {
			offset = mod_info.BaseOfImage;
			if (offset != (uint64_t)ch_data->base) {
				if (mod_info.ImageName[0] != 0) {
					mod_name = String((const char *)mod_info.ImageName).to_lower().get_file();
				} else if (mod_info.ModuleName[0] != 0) {
					mod_name = String((const char *)mod_info.ModuleName).to_lower();
				} else {
					mod_name = "<unknown module>";
				}
			}
		}
	}

	if (function) {
		char fname[1024];
		snprintf(fname, 1024, "%s", function);

		if (function[0] == '_') {
			int status;
			char *demangled = abi::__cxa_demangle(function, nullptr, nullptr, &status);

			if (status == 0 && demangled) {
				snprintf(fname, 1024, "%s", demangled);
			}

			if (demangled) {
				free(demangled);
			}
		}
		print_error(vformat("[%d] %x (%s+%x) - %s (%s:%d)", ch_data->index++, ch_data->pc, mod_name, ch_data->pc - offset, String::utf8(fname), String::utf8(filename), lineno));
	} else if ((int64_t)ch_data->pc > 0) {
		print_error(vformat("[%d] %x (%s+%x) - ???", ch_data->index++, ch_data->pc, mod_name, ch_data->pc - offset));
	}
	return 0;
}

void error_callback(void *data, const char *msg, int errnum) {
	CrashHandlerData *ch_data = reinterpret_cast<CrashHandlerData *>(data);
	if (ch_data->index == 0) {
		print_error(vformat("Error(%d): %s", errnum, String::utf8(msg)));
	} else {
		uint64_t offset = (uint64_t)ch_data->base;
		String mod_name = "main";
		if (ch_data->sym_ok) {
			IMAGEHLP_MODULE64 mod_info;
			memset(&mod_info, 0, sizeof(IMAGEHLP_MODULE64));
			mod_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
			if (SymGetModuleInfo64(ch_data->process, ch_data->pc, &mod_info)) {
				offset = mod_info.BaseOfImage;
				if (offset != (uint64_t)ch_data->base) {
					if (mod_info.ImageName[0] != 0) {
						mod_name = String((const char *)mod_info.ImageName).to_lower().get_file();
					} else if (mod_info.ModuleName[0] != 0) {
						mod_name = String((const char *)mod_info.ModuleName).to_lower();
					} else {
						mod_name = "<unknown module>";
					}
				}
			}
		}
		print_error(vformat("[%d] %x (%s+%x) - %s", ch_data->index++, ch_data->pc, mod_name, ch_data->pc - offset, String::utf8(msg)));
	}
}

int trace_callback(void *data, uintptr_t pc) {
	CrashHandlerData *ch_data = reinterpret_cast<CrashHandlerData *>(data);
	ch_data->pc = (uint64_t)pc;
	backtrace_pcinfo(ch_data->state, pc - ch_data->offset, &symbol_callback, &error_callback, data);
	return 0;
}

int64_t get_image_base(const String &p_path) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return 0;
	}
	{
		f->seek(0x3c);
		uint32_t pe_pos = f->get_32();

		f->seek(pe_pos);
		uint32_t magic = f->get_32();
		if (magic != 0x00004550) {
			return 0;
		}
	}
	int64_t opt_header_pos = f->get_position() + 0x14;
	f->seek(opt_header_pos);

	uint16_t opt_header_magic = f->get_16();
	if (opt_header_magic == 0x10B) {
		f->seek(opt_header_pos + 0x1C);
		return f->get_32();
	} else if (opt_header_magic == 0x20B) {
		f->seek(opt_header_pos + 0x18);
		return f->get_64();
	} else {
		return 0;
	}
}

extern void CrashHandlerException(int signal) {
	CrashHandlerData data;

	if (!OS::is_initialized() || OS::is_disable_crash_handler() || IsDebuggerPresent()) {
		return;
	}

	if (OS::is_crash_handler_silent()) {
		std::_Exit(0);
	}

	String msg;
	if (ProjectSettings::is_initialized()) {
		msg = GLOBAL_GET("debug/settings/crash_handler/message");
	}

	// Tell MainLoop about the crash. This can be handled by users too in Node.
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_CRASH);
	}

	print_error("\n================================================================");
	print_error(vformat("%s: Program crashed with signal %d", __FUNCTION__, signal));

	// Print the engine version just before, so that people are reminded to include the version in backtrace reports.
	if (String(VLTR_VERSION_HASH).is_empty()) {
		print_error(vformat("Engine version: %s", VLTR_VERSION_FULL_NAME));
	} else {
		print_error(vformat("Engine version: %s (%s)", VLTR_VERSION_FULL_NAME, VLTR_VERSION_HASH));
	}
	print_error(vformat("Dumping the backtrace. %s", msg));

	String exec_path = OS::get_executable_path();

	// Load process and image info to determine ASLR addresses offset.
	MODULEINFO mi;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &mi, sizeof(mi));
	int64_t image_mem_base = reinterpret_cast<int64_t>(mi.lpBaseOfDll);
	int64_t image_file_base = get_image_base(exec_path);
	data.offset = image_mem_base - image_file_base;

	DWORD cbNeeded;
	std::vector<HMODULE> module_handles(1);

	data.process = GetCurrentProcess();
	data.sym_ok = SymInitialize(data.process, nullptr, false);

	if (data.sym_ok) {
		SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_EXACT_SYMBOLS);
		EnumProcessModules(data.process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
		module_handles.resize(cbNeeded / sizeof(HMODULE));
		EnumProcessModules(data.process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
		data.base = (uint64_t)load_all_modules_mingw(data.process, module_handles);
	}

	print_error(vformat("Load address: %x\n", (uint64_t)data.offset));

	if (FileAccess::exists(exec_path + ".debugsymbols")) {
		exec_path = exec_path + ".debugsymbols";
	}
	exec_path = exec_path.replace_char('/', '\\');

	CharString cs = exec_path.utf8(); // Note: should remain in scope during backtrace_simple call.
	data.state = backtrace_create_state(cs.get_data(), 0, &error_callback, reinterpret_cast<void *>(&data));
	if (data.state != nullptr) {
		data.index = 1;
		backtrace_simple(data.state, 1, &trace_callback, &error_callback, reinterpret_cast<void *>(&data));
	}

	print_error("-- END OF C++ BACKTRACE --");
	print_error("================================================================");

	if (data.sym_ok) {
		SymCleanup(data.process);
	}

	for (const Ref<ScriptBacktrace> &backtrace : ScriptServer::capture_script_backtraces(false)) {
		if (!backtrace->is_empty()) {
			print_error(backtrace->format());
			print_error(vformat("-- END OF %s BACKTRACE --", backtrace->get_language_name().to_upper()));
			print_error("================================================================");
		}
	}
}
#endif

CrashHandler::CrashHandler() {
	disabled = false;
}

CrashHandler::~CrashHandler() {
}

void CrashHandler::disable() {
	if (disabled) {
		return;
	}

#if defined(CRASH_HANDLER_EXCEPTION)
	signal(SIGSEGV, nullptr);
	signal(SIGFPE, nullptr);
	signal(SIGILL, nullptr);
#endif

	disabled = true;
}

void CrashHandler::initialize() {
#if defined(CRASH_HANDLER_EXCEPTION)
	signal(SIGSEGV, CrashHandlerException);
	signal(SIGFPE, CrashHandlerException);
	signal(SIGILL, CrashHandlerException);
#endif
}
#endif // defined(_MSC_VER)
