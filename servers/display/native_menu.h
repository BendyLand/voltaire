/**************************************************************************/
/*  native_menu.h                                                         */
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

#pragma once

#include "core/os/keyboard.h"
#include "core/types.h"

class Texture2D;

namespace NativeMenuEnums
{
enum Feature
{
	FEATURE_GLOBAL_MENU,
	FEATURE_POPUP_MENU,
	FEATURE_OPEN_CLOSE_CALLBACK,
	FEATURE_HOVER_CALLBACK,
	FEATURE_KEY_CALLBACK,
};

enum SystemMenus
{
	INVALID_MENU_ID,
	MAIN_MENU_ID,
	APPLICATION_MENU_ID,
	WINDOW_MENU_ID,
	HELP_MENU_ID,
	DOCK_MENU_ID,
};
} // namespace NativeMenuEnums

class NativeMenuDriver
{
public:
	using Feature = NativeMenuEnums::Feature;
	using SystemMenus = NativeMenuEnums::SystemMenus;

	static constexpr Feature FEATURE_GLOBAL_MENU = NativeMenuEnums::FEATURE_GLOBAL_MENU;
	static constexpr Feature FEATURE_POPUP_MENU = NativeMenuEnums::FEATURE_POPUP_MENU;
	static constexpr Feature FEATURE_OPEN_CLOSE_CALLBACK =
		NativeMenuEnums::FEATURE_OPEN_CLOSE_CALLBACK;
	static constexpr Feature FEATURE_HOVER_CALLBACK = NativeMenuEnums::FEATURE_HOVER_CALLBACK;
	static constexpr Feature FEATURE_KEY_CALLBACK = NativeMenuEnums::FEATURE_KEY_CALLBACK;

	static constexpr SystemMenus INVALID_MENU_ID = NativeMenuEnums::INVALID_MENU_ID;
	static constexpr SystemMenus MAIN_MENU_ID = NativeMenuEnums::MAIN_MENU_ID;
	static constexpr SystemMenus APPLICATION_MENU_ID = NativeMenuEnums::APPLICATION_MENU_ID;
	static constexpr SystemMenus WINDOW_MENU_ID = NativeMenuEnums::WINDOW_MENU_ID;
	static constexpr SystemMenus HELP_MENU_ID = NativeMenuEnums::HELP_MENU_ID;
	static constexpr SystemMenus DOCK_MENU_ID = NativeMenuEnums::DOCK_MENU_ID;

	virtual bool has_feature(Feature p_feature) const;

	virtual bool has_system_menu(SystemMenus p_menu_id) const;
	virtual RID get_system_menu(SystemMenus p_menu_id) const;
	virtual String get_system_menu_name(SystemMenus p_menu_id) const;

	virtual String get_system_menu_text(SystemMenus p_menu_id) const;
	virtual void set_system_menu_text(SystemMenus p_menu_id, const String& p_name);

	virtual RID create_menu();
	virtual bool has_menu(const RID& p_rid) const;
	virtual void free_menu(const RID& p_rid);

	virtual Size2 get_size(const RID& p_rid) const;
	virtual void popup(const RID& p_rid, const Vector2i& p_position);

	virtual void set_interface_direction(const RID& p_rid, bool p_is_rtl);

	virtual void set_minimum_width(const RID& p_rid, float p_width);
	virtual float get_minimum_width(const RID& p_rid) const;

	virtual bool is_opened(const RID& p_rid) const;

	virtual int add_separator(const RID& p_rid, int p_index = -1);

	virtual int find_item_index_with_text(const RID& p_rid, const String& p_text) const;
	virtual int find_item_index_with_submenu(const RID& p_rid, const RID& p_submenu_rid) const;

	virtual bool is_item_checked(const RID& p_rid, int p_idx) const;
	virtual bool is_item_indeterminate(const RID& p_rid, int p_idx) const;
	virtual bool is_item_checkable(const RID& p_rid, int p_idx) const;
	virtual bool is_item_radio_checkable(const RID& p_rid, int p_idx) const;
	virtual String get_item_text(const RID& p_rid, int p_idx) const;
	virtual RID get_item_submenu(const RID& p_rid, int p_idx) const;
	virtual Key get_item_accelerator(const RID& p_rid, int p_idx) const;
	virtual bool is_item_disabled(const RID& p_rid, int p_idx) const;
	virtual bool is_item_hidden(const RID& p_rid, int p_idx) const;
	virtual String get_item_tooltip(const RID& p_rid, int p_idx) const;
	virtual int get_item_state(const RID& p_rid, int p_idx) const;
	virtual int get_item_max_states(const RID& p_rid, int p_idx) const;
	virtual Ref<Texture2D> get_item_icon(const RID& p_rid, int p_idx) const;
	virtual int get_item_indentation_level(const RID& p_rid, int p_idx) const;

	virtual void set_item_checked(const RID& p_rid, int p_idx, bool p_checked);
	virtual void set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate);
	virtual void set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable);
	virtual void set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable);
	virtual void set_item_text(const RID& p_rid, int p_idx, const String& p_text);
	virtual void set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid);
	virtual void set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode);
	virtual void set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled);
	virtual void set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden);
	virtual void set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip);
	virtual void set_item_state(const RID& p_rid, int p_idx, int p_state);
	virtual void set_item_max_states(const RID& p_rid, int p_idx, int p_max_states);
	virtual void set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon);
	virtual void set_item_indentation_level(const RID& p_rid, int p_idx, int p_level);
	virtual int set_item_index(const RID& p_rid, int p_idx, int p_target_idx);

	virtual int get_item_count(const RID& p_rid) const;
	virtual bool is_system_menu(const RID& p_rid) const;

	virtual void remove_item(const RID& p_rid, int p_idx);
	virtual void clear(const RID& p_rid);

	NativeMenuDriver();
	virtual ~NativeMenuDriver();
};

