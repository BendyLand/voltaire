/**************************************************************************/
/*  display_server.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "display_server.compat.inc"
#include "display_server.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Input);
STATIC_ASSERT_INCOMPLETE_TYPE(class, NativeMenu);
STATIC_ASSERT_INCOMPLETE_TYPE(class, Texture2D);
STATIC_ASSERT_INCOMPLETE_TYPE(class, RenderingServer);

#include "core/input/input.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "scene/resources/texture.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server_headless.h"
#include "servers/display/native_menu.h"
#include "servers/rendering/rendering_server.h"

#if defined(RD_ENABLED)
#include "servers/rendering/rendering_device.h"
#endif

#if defined(VULKAN_ENABLED)
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#endif
#if defined(D3D12_ENABLED)
#include "drivers/d3d12/rendering_context_driver_d3d12.h"
#endif
#if defined(METAL_ENABLED)
#include "drivers/metal/rendering_context_driver_metal.h"
#endif

DisplayServer* DisplayServer::singleton = nullptr;

bool DisplayServer::window_early_clear_override_enabled = false;
Color DisplayServer::window_early_clear_override_color = Color(0, 0, 0, 0);

DisplayServer::DisplayServerCreate
	DisplayServer::server_create_functions[DisplayServer::MAX_SERVERS] = {{"headless",
		&DisplayServerHeadless::create_func, &DisplayServerHeadless::get_rendering_drivers_func}};

int DisplayServer::server_create_count = 1;

void DisplayServer::help_set_search_callbacks(
	const Callable& p_search_callback, const Callable& p_action_callback)
{
	WARN_PRINT("Native help is not supported by this display server.");
}

#ifndef DISABLE_DEPRECATED

RID DisplayServer::_get_rid_from_name(NativeMenu* p_nmenu, const String& p_menu_root) const
{
	if (p_menu_root == "_main") {
		return p_nmenu->get_system_menu(NativeMenu::MAIN_MENU_ID);
	}
	else if (p_menu_root == "_apple") {
		return p_nmenu->get_system_menu(NativeMenu::APPLICATION_MENU_ID);
	}
	else if (p_menu_root == "_dock") {
		return p_nmenu->get_system_menu(NativeMenu::DOCK_MENU_ID);
	}
	else if (p_menu_root == "_help") {
		return p_nmenu->get_system_menu(NativeMenu::HELP_MENU_ID);
	}
	else if (p_menu_root == "_window") {
		return p_nmenu->get_system_menu(NativeMenu::WINDOW_MENU_ID);
	}
	else if (menu_names.has(p_menu_root)) {
		return menu_names[p_menu_root];
	}

	RID rid = p_nmenu->create_menu();
	menu_names[p_menu_root] = rid;
	return rid;
}

int DisplayServer::global_menu_add_item(const String& p_menu_root, const String& p_label,
	const Callable& p_callback, const Callable& p_key_callback, const Variant& p_tag, Key p_accel,
	int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_item(_get_rid_from_name(nmenu, p_menu_root), p_label, p_callback,
		p_key_callback, p_tag, p_accel, p_index);
}

int DisplayServer::global_menu_add_check_item(const String& p_menu_root, const String& p_label,
	const Callable& p_callback, const Callable& p_key_callback, const Variant& p_tag, Key p_accel,
	int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_check_item(_get_rid_from_name(nmenu, p_menu_root), p_label, p_callback,
		p_key_callback, p_tag, p_accel, p_index);
}

int DisplayServer::global_menu_add_icon_item(const String& p_menu_root,
	const Ref<Texture2D>& p_icon, const String& p_label, const Callable& p_callback,
	const Callable& p_key_callback, const Variant& p_tag, Key p_accel, int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_icon_item(_get_rid_from_name(nmenu, p_menu_root), p_icon, p_label, p_callback,
		p_key_callback, p_tag, p_accel, p_index);
}

int DisplayServer::global_menu_add_icon_check_item(const String& p_menu_root,
	const Ref<Texture2D>& p_icon, const String& p_label, const Callable& p_callback,
	const Callable& p_key_callback, const Variant& p_tag, Key p_accel, int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_icon_check_item(_get_rid_from_name(nmenu, p_menu_root), p_icon, p_label,
		p_callback, p_key_callback, p_tag, p_accel, p_index);
}

int DisplayServer::global_menu_add_radio_check_item(const String& p_menu_root,
	const String& p_label, const Callable& p_callback, const Callable& p_key_callback,
	const Variant& p_tag, Key p_accel, int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_radio_check_item(_get_rid_from_name(nmenu, p_menu_root), p_label, p_callback,
		p_key_callback, p_tag, p_accel, p_index);
}

int DisplayServer::global_menu_add_icon_radio_check_item(const String& p_menu_root,
	const Ref<Texture2D>& p_icon, const String& p_label, const Callable& p_callback,
	const Callable& p_key_callback, const Variant& p_tag, Key p_accel, int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_icon_radio_check_item(_get_rid_from_name(nmenu, p_menu_root), p_icon, p_label,
		p_callback, p_key_callback, p_tag, p_accel, p_index);
}

int DisplayServer::global_menu_add_multistate_item(const String& p_menu_root, const String& p_label,
	int p_max_states, int p_default_state, const Callable& p_callback,
	const Callable& p_key_callback, const Variant& p_tag, Key p_accel, int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_multistate_item(_get_rid_from_name(nmenu, p_menu_root), p_label, p_max_states,
		p_default_state, p_callback, p_key_callback, p_tag, p_accel, p_index);
}

void DisplayServer::global_menu_set_popup_callbacks(
	const String& p_menu_root, const Callable& p_open_callback, const Callable& p_close_callback)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_popup_open_callback(_get_rid_from_name(nmenu, p_menu_root), p_open_callback);
	nmenu->set_popup_open_callback(_get_rid_from_name(nmenu, p_menu_root), p_close_callback);
}

int DisplayServer::global_menu_add_submenu_item(
	const String& p_menu_root, const String& p_label, const String& p_submenu, int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_submenu_item(_get_rid_from_name(nmenu, p_menu_root), p_label,
		_get_rid_from_name(nmenu, p_submenu), Variant(), p_index);
}

int DisplayServer::global_menu_add_separator(const String& p_menu_root, int p_index)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->add_separator(_get_rid_from_name(nmenu, p_menu_root), p_index);
}

int DisplayServer::global_menu_get_item_index_from_text(
	const String& p_menu_root, const String& p_text) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->find_item_index_with_text(_get_rid_from_name(nmenu, p_menu_root), p_text);
}

int DisplayServer::global_menu_get_item_index_from_tag(
	const String& p_menu_root, const Variant& p_tag) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->find_item_index_with_tag(_get_rid_from_name(nmenu, p_menu_root), p_tag);
}

void DisplayServer::global_menu_set_item_callback(
	const String& p_menu_root, int p_idx, const Callable& p_callback)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_callback(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_callback);
}

void DisplayServer::global_menu_set_item_hover_callbacks(
	const String& p_menu_root, int p_idx, const Callable& p_callback)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_hover_callbacks(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_callback);
}

void DisplayServer::global_menu_set_item_key_callback(
	const String& p_menu_root, int p_idx, const Callable& p_key_callback)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_key_callback(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_key_callback);
}

bool DisplayServer::global_menu_is_item_checked(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, false);
	return nmenu->is_item_checked(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

bool DisplayServer::global_menu_is_item_checkable(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, false);
	return nmenu->is_item_checkable(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

bool DisplayServer::global_menu_is_item_radio_checkable(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, false);
	return nmenu->is_item_radio_checkable(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

Callable DisplayServer::global_menu_get_item_callback(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, Callable());
	return nmenu->get_item_callback(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

Callable DisplayServer::global_menu_get_item_key_callback(
	const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, Callable());
	return nmenu->get_item_key_callback(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

Variant DisplayServer::global_menu_get_item_tag(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, Variant());
	return nmenu->get_item_tag(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

String DisplayServer::global_menu_get_item_text(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, String());
	return nmenu->get_item_text(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

String DisplayServer::global_menu_get_item_submenu(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, String());
	RID rid = nmenu->get_item_submenu(_get_rid_from_name(nmenu, p_menu_root), p_idx);
	if (!nmenu->is_system_menu(rid)) {
		for (HashMap<String, RID>::Iterator E = menu_names.begin(); E; ++E) {
			if (E->value == rid) {
				return E->key;
			}
		}
	}
	return String();
}

Key DisplayServer::global_menu_get_item_accelerator(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, Key::NONE);
	return nmenu->get_item_accelerator(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

bool DisplayServer::global_menu_is_item_disabled(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, false);
	return nmenu->is_item_disabled(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

bool DisplayServer::global_menu_is_item_hidden(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, false);
	return nmenu->is_item_hidden(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

String DisplayServer::global_menu_get_item_tooltip(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, String());
	return nmenu->get_item_tooltip(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

int DisplayServer::global_menu_get_item_state(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->get_item_state(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

int DisplayServer::global_menu_get_item_max_states(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, -1);
	return nmenu->get_item_max_states(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

Ref<Texture2D> DisplayServer::global_menu_get_item_icon(const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, Ref<Texture2D>());
	return nmenu->get_item_icon(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

int DisplayServer::global_menu_get_item_indentation_level(
	const String& p_menu_root, int p_idx) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, 0);
	return nmenu->get_item_indentation_level(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

void DisplayServer::global_menu_set_item_checked(
	const String& p_menu_root, int p_idx, bool p_checked)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_checked(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_checked);
}

void DisplayServer::global_menu_set_item_checkable(
	const String& p_menu_root, int p_idx, bool p_checkable)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_checkable(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_checkable);
}

void DisplayServer::global_menu_set_item_radio_checkable(
	const String& p_menu_root, int p_idx, bool p_checkable)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_radio_checkable(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_checkable);
}

void DisplayServer::global_menu_set_item_tag(
	const String& p_menu_root, int p_idx, const Variant& p_tag)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_tag(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_tag);
}

void DisplayServer::global_menu_set_item_text(
	const String& p_menu_root, int p_idx, const String& p_text)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_text(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_text);
}

void DisplayServer::global_menu_set_item_submenu(
	const String& p_menu_root, int p_idx, const String& p_submenu)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_submenu(
		_get_rid_from_name(nmenu, p_menu_root), p_idx, _get_rid_from_name(nmenu, p_submenu));
}

void DisplayServer::global_menu_set_item_accelerator(
	const String& p_menu_root, int p_idx, Key p_keycode)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_accelerator(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_keycode);
}

void DisplayServer::global_menu_set_item_disabled(
	const String& p_menu_root, int p_idx, bool p_disabled)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_disabled(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_disabled);
}

void DisplayServer::global_menu_set_item_hidden(const String& p_menu_root, int p_idx, bool p_hidden)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_hidden(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_hidden);
}

void DisplayServer::global_menu_set_item_tooltip(
	const String& p_menu_root, int p_idx, const String& p_tooltip)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_tooltip(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_tooltip);
}

void DisplayServer::global_menu_set_item_state(const String& p_menu_root, int p_idx, int p_state)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_state(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_state);
}

void DisplayServer::global_menu_set_item_max_states(
	const String& p_menu_root, int p_idx, int p_max_states)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_max_states(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_max_states);
}

void DisplayServer::global_menu_set_item_icon(
	const String& p_menu_root, int p_idx, const Ref<Texture2D>& p_icon)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_icon(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_icon);
}

void DisplayServer::global_menu_set_item_indentation_level(
	const String& p_menu_root, int p_idx, int p_level)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->set_item_indentation_level(_get_rid_from_name(nmenu, p_menu_root), p_idx, p_level);
}

int DisplayServer::global_menu_get_item_count(const String& p_menu_root) const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, 0);
	return nmenu->get_item_count(_get_rid_from_name(nmenu, p_menu_root));
}

void DisplayServer::global_menu_remove_item(const String& p_menu_root, int p_idx)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	nmenu->remove_item(_get_rid_from_name(nmenu, p_menu_root), p_idx);
}

void DisplayServer::global_menu_clear(const String& p_menu_root)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL(nmenu);
	RID rid = _get_rid_from_name(nmenu, p_menu_root);
	nmenu->clear(rid);
	if (!nmenu->is_system_menu(rid)) {
		nmenu->free_menu(rid);
		menu_names.erase(p_menu_root);
	}
}

Dictionary DisplayServer::global_menu_get_system_menu_roots() const
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	ERR_FAIL_NULL_V(nmenu, Dictionary());

	Dictionary out;
	if (nmenu->has_system_menu(NativeMenu::DOCK_MENU_ID)) {
		out["_dock"] = "@Dock";
	}
	if (nmenu->has_system_menu(NativeMenu::APPLICATION_MENU_ID)) {
		out["_apple"] = "@Apple";
	}
	if (nmenu->has_system_menu(NativeMenu::WINDOW_MENU_ID)) {
		out["_window"] = "Window";
	}
	if (nmenu->has_system_menu(NativeMenu::HELP_MENU_ID)) {
		out["_help"] = "Help";
	}
	return out;
}

#endif

bool DisplayServer::tts_is_speaking() const
{
	WARN_PRINT("TTS is not supported by this display server.");
	return false;
}

bool DisplayServer::tts_is_paused() const
{
	WARN_PRINT("TTS is not supported by this display server.");
	return false;
}

void DisplayServer::tts_pause() { WARN_PRINT("TTS is not supported by this display server."); }

void DisplayServer::tts_resume() { WARN_PRINT("TTS is not supported by this display server."); }

TypedArray<Dictionary> DisplayServer::tts_get_voices() const
{
	WARN_PRINT("TTS is not supported by this display server.");
	return TypedArray<Dictionary>();
}

PackedStringArray DisplayServer::tts_get_voices_for_language(const String& p_language) const
{
	PackedStringArray ret;
	TypedArray<Dictionary> voices = tts_get_voices();
	for (int i = 0; i < voices.size(); i++) {
		const Dictionary& voice = voices[i];
		if (voice.has("id") && voice.has("language") &&
			voice["language"].operator String().begins_with(p_language)) {
			ret.push_back(voice["id"]);
		}
	}
	return ret;
}

void DisplayServer::tts_speak(const String& p_text, const String& p_voice, int p_volume,
	float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	WARN_PRINT("TTS is not supported by this display server.");
}

void DisplayServer::tts_stop() { WARN_PRINT("TTS is not supported by this display server."); }

void DisplayServer::tts_set_utterance_callback(
	DisplayServerEnums::TTSUtteranceEvent p_event, const Callable& p_callable)
{
	ERR_FAIL_INDEX(p_event, DisplayServerEnums::TTS_UTTERANCE_MAX);
	utterance_callback[p_event] = p_callable;
}

void DisplayServer::tts_post_utterance_event(
	DisplayServerEnums::TTSUtteranceEvent p_event, int64_t p_id, int p_pos)
{
	ERR_FAIL_INDEX(p_event, DisplayServerEnums::TTS_UTTERANCE_MAX);
	switch (p_event) {
	case DisplayServerEnums::TTS_UTTERANCE_STARTED:
	case DisplayServerEnums::TTS_UTTERANCE_ENDED:
	case DisplayServerEnums::TTS_UTTERANCE_CANCELED: {
		if (utterance_callback[p_event].is_valid()) {
			utterance_callback[p_event].call_deferred(
				p_id); // Should be deferred, on some platforms utterance events can be called from
					   // different threads in a rapid succession.
		}
	} break;
	case DisplayServerEnums::TTS_UTTERANCE_BOUNDARY: {
		if (utterance_callback[p_event].is_valid()) {
			utterance_callback[p_event].call_deferred(
				p_pos, p_id); // Should be deferred, on some platforms utterance events can be
							  // called from different threads in a rapid succession.
		}
	} break;
	default:
		break;
	}
}

bool DisplayServer::_get_window_early_clear_override(Color& r_color)
{
	if (window_early_clear_override_enabled) {
		r_color = window_early_clear_override_color;
		return true;
	}
	else if (RenderingServer::get_singleton()) {
		r_color = RenderingServer::get_singleton()->get_default_clear_color();
		return true;
	}
	else {
		return false;
	}
}

void DisplayServer::set_early_window_clear_color_override(bool p_enabled, Color p_color)
{
	window_early_clear_override_enabled = p_enabled;
	window_early_clear_override_color = p_color;
}

void DisplayServer::mouse_set_mode(DisplayServerEnums::MouseMode p_mode)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

DisplayServerEnums::MouseMode DisplayServer::mouse_get_mode() const
{
	return DisplayServerEnums::MOUSE_MODE_VISIBLE;
}

void DisplayServer::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

DisplayServerEnums::MouseMode DisplayServer::mouse_get_mode_override() const
{
	return DisplayServerEnums::MOUSE_MODE_VISIBLE;
}

void DisplayServer::mouse_set_mode_override_enabled(bool p_override_enabled)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

bool DisplayServer::mouse_is_mode_override_enabled() const { return false; }

void DisplayServer::warp_mouse(const Point2i& p_position) {}

Point2i DisplayServer::mouse_get_position() const
{
	ERR_FAIL_V_MSG(Point2i(), "Mouse is not supported by this display server.");
}

BitField<MouseButtonMask> DisplayServer::mouse_get_button_state() const
{
	ERR_FAIL_V_MSG(MouseButtonMask::NONE, "Mouse is not supported by this display server.");
}

void DisplayServer::clipboard_set(const String& p_text)
{
	WARN_PRINT("Clipboard is not supported by this display server.");
}

String DisplayServer::clipboard_get() const
{
	ERR_FAIL_V_MSG(String(), "Clipboard is not supported by this display server.");
}

Ref<Image> DisplayServer::clipboard_get_image() const
{
	ERR_FAIL_V_MSG(Ref<Image>(), "Clipboard is not supported by this display server.");
}

bool DisplayServer::clipboard_has() const { return !clipboard_get().is_empty(); }

bool DisplayServer::clipboard_has_image() const { return clipboard_get_image().is_valid(); }

void DisplayServer::clipboard_set_primary(const String& p_text)
{
	WARN_PRINT("Primary clipboard is not supported by this display server.");
}

String DisplayServer::clipboard_get_primary() const
{
	ERR_FAIL_V_MSG(String(), "Primary clipboard is not supported by this display server.");
}

void DisplayServer::screen_set_orientation(
	DisplayServerEnums::ScreenOrientation p_orientation, int p_screen)
{
	WARN_PRINT("Orientation not supported by this display server.");
}

DisplayServerEnums::ScreenOrientation DisplayServer::screen_get_orientation(int p_screen) const
{
	return DisplayServerEnums::SCREEN_LANDSCAPE;
}

float DisplayServer::screen_get_scale(int p_screen) const { return 1.0f; }

bool DisplayServer::is_touchscreen_available() const
{
	return Input::get_singleton() && Input::get_singleton()->is_emulating_touch_from_mouse();
}

void DisplayServer::screen_set_keep_on(bool p_enable)
{
	WARN_PRINT("Keeping screen on not supported by this display server.");
}

bool DisplayServer::screen_is_kept_on() const { return false; }

int DisplayServer::get_screen_from_rect(const Rect2& p_rect) const
{
	int nearest_area = 0;
	int pos_screen = DisplayServerEnums::INVALID_SCREEN;
	for (int i = 0; i < get_screen_count(); i++) {
		Rect2i r;
		r.position = screen_get_position(i);
		r.size = screen_get_size(i);
		Rect2 inters = r.intersection(p_rect);
		int area = inters.size.width * inters.size.height;
		if (area > nearest_area) {
			pos_screen = i;
			nearest_area = area;
		}
	}
	return pos_screen;
}

DisplayServerEnums::WindowID DisplayServer::create_sub_window(DisplayServerEnums::WindowMode p_mode,
	DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i& p_rect,
	bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent)
{
	ERR_FAIL_V_MSG(
		DisplayServerEnums::INVALID_WINDOW_ID, "Sub-windows not supported by this display server.");
}

void DisplayServer::show_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_MSG("Sub-windows not supported by this display server.");
}

void DisplayServer::delete_sub_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_MSG("Sub-windows not supported by this display server.");
}

void DisplayServer::window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive)
{
	// Do nothing, if not supported.
}

void DisplayServer::window_set_mouse_passthrough(
	const Vector<Vector2>& p_region, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_MSG("Mouse passthrough not supported by this display server.");
}

void DisplayServer::gl_window_make_current(DisplayServerEnums::WindowID p_window_id)
{
	// noop except in gles
}

void DisplayServer::window_set_ime_active(
	const bool p_active, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("IME not supported by this display server.");
}

void DisplayServer::window_set_ime_position(
	const Point2i& p_pos, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("IME not supported by this display server.");
}

#ifndef DISABLE_DEPRECATED

RID DisplayServer::accessibility_create_element(
	DisplayServerEnums::WindowID p_window, DisplayServerEnums::AccessibilityRole p_role)
{
	if (AccessibilityServer::get_singleton()) {
		return AccessibilityServer::get_singleton()->create_element(
			p_window, (AccessibilityServerEnums::AccessibilityRole)p_role);
	}
	else {
		return RID();
	}
}

RID DisplayServer::accessibility_create_sub_element(
	const RID& p_parent_rid, DisplayServerEnums::AccessibilityRole p_role, int p_insert_pos)
{
	if (AccessibilityServer::get_singleton()) {
		return AccessibilityServer::get_singleton()->create_sub_element(
			p_parent_rid, (AccessibilityServerEnums::AccessibilityRole)p_role, p_insert_pos);
	}
	else {
		return RID();
	}
}

RID DisplayServer::accessibility_create_sub_text_edit_elements(const RID& p_parent_rid,
	const RID& p_shaped_text, float p_min_height, int p_insert_pos, bool p_is_last_line)
{
	if (AccessibilityServer::get_singleton()) {
		return AccessibilityServer::get_singleton()->create_sub_text_edit_elements(
			p_parent_rid, p_shaped_text, p_min_height, p_insert_pos, p_is_last_line);
	}
	else {
		return RID();
	}
}

bool DisplayServer::accessibility_has_element(const RID& p_id) const
{
	if (AccessibilityServer::get_singleton()) {
		return AccessibilityServer::get_singleton()->has_element(p_id);
	}
	else {
		return false;
	}
}

void DisplayServer::accessibility_free_element(const RID& p_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->free_element(p_id);
	}
}

void DisplayServer::accessibility_element_set_meta(const RID& p_id, const Variant& p_meta)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->element_set_meta(p_id, p_meta);
	}
}

Variant DisplayServer::accessibility_element_get_meta(const RID& p_id) const
{
	if (AccessibilityServer::get_singleton()) {
		return AccessibilityServer::get_singleton()->element_get_meta(p_id);
	}
	else {
		return Variant();
	}
}

void DisplayServer::accessibility_update_if_active(const Callable& p_callable)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_if_active(p_callable);
	}
}

void DisplayServer::accessibility_update_set_focus(const RID& p_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_focus(p_id);
	}
}

RID DisplayServer::accessibility_get_window_root(DisplayServerEnums::WindowID p_window_id) const
{
	if (AccessibilityServer::get_singleton()) {
		return AccessibilityServer::get_singleton()->get_window_root(p_window_id);
	}
	else {
		return RID();
	}
}

void DisplayServer::accessibility_set_window_rect(
	DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->set_window_rect(p_window_id, p_rect_out, p_rect_in);
	}
}

void DisplayServer::accessibility_set_window_focused(
	DisplayServerEnums::WindowID p_window_id, bool p_focused)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->set_window_focused(p_window_id, p_focused);
	}
}

void DisplayServer::accessibility_set_window_callbacks(DisplayServerEnums::WindowID p_window_id,
	const Callable& p_activate_callable, const Callable& p_deativate_callable)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->set_window_callbacks(
			p_window_id, p_activate_callable, p_deativate_callable);
	}
}

void DisplayServer::accessibility_window_activation_completed(
	DisplayServerEnums::WindowID p_window_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->window_activation_completed(p_window_id);
	}
}

void DisplayServer::accessibility_window_deactivation_completed(
	DisplayServerEnums::WindowID p_window_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->window_deactivation_completed(p_window_id);
	}
}

void DisplayServer::accessibility_update_set_role(
	const RID& p_id, DisplayServerEnums::AccessibilityRole p_role)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_role(
			p_id, (AccessibilityServerEnums::AccessibilityRole)p_role);
	}
}

void DisplayServer::accessibility_update_set_name(const RID& p_id, const String& p_name)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_name(p_id, p_name);
	}
}

void DisplayServer::accessibility_update_set_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_description(p_id, p_description);
	}
}

void DisplayServer::accessibility_update_set_extra_info(
	const RID& p_id, const String& p_name_extra_info)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_extra_info(p_id, p_name_extra_info);
	}
}

void DisplayServer::accessibility_update_set_value(const RID& p_id, const String& p_value)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_value(p_id, p_value);
	}
}

void DisplayServer::accessibility_update_set_tooltip(const RID& p_id, const String& p_tooltip)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_tooltip(p_id, p_tooltip);
	}
}

void DisplayServer::accessibility_update_set_bounds(const RID& p_id, const Rect2& p_rect)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_bounds(p_id, p_rect);
	}
}

void DisplayServer::accessibility_update_set_transform(
	const RID& p_id, const Transform2D& p_transform)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_transform(p_id, p_transform);
	}
}

void DisplayServer::accessibility_update_add_child(const RID& p_id, const RID& p_child_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_child(p_id, p_child_id);
	}
}

void DisplayServer::accessibility_update_add_related_controls(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_related_controls(p_id, p_related_id);
	}
}

void DisplayServer::accessibility_update_add_related_details(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_related_details(p_id, p_related_id);
	}
}

void DisplayServer::accessibility_update_add_related_described_by(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_related_described_by(p_id, p_related_id);
	}
}

void DisplayServer::accessibility_update_add_related_flow_to(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_related_flow_to(p_id, p_related_id);
	}
}

void DisplayServer::accessibility_update_add_related_labeled_by(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_related_labeled_by(p_id, p_related_id);
	}
}

void DisplayServer::accessibility_update_add_related_radio_group(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_related_radio_group(p_id, p_related_id);
	}
}

void DisplayServer::accessibility_update_set_active_descendant(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_active_descendant(p_id, p_other_id);
	}
}

void DisplayServer::accessibility_update_set_next_on_line(const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_next_on_line(p_id, p_other_id);
	}
}

void DisplayServer::accessibility_update_set_previous_on_line(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_previous_on_line(p_id, p_other_id);
	}
}

void DisplayServer::accessibility_update_set_member_of(const RID& p_id, const RID& p_group_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_member_of(p_id, p_group_id);
	}
}

void DisplayServer::accessibility_update_set_in_page_link_target(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_in_page_link_target(p_id, p_other_id);
	}
}

void DisplayServer::accessibility_update_set_error_message(const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_error_message(p_id, p_other_id);
	}
}

void DisplayServer::accessibility_update_set_live(
	const RID& p_id, DisplayServerEnums::AccessibilityLiveMode p_live)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_live(
			p_id, (AccessibilityServerEnums::AccessibilityLiveMode)p_live);
	}
}

void DisplayServer::accessibility_update_add_action(
	const RID& p_id, DisplayServerEnums::AccessibilityAction p_action, const Callable& p_callable)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_action(
			p_id, (AccessibilityServerEnums::AccessibilityAction)p_action, p_callable);
	}
}

void DisplayServer::accessibility_update_add_custom_action(
	const RID& p_id, int p_action_id, const String& p_action_description)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_add_custom_action(
			p_id, p_action_id, p_action_description);
	}
}

void DisplayServer::accessibility_update_set_table_row_count(const RID& p_id, int p_count)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_table_row_count(p_id, p_count);
	}
}

void DisplayServer::accessibility_update_set_table_column_count(const RID& p_id, int p_count)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_table_column_count(p_id, p_count);
	}
}

void DisplayServer::accessibility_update_set_table_row_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_table_row_index(p_id, p_index);
	}
}

void DisplayServer::accessibility_update_set_table_column_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_table_column_index(p_id, p_index);
	}
}

void DisplayServer::accessibility_update_set_table_cell_position(
	const RID& p_id, int p_row_index, int p_column_index)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_table_cell_position(
			p_id, p_row_index, p_column_index);
	}
}

void DisplayServer::accessibility_update_set_table_cell_span(
	const RID& p_id, int p_row_span, int p_column_span)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_table_cell_span(
			p_id, p_row_span, p_column_span);
	}
}

void DisplayServer::accessibility_update_set_list_item_count(const RID& p_id, int p_size)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_list_item_count(p_id, p_size);
	}
}

void DisplayServer::accessibility_update_set_list_item_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_list_item_index(p_id, p_index);
	}
}

void DisplayServer::accessibility_update_set_list_item_level(const RID& p_id, int p_level)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_list_item_level(p_id, p_level);
	}
}

void DisplayServer::accessibility_update_set_list_item_selected(const RID& p_id, bool p_selected)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_list_item_selected(p_id, p_selected);
	}
}

void DisplayServer::accessibility_update_set_list_item_expanded(const RID& p_id, bool p_expanded)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_list_item_expanded(p_id, p_expanded);
	}
}

void DisplayServer::accessibility_update_set_popup_type(
	const RID& p_id, DisplayServerEnums::AccessibilityPopupType p_popup)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_popup_type(
			p_id, (AccessibilityServerEnums::AccessibilityPopupType)p_popup);
	}
}

void DisplayServer::accessibility_update_set_checked(const RID& p_id, bool p_checekd)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_checked(p_id, p_checekd);
	}
}

void DisplayServer::accessibility_update_set_num_value(const RID& p_id, double p_position)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_num_value(p_id, p_position);
	}
}

void DisplayServer::accessibility_update_set_num_range(const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_num_range(p_id, p_min, p_max);
	}
}

void DisplayServer::accessibility_update_set_num_step(const RID& p_id, double p_step)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_num_step(p_id, p_step);
	}
}

void DisplayServer::accessibility_update_set_num_jump(const RID& p_id, double p_jump)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_num_jump(p_id, p_jump);
	}
}

void DisplayServer::accessibility_update_set_scroll_x(const RID& p_id, double p_position)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_scroll_x(p_id, p_position);
	}
}

void DisplayServer::accessibility_update_set_scroll_x_range(
	const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_scroll_x_range(p_id, p_min, p_max);
	}
}

void DisplayServer::accessibility_update_set_scroll_y(const RID& p_id, double p_position)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_scroll_y(p_id, p_position);
	}
}

void DisplayServer::accessibility_update_set_scroll_y_range(
	const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_scroll_y_range(p_id, p_min, p_max);
	}
}

void DisplayServer::accessibility_update_set_text_decorations(
	const RID& p_id, bool p_underline, bool p_strikethrough, bool p_overline)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_text_decorations(
			p_id, p_underline, p_strikethrough, p_overline, Color(0, 0, 0, 1));
	}
}

void DisplayServer::accessibility_update_set_text_align(
	const RID& p_id, HorizontalAlignment p_align)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_text_align(p_id, p_align);
	}
}

void DisplayServer::accessibility_update_set_text_selection(const RID& p_id,
	const RID& p_text_start_id, int p_start_char, const RID& p_text_end_id, int p_end_char)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_text_selection(
			p_id, p_text_start_id, p_start_char, p_text_end_id, p_end_char);
	}
}

void DisplayServer::accessibility_update_set_flag(
	const RID& p_id, DisplayServerEnums::AccessibilityFlags p_flag, bool p_value)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_flag(
			p_id, (AccessibilityServerEnums::AccessibilityFlags)p_flag, p_value);
	}
}

void DisplayServer::accessibility_update_set_classname(const RID& p_id, const String& p_classname)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_classname(p_id, p_classname);
	}
}

void DisplayServer::accessibility_update_set_placeholder(
	const RID& p_id, const String& p_placeholder)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_placeholder(p_id, p_placeholder);
	}
}

void DisplayServer::accessibility_update_set_language(const RID& p_id, const String& p_language)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_language(p_id, p_language);
	}
}

void DisplayServer::accessibility_update_set_text_orientation(const RID& p_id, bool p_vertical)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_text_orientation(p_id, p_vertical);
	}
}

void DisplayServer::accessibility_update_set_list_orientation(const RID& p_id, bool p_vertical)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_list_orientation(p_id, p_vertical);
	}
}

void DisplayServer::accessibility_update_set_shortcut(const RID& p_id, const String& p_shortcut)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_shortcut(p_id, p_shortcut);
	}
}

void DisplayServer::accessibility_update_set_url(const RID& p_id, const String& p_url)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_url(p_id, p_url);
	}
}

void DisplayServer::accessibility_update_set_role_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_role_description(p_id, p_description);
	}
}

void DisplayServer::accessibility_update_set_state_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_state_description(p_id, p_description);
	}
}

void DisplayServer::accessibility_update_set_color_value(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_color_value(p_id, p_color);
	}
}

void DisplayServer::accessibility_update_set_background_color(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_background_color(p_id, p_color);
	}
}

void DisplayServer::accessibility_update_set_foreground_color(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::get_singleton()) {
		AccessibilityServer::get_singleton()->update_set_foreground_color(p_id, p_color);
	}
}

#endif // DISABLE_DEPRECATED

Point2i DisplayServer::ime_get_selection() const
{
	ERR_FAIL_V_MSG(
		Point2i(), "IME or NOTIFICATION_WM_IME_UPDATE not supported by this display server.");
}

String DisplayServer::ime_get_text() const
{
	ERR_FAIL_V_MSG(
		String(), "IME or NOTIFICATION_WM_IME_UPDATE not supported by this display server.");
}

void DisplayServer::virtual_keyboard_show(const String& p_existing_text, const Rect2& p_screen_rect,
	DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start,
	int p_cursor_end)
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
}

void DisplayServer::virtual_keyboard_hide()
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
}

// returns height of the currently shown keyboard (0 if keyboard is hidden)
int DisplayServer::virtual_keyboard_get_height() const
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
	return 0;
}

bool DisplayServer::has_hardware_keyboard() const { return true; }

void DisplayServer::cursor_set_shape(DisplayServerEnums::CursorShape p_shape)
{
	WARN_PRINT("Cursor shape not supported by this display server.");
}

DisplayServerEnums::CursorShape DisplayServer::cursor_get_shape() const
{
	return DisplayServerEnums::CURSOR_ARROW;
}

void DisplayServer::cursor_set_custom_image(const Ref<Resource>& p_cursor,
	DisplayServerEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	WARN_PRINT("Custom cursor shape not supported by this display server.");
}

bool DisplayServer::get_swap_cancel_ok() { return false; }

void DisplayServer::enable_for_stealing_focus(ProcessID pid) {}

Error DisplayServer::embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid,
	const Rect2i& p_rect, bool p_visible, bool p_grab_focus)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServer::request_close_embedded_process(ProcessID p_pid)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServer::remove_embedded_process(ProcessID p_pid)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

ProcessID DisplayServer::get_focused_process_id()
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return 0;
}

Error DisplayServer::dialog_show(
	String p_title, String p_description, Vector<String> p_buttons, const Callable& p_callback)
{
	WARN_PRINT("Native dialogs not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServer::dialog_input_text(
	String p_title, String p_description, String p_partial, const Callable& p_callback)
{
	WARN_PRINT("Native dialogs not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServer::file_dialog_show(const String& p_title, const String& p_current_directory,
	const String& p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode,
	const Vector<String>& p_filters, const Callable& p_callback,
	DisplayServerEnums::WindowID p_window_id)
{
	WARN_PRINT("Native dialogs not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServer::file_dialog_with_options_show(const String& p_title,
	const String& p_current_directory, const String& p_root, const String& p_filename,
	bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String>& p_filters,
	const TypedArray<Dictionary>& p_options, const Callable& p_callback,
	DisplayServerEnums::WindowID p_window_id)
{
	WARN_PRINT("Native dialogs not supported by this display server.");
	return ERR_UNAVAILABLE;
}

void DisplayServer::beep() const {}

int DisplayServer::keyboard_get_layout_count() const { return 0; }

int DisplayServer::keyboard_get_current_layout() const { return -1; }

void DisplayServer::keyboard_set_current_layout(int p_index) {}

String DisplayServer::keyboard_get_layout_language(int p_index) const { return ""; }

String DisplayServer::keyboard_get_layout_name(int p_index) const { return "Not supported"; }

Key DisplayServer::keyboard_get_keycode_from_physical(Key p_keycode) const
{
	ERR_FAIL_V_MSG(p_keycode, "Not supported by this display server.");
}

Key DisplayServer::keyboard_get_label_from_physical(Key p_keycode) const
{
	ERR_FAIL_V_MSG(p_keycode, "Not supported by this display server.");
}

void DisplayServer::show_emoji_and_symbol_picker() const {}

bool DisplayServer::color_picker(const Callable& p_callback) { return false; }

void DisplayServer::force_process_and_drop_events() {}

void DisplayServer::release_rendering_thread()
{
	WARN_PRINT("Rendering thread not supported by this display server.");
}

void DisplayServer::swap_buffers()
{
	WARN_PRINT("Swap buffers not supported by this display server.");
}

void DisplayServer::set_native_icon(const String& p_filename)
{
	WARN_PRINT("Native icon not supported by this display server.");
}

void DisplayServer::set_icon(const Ref<Image>& p_icon)
{
	WARN_PRINT("Icon not supported by this display server.");
}

DisplayServerEnums::IndicatorID DisplayServer::create_status_indicator(
	const Ref<Texture2D>& p_icon, const String& p_tooltip, const Callable& p_callback)
{
	WARN_PRINT("Status indicator not supported by this display server.");
	return DisplayServerEnums::INVALID_INDICATOR_ID;
}

void DisplayServer::status_indicator_set_icon(
	DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D>& p_icon)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

void DisplayServer::status_indicator_set_tooltip(
	DisplayServerEnums::IndicatorID p_id, const String& p_tooltip)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

void DisplayServer::status_indicator_set_menu(
	DisplayServerEnums::IndicatorID p_id, const RID& p_menu_rid)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

void DisplayServer::status_indicator_set_callback(
	DisplayServerEnums::IndicatorID p_id, const Callable& p_callback)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

Rect2 DisplayServer::status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const
{
	WARN_PRINT("Status indicator not supported by this display server.");
	return Rect2();
}

void DisplayServer::delete_status_indicator(DisplayServerEnums::IndicatorID p_id)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

int64_t DisplayServer::window_get_native_handle(
	DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const
{
	WARN_PRINT("Native handle not supported by this display server.");
	return 0;
}

void DisplayServer::window_set_vsync_mode(
	DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("Changing the V-Sync mode is not supported by this display server.");
}

DisplayServerEnums::VSyncMode DisplayServer::window_get_vsync_mode(
	DisplayServerEnums::WindowID p_window) const
{
	WARN_PRINT("Changing the V-Sync mode is not supported by this display server.");
	return DisplayServerEnums::VSyncMode::VSYNC_ENABLED;
}

bool DisplayServer::window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

void DisplayServer::window_request_hdr_output(
	const bool p_enable, DisplayServerEnums::WindowID p_window)
{
	if (p_enable) {
		WARN_PRINT("HDR output requested, but it is not supported by this display server.");
	}
}

bool DisplayServer::window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

bool DisplayServer::window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

void DisplayServer::window_set_hdr_output_reference_luminance(
	const float p_reference_luminance, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("Attempting to set reference luminance, but HDR output is not supported by this "
			   "display server.");
}

float DisplayServer::window_get_hdr_output_reference_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return -1.0f;
}

float DisplayServer::window_get_hdr_output_current_reference_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return 0.0f;
}

void DisplayServer::window_set_hdr_output_max_luminance(
	const float p_max_luminance, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT(
		"Attempting to set max luminance, but HDR output is not supported by this display server.");
}

float DisplayServer::window_get_hdr_output_max_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return -1.0f;
}

float DisplayServer::window_get_hdr_output_current_max_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return 0.0f;
}

float DisplayServer::window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window) const
{
	return 1.0f;
}

DisplayServerEnums::WindowID DisplayServer::get_focused_window() const
{
	return DisplayServerEnums::MAIN_WINDOW_ID; // Proper value for single windows.
}

void DisplayServer::set_context(DisplayServerEnums::Context p_context) {}

void DisplayServer::register_additional_output(Object* p_object)
{
	ObjectID id = p_object->get_instance_id();
	if (!additional_outputs.has(id)) {
		additional_outputs.push_back(id);
	}
}

void DisplayServer::unregister_additional_output(Object* p_object)
{
	additional_outputs.erase(p_object->get_instance_id());
}

void DisplayServer::_bind_methods() {}

Ref<Image> DisplayServer::_get_cursor_image_from_resource(
	const Ref<Resource>& p_cursor, const Vector2& p_hotspot)
{
	Ref<Image> image;
	ERR_FAIL_COND_V_MSG(p_hotspot.x < 0 || p_hotspot.y < 0, image, "Hotspot outside cursor image.");

	Ref<Texture2D> texture = p_cursor;
	if (texture.is_valid()) {
		image = texture->get_image();
	}
	else {
		image = p_cursor;
	}
	ERR_FAIL_COND_V(image.is_null(), image);

	Size2 image_size = image->get_size();
	ERR_FAIL_COND_V_MSG(p_hotspot.x > image_size.width || p_hotspot.y > image_size.height, image,
		"Hotspot outside cursor image.");
	ERR_FAIL_COND_V_MSG(image_size.width > 256 || image_size.height > 256, image,
		"Cursor image too big. Max supported size is 256x256.");

	if (image->is_compressed()) {
		image = image->duplicate(true);
		Error err = image->decompress();
		ERR_FAIL_COND_V_MSG(err != OK, Ref<Image>(),
			"Couldn't decompress VRAM-compressed custom mouse cursor image. Switch to a lossless "
			"compression mode in the Import dock.");
	}
	return image;
}

void DisplayServer::register_create_function(
	const char* p_name, CreateFunction p_function, GetRenderingDriversFunction p_get_drivers)
{
	ERR_FAIL_COND(server_create_count == MAX_SERVERS);
	// Headless display server is always last
	server_create_functions[server_create_count] = server_create_functions[server_create_count - 1];
	server_create_functions[server_create_count - 1].name = p_name;
	server_create_functions[server_create_count - 1].create_function = p_function;
	server_create_functions[server_create_count - 1].get_rendering_drivers_function = p_get_drivers;
	server_create_count++;
}

int DisplayServer::get_create_function_count() { return server_create_count; }

const char* DisplayServer::get_create_function_name(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	return server_create_functions[p_index].name;
}

Vector<String> DisplayServer::get_create_function_rendering_drivers(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, Vector<String>());
	return server_create_functions[p_index].get_rendering_drivers_function();
}

DisplayServer* DisplayServer::create(int p_index, const String& p_rendering_driver,
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Vector2i* p_position, const Vector2i& p_resolution, int p_screen,
	DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	return server_create_functions[p_index].create_function(p_rendering_driver, p_mode,
		p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window,
		r_error);
}

void DisplayServer::_input_set_mouse_mode(InputClassEnums::MouseMode p_mode)
{
	singleton->mouse_set_mode(DisplayServerEnums::MouseMode(p_mode));
}

InputClassEnums::MouseMode DisplayServer::_input_get_mouse_mode()
{
	return InputClassEnums::MouseMode(singleton->mouse_get_mode());
}

void DisplayServer::_input_set_mouse_mode_override(InputClassEnums::MouseMode p_mode)
{
	singleton->mouse_set_mode_override(DisplayServerEnums::MouseMode(p_mode));
}

InputClassEnums::MouseMode DisplayServer::_input_get_mouse_mode_override()
{
	return InputClassEnums::MouseMode(singleton->mouse_get_mode_override());
}

void DisplayServer::_input_set_mouse_mode_override_enabled(bool p_enabled)
{
	singleton->mouse_set_mode_override_enabled(p_enabled);
}

bool DisplayServer::_input_is_mouse_mode_override_enabled()
{
	return singleton->mouse_is_mode_override_enabled();
}

void DisplayServer::_input_warp(const Vector2& p_to_pos) { singleton->warp_mouse(p_to_pos); }

InputClassEnums::CursorShape DisplayServer::_input_get_current_cursor_shape()
{
	return (InputClassEnums::CursorShape)singleton->cursor_get_shape();
}

void DisplayServer::_input_set_custom_mouse_cursor_func(
	const Ref<Resource>& p_image, InputClassEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	singleton->cursor_set_custom_image(
		p_image, (DisplayServerEnums::CursorShape)p_shape, p_hotspot);
}

bool DisplayServer::is_rendering_device_supported()
{
#if defined(RD_ENABLED)
	RenderingDevice* device = RenderingDevice::get_singleton();
	if (device) {
		return true;
	}

	if (supported_rendering_device == DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS) {
		return true;
	}
	else if (supported_rendering_device ==
			   DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE) {
		return false;
	}

	Error err;

#if defined(WINDOWS_ENABLED) || defined(LINUXBSD_ENABLED)
	// On some drivers combining OpenGL and RenderingDevice can result in crash, offload the check
	// to the subprocess.
	List<String> arguments;
	arguments.push_back("--test-rd-support");
	if (get_singleton()) {
		arguments.push_back("--display-driver");
		arguments.push_back(get_singleton()->get_name().to_lower());
	}

	String pipe;
	int exitcode = 0;
	err = OS::get_singleton()->execute(
		OS::get_singleton()->get_executable_path(), arguments, &pipe, &exitcode);
	if (err == OK && exitcode == 0) {
		supported_rendering_device = DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS;
		return true;
	}
	else {
		supported_rendering_device = DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE;
	}
#else // WINDOWS_ENABLED

	RenderingContextDriver* rcd = nullptr;

#if defined(VULKAN_ENABLED)
	rcd = memnew(RenderingContextDriverVulkan);
#endif
#ifdef D3D12_ENABLED
	if (rcd == nullptr) {
		rcd = memnew(RenderingContextDriverD3D12);
	}
#endif
#ifdef METAL_ENABLED
	if (rcd == nullptr) {
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
		// Eliminate "RenderingContextDriverMetal is only available on iOS 14.0 or newer".
		rcd = memnew(RenderingContextDriverMetal);
		GODOT_CLANG_WARNING_POP
	}
#endif

	if (rcd != nullptr) {
		err = rcd->initialize();
		if (err == OK) {
			RenderingDevice* rd = memnew(RenderingDevice);
			err = rd->initialize(rcd);
			memdelete(rd);
			rd = nullptr;
			if (err == OK) {
				// Creating a RenderingDevice is quite slow.
				// Cache the result for future usage, so that it's much faster on subsequent calls.
				supported_rendering_device =
					DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS;
				memdelete(rcd);
				rcd = nullptr;
				return true;
			}
			else {
				supported_rendering_device =
					DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE;
			}
		}

		memdelete(rcd);
		rcd = nullptr;
	}

#endif // WINDOWS_ENABLED
#endif // RD_ENABLED
	return false;
}

bool DisplayServer::can_create_rendering_device()
{
	if (get_singleton() && get_singleton()->get_name() == "headless") {
		return false;
	}

#if defined(RD_ENABLED)
	RenderingDevice* device = RenderingDevice::get_singleton();
	if (device) {
		return true;
	}

	if (created_rendering_device == DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS) {
		return true;
	}
	else if (created_rendering_device ==
			   DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE) {
		return false;
	}

	Error err;

#ifdef WINDOWS_ENABLED
	// On some NVIDIA drivers combining OpenGL and RenderingDevice can result in crash, offload the
	// check to the subprocess.
	List<String> arguments;
	arguments.push_back("--test-rd-creation");

	String pipe;
	int exitcode = 0;
	err = OS::get_singleton()->execute(
		OS::get_singleton()->get_executable_path(), arguments, &pipe, &exitcode);
	if (err == OK && exitcode == 0) {
		created_rendering_device = DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS;
		return true;
	}
	else {
		created_rendering_device = DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE;
	}
#else // WINDOWS_ENABLED

	RenderingContextDriver* rcd = nullptr;

#if defined(VULKAN_ENABLED)
	rcd = memnew(RenderingContextDriverVulkan);
#endif
#ifdef D3D12_ENABLED
	if (rcd == nullptr) {
		rcd = memnew(RenderingContextDriverD3D12);
	}
#endif
#ifdef METAL_ENABLED
	if (rcd == nullptr) {
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
		// Eliminate "RenderingContextDriverMetal is only available on iOS 14.0 or newer".
		rcd = memnew(RenderingContextDriverMetal);
		GODOT_CLANG_WARNING_POP
	}
#endif

	if (rcd != nullptr) {
		err = rcd->initialize();
		if (err == OK) {
			RenderingDevice* rd = memnew(RenderingDevice);
			err = rd->initialize(rcd);
			memdelete(rd);
			rd = nullptr;
			if (err == OK) {
				// Creating a RenderingDevice is quite slow.
				// Cache the result for future usage, so that it's much faster on subsequent calls.
				created_rendering_device =
					DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS;
				memdelete(rcd);
				rcd = nullptr;
				return true;
			}
			else {
				created_rendering_device =
					DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE;
			}
		}

		memdelete(rcd);
		rcd = nullptr;
	}

#endif // WINDOWS_ENABLED
#endif // RD_ENABLED
	return false;
}

DisplayServer::DisplayServer()
{
	singleton = this;
	Input::set_mouse_mode_func = _input_set_mouse_mode;
	Input::get_mouse_mode_func = _input_get_mouse_mode;
	Input::set_mouse_mode_override_func = _input_set_mouse_mode_override;
	Input::get_mouse_mode_override_func = _input_get_mouse_mode_override;
	Input::set_mouse_mode_override_enabled_func = _input_set_mouse_mode_override_enabled;
	Input::is_mouse_mode_override_enabled_func = _input_is_mouse_mode_override_enabled;
	Input::warp_mouse_func = _input_warp;
	Input::get_current_cursor_shape_func = _input_get_current_cursor_shape;
	Input::set_custom_mouse_cursor_func = _input_set_custom_mouse_cursor_func;
}

DisplayServer::~DisplayServer() { singleton = nullptr; }


