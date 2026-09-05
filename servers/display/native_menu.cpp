/**************************************************************************/
/*  native_menu.cpp                                                       */
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

#include "native_menu.h"
#include "scene/resources/texture.h"

NativeMenu* NativeMenu::singleton = nullptr;

void NativeMenu::_bind_methods() {}

bool NativeMenu::has_feature(Feature p_feature) const { return false; }

bool NativeMenu::has_system_menu(SystemMenus p_menu_id) const { return false; }

RID NativeMenu::get_system_menu(SystemMenus p_menu_id) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

String NativeMenu::get_system_menu_name(SystemMenus p_menu_id) const
{
	switch (p_menu_id) {
	case MAIN_MENU_ID:
		return "Main menu";
	case APPLICATION_MENU_ID:
		return "Application menu";
	case WINDOW_MENU_ID:
		return "Window menu";
	case HELP_MENU_ID:
		return "Help menu";
	case DOCK_MENU_ID:
		return "Dock menu";
	default:
		return "Invalid";
	}
}

String NativeMenu::get_system_menu_text(SystemMenus p_menu_id) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

void NativeMenu::set_system_menu_text(SystemMenus p_menu_id, const String& p_name)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

RID NativeMenu::create_menu()
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

bool NativeMenu::has_menu(const RID& p_rid) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenu::free_menu(const RID& p_rid)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

Size2 NativeMenu::get_size(const RID& p_rid) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return Size2();
}

void NativeMenu::popup(const RID& p_rid, const Vector2i& p_position)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_interface_direction(const RID& p_rid, bool p_is_rtl)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_popup_open_callback(const RID& p_rid, const Callable& p_callback)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

Callable NativeMenu::get_popup_open_callback(const RID& p_rid) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return Callable();
}

void NativeMenu::set_popup_close_callback(const RID& p_rid, const Callable& p_callback)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

Callable NativeMenu::get_popup_close_callback(const RID& p_rid) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return Callable();
}

bool NativeMenu::is_opened(const RID& p_rid) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenu::set_minimum_width(const RID& p_rid, float p_width)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

float NativeMenu::get_minimum_width(const RID& p_rid) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0.f;
}

int NativeMenu::add_submenu_item(const RID& p_rid, const String& p_label, const RID& p_submenu_rid,
	const Variant& p_tag, int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::add_item(const RID& p_rid, const String& p_label, const Callable& p_callback,
	const Callable& p_key_callback, const Variant& p_tag, Key p_accel, int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::add_check_item(const RID& p_rid, const String& p_label, const Callable& p_callback,
	const Callable& p_key_callback, const Variant& p_tag, Key p_accel, int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::add_icon_item(const RID& p_rid, const Ref<Texture2D>& p_icon, const String& p_label,
	const Callable& p_callback, const Callable& p_key_callback, const Variant& p_tag, Key p_accel,
	int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::add_icon_check_item(const RID& p_rid, const Ref<Texture2D>& p_icon,
	const String& p_label, const Callable& p_callback, const Callable& p_key_callback,
	const Variant& p_tag, Key p_accel, int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::add_radio_check_item(const RID& p_rid, const String& p_label,
	const Callable& p_callback, const Callable& p_key_callback, const Variant& p_tag, Key p_accel,
	int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::add_icon_radio_check_item(const RID& p_rid, const Ref<Texture2D>& p_icon,
	const String& p_label, const Callable& p_callback, const Callable& p_key_callback,
	const Variant& p_tag, Key p_accel, int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::add_multistate_item(const RID& p_rid, const String& p_label, int p_max_states,
	int p_default_state, const Callable& p_callback, const Callable& p_key_callback,
	const Variant& p_tag, Key p_accel, int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::add_separator(const RID& p_rid, int p_index)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::find_item_index_with_text(const RID& p_rid, const String& p_text) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::find_item_index_with_tag(const RID& p_rid, const Variant& p_tag) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::find_item_index_with_submenu(const RID& p_rid, const RID& p_submenu_rid) const
{
	if (!has_menu(p_rid) || !has_menu(p_submenu_rid)) {
		return -1;
	}
	int count = get_item_count(p_rid);
	for (int i = 0; i < count; i++) {
		if (p_submenu_rid == get_item_submenu(p_rid, i)) {
			return i;
		}
	}
	return -1;
}

bool NativeMenu::is_item_checked(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenu::is_item_indeterminate(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenu::is_item_checkable(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenu::is_item_radio_checkable(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

Callable NativeMenu::get_item_callback(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return Callable();
}

Callable NativeMenu::get_item_key_callback(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return Callable();
}

Variant NativeMenu::get_item_tag(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return Variant();
}

String NativeMenu::get_item_text(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

RID NativeMenu::get_item_submenu(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

Key NativeMenu::get_item_accelerator(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return Key::NONE;
}

bool NativeMenu::is_item_disabled(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenu::is_item_hidden(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

String NativeMenu::get_item_tooltip(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

int NativeMenu::get_item_state(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::get_item_max_states(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

Ref<Texture2D> NativeMenu::get_item_icon(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return Ref<Texture2D>();
}

int NativeMenu::get_item_indentation_level(const RID& p_rid, int p_idx) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0;
}

void NativeMenu::set_item_checked(const RID& p_rid, int p_idx, bool p_checked)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_callback(const RID& p_rid, int p_idx, const Callable& p_callback)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_key_callback(const RID& p_rid, int p_idx, const Callable& p_key_callback)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_hover_callbacks(const RID& p_rid, int p_idx, const Callable& p_callback)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_tag(const RID& p_rid, int p_idx, const Variant& p_tag)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_text(const RID& p_rid, int p_idx, const String& p_text)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_state(const RID& p_rid, int p_idx, int p_state)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_max_states(const RID& p_rid, int p_idx, int p_max_states)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::set_item_indentation_level(const RID& p_rid, int p_idx, int p_level)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

int NativeMenu::set_item_index(const RID& p_rid, int p_idx, int p_target_idx)
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenu::get_item_count(const RID& p_rid) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0;
}

bool NativeMenu::is_system_menu(const RID& p_rid) const
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenu::remove_item(const RID& p_rid, int p_idx)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenu::clear(const RID& p_rid)
{
	WARN_PRINT("Global menus are not supported on this platform.");
}


