/**************************************************************************/
/*  display_server_windows.cpp                                            */
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

#include "display_server_windows.h"

#include "platform/windows/display_server_windows.h"
#include "os_windows.h"
#include "platform/windows/display_server_windows.h"
#include "platform/windows/os_windows.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/marshalls.h"
#include "core/io/xml_parser.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/version.h"
#include "drivers/png/png_driver_common.h"
#include "main/main.h"
#include "scene/resources/texture.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/dummy/rasterizer_dummy.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"

#ifdef SDL_ENABLED
#include "drivers/sdl/joypad_sdl.h"
#endif

#if defined(VULKAN_ENABLED)
#include "platform/windows/display_server_windows.h"
#endif
#if defined(D3D12_ENABLED)
#include "drivers/d3d12/rendering_context_driver_d3d12.h"

#include <dxgi1_6.h>
#endif
#if defined(GLES3_ENABLED)
#include "drivers/gles3/rasterizer_gles3.h"
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

static String format_error_message(DWORD id) {
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
	ERR_FAIL_NULL_MSG(wd.icon_big, "Could not create " + itos(big_icon_width) + "x" + itos(big_icon_width) + " @" + itos(big_icon_cc) + " icon, error: " + format_error_message(GetLastError()) + ".");

	// Read the small icon.
	DWORD bytecount_small = icon_dir->idEntries[small_icon_index].dwBytesInRes;
	wd.icon_buffer_small.resize(bytecount_small);
	pos = icon_dir->idEntries[small_icon_index].dwImageOffset;
	f->seek(pos);
	f->get_buffer((uint8_t *)&wd.icon_buffer_small.write[0], bytecount_small);
	wd.icon_small = CreateIconFromResourceEx((PBYTE)&wd.icon_buffer_small.write[0], bytecount_small, TRUE, 0x00030000, 0, 0, LR_DEFAULTSIZE);
	ERR_FAIL_NULL_MSG(wd.icon_small, "Could not create 16x16 @" + itos(small_icon_cc) + " icon, error: " + format_error_message(GetLastError()) + ".");

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
		RenderingContextDriverVulkanWindows::WindowPlatformData vulkan;
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
			rendering_context = memnew(RenderingContextDriverVulkanWindows);
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
#include "platform/windows/display_server_windows.h"

#include "core/templates/hash_map.h"

// This provides translation from Windows virtual key codes to Godot and back.
// See WinUser.h and the below for documentation:
// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

struct HashMapHasherKeys {
	static _FORCE_INLINE_ uint32_t hash(const Key p_key) { return hash_fmix32(static_cast<uint32_t>(p_key)); }
	static _FORCE_INLINE_ uint32_t hash(const char32_t p_uchar) { return hash_fmix32(p_uchar); }
	static _FORCE_INLINE_ uint32_t hash(const unsigned p_key) { return hash_fmix32(p_key); }
};

HashMap<unsigned int, Key, HashMapHasherKeys> vk_map;
HashMap<unsigned int, Key, HashMapHasherKeys> scansym_map;
HashMap<Key, unsigned int, HashMapHasherKeys> scansym_map_inv;
HashMap<unsigned int, Key, HashMapHasherKeys> scansym_map_ext;
HashMap<unsigned int, KeyLocation, HashMapHasherKeys> location_map;

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
#include "platform/windows/display_server_windows.h"

#include "platform/windows/display_server_windows.h"

#include "scene/resources/image_texture.h"

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
#include "platform/windows/display_server_windows.h"

#if defined(WINDOWS_ENABLED) && defined(GLES3_ENABLED)

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/version.h"

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

#include "platform/windows/display_server_windows.h"

#include "platform/windows/os_windows.h"

#include "core/os/os.h"
#include "core/string/print_string.h"
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

#include "platform/windows/display_server_windows.h"

#include "core/os/os.h"

#include <drivers/vulkan/godot_vulkan.h>

const char *RenderingContextDriverVulkanWindows::_get_platform_surface_extension() const {
	return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

RenderingContextDriverVulkanWindows::RenderingContextDriverVulkanWindows() {
	// Workaround for Vulkan not working on setups with AMD integrated graphics + NVIDIA dedicated GPU (GH-57708).
	// This prevents using AMD integrated graphics with Vulkan entirely, but it allows the engine to start
	// even on outdated/broken driver setups.
	OS::set_environment("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");
}

RenderingContextDriverVulkanWindows::~RenderingContextDriverVulkanWindows() {
	// Does nothing.
}

RenderingContextDriver::SurfaceID RenderingContextDriverVulkanWindows::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	VkWin32SurfaceCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.hinstance = wpd->instance;
	create_info.hwnd = wpd->window;

	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkResult err = vkCreateWin32SurfaceKHR(instance_get(), &create_info, get_allocation_callbacks(VK_OBJECT_TYPE_SURFACE_KHR), &vk_surface);
	ERR_FAIL_COND_V(err != VK_SUCCESS, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->vk_surface = vk_surface;
	return SurfaceID(surface);
}

#endif // WINDOWS_ENABLED && VULKAN_ENABLED


// --- Merged from platform/windows/drop_target_windows.cpp ---
#include "platform/windows/display_server_windows.h"

#include "core/io/dir_access.h"
#include "core/math/random_pcg.h"
#include "core/os/time.h"

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
#include "platform/windows/display_server_windows.h"

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
