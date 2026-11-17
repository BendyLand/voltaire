/**************************************************************************/
/*  native_menu.cpp                                                       */
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

#include "native_menu.h"
#include "scene/resources/texture.h"

NativeMenuDriver::NativeMenuDriver() { NativeMenu::_bind_driver(this); }

NativeMenuDriver::~NativeMenuDriver() { NativeMenu::_unbind_driver(this); }

bool NativeMenuDriver::has_feature(Feature p_feature) const
{
	(void)p_feature;
	return false;
}

bool NativeMenuDriver::has_system_menu(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	return false;
}

RID NativeMenuDriver::get_system_menu(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

String NativeMenuDriver::get_system_menu_name(SystemMenus p_menu_id) const
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

String NativeMenuDriver::get_system_menu_text(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

void NativeMenuDriver::set_system_menu_text(SystemMenus p_menu_id, const String& p_name)
{
	(void)p_menu_id;
	(void)p_name;
	WARN_PRINT("Global menus are not supported on this platform.");
}

RID NativeMenuDriver::create_menu()
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

bool NativeMenuDriver::has_menu(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::free_menu(const RID& p_rid)
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

Size2 NativeMenuDriver::get_size(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Size2();
}

void NativeMenuDriver::popup(const RID& p_rid, const Vector2i& p_position)
{
	(void)p_rid;
	(void)p_position;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_interface_direction(const RID& p_rid, bool p_is_rtl)
{
	(void)p_rid;
	(void)p_is_rtl;
	WARN_PRINT("Global menus are not supported on this platform.");
}

bool NativeMenuDriver::is_opened(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::set_minimum_width(const RID& p_rid, float p_width)
{
	(void)p_rid;
	(void)p_width;
	WARN_PRINT("Global menus are not supported on this platform.");
}

float NativeMenuDriver::get_minimum_width(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0.f;
}

int NativeMenuDriver::add_separator(const RID& p_rid, int p_index)
{
	(void)p_rid;
	(void)p_index;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::find_item_index_with_text(const RID& p_rid, const String& p_text) const
{
	(void)p_rid;
	(void)p_text;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::find_item_index_with_submenu(
	const RID& p_rid, const RID& p_submenu_rid) const
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

bool NativeMenuDriver::is_item_checked(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_indeterminate(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_checkable(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_radio_checkable(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

String NativeMenuDriver::get_item_text(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

RID NativeMenuDriver::get_item_submenu(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

Key NativeMenuDriver::get_item_accelerator(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Key::NONE;
}

bool NativeMenuDriver::is_item_disabled(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_hidden(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

String NativeMenuDriver::get_item_tooltip(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

int NativeMenuDriver::get_item_state(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::get_item_max_states(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

Ref<Texture2D> NativeMenuDriver::get_item_icon(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Ref<Texture2D>();
}

int NativeMenuDriver::get_item_indentation_level(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0;
}

void NativeMenuDriver::set_item_checked(const RID& p_rid, int p_idx, bool p_checked)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checked;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_indeterminate;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checkable;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checkable;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_text(const RID& p_rid, int p_idx, const String& p_text)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_text;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_submenu_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_keycode;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_disabled;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_hidden;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_tooltip;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_state(const RID& p_rid, int p_idx, int p_state)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_state;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_max_states(const RID& p_rid, int p_idx, int p_max_states)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_max_states;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_icon;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_indentation_level(const RID& p_rid, int p_idx, int p_level)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_level;
	WARN_PRINT("Global menus are not supported on this platform.");
}

int NativeMenuDriver::set_item_index(const RID& p_rid, int p_idx, int p_target_idx)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_target_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::get_item_count(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0;
}

bool NativeMenuDriver::is_system_menu(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::remove_item(const RID& p_rid, int p_idx)
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::clear(const RID& p_rid)
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

/* NativeMenu Static Dispatch */

void NativeMenu::_bind_driver(NativeMenuDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
}

void NativeMenu::_unbind_driver(NativeMenuDriver* p_driver)
{
	if (data && data->driver == p_driver) {
		data->driver = nullptr;
	}
}

void NativeMenu::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
	if (!data->driver) {
		memnew(NativeMenuDriver);
	}
}

void NativeMenu::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		NativeMenuDriver* drv = data->driver;
		data->driver = nullptr;
		memdelete(drv);
	}
	memdelete(data);
	data = nullptr;
}

bool NativeMenu::has_feature(Feature p_feature)
{
	return is_initialized() ? data->driver->has_feature(p_feature) : false;
}

bool NativeMenu::has_system_menu(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->has_system_menu(p_menu_id) : false;
}

RID NativeMenu::get_system_menu(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu(p_menu_id) : RID();
}

String NativeMenu::get_system_menu_name(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu_name(p_menu_id) : String();
}

String NativeMenu::get_system_menu_text(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu_text(p_menu_id) : String();
}

void NativeMenu::set_system_menu_text(SystemMenus p_menu_id, const String& p_name)
{
	if (is_initialized()) {
		data->driver->set_system_menu_text(p_menu_id, p_name);
	}
}

RID NativeMenu::create_menu() { return is_initialized() ? data->driver->create_menu() : RID(); }

bool NativeMenu::has_menu(const RID& p_rid)
{
	return is_initialized() ? data->driver->has_menu(p_rid) : false;
}

void NativeMenu::free_menu(const RID& p_rid)
{
	if (is_initialized()) {
		data->driver->free_menu(p_rid);
	}
}

Size2 NativeMenu::get_size(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_size(p_rid) : Size2();
}

void NativeMenu::popup(const RID& p_rid, const Vector2i& p_position)
{
	if (is_initialized()) {
		data->driver->popup(p_rid, p_position);
	}
}

void NativeMenu::set_interface_direction(const RID& p_rid, bool p_is_rtl)
{
	if (is_initialized()) {
		data->driver->set_interface_direction(p_rid, p_is_rtl);
	}
}

void NativeMenu::set_minimum_width(const RID& p_rid, float p_width)
{
	if (is_initialized()) {
		data->driver->set_minimum_width(p_rid, p_width);
	}
}

float NativeMenu::get_minimum_width(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_minimum_width(p_rid) : 0.f;
}

bool NativeMenu::is_opened(const RID& p_rid)
{
	return is_initialized() ? data->driver->is_opened(p_rid) : false;
}

int NativeMenu::add_separator(const RID& p_rid, int p_index)
{
	return is_initialized() ? data->driver->add_separator(p_rid, p_index) : -1;
}

int NativeMenu::find_item_index_with_text(const RID& p_rid, const String& p_text)
{
	return is_initialized() ? data->driver->find_item_index_with_text(p_rid, p_text) : -1;
}

int NativeMenu::find_item_index_with_submenu(const RID& p_rid, const RID& p_submenu_rid)
{
	return is_initialized() ? data->driver->find_item_index_with_submenu(p_rid, p_submenu_rid) : -1;
}

bool NativeMenu::is_item_checked(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_checked(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_indeterminate(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_indeterminate(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_checkable(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_checkable(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_radio_checkable(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_radio_checkable(p_rid, p_idx) : false;
}

String NativeMenu::get_item_text(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_text(p_rid, p_idx) : String();
}

RID NativeMenu::get_item_submenu(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_submenu(p_rid, p_idx) : RID();
}

Key NativeMenu::get_item_accelerator(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_accelerator(p_rid, p_idx) : Key::NONE;
}

bool NativeMenu::is_item_disabled(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_disabled(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_hidden(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_hidden(p_rid, p_idx) : false;
}

String NativeMenu::get_item_tooltip(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_tooltip(p_rid, p_idx) : String();
}

int NativeMenu::get_item_state(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_state(p_rid, p_idx) : -1;
}

int NativeMenu::get_item_max_states(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_max_states(p_rid, p_idx) : -1;
}

Ref<Texture2D> NativeMenu::get_item_icon(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_icon(p_rid, p_idx) : Ref<Texture2D>();
}

int NativeMenu::get_item_indentation_level(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_indentation_level(p_rid, p_idx) : 0;
}

void NativeMenu::set_item_checked(const RID& p_rid, int p_idx, bool p_checked)
{
	if (is_initialized()) {
		data->driver->set_item_checked(p_rid, p_idx, p_checked);
	}
}

void NativeMenu::set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate)
{
	if (is_initialized()) {
		data->driver->set_item_indeterminate(p_rid, p_idx, p_indeterminate);
	}
}

void NativeMenu::set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	if (is_initialized()) {
		data->driver->set_item_checkable(p_rid, p_idx, p_checkable);
	}
}

void NativeMenu::set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	if (is_initialized()) {
		data->driver->set_item_radio_checkable(p_rid, p_idx, p_checkable);
	}
}

void NativeMenu::set_item_text(const RID& p_rid, int p_idx, const String& p_text)
{
	if (is_initialized()) {
		data->driver->set_item_text(p_rid, p_idx, p_text);
	}
}

void NativeMenu::set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid)
{
	if (is_initialized()) {
		data->driver->set_item_submenu(p_rid, p_idx, p_submenu_rid);
	}
}

void NativeMenu::set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode)
{
	if (is_initialized()) {
		data->driver->set_item_accelerator(p_rid, p_idx, p_keycode);
	}
}

void NativeMenu::set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled)
{
	if (is_initialized()) {
		data->driver->set_item_disabled(p_rid, p_idx, p_disabled);
	}
}

void NativeMenu::set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden)
{
	if (is_initialized()) {
		data->driver->set_item_hidden(p_rid, p_idx, p_hidden);
	}
}

void NativeMenu::set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip)
{
	if (is_initialized()) {
		data->driver->set_item_tooltip(p_rid, p_idx, p_tooltip);
	}
}

void NativeMenu::set_item_state(const RID& p_rid, int p_idx, int p_state)
{
	if (is_initialized()) {
		data->driver->set_item_state(p_rid, p_idx, p_state);
	}
}

void NativeMenu::set_item_max_states(const RID& p_rid, int p_idx, int p_max_states)
{
	if (is_initialized()) {
		data->driver->set_item_max_states(p_rid, p_idx, p_max_states);
	}
}

void NativeMenu::set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon)
{
	if (is_initialized()) {
		data->driver->set_item_icon(p_rid, p_idx, p_icon);
	}
}

void NativeMenu::set_item_indentation_level(const RID& p_rid, int p_idx, int p_level)
{
	if (is_initialized()) {
		data->driver->set_item_indentation_level(p_rid, p_idx, p_level);
	}
}

int NativeMenu::set_item_index(const RID& p_rid, int p_idx, int p_target_idx)
{
	return is_initialized() ? data->driver->set_item_index(p_rid, p_idx, p_target_idx) : -1;
}

int NativeMenu::get_item_count(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_item_count(p_rid) : 0;
}

bool NativeMenu::is_system_menu(const RID& p_rid)
{
	return is_initialized() ? data->driver->is_system_menu(p_rid) : false;
}

void NativeMenu::remove_item(const RID& p_rid, int p_idx)
{
	if (is_initialized()) {
		data->driver->remove_item(p_rid, p_idx);
	}
}

void NativeMenu::clear(const RID& p_rid)
{
	if (is_initialized()) {
		data->driver->clear(p_rid);
	}
}
