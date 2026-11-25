/**************************************************************************/
/*  tts_windows.cpp                                                       */
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


#ifdef WINRT_ENABLED
#include "servers/display/display_server.h"
#endif

bool TTS_Windows::is_speaking()
{
	if (data && data->driver) {
		return data->driver->is_speaking();
	}
	return false;
}

bool TTS_Windows::is_paused()
{
	if (data && data->driver) {
		return data->driver->is_paused();
	}
	return false;
}

Array TTS_Windows::get_voices()
{
	if (data && data->driver) {
		return data->driver->get_voices();
	}
	return Array();
}

void TTS_Windows::speak(const String& p_text, const String& p_voice, int p_volume, float p_pitch,
	float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	if (data && data->driver) {
		data->driver->speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
	}
}

void TTS_Windows::pause()
{
	if (data && data->driver) {
		data->driver->pause();
	}
}

void TTS_Windows::resume()
{
	if (data && data->driver) {
		data->driver->resume();
	}
}

void TTS_Windows::stop()
{
	if (data && data->driver) {
		data->driver->stop();
	}
}

void TTS_Windows::process_events()
{
	if (data && data->driver) {
		data->driver->process_events();
	}
}

void TTS_Windows::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
#ifdef WINRT_ENABLED
	// Try OneCore driver.
	if (!data->driver) {
		data->driver = memnew(TTSDriverOneCore);
		if (!data->driver->init()) {
			memdelete(data->driver);
			data->driver = nullptr;
		}
	}
#endif
	// Try SAPI driver.
	if (!data->driver) {
		data->driver = memnew(TTSDriverSAPI);
		if (!data->driver->init()) {
			memdelete(data->driver);
			data->driver = nullptr;
		}
	}
}

void TTS_Windows::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

// === Merged from platform/windows/tts_driver_sapi.cpp ===
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


void __stdcall TTSDriverSAPI::speech_event_callback(WPARAM wParam, LPARAM lParam) {
	TTSDriverSAPI *self = reinterpret_cast<TTSDriverSAPI *>(wParam);
	ERR_FAIL_NULL(self);

	SPEVENT event;
	while (self->synth->GetEvents(1, &event, nullptr) == S_OK) {
		uint32_t stream_num = (uint32_t)event.ulStreamNum;
		if (self->ids.has(stream_num)) {
			if (event.eEventId == SPEI_START_INPUT_STREAM) {
				DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_STARTED, self->ids[stream_num].id);
			} else if (event.eEventId == SPEI_END_INPUT_STREAM) {
				DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_ENDED, self->ids[stream_num].id);
				self->ids.erase(stream_num);
				self->update_requested = true;
			} else if (event.eEventId == SPEI_WORD_BOUNDARY) {
				const Char16String &string = self->ids[stream_num].string;
				int pos = 0;
				for (int i = 0; i < MIN(event.lParam, string.length()); i++) {
					char16_t c = string[i];
					if ((c & 0xfffffc00) == 0xd800) {
						i++;
					}
					pos++;
				}
				DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_BOUNDARY, self->ids[stream_num].id, pos - self->ids[stream_num].offset);
			}
		}
	}
}