class NativeMenu final
{
	friend class NativeMenuDriver;

	static void _bind_driver(NativeMenuDriver* p_driver);
	static void _unbind_driver(NativeMenuDriver* p_driver);

public:
	NativeMenu() = delete;

	using Feature = NativeMenuEnums::Feature;
	using SystemMenus = NativeMenuEnums::SystemMenus;

	static constexpr Feature FEATURE_GLOBAL_MENU = NativeMenuEnums::FEATURE_GLOBAL_MENU;
	static constexpr Feature FEATURE_POPUP_MENU = NativeMenuEnums::FEATURE_POPUP_MENU;
	static constexpr Feature FEATURE_OPEN_CLOSE_CALLBACK =
		NativeMenuEnums::FEATURE_OPEN_CLOSE_CALLBACK;
	static constexpr Feature FEATURE_HOVER_CALLBACK = NativeMenuEnums::FEATURE_HOVER_CALLBACK;
	static constexpr Feature FEATURE_KEY_CALLBACK = NativeMenuEnums::FEATURE_KEY_CALLBACK;

	static constexpr SystemMenus INVALID_MENU_ID = NativeMenuEnums::INVALID_MENU_ID;
	static constexpr SystemMenus MAIN_MENU_ID = NativeMenuEnums::MAIN_MENU_ID;
	static constexpr SystemMenus APPLICATION_MENU_ID = NativeMenuEnums::APPLICATION_MENU_ID;
	static constexpr SystemMenus WINDOW_MENU_ID = NativeMenuEnums::WINDOW_MENU_ID;
	static constexpr SystemMenus HELP_MENU_ID = NativeMenuEnums::HELP_MENU_ID;
	static constexpr SystemMenus DOCK_MENU_ID = NativeMenuEnums::DOCK_MENU_ID;

	struct Data
	{
		NativeMenuDriver* driver = nullptr;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized()
	{
		return data != nullptr && data->driver != nullptr;
	}
	_FORCE_INLINE_ static NativeMenuDriver* get_driver() { return data ? data->driver : nullptr; }
	template <typename T> _FORCE_INLINE_ static T* driver_as()
	{
		return data ? dynamic_cast<T*>(data->driver) : nullptr;
	}

	static bool has_feature(Feature p_feature);

	static bool has_system_menu(SystemMenus p_menu_id);
	static RID get_system_menu(SystemMenus p_menu_id);
	static String get_system_menu_name(SystemMenus p_menu_id);

	static String get_system_menu_text(SystemMenus p_menu_id);
	static void set_system_menu_text(SystemMenus p_menu_id, const String& p_name);

	static RID create_menu();
	static bool has_menu(const RID& p_rid);
	static void free_menu(const RID& p_rid);

	static Size2 get_size(const RID& p_rid);
	static void popup(const RID& p_rid, const Vector2i& p_position);

	static void set_interface_direction(const RID& p_rid, bool p_is_rtl);

	static void set_minimum_width(const RID& p_rid, float p_width);
	static float get_minimum_width(const RID& p_rid);

	static bool is_opened(const RID& p_rid);

	static int add_separator(const RID& p_rid, int p_index = -1);

	static int find_item_index_with_text(const RID& p_rid, const String& p_text);
	static int find_item_index_with_submenu(const RID& p_rid, const RID& p_submenu_rid);

	static bool is_item_checked(const RID& p_rid, int p_idx);
	static bool is_item_indeterminate(const RID& p_rid, int p_idx);
	static bool is_item_checkable(const RID& p_rid, int p_idx);
	static bool is_item_radio_checkable(const RID& p_rid, int p_idx);
	static String get_item_text(const RID& p_rid, int p_idx);
	static RID get_item_submenu(const RID& p_rid, int p_idx);
	static Key get_item_accelerator(const RID& p_rid, int p_idx);
	static bool is_item_disabled(const RID& p_rid, int p_idx);
	static bool is_item_hidden(const RID& p_rid, int p_idx);
	static String get_item_tooltip(const RID& p_rid, int p_idx);
	static int get_item_state(const RID& p_rid, int p_idx);
	static int get_item_max_states(const RID& p_rid, int p_idx);
	static Ref<Texture2D> get_item_icon(const RID& p_rid, int p_idx);
	static int get_item_indentation_level(const RID& p_rid, int p_idx);

	static void set_item_checked(const RID& p_rid, int p_idx, bool p_checked);
	static void set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate);
	static void set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable);
	static void set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable);
	static void set_item_text(const RID& p_rid, int p_idx, const String& p_text);
	static void set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid);
	static void set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode);
	static void set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled);
	static void set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden);
	static void set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip);
	static void set_item_state(const RID& p_rid, int p_idx, int p_state);
	static void set_item_max_states(const RID& p_rid, int p_idx, int p_max_states);
	static void set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon);
	static void set_item_indentation_level(const RID& p_rid, int p_idx, int p_level);
	static int set_item_index(const RID& p_rid, int p_idx, int p_target_idx);

	static int get_item_count(const RID& p_rid);
	static bool is_system_menu(const RID& p_rid);

	static void remove_item(const RID& p_rid, int p_idx);
	static void clear(const RID& p_rid);
};