void TTSDriverSAPI::process_events() {
	if (update_requested && !paused && queue.size() > 0 && !is_speaking()) {
		TTSUtterance &message = queue.front()->get();

		String text;
		DWORD flags = SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_XML;
		String pitch_tag = String("<pitch absmiddle=\"") + String::num_int64(message.pitch * 10 - 10, 10) + String("\">");
		text = pitch_tag + message.text + String("</pitch>");

		IEnumSpObjectTokens *cpEnum;
		ISpObjectToken *cpVoiceToken;
		ULONG ulCount = 0;
		ULONG stream_number = 0;
		ISpObjectTokenCategory *cpCategory;
		HRESULT hr = CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_INPROC_SERVER, IID_ISpObjectTokenCategory, (void **)&cpCategory);
		if (SUCCEEDED(hr)) {
			hr = cpCategory->SetId(SPCAT_VOICES, false);
			if (SUCCEEDED(hr)) {
				hr = cpCategory->EnumTokens(nullptr, nullptr, &cpEnum);
				if (SUCCEEDED(hr)) {
					hr = cpEnum->GetCount(&ulCount);
					while (SUCCEEDED(hr) && ulCount--) {
						wchar_t *w_id = nullptr;
						hr = cpEnum->Next(1, &cpVoiceToken, nullptr);
						cpVoiceToken->GetId(&w_id);
						if (String::utf16((const char16_t *)w_id) == message.voice) {
							synth->SetVoice(cpVoiceToken);
							cpVoiceToken->Release();
							break;
						}
						cpVoiceToken->Release();
					}
					cpEnum->Release();
				}
			}
			cpCategory->Release();
		}

		UTData ut;
		ut.string = text.utf16();
		ut.offset = pitch_tag.length(); // Subtract injected <pitch> tag offset.
		ut.id = message.id;

		synth->SetVolume(message.volume);
		synth->SetRate(10.f * std::log10(message.rate) / std::log10(3.f));
		synth->Speak((LPCWSTR)ut.string.get_data(), flags, &stream_number);

		ids[(uint32_t)stream_number] = ut;

		queue.pop_front();

		update_requested = false;
	}
}

bool TTSDriverSAPI::is_speaking() const {
	ERR_FAIL_NULL_V(synth, false);

	SPVOICESTATUS status;
	synth->GetStatus(&status, nullptr);
	return (status.dwRunningState == SPRS_IS_SPEAKING || status.dwRunningState == 0 /* Waiting To Speak */);
}

bool TTSDriverSAPI::is_paused() const {
	ERR_FAIL_NULL_V(synth, false);
	return paused;
}

Array TTSDriverSAPI::get_voices() const {
	Array list;
	IEnumSpObjectTokens *cpEnum;
	ISpObjectToken *cpVoiceToken;
	ISpDataKey *cpDataKeyAttribs;
	ULONG ulCount = 0;
	ISpObjectTokenCategory *cpCategory;
	HRESULT hr = CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_INPROC_SERVER, IID_ISpObjectTokenCategory, (void **)&cpCategory);
	if (SUCCEEDED(hr)) {
		hr = cpCategory->SetId(SPCAT_VOICES, false);
		if (SUCCEEDED(hr)) {
			hr = cpCategory->EnumTokens(nullptr, nullptr, &cpEnum);
			if (SUCCEEDED(hr)) {
				hr = cpEnum->GetCount(&ulCount);
				while (SUCCEEDED(hr) && ulCount--) {
					hr = cpEnum->Next(1, &cpVoiceToken, nullptr);
					HRESULT hr_attr = cpVoiceToken->OpenKey(SPTOKENKEY_ATTRIBUTES, &cpDataKeyAttribs);
					if (SUCCEEDED(hr_attr)) {
						wchar_t *w_id = nullptr;
						wchar_t *w_lang = nullptr;
						wchar_t *w_name = nullptr;
						cpVoiceToken->GetId(&w_id);
						cpDataKeyAttribs->GetStringValue(L"Language", &w_lang);
						cpDataKeyAttribs->GetStringValue(nullptr, &w_name);
						LCID locale = wcstol(w_lang, nullptr, 16);

						int locale_chars = GetLocaleInfoW(locale, LOCALE_SISO639LANGNAME, nullptr, 0);
						int region_chars = GetLocaleInfoW(locale, LOCALE_SISO3166CTRYNAME, nullptr, 0);
						wchar_t *w_lang_code = new wchar_t[locale_chars];
						wchar_t *w_reg_code = new wchar_t[region_chars];
						GetLocaleInfoW(locale, LOCALE_SISO639LANGNAME, w_lang_code, locale_chars);
						GetLocaleInfoW(locale, LOCALE_SISO3166CTRYNAME, w_reg_code, region_chars);

						Dictionary voice_d;
						voice_d["id"] = String::utf16((const char16_t *)w_id);
						if (w_name) {
							voice_d["name"] = String::utf16((const char16_t *)w_name);
						} else {
							voice_d["name"] = voice_d["id"].operator String().replace("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Voices\\Tokens\\", "");
						}
						voice_d["language"] = String::utf16((const char16_t *)w_lang_code) + "_" + String::utf16((const char16_t *)w_reg_code);
						list.push_back(voice_d);

						delete[] w_lang_code;
						delete[] w_reg_code;

						cpDataKeyAttribs->Release();
					}
					cpVoiceToken->Release();
				}
				cpEnum->Release();
			}
		}
		cpCategory->Release();
	}
	return list;
}

void TTSDriverSAPI::speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	ERR_FAIL_NULL(synth);
	if (p_interrupt) {
		stop();
	}

	if (p_text.is_empty()) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, p_utterance_id);
		return;
	}

	TTSUtterance message;
	message.text = p_text;
	message.voice = p_voice;
	message.volume = CLAMP(p_volume, 0, 100);
	message.pitch = CLAMP(p_pitch, 0.f, 2.f);
	message.rate = CLAMP(p_rate, 0.1f, 10.f);
	message.id = p_utterance_id;
	queue.push_back(message);

	if (is_paused()) {
		resume();
	} else {
		update_requested = true;
	}
}

void TTSDriverSAPI::pause() {
	ERR_FAIL_NULL(synth);
	if (!paused) {
		if (synth->Pause() == S_OK) {
			paused = true;
		}
	}
}

void TTSDriverSAPI::resume() {
	ERR_FAIL_NULL(synth);
	synth->Resume();
	paused = false;
}

void TTSDriverSAPI::stop() {
	ERR_FAIL_NULL(synth);

	SPVOICESTATUS status;
	synth->GetStatus(&status, nullptr);
	uint32_t current_stream = (uint32_t)status.ulCurrentStream;
	if (ids.has(current_stream)) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, ids[current_stream].id);
		ids.erase(current_stream);
	}
	for (TTSUtterance &message : queue) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, message.id);
	}
	queue.clear();
	synth->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
	synth->Resume();
	paused = false;
}

bool TTSDriverSAPI::init() {
	if (SUCCEEDED(CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, (void **)&synth))) {
		ULONGLONG event_mask = SPFEI(SPEI_END_INPUT_STREAM) | SPFEI(SPEI_START_INPUT_STREAM) | SPFEI(SPEI_WORD_BOUNDARY);
		synth->SetInterest(event_mask, event_mask);
		synth->SetNotifyCallbackFunction(&speech_event_callback, (WPARAM)(this), 0);
		print_verbose("Text-to-Speech: SAPI initialized.");
		return true;
	} else {
		print_verbose("Text-to-Speech: Cannot initialize SAPI driver!");
		return false;
	}
}

TTSDriverSAPI::TTSDriverSAPI() {
}

TTSDriverSAPI::~TTSDriverSAPI() {
	if (synth) {
		synth->Release();
	}
}

// === Merged from platform/windows/winrt_utils.cpp ===
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

#include "core/typedefs.h"

#ifdef WINRT_ENABLED

VLTR_GCC_WARNING_PUSH
VLTR_GCC_WARNING_IGNORE("-Wnon-virtual-dtor")
VLTR_GCC_WARNING_IGNORE("-Wctor-dtor-privacy")
VLTR_GCC_WARNING_IGNORE("-Wshadow")
VLTR_GCC_WARNING_IGNORE("-Wstrict-aliasing")
GODOT_CLANG_WARNING_PUSH
GODOT_CLANG_WARNING_IGNORE("-Wnon-virtual-dtor")

#include <inspectable.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.System.UserProfile.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.ViewManagement.Core.h>

enum DISPATCHERQUEUE_THREAD_APARTMENTTYPE {
	DQTAT_COM_NONE = 0,
	DQTAT_COM_ASTA = 1,
	DQTAT_COM_STA = 2
};

enum DISPATCHERQUEUE_THREAD_TYPE {
	DQTYPE_THREAD_DEDICATED = 1,
	DQTYPE_THREAD_CURRENT = 2,
};

struct DispatcherQueueOptions {
	DWORD dwSize;
	DISPATCHERQUEUE_THREAD_TYPE threadType;
	DISPATCHERQUEUE_THREAD_APARTMENTTYPE apartmentType;
};

typedef HRESULT(WINAPI *CreateDispatcherQueueControllerPtr)(DispatcherQueueOptions options, void *dispatcherQueueController);
CreateDispatcherQueueControllerPtr CreateDispatcherQueueController;

#ifndef E_BOUNDS
#define E_BOUNDS _HRESULT_TYPEDEF_(0x8000000B)
#endif // E_BOUNDS

#if defined __MINGW32__ || defined __MINGW64__

#ifndef __IDisplayInformationStaticsInterop_INTERFACE_DEFINED__
#define __IDisplayInformationStaticsInterop_INTERFACE_DEFINED__

// clang-format off
MIDL_INTERFACE("7449121c-382b-4705-8da7-a795ba482013")
IDisplayInformationStaticsInterop : public IInspectable {
public:
	virtual HRESULT STDMETHODCALLTYPE GetForWindow(
			HWND window,
			REFIID riid,
			void **displayInfo) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetForMonitor(
			HMONITOR monitor,
			REFIID riid,
			void **displayInfo) = 0;
};
// clang-format on
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IDisplayInformationStaticsInterop, 0x7449121c, 0x382b, 0x4705, 0x8d, 0xa7, 0xa7, 0x95, 0xba, 0x48, 0x20, 0x13)
#endif // __CRT_UUID_DECL

#endif // __IDisplayInformationStaticsInterop_INTERFACE_DEFINED__

#else // defined __MINGW32__ || defined __MINGW64__

#include <windows.graphics.display.interop.h>

#endif // defined __MINGW32__ || defined __MINGW64__

VLTR_GCC_WARNING_POP
GODOT_CLANG_WARNING_POP

using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::System;
using namespace winrt::Windows::System::UserProfile;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Foundation::Metadata;
using namespace winrt::Windows::UI::ViewManagement::Core;

DispatcherQueueController controller{ nullptr };

class WinRTWindowData {
	friend class WinRTUtils;

	int64_t id = 0;
	bool has_disp_info = false;
	DisplayInformation disp_info{ nullptr };
	Callable cb;
	winrt::event_token token{};
};

bool WinRTUtils::create_queue() {
	HMODULE coremessaging = LoadLibraryW(L"coremessaging.dll");
	if (!coremessaging) {
		return false;
	}
	CreateDispatcherQueueController = (CreateDispatcherQueueControllerPtr)(void *)GetProcAddress(coremessaging, "CreateDispatcherQueueController");
	if (!CreateDispatcherQueueController) {
		return false;
	}

	DispatcherQueueOptions options{ sizeof(options), DQTYPE_THREAD_CURRENT, DQTAT_COM_NONE };
	HRESULT res = CreateDispatcherQueueController(options, winrt::put_abi(controller));
	return SUCCEEDED(res);
}

void WinRTUtils::destroy_queue() {
	IAsyncAction action = controller.ShutdownQueueAsync();
	while (action.Status() == AsyncStatus::Started) {
		MSG msg = {};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	ERR_FAIL_COND_MSG(action.Status() == AsyncStatus::Error, "DispatcherQueueController shutdown failed.");
}

Vector<String> WinRTUtils::get_preferred_locales() {
	Vector<String> out;
	IVectorView<winrt::hstring> languages = GlobalizationPreferences::Languages();
	for (uint32_t i = 0; i < languages.Size(); i++) {
		winrt::hstring lang = languages.GetAt(i);
		out.push_back(String::utf16((const char16_t *)lang.c_str(), lang.size()).replace_char('-', '_'));
	}
	return out;
}

bool WinRTUtils::try_show_onecore_emoji_picker() {
	if (ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 7)) { // Windows 10, 1809+
		return CoreInputView::GetForCurrentView().TryShow(CoreInputViewKind::Emoji);
	}
	return false;
}

bool WinRTUtils::window_has_display_info(const WinRTWindowData *p_data) {
	if (p_data) {
		return p_data->has_disp_info;
	} else {
		return false;
	}
}

void WinRTUtils::window_get_advanced_color_info(const WinRTWindowData *p_data, bool &r_hdr_supported, float &r_min_luminance, float &r_max_luminance, float &r_max_average_luminance, float &r_sdr_white_level) {
	if (p_data && p_data->has_disp_info) {
		AdvancedColorInfo adv_info = p_data->disp_info.GetAdvancedColorInfo();

		r_hdr_supported = (adv_info.CurrentAdvancedColorKind() == AdvancedColorKind::HighDynamicRange);
		r_min_luminance = adv_info.MinLuminanceInNits();
		r_max_luminance = adv_info.MaxLuminanceInNits();
		r_max_average_luminance = adv_info.MaxAverageFullFrameLuminanceInNits();
		r_sdr_white_level = adv_info.SdrWhiteLevelInNits();
	}
}

WinRTWindowData *WinRTUtils::create_wd(HWND p_window, const Callable &p_color_cb, int64_t p_window_id) {
	WinRTWindowData *wd = memnew(WinRTWindowData);

	wd->id = p_window_id;
	wd->cb = p_color_cb;
	if (ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 6)) {
		try {
			HRESULT res = winrt::get_activation_factory<DisplayInformation, IDisplayInformationStaticsInterop>()->GetForWindow(p_window, winrt::guid_of<DisplayInformation>(), winrt::put_abi(wd->disp_info));
			if (res == S_OK && wd->disp_info) {
				wd->has_disp_info = true;
				wd->token = wd->disp_info.AdvancedColorInfoChanged([wd](const DisplayInformation &p_sender, auto &&) {
					wd->cb.call_deferred(wd->id);
				});
			}
		} catch (...) {
			memdelete(wd);
			return nullptr;
		}
	}

	return wd;
}

void WinRTUtils::destroy_wd(WinRTWindowData *p_data) {
	if (p_data) {
		if (p_data->token) {
			p_data->disp_info.AdvancedColorInfoChanged(p_data->token);
		}
		p_data->disp_info = nullptr;

		memdelete(p_data);
	}
}

#else

bool WinRTUtils::try_show_onecore_emoji_picker() {
	return false;
}

bool WinRTUtils::create_queue() {
	return false;
}

void WinRTUtils::destroy_queue() {}

Vector<String> WinRTUtils::get_preferred_locales() {
	return Vector<String>();
}

bool WinRTUtils::window_has_display_info(const WinRTWindowData *p_data) {
	return false;
}

void WinRTUtils::window_get_advanced_color_info(const WinRTWindowData *p_data, bool &r_hdr_supported, float &r_min_luminance, float &r_max_luminance, float &r_max_average_luminance, float &r_sdr_white_level) {}

WinRTWindowData *WinRTUtils::create_wd(HWND p_window, const Callable &p_color_cb, int64_t p_window_id) {
	return nullptr;
}

void WinRTUtils::destroy_wd(WinRTWindowData *p_data) {}

#endif

// === Merged from platform/windows/tts_driver_onecore.cpp ===
#ifdef WINRT_ENABLED
/*  tts_driver_onecore.cpp                                                */
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


void TTSDriverOneCore::_speech_index_mark(int p_msg_id, int p_index_mark) {
	DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_BOUNDARY, p_msg_id, p_index_mark);
}

void TTSDriverOneCore::_speech_cancel(int p_msg_id) {
	DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, p_msg_id);
}

void TTSDriverOneCore::_speech_end(int p_msg_id) {
	DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_ENDED, p_msg_id);
}

void TTSDriverOneCore::_dispose_current(bool p_silent, bool p_canceled) {
	if (media.get() != nullptr) {
		for (const TrackData &T : tracks) {
			T.track.CueEntered(T.token);
		}
		tracks.clear();
		media->MediaFailed(token_f);
		media->MediaEnded(token_e);
		if (!ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 4)) {
			media->PlaybackMediaMarkerReached(token_s);
		}
		media->Close();
		media.reset();

		if (!p_silent) {
			if (p_canceled) {
				callable_mp(this, &TTSDriverOneCore::_speech_cancel).call_deferred(id);
			} else {
				callable_mp(this, &TTSDriverOneCore::_speech_end).call_deferred(id);
			}
		}
		id = -1;
		string = Char16String();
		playing = false;
		paused = false;
		offset = 0;
	}
}

void TTSDriverOneCore::process_events() {
	if (update_requested && !paused && queue.size() > 0 && !is_speaking()) {
		TTSUtterance &message = queue.front()->get();
		_dispose_current(true);
		playing = true;

		SpeechSynthesizer synth = SpeechSynthesizer();

		if (ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 4)) {
			synth.Options().IncludeWordBoundaryMetadata(true);
		}
		if (ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 5)) {
			synth.Options().SpeakingRate(CLAMP(message.rate, 0.5, 6.0));
			synth.Options().AudioPitch(CLAMP(message.pitch, 0.0, 2.0));
			synth.Options().AudioVolume(CLAMP((double)message.volume / 100.0, 0.0, 1.0));
		}

		winrt::hstring name = winrt::hstring((const wchar_t *)message.voice.utf16().get_data());
		IVectorView<VoiceInformation> voices = SpeechSynthesizer::AllVoices();
		for (uint32_t i = 0; i < voices.Size(); i++) {
			VoiceInformation voice = voices.GetAt(i);
			if (voice.Id() == name) {
				synth.Voice(voice);
				break;
			}
		}

		string = message.text.utf16();
		winrt::hstring text = winrt::hstring((const wchar_t *)string.get_data());

		SpeechSynthesisStream stream = synth.SynthesizeTextToStreamAsync(text).get();

		media = std::make_shared<MediaPlayer>();
		token_f = media->MediaFailed([=, this](const MediaPlayer &p_sender, const MediaPlayerFailedEventArgs &p_args) {
			_dispose_current(false, true);
		});
		token_e = media->MediaEnded([=, this](const MediaPlayer &p_sender, const IInspectable &p_args) {
			_dispose_current(false, false);
		});
		if (ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 4)) {
			MediaPlaybackItem mitem = MediaPlaybackItem(MediaSource::CreateFromStream(stream, stream.ContentType()));
			media->Source(mitem);
			MediaPlaybackTimedMetadataTrackList list = mitem.TimedMetadataTracks();

			for (uint32_t i = 0; i < list.Size(); i++) {
				TimedMetadataTrack track = list.GetAt(i);
				if (track.TimedMetadataKind() == TimedMetadataKind::Speech) {
					winrt::event_token token = track.CueEntered([=, this](const TimedMetadataTrack &p_sender, const MediaCueEventArgs &p_args) {
						SpeechCue sq;
						p_args.Cue().as(sq);
						int32_t pos16 = sq.StartPositionInInput().Value();
						int pos = 0;
						for (int j = 0; j < MIN(pos16, string.length()); j++) {
							char16_t c = string[j];
							if ((c & 0xfffffc00) == 0xd800) {
								j++;
							}
							pos++;
						}
						callable_mp(this, &TTSDriverOneCore::_speech_index_mark).call_deferred(id, pos);
					});
					tracks.push_back({ track, token });
					list.SetPresentationMode(i, TimedMetadataTrackPresentationMode::ApplicationPresented);
				}
			}
		} else {
			media->Source(MediaSource::CreateFromStream(stream, stream.ContentType()));
			token_s = media->PlaybackMediaMarkerReached([=, this](const MediaPlayer &p_sender, const PlaybackMediaMarkerReachedEventArgs &p_args) {
				offset += p_args.PlaybackMediaMarker().Text().size() + 1;
				int pos = 0;
				for (int j = 0; j < MIN(offset, string.length()); j++) {
					char16_t c = string[j];
					if ((c & 0xfffffc00) == 0xd800) {
						j++;
					}
					pos++;
				}
				callable_mp(this, &TTSDriverOneCore::_speech_index_mark).call_deferred(id, pos);
			});
		}
		media->AutoPlay(true);

		id = message.id;
		update_requested = false;
		paused = false;

		media->Play();

		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_STARTED, message.id);
		queue.pop_front();
	}
}

bool TTSDriverOneCore::is_speaking() const {
	return playing;
}

bool TTSDriverOneCore::is_paused() const {
	return paused;
}

Array TTSDriverOneCore::get_voices() const {
	Array list;

	IVectorView<VoiceInformation> voices = SpeechSynthesizer::AllVoices();
	for (uint32_t i = 0; i < voices.Size(); i++) {
		VoiceInformation voice = voices.GetAt(i);
		winrt::hstring vname = voice.DisplayName();
		winrt::hstring vid = voice.Id();
		winrt::hstring vlang = voice.Language();

		Dictionary voice_d;
		voice_d["id"] = String::utf16((const char16_t *)vid.c_str(), vid.size());
		voice_d["name"] = String::utf16((const char16_t *)vname.c_str(), vname.size());
		voice_d["language"] = String::utf16((const char16_t *)vlang.c_str(), vlang.size()).replace_char('-', '_');
		list.push_back(voice_d);
	}
	return list;
}

void TTSDriverOneCore::speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	if (p_interrupt) {
		stop();
	}

	if (p_text.is_empty()) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, p_utterance_id);
		return;
	}

	TTSUtterance message;
	message.text = p_text;
	message.voice = p_voice;
	message.volume = CLAMP(p_volume, 0, 100);
	message.pitch = CLAMP(p_pitch, 0.f, 2.f);
	message.rate = CLAMP(p_rate, 0.1f, 10.f);
	message.id = p_utterance_id;
	queue.push_back(message);

	if (is_paused()) {
		resume();
	} else {
		update_requested = true;
	}
}

void TTSDriverOneCore::pause() {
	if (!paused && playing) {
		media->Pause();
		paused = true;
	}
}

void TTSDriverOneCore::resume() {
	if (paused && playing) {
		media->Play();
		paused = false;
	}
}

void TTSDriverOneCore::stop() {
	for (TTSUtterance &message : queue) {
		DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTS_UTTERANCE_CANCELED, message.id);
	}
	queue.clear();
	_dispose_current(false, true);
}

bool TTSDriverOneCore::init() {
	if (!ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 1)) {
		print_verbose("Text-to-Speech: Cannot initialize OneCore driver, API contract not present!");
		return false;
	}
	if (SpeechSynthesizer::AllVoices().Size() == 0) {
		print_verbose("Text-to-Speech: Cannot initialize OneCore driver, no voices found!");
		return false;
	}
	print_verbose("Text-to-Speech: OneCore initialized.");
	return true;
}

TTSDriverOneCore::TTSDriverOneCore() {
}

TTSDriverOneCore::~TTSDriverOneCore() {
	_dispose_current(false, true);
}
#endif // WINRT_ENABLED
