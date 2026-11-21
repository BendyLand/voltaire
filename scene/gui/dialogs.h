/**************************************************************************/
/*  dialogs.h                                                             */
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

#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
// --- Folded from scene/gui/panel.h ---
#include "scene/gui/control.h"

class Panel : public Control
{
	struct ThemeCache
	{
		Ref<StyleBox> panel_style;
	} theme_cache;

	bool fit_children = false;

protected:
	void _notification(int p_what);

public:
	void set_fit_children(bool p_fit) { fit_children = p_fit; }
	bool is_fitting_children() const { return fit_children; }

	virtual Size2 get_inner_combined_maximum_size() const override;

	Panel(bool p_fit_children = false);
};

// --- End scene/gui/panel.h ---
#include "scene/main/window.h"

class LineEdit;

struct DialogChrome
{
	Window* parent_visible = nullptr;

	Panel* bg_panel = nullptr;
	Label* message_label = nullptr;
	Control* buttons_hbox = nullptr;
	Button* ok_button = nullptr;
	Button* cancel = nullptr;

	bool has_cancel_button = false;
	bool popped_up = false;
	String ok_text;
	String default_ok_text = "OK";

	bool hide_on_ok = true;
	bool close_on_escape = true;

	struct ThemeCache
	{
		Ref<StyleBox> panel_style;
		int buttons_separation = 0;
		int buttons_min_width = 0;
		int buttons_min_height = 0;
	} theme_cache;

	inline static bool swap_cancel_ok = false;

	DialogChrome(bool p_with_cancel = false) : has_cancel_button(p_with_cancel) {}

	void init_controls(Window* p_owner);
	bool handle_escape_input(const InputEvent& p_event) const;
	bool should_cancel_on_parent_focus(const Window* p_owner) const;
	void post_popup() { popped_up = true; }
	void update_child_rects(Window* p_owner);
	void update_ok_text(Window* p_owner);
	void set_default_ok_text(Window* p_owner, const String& p_text);

	Label* get_label() const { return message_label; }
	Button* get_ok_button() const { return ok_button; }
	Button* get_cancel_button() const { return cancel; }

	void set_cancel_button_enabled(bool p_enabled) { has_cancel_button = p_enabled; }
	bool is_cancel_button_enabled() const { return has_cancel_button; }

	void set_cancel_button_text(const String& p_cancel_button_text);
	String get_cancel_button_text() const;

	void set_hide_on_ok(bool p_hide) { hide_on_ok = p_hide; }
	bool get_hide_on_ok() const { return hide_on_ok; }

	void set_close_on_escape(bool p_enable) { close_on_escape = p_enable; }
	bool get_close_on_escape() const { return close_on_escape; }

	void set_text(Window* p_owner, const String& p_text);
	String get_text() const;

	void set_autowrap(bool p_autowrap);
	bool has_autowrap() const;

	void set_ok_button_text(Window* p_owner, const String& p_ok_button_text);
	String get_ok_button_text() const { return ok_text; }
};

class Dialog : public Window
{
	DialogChrome chrome;

	void _parent_focused();
	void _text_submitted(const String& p_text);
	void _ok_pressed();
	void _cancel_pressed();
	void _update_child_rects() { chrome.update_child_rects(this); }

protected:
	virtual void _input_from_window(const InputEvent& p_event) override;
	virtual Size2 _get_contents_minimum_size() const override;
	virtual void _post_popup() override;

	void _notification(int p_what);

public:
	Label* get_label() { return chrome.get_label(); }

	Button* get_ok_button() { return chrome.get_ok_button(); }
	Button* get_cancel_button() { return chrome.get_cancel_button(); }

	void set_cancel_button_enabled(bool p_enabled) { chrome.set_cancel_button_enabled(p_enabled); }
	bool is_cancel_button_enabled() const { return chrome.is_cancel_button_enabled(); }

	void set_cancel_button_text(String p_cancel_button_text)
	{
		chrome.set_cancel_button_text(p_cancel_button_text);
	}
	String get_cancel_button_text() const { return chrome.get_cancel_button_text(); }

	static void set_swap_cancel_ok(bool p_swap);

	void set_hide_on_ok(bool p_hide) { chrome.set_hide_on_ok(p_hide); }
	bool get_hide_on_ok() const { return chrome.get_hide_on_ok(); }

	void set_close_on_escape(bool p_enable) { chrome.set_close_on_escape(p_enable); }
	bool get_close_on_escape() const { return chrome.get_close_on_escape(); }

	void set_text(String p_text) { chrome.set_text(this, p_text); }
	String get_text() const { return chrome.get_text(); }

	void set_autowrap(bool p_autowrap) { chrome.set_autowrap(p_autowrap); }
	bool has_autowrap() const { return chrome.has_autowrap(); }

	void set_ok_button_text(String p_ok_button_text)
	{
		chrome.set_ok_button_text(this, p_ok_button_text);
	}
	String get_ok_button_text() const { return chrome.get_ok_button_text(); }

	Dialog(bool p_with_cancel = false);
	~Dialog() = default;
};

// --- Folded from scene/gui/file_dialog.h ---
#include "core/io/dir_access.h"

#include "scene/property_list_helper.h"

class DirAccess;
class ItemList;
class LineEdit;
class MenuButton;
class OptionButton;
class PopupMenu;
class Separator;

class FileDialog : public Window
{
	inline static constexpr int MAX_RECENTS = 20;

	DialogChrome chrome{true};

	struct Option
	{
		String name;
		Vector<String> values;
		int default_idx = 0;
	};

	struct DirInfo
	{
		String name;
		uint64_t modified_time = 0;
		bool bundle = false;

		struct NameComparator
		{
			bool operator()(const DirInfo& p_a, const DirInfo& p_b) const
			{
				return FileNoCaseComparator()(p_a.name, p_b.name);
			}
		};

		struct TimeComparator
		{
			bool operator()(const DirInfo& p_a, const DirInfo& p_b) const
			{
				return p_a.modified_time > p_b.modified_time;
			}
		};
	};

	struct FileInfo
	{
		String name;
		String match_string;
		String type_sort;
		uint64_t modified_time = 0;

		struct NameComparator
		{
			bool operator()(const FileInfo& p_a, const FileInfo& p_b) const
			{
				return FileNoCaseComparator()(p_a.name, p_b.name);
			}
		};

		struct TypeComparator
		{
			bool operator()(const FileInfo& p_a, const FileInfo& p_b) const
			{
				return FileNoCaseComparator()(p_a.type_sort, p_b.type_sort);
			}
		};

		struct TimeComparator
		{
			bool operator()(const FileInfo& p_a, const FileInfo& p_b) const
			{
				return p_a.modified_time > p_b.modified_time;
			}
		};
	};

	enum class FileSortOption
	{
		NAME,
		NAME_REVERSE,
		TYPE,
		TYPE_REVERSE,
		MODIFIED_TIME,
		MODIFIED_TIME_REVERSE,
		MAX
	};

public:
	enum Access
	{
		ACCESS_RESOURCES,
		ACCESS_USERDATA,
		ACCESS_FILESYSTEM,
	};

	enum FileMode
	{
		FILE_MODE_OPEN_FILE,
		FILE_MODE_OPEN_FILES,
		FILE_MODE_OPEN_DIR,
		FILE_MODE_OPEN_ANY,
		FILE_MODE_SAVE_FILE,
	};

	enum DisplayMode
	{
		DISPLAY_THUMBNAILS,
		DISPLAY_LIST,
		DISPLAY_MAX
	};

	enum ItemMenu
	{
		ITEM_MENU_COPY_PATH,
		ITEM_MENU_DELETE,
		ITEM_MENU_REFRESH,
		ITEM_MENU_NEW_FOLDER,
		ITEM_MENU_SHOW_IN_EXPLORER,
		ITEM_MENU_SHOW_BUNDLE_CONTENT,
		// Not in the menu, only for shortcuts.
		ITEM_MENU_GO_UP,
		ITEM_MENU_TOGGLE_HIDDEN,
		ITEM_MENU_FIND,
		ITEM_MENU_FOCUS_PATH,
	};

	enum Customization
	{
		CUSTOMIZATION_HIDDEN_FILES,
		CUSTOMIZATION_CREATE_FOLDER,
		CUSTOMIZATION_FILE_FILTER,
		CUSTOMIZATION_FILE_SORT,
		CUSTOMIZATION_FAVORITES,
		CUSTOMIZATION_RECENT,
		CUSTOMIZATION_LAYOUT,
		CUSTOMIZATION_OVERWRITE_WARNING,
		CUSTOMIZATION_DELETE,
		CUSTOMIZATION_MAX
	};

	typedef void (*RegisterFunc)(FileDialog*);

	inline static RegisterFunc register_func = nullptr;
	inline static RegisterFunc unregister_func = nullptr;

private:
	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	inline static bool default_show_hidden_files = false;
	static inline DisplayMode default_display_mode = DISPLAY_THUMBNAILS;
	bool show_hidden_files = false;
	bool use_native_dialog = false;
	bool can_create_folders = true;
	bool customization_flags[CUSTOMIZATION_MAX]; // Initialized to true in the constructor.

	HashMap<ItemMenu, Ref<Shortcut>> action_shortcuts;

	inline static LocalVector<String> global_favorites;
	inline static LocalVector<String> global_recents;

	Access access = ACCESS_RESOURCES;
	FileMode mode = FILE_MODE_SAVE_FILE;
	DisplayMode display_mode = DISPLAY_THUMBNAILS;
	FileSortOption file_sort = FileSortOption::NAME;

	Vector<String> filters;
	Vector<String> processed_filters;

	Vector<Option> options;
	bool options_dirty = false;

	String file_name_filter;
	bool show_filename_filter = false;

	Vector<String> local_history;
	int local_history_pos = 0;

	bool mode_overrides_title = true;
	String root_subfolder;
	String root_prefix;
	String full_dir;

	bool is_invalidating = false;
	bool ensure_visible_after_invalidating = false;

	Control* main_vbox = nullptr;

	Button* dir_prev = nullptr;
	Button* dir_next = nullptr;
	Button* dir_up = nullptr;

	Control* drives_container = nullptr;
	MenuButton* drives = nullptr;
	int selected_drive = 0;
	LineEdit* directory_edit = nullptr;
	Control* shortcuts_container = nullptr;

	Button* refresh_button = nullptr;
	Button* favorite_button = nullptr;
	Control* make_dir_container = nullptr;
	Button* make_dir_button = nullptr;

	Button* show_hidden = nullptr;
	Separator* show_hidden_separator = nullptr;
	Control* layout_container = nullptr;
	Separator* layout_separator = nullptr;
	Button* thumbnail_mode_button = nullptr;
	Button* list_mode_button = nullptr;
	Button* show_filename_filter_button = nullptr;
	MenuButton* file_sort_button = nullptr;

	Control* favorite_vbox = nullptr;
	Button* fav_up_button = nullptr;
	Button* fav_down_button = nullptr;
	ItemList* favorite_list = nullptr;
	Control* recent_vbox = nullptr;
	ItemList* recent_list = nullptr;

	ItemList* file_list = nullptr;
	Label* message = nullptr;
	PopupMenu* item_menu = nullptr;

	Control* filename_filter_box = nullptr;
	LineEdit* filename_filter = nullptr;

	Control* file_box = nullptr;
	LineEdit* filename_edit = nullptr;
	OptionButton* filter = nullptr;

	Control* flow_checkbox_options = nullptr;
	Control* grid_select_options = nullptr;

	Dialog* delete_dialog = nullptr;
	Dialog* make_dir_dialog = nullptr;
	LineEdit* new_dir_name = nullptr;
	Dialog* mkdirerr = nullptr;
	Dialog* exterr = nullptr;
	Dialog* confirm_save = nullptr;

	struct ThemeCache
	{
		int thumbnail_size = 64;

		Ref<Texture2D> parent_folder;
		Ref<Texture2D> forward_folder;
		Ref<Texture2D> back_folder;
		Ref<Texture2D> reload;
		Ref<Texture2D> toggle_hidden;
		Ref<Texture2D> toggle_filename_filter;
		Ref<Texture2D> thumbnail_mode;
		Ref<Texture2D> list_mode;
		Ref<Texture2D> folder;
		Ref<Texture2D> file;
		Ref<Texture2D> create_folder;
		Ref<Texture2D> sort;
		Ref<Texture2D> favorite;
		Ref<Texture2D> favorite_up;
		Ref<Texture2D> favorite_down;
		Ref<Texture2D> file_thumbnail;
		Ref<Texture2D> folder_thumbnail;

		Ref<Texture2D> menu_copy_path;
		Ref<Texture2D> menu_delete;
		Ref<Texture2D> menu_refresh;
		Ref<Texture2D> menu_new_folder;
		Ref<Texture2D> menu_show_in_file_manager;
		Ref<Texture2D> menu_open_bundle;

		Color folder_icon_color;
		Color file_icon_color;
		Color file_disabled_color;

		Color icon_normal_color;
		Color icon_hover_color;
		Color icon_focus_color;
		Color icon_pressed_color;
	};

	ThemeCache theme_cache;

	void update_dir();
	void update_file_name();
	void update_file_list();
	void update_filename_filter();
	void update_filters();

	void _empty_clicked(const Vector2& p_pos, MouseButton p_button);
	void _item_clicked(int p_item, const Vector2& p_pos, MouseButton p_button);
	void _popup_menu(const Vector2& p_pos, int p_for_item);

	void _focus_file_text();

	int _get_selected_file_idx();
	String _get_item_path(int p_idx) const;
	void _file_list_multi_selected(int p_item, bool p_selected);
	void _file_list_selected(int p_item);
	void _file_list_item_activated(int p_item);

	void _select_drive(int p_idx);
	void _dir_submitted(String p_dir);
	void _action_pressed();
	void _save_confirm_pressed();
	void _cancel_pressed();
	void _filter_selected(int);
	void _filename_filter_changed();
	void _filename_filter_selected();
	void _file_list_select_first();
	void _delete_confirm();
	void _make_dir_confirm();
	void _go_up();
	void _go_back();
	void _go_forward();
	void _push_history();

	void _change_dir(const String& p_new_dir);
	void _update_drives(bool p_select = true);

	void _favorite_selected(int p_item);
	void _favorite_pressed();
	void _favorite_move_up();
	void _favorite_move_down();
	void _update_favorite_list();
	void _update_fav_buttons();

	void _recent_selected(int p_item);
	void _save_to_recent();
	void _update_recent_list();
	bool _path_matches_access(const String& p_path) const;

	void _invalidate();
	void _setup_button(Button* p_button, const Ref<Texture2D>& p_icon);

	void _native_popup();
	void _native_dialog_cb(bool p_ok, const Vector<String>& p_files, int p_filter);

	bool _is_open_should_be_disabled();
	void _thumbnail_callback(const Ref<Texture2D>& p_texture, const String& p_path);

	void _update_option_controls();
	void _option_changed_checkbox_toggled(bool p_pressed, const String& p_name);
	void _option_changed_item_selected(int p_idx, const String& p_name);

protected:
	Ref<DirAccess> dir_access;

	bool favorites_changed = false;
	bool recents_changed = false;

	bool _can_use_native_popup() const;
	virtual bool _should_use_native_popup() const;

	virtual void _item_menu_id_pressed(int p_option);

	virtual void _dir_contents_changed() {}

	virtual bool _should_hide_file(const String& p_file) const { return false; }

	virtual Color _get_folder_color(const String& p_path) const
	{
		return theme_cache.folder_icon_color;
	}

	Vector2i _get_list_mode_icon_size() const;

	virtual void _input_from_window(const InputEvent& p_event) override;
	virtual void _post_popup() override;
	virtual void _popup_base(const Rect2i& p_screen_rect = Rect2i()) override;
	void _clear_changed_status();

	void _notification(int p_what);

	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}

#ifndef DISABLE_DEPRECATED
	void _add_filter_bind_compat_111439(const String& p_filter, const String& p_description = "");

	static void _bind_compatibility_methods();
#endif

public:
	Label* get_label() { return chrome.get_label(); }
	Button* get_ok_button() { return chrome.get_ok_button(); }
	Button* get_cancel_button() { return chrome.get_cancel_button(); }

	void set_ok_button_text(String p_ok_button_text)
	{
		chrome.set_ok_button_text(this, p_ok_button_text);
	}
	String get_ok_button_text() const { return chrome.get_ok_button_text(); }

	void set_cancel_button_text(String p_cancel_button_text)
	{
		chrome.set_cancel_button_text(p_cancel_button_text);
	}
	String get_cancel_button_text() const { return chrome.get_cancel_button_text(); }

	void set_hide_on_ok(bool p_hide) { chrome.set_hide_on_ok(p_hide); }
	bool get_hide_on_ok() const { return chrome.get_hide_on_ok(); }

	void set_close_on_escape(bool p_enable) { chrome.set_close_on_escape(p_enable); }
	bool get_close_on_escape() const { return chrome.get_close_on_escape(); }

	void set_text(String p_text) { chrome.set_text(this, p_text); }
	String get_text() const { return chrome.get_text(); }

	void set_autowrap(bool p_autowrap) { chrome.set_autowrap(p_autowrap); }
	bool has_autowrap() const { return chrome.has_autowrap(); }

	virtual void set_visible(bool p_visible) override;

	void popup_file_dialog();
	void clear_filters();
	void add_filter(
		const String& p_filter, const String& p_description = "", const String& p_mime = "");
	void set_filters(const Vector<String>& p_filters);
	Vector<String> get_filters() const;
	void set_filename_filter(const String& p_filename_filter);
	String get_filename_filter() const;

	Vector<String> get_selected_files() const;

	String get_current_dir() const;
	String get_current_file() const;
	String get_current_path() const;
	void set_current_dir(const String& p_dir);
	void set_current_file(const String& p_file);
	void set_current_path(const String& p_path);

	String get_option_name(int p_option) const;
	Vector<String> get_option_values(int p_option) const;
	int get_option_default(int p_option) const;
	void set_option_name(int p_option, const String& p_name);
	void set_option_values(int p_option, const Vector<String>& p_values);
	void set_option_default(int p_option, int p_index);

	void add_option(const String& p_name, const Vector<String>& p_values, int p_index);

	void set_option_count(int p_count);
	int get_option_count() const;

	void set_root_subfolder(const String& p_root);
	String get_root_subfolder() const;

	void set_mode_overrides_title(bool p_override);
	bool is_mode_overriding_title() const;

	void set_use_native_dialog(bool p_native);
	bool get_use_native_dialog() const;

	void set_file_mode(FileMode p_mode);
	FileMode get_file_mode() const;

	DisplayMode get_display_mode() const;

	static void set_favorite_list(const PackedStringArray& p_favorites);
	static PackedStringArray get_favorite_list();

	static void set_recent_list(const PackedStringArray& p_recents);
	static PackedStringArray get_recent_list();

	bool is_customization_flag_enabled(Customization p_flag) const;

	Control* get_vbox() { return main_vbox; }

	LineEdit* get_line_edit() { return filename_edit; }

	ItemList* get_file_item_list() { return file_list; }

	void set_access(Access p_access);
	Access get_access() const;

	bool is_showing_hidden_files() const;
	void set_show_filename_filter(bool p_show);
	bool get_show_filename_filter() const;

	static void set_default_show_hidden_files(bool p_show);
	static void set_default_display_mode(DisplayMode p_mode);

	void invalidate();

	void deselect_all();

	FileDialog();
	~FileDialog();
};



// --- End scene/gui/file_dialog.h ---

// --- Folded from scene/gui/graph_edit.h ---
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "scene/gui/control.h"
// --- Folded from scene/gui/graph_node.h ---
#include "scene/gui/control.h"

class Label;

class GraphNode : public Control
{
	friend class GraphEdit;

public:
	enum Mode
	{
		MODE_ELEMENT,
		MODE_NODE,
		MODE_FRAME,
	};

private:
	struct Slot
	{
		bool enable_left = false;
		int type_left = 0;
		Color color_left = Color(1, 1, 1, 1);
		Ref<Texture2D> custom_port_icon_left;

		bool enable_right = false;
		int type_right = 0;
		Color color_right = Color(1, 1, 1, 1);
		Ref<Texture2D> custom_port_icon_right;

		bool draw_stylebox = true;
	};

	struct PortCache
	{
		Vector2 pos;
		int slot_index = 0;
		int type = 0;
		Color color;
	};

	struct _MinSizeCache
	{
		int min_size = 0;
		int max_size = -1;
		bool will_stretch = false;
		int final_size = 0;
	};

	enum CustomAccessibilityAction
	{
		ACTION_CONNECT_INPUT,
		ACTION_CONNECT_OUTPUT,
		ACTION_FOLLOW_INPUT,
		ACTION_FOLLOW_OUTPUT,
	};

	Mode mode = MODE_NODE;

	// Shared graph element state.
	bool selected = false;
	bool resizable = false;
	bool resizing = false;
	bool draggable = true;
	bool selectable = true;

	Vector2 drag_from;
	Vector2 resizing_from;
	Vector2 resizing_from_size;
	Vector2 position_offset;

	bool scaling_menus = false;

	// Shared titlebar state (used in MODE_NODE and MODE_FRAME).
	Control* titlebar_hbox = nullptr;
	Label* title_label = nullptr;
	String title;

	// Slot/port state (active in MODE_NODE).
	Vector<PortCache> left_port_cache;
	Vector<PortCache> right_port_cache;
	HashMap<int, Slot> slot_table;
	Vector<int> slot_y_cache;

	Control::FocusMode slots_focus_mode = Control::FOCUS_ACCESSIBILITY;
	int slot_count = 0;
	int selected_slot = -1;
	bool port_pos_dirty = true;
	bool ignore_invalid_connection_type = false;

	// Frame autoshrink/tint state (active in MODE_FRAME).
	bool autoshrink_enabled = true;
	int autoshrink_margin = 40;
	int drag_margin = 16;
	bool tint_color_enabled = false;
	Color tint_color = Color(0.3, 0.3, 0.3, 0.75);

	struct ThemeCache
	{
		Ref<StyleBox> panel;
		Ref<StyleBox> panel_selected;
		Ref<StyleBox> panel_focus;
		Ref<StyleBox> titlebar;
		Ref<StyleBox> titlebar_selected;
		Ref<StyleBox> slot;
		Ref<StyleBox> slot_selected;

		int separation = 0;
		int port_h_offset = 0;

		Ref<Texture2D> port;
		Ref<Texture2D> resizer;
		Color resizer_color;
	} theme_cache;

#ifdef TOOLS_ENABLED
	void _edit_set_position(const Point2& p_position) override;
#endif

	void _port_pos_update();
	Size2 _get_minimum_size(bool p_use_desired_sizes) const;

protected:
	void _notification(int p_what);
	void draw_port(int p_slot_index, Point2i p_pos, bool p_left, const Color& p_color);
	void _resort();

	virtual String _get_accessibility_name() const override;

public:
	Mode get_mode() const { return mode; }
	void set_mode(Mode p_mode);
	bool is_frame() const { return mode == MODE_FRAME; }
	bool is_node() const { return mode == MODE_NODE; }
	bool is_element_only() const { return mode == MODE_ELEMENT; }

	// GraphElement API.
	void set_position_offset(const Vector2& p_offset);
	Vector2 get_position_offset() const { return position_offset; }

	void set_selected(bool p_selected);
	bool is_selected() const { return selected; }

	void set_drag(bool p_drag);
	Vector2 get_drag_from() const { return drag_from; }

	void set_resizable(bool p_enable);
	bool is_resizable() const { return resizable; }

	void set_draggable(bool p_draggable) { draggable = p_draggable; }
	bool is_draggable() const { return draggable; }

	void set_selectable(bool p_selectable);
	bool is_selectable() const { return selectable; }

	void set_scaling_menus(bool p_scaling_menus) { scaling_menus = p_scaling_menus; }
	bool is_scaling_menus() const { return scaling_menus; }

	bool is_resizing() const { return resizing; }

	// Shared titlebar API.
	void set_title(const String& p_title);
	String get_title() const { return title; }

	Control* get_titlebar_hbox() { return titlebar_hbox; }
	Size2 get_titlebar_size() const;

	// MODE_FRAME API.
	void set_autoshrink_enabled(bool p_enable);
	bool is_autoshrink_enabled() const { return autoshrink_enabled; }

	void set_autoshrink_margin(const int& p_margin);
	int get_autoshrink_margin() const { return autoshrink_margin; }

	void set_drag_margin(int p_margin) { drag_margin = p_margin; }
	int get_drag_margin() const { return drag_margin; }

	void set_tint_color_enabled(bool p_enable);
	bool is_tint_color_enabled() const { return tint_color_enabled; }

	void set_tint_color(const Color& p_tint_color);
	Color get_tint_color() const { return tint_color; }

	// MODE_NODE slot/port API.
	virtual String get_accessibility_container_name(const Node* p_node) const override;

	void set_slot(int p_slot_index, bool p_enable_left, int p_type_left, const Color& p_color_left,
		bool p_enable_right, int p_type_right, const Color& p_color_right,
		const Ref<Texture2D>& p_custom_left = Ref<Texture2D>(),
		const Ref<Texture2D>& p_custom_right = Ref<Texture2D>(), bool p_draw_stylebox = true);
	void clear_slot(int p_slot_index);
	void clear_all_slots();

	bool is_slot_enabled_left(int p_slot_index) const;
	void set_slot_enabled_left(int p_slot_index, bool p_enable);

	void set_slot_type_left(int p_slot_index, int p_type);
	int get_slot_type_left(int p_slot_index) const;

	void set_slot_color_left(int p_slot_index, const Color& p_color);
	Color get_slot_color_left(int p_slot_index) const;

	void set_slot_custom_icon_left(int p_slot_index, const Ref<Texture2D>& p_custom_icon);
	Ref<Texture2D> get_slot_custom_icon_left(int p_slot_index) const;

	bool is_slot_enabled_right(int p_slot_index) const;
	void set_slot_enabled_right(int p_slot_index, bool p_enable);

	void set_slot_type_right(int p_slot_index, int p_type);
	int get_slot_type_right(int p_slot_index) const;

	void set_slot_color_right(int p_slot_index, const Color& p_color);
	Color get_slot_color_right(int p_slot_index) const;

	void set_slot_custom_icon_right(int p_slot_index, const Ref<Texture2D>& p_custom_icon);
	Ref<Texture2D> get_slot_custom_icon_right(int p_slot_index) const;

	bool is_slot_draw_stylebox(int p_slot_index) const;
	void set_slot_draw_stylebox(int p_slot_index, bool p_enable);

	void set_ignore_invalid_connection_type(bool p_ignore);
	bool is_ignoring_valid_connection_type() const;

	int get_input_port_count();
	Vector2 get_input_port_position(int p_port_idx);
	int get_input_port_type(int p_port_idx);
	Color get_input_port_color(int p_port_idx);
	int get_input_port_slot(int p_port_idx);

	int get_output_port_count();
	Vector2 get_output_port_position(int p_port_idx);
	int get_output_port_type(int p_port_idx);
	Color get_output_port_color(int p_port_idx);
	int get_output_port_slot(int p_port_idx);

	void set_slots_focus_mode(Control::FocusMode p_focus_mode);
	Control::FocusMode get_slots_focus_mode() const;

	virtual bool has_point(const Point2& p_point) const override;
	virtual Size2 get_minimum_size() const override;
	virtual Size2 get_desired_size() const override;

	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	virtual Vector<int> get_allowed_size_flags_horizontal() const override;
	virtual Vector<int> get_allowed_size_flags_vertical() const override;

	GraphNode(Mode p_mode = MODE_NODE);
};

// --- End scene/gui/graph_node.h ---
#include "scene/resources/shader.h"

class Button;
class GraphEdit;
class Label;
class Line2D;
class Panel;
class ScrollBar;
class Shortcut;
class SpinBox;

class ViewPanner final : public RefCounted
{
public:
	enum ControlScheme
	{
		SCROLL_ZOOMS,
		SCROLL_PANS,
	};

	enum PanAxis
	{
		PAN_AXIS_BOTH,
		PAN_AXIS_HORIZONTAL,
		PAN_AXIS_VERTICAL,
	};

	enum DragType
	{
		DRAG_TYPE_NONE,
		DRAG_TYPE_PAN,
		DRAG_TYPE_ZOOM,
	};

	enum ZoomStyle
	{
		ZOOM_VERTICAL,
		ZOOM_HORIZONTAL,
	};

private:
	int scroll_speed = 32;
	float scroll_zoom_factor = 1.1;
	PanAxis pan_axis = PAN_AXIS_BOTH;

	bool pan_key_pressed = false;
	bool force_drag = false;

	DragType drag_type = DragType::DRAG_TYPE_NONE;

	ZoomStyle zoom_style = ZoomStyle::ZOOM_VERTICAL;

	Vector2 drag_zoom_position;
	float drag_zoom_sensitivity_factor = -0.01f;

	bool enable_rmb = false;
	bool simple_panning_enabled = false;

	Ref<Shortcut> pan_view_shortcut;

	ControlScheme control_scheme = SCROLL_ZOOMS;
	Node* warped_panning_owner = nullptr;

public:
	void set_control_scheme(ControlScheme p_scheme);
	void set_enable_rmb(bool p_enable);
	void set_pan_shortcut(Ref<Shortcut> p_shortcut);
	void set_simple_panning_enabled(bool p_enabled);
	void set_scroll_speed(int p_scroll_speed);
	void set_scroll_zoom_factor(float p_scroll_zoom_factor);
	void set_pan_axis(PanAxis p_pan_axis);
	void set_zoom_style(ZoomStyle p_zoom_style);

	void setup(ControlScheme p_scheme, Ref<Shortcut> p_shortcut, bool p_simple_panning);
	void setup_warped_panning(Node* p_owner, bool p_allowed);

	bool is_panning() const;
	void set_force_drag(bool p_force);

	bool gui_input(const InputEvent& p_ev, Rect2 p_canvas_rect = Rect2());
	void release_pan_key();

	ViewPanner() = default;
};

class GraphEditArranger final : public RefCounted
{
	enum SET_OPERATIONS
	{
		IS_EQUAL,
		IS_SUBSET,
		DIFFERENCE,
		UNION,
	};

	GraphEdit* graph_edit = nullptr;
	bool arranging_graph = false;

	int _set_operations(
		SET_OPERATIONS p_operation, HashSet<String>& r_u, const HashSet<String>& r_v);
	HashMap<int, Vector<String>> _layering(const HashSet<String>& r_selected_nodes,
		const HashMap<String, HashSet<String>>& r_upper_neighbours);
	void _crossing_minimisation(HashMap<int, Vector<String>>& r_layers,
		const HashMap<String, HashSet<String>>& r_upper_neighbours);

public:
	void arrange_nodes();

	GraphEditArranger(GraphEdit* p_graph_edit) : graph_edit(p_graph_edit) {}
};

class GraphEditFilter : public Control
{
	friend class GraphEdit;
	friend class GraphEditMinimap;

	GraphEdit* ge = nullptr;

	virtual bool has_point(const Point2& p_point) const override;

public:
	GraphEditFilter(GraphEdit* p_edit);
};

class GraphEditMinimap : public Control
{
	friend class GraphEdit;
	friend class GraphEditFilter;

	GraphEdit* ge = nullptr;

	Vector2 minimap_padding;
	Vector2 minimap_offset;
	Vector2 graph_proportions = Vector2(1, 1);
	Vector2 graph_padding = Vector2(0, 0);
	Vector2 camera_position = Vector2(100, 50);
	Vector2 camera_size = Vector2(200, 200);

	bool is_pressing = false;
	bool is_resizing = false;

	struct ThemeCache
	{
		Ref<StyleBox> panel;
		Ref<StyleBox> node_style;
		Ref<StyleBox> camera_style;

		Ref<Texture2D> resizer;
		Color resizer_color;
	} theme_cache;

	Vector2 _get_render_size();
	Vector2 _get_graph_offset();
	Vector2 _get_graph_size();

	Vector2 _convert_from_graph_position(const Vector2& p_position);
	Vector2 _convert_to_graph_position(const Vector2& p_position);

	void _adjust_graph_scroll(const Vector2& p_offset);

protected:

public:
	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	void update_minimap();
	Rect2 get_camera_rect();

	GraphEditMinimap(GraphEdit* p_edit);
};

class GraphEdit : public Control
{
public:
	struct Connection : RefCounted
	{
		String from_node;
		String to_node;
		int from_port = 0;
		int to_port = 0;
		float activity = 0.0;
		bool keep_alive = true;

	private:
		struct Cache
		{
			bool dirty = true;
			Vector2 from_pos; // In graph space.
			Vector2 to_pos;	  // In graph space.
			Color from_color;
			Color to_color;
			Rect2 aabb;				// In local screen space.
			Line2D* line = nullptr; // In local screen space.
		} _cache;

		friend class GraphEdit;
	};

	// Should be in sync with ControlScheme in ViewPanner.
	enum PanningScheme
	{
		SCROLL_ZOOMS,
		SCROLL_PANS,
	};

	enum GridPattern
	{
		GRID_PATTERN_LINES,
		GRID_PATTERN_DOTS
	};

private:
	struct ConnectionType
	{
		union
		{
			uint64_t key = 0;

			struct
			{
				uint32_t type_a;
				uint32_t type_b;
			};
		};

		static uint32_t hash(const ConnectionType& p_conn) { return hash_one_uint64(p_conn.key); }

		bool operator==(const ConnectionType& p_type) const { return key == p_type.key; }

		ConnectionType(uint32_t a = 0, uint32_t b = 0)
		{
			type_a = a;
			type_b = b;
		}
	};

	Label* zoom_label = nullptr;
	Button* zoom_minus_button = nullptr;
	Button* zoom_reset_button = nullptr;
	Button* zoom_plus_button = nullptr;

	Button* toggle_snapping_button = nullptr;
	SpinBox* snapping_distance_spinbox = nullptr;
	Button* toggle_grid_button = nullptr;
	Button* minimap_button = nullptr;
	Button* arrange_button = nullptr;

	ScrollBar* h_scrollbar = nullptr;
	ScrollBar* v_scrollbar = nullptr;

	Ref<ViewPanner> panner;
	bool warped_panning = true;

	bool show_menu = true;
	bool show_zoom_label = false;
	bool show_grid_buttons = true;
	bool show_zoom_buttons = true;
	bool show_minimap_button = true;
	bool show_arrange_button = true;

	bool snapping_enabled = true;
	int snapping_distance = 20;
	bool show_grid = true;
	GridPattern grid_pattern = GRID_PATTERN_LINES;

	bool keyboard_connecting = false;
	bool connecting = false;
	String connecting_from_node;
	bool connecting_from_output = false;
	int connecting_type = 0;
	Color connecting_color;
	Vector2 connecting_to_point; // In local screen space.
	bool connecting_target_valid = false;
	String connecting_target_node;
	int connecting_from_port_index = 0;
	int connecting_target_port_index = 0;

	bool just_disconnected = false;
	bool connecting_valid = false;

	Vector2 click_pos;

	PanningScheme panning_scheme = SCROLL_ZOOMS;
	bool dragging = false;
	bool just_selected = false;
	bool moving_selection = false;
	Vector2 drag_accum;

	float zoom = 1.0;
	float zoom_step = 1.2;
	// Proper values set in constructor.
	float zoom_min = 0.0;
	float zoom_max = 0.0;

	Vector2 min_scroll_offset;
	Vector2 max_scroll_offset;
	Vector2 scroll_offset;

	bool box_selecting = false;
	bool box_selection_mode_additive = false;
	Point2 box_selecting_from;
	Point2 box_selecting_to;
	Rect2 box_selecting_rect;
	List<GraphNode*> prev_selected;

	bool setting_scroll_offset = false;
	bool right_disconnects = false;
	bool updating = false;
	bool awaiting_scroll_offset_update = false;

	Vector<Ref<Connection>> connections;
	HashMap<String, List<Ref<Connection>>> connection_map;
	Ref<Connection> hovered_connection;

	float lines_thickness = 4.0f;
	float lines_curvature = 0.5f;
	bool lines_antialiased = true;

	Panel* menu_panel = nullptr;
	Control* menu_hbox = nullptr;
	Control* connections_layer = nullptr;

	GraphEditFilter* top_connection_layer =
		nullptr; // Draws a dragged connection. Necessary since the connection line shader can't be
				 // applied to the whole top layer.
	Line2D* dragged_connection_line = nullptr;
	Control* top_layer = nullptr; // Used for drawing the box selection rect. Contains the minimap,
								  // menu panel and the scrollbars.

	GraphEditMinimap* minimap = nullptr;

	static Ref<Shader> default_connections_shader;
	Ref<Shader> connections_shader;

	Ref<GraphEditArranger> arranger;

	HashSet<ConnectionType, ConnectionType> valid_connection_types;
	HashSet<int> valid_left_disconnect_types;
	HashSet<int> valid_right_disconnect_types;

	struct ThemeCache
	{
		float base_scale = 1.0;

		Ref<StyleBox> panel;
		Ref<StyleBox> panel_focus;
		Color grid_major;
		Color grid_minor;

		Color activity_color;
		Color connection_hover_tint_color;
		int connection_hover_thickness;
		Color connection_valid_target_tint_color;
		Color connection_rim_color;

		Color selection_fill;
		Color selection_stroke;

		Ref<StyleBox> menu_panel;

		Ref<Texture2D> zoom_in;
		Ref<Texture2D> zoom_out;
		Ref<Texture2D> zoom_reset;

		Ref<Texture2D> snapping_toggle;
		Ref<Texture2D> grid_toggle;
		Ref<Texture2D> minimap_toggle;
		Ref<Texture2D> layout;

		float port_hotzone_inner_extent = 0.0;
		float port_hotzone_outer_extent = 0.0;
	} theme_cache;

	// This separates the children in two layers to ensure the order
	// of both background nodes (e.g frame nodes) and foreground nodes (connectable nodes).
	int background_nodes_separator_idx = 0;

	HashMap<String, HashSet<String>> frame_attached_nodes;
	HashMap<String, String> linked_parent_map;

	void _pan_callback(Vector2 p_scroll_vec, const InputEvent& p_event);
	void _zoom_callback(float p_zoom_factor, Vector2 p_origin, const InputEvent& p_event);

	void _zoom_minus();
	void _zoom_reset();
	void _zoom_plus();
	void _update_zoom_label();

	void _graph_element_selected(Node* p_node);
	void _graph_element_deselected(Node* p_node);
	void _graph_element_visibility_changed(GraphNode* p_graph_element);
	void _graph_element_resize_request(const Vector2& p_new_minsize, Node* p_node);
	void _graph_frame_autoshrink_changed(const Vector2& p_new_minsize, GraphNode* p_frame);
	void _graph_element_moved(Node* p_node);
	void _graph_node_slot_updated(int p_index, Node* p_node);
	void _graph_node_rect_changed(GraphNode* p_node);

	void _ensure_node_order_from_root(const String& p_node);
	void _ensure_node_order_from(Node* p_node);

	void _update_scrollbars();
	void _update_scroll_offset();
	void _scrollbar_moved(double);
	void _top_connection_layer_input(const InputEvent& p_ev);

	float _get_shader_line_width();
	void _draw_minimap_connection_line(const Vector2& p_from_graph_position,
		const Vector2& p_to_graph_position, const Color& p_from_color, const Color& p_to_color);
	void _invalidate_connection_line_cache();
	void _update_top_connection_layer();
	void _update_connections();

	void _top_layer_draw();
	void _minimap_draw();
	void _draw_grid();

	bool is_in_port_hotzone(
		const Vector2& p_pos, const Vector2& p_mouse_pos, const Vector2i& p_port_size, bool p_left);

	Rect2 _compute_shrinked_frame_rect(const GraphNode* p_frame);
	void _set_drag_frame_attached_nodes(GraphNode* p_frame, bool p_drag);
	void _set_position_of_frame_attached_nodes(GraphNode* p_frame, const Vector2& p_pos);

	friend class GraphEditFilter;
	bool _filter_input(const Point2& p_point);
	void _snapping_toggled();
	void _snapping_distance_changed(double);
	void _show_grid_toggled();

	friend class GraphEditMinimap;
	void _minimap_toggled();

	bool _check_clickable_control(
		Control* p_control, const Vector2& r_mouse_pos, const Vector2& p_offset);

#ifndef DISABLE_DEPRECATED
	bool _is_arrange_nodes_button_hidden_bind_compat_81582() const;
	PackedVector2Array _get_connection_line_bind_compat_86158(
		const Vector2& p_from, const Vector2& p_to);
	Error _connect_node_bind_compat_97449(
		const String& p_from, int p_from_port, const String& p_to, int p_to_port);
#endif

protected:
	virtual void _update_theme_item_cache() override;

	virtual void add_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

	void _notification(int p_what);
#ifndef DISABLE_DEPRECATED
	static void _bind_compatibility_methods();
#endif

	virtual bool is_in_input_hotzone(GraphNode* p_graph_node, int p_port_idx,
		const Vector2& p_mouse_pos, const Vector2i& p_port_size);
	virtual bool is_in_output_hotzone(GraphNode* p_graph_node, int p_port_idx,
		const Vector2& p_mouse_pos, const Vector2i& p_port_size);

public:
	static void init_shaders();
	static void finish_shaders();

	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	void key_input(const InputEvent& p_ev);

	// This method has to be public (for undo redo).
	// TODO: Find a better way to do this.
	void _update_graph_frame(GraphNode* p_frame);

	// Connection related methods.
	Error connect_node(const String& p_from, int p_from_port, const String& p_to,
		int p_to_port, bool keep_alive = false);
	bool is_node_connected(
		const String& p_from, int p_from_port, const String& p_to, int p_to_port);
	int get_connection_count(const String& p_node, int p_port);
	GraphNode* get_input_connection_target(const String& p_node, int p_port);
	GraphNode* get_output_connection_target(const String& p_node, int p_port);
	String get_connections_description(const String& p_node, int p_port);
	void disconnect_node(
		const String& p_from, int p_from_port, const String& p_to, int p_to_port);

	void force_connection_drag_end();
	const Vector<Ref<Connection>>& get_connections() const;
	void clear_connections();
	virtual PackedVector2Array get_connection_line(
		const Vector2& p_from, const Vector2& p_to) const;
	Ref<Connection> get_closest_connection_at_point(
		const Vector2& p_point, float p_max_distance = 4.0) const;
	List<Ref<Connection>> get_connections_intersecting_with_rect(const Rect2& p_rect) const;

	bool is_keyboard_connecting() const { return keyboard_connecting; }

	void start_keyboard_connecting(GraphNode* p_node, int p_in_port, int p_out_port);
	void end_keyboard_connecting(GraphNode* p_node, int p_in_port, int p_out_port);

	virtual bool is_node_hover_valid(
		const String& p_from, int p_from_port, const String& p_to, int p_to_port);

	void set_connection_activity(const String& p_from, int p_from_port, const String& p_to,
		int p_to_port, float p_activity);
	void reset_all_connection_activity();

	void add_valid_connection_type(int p_type, int p_with_type);
	void remove_valid_connection_type(int p_type, int p_with_type);
	bool is_valid_connection_type(int p_type, int p_with_type) const;

	// Frame related methods (GraphNode in MODE_FRAME).
	void attach_graph_element_to_frame(
		const String& p_graph_element, const String& p_parent_frame);
	void detach_graph_element_from_frame(const String& p_graph_element);
	GraphNode* get_element_frame(const String& p_attached_graph_element);
	Vector<String> get_attached_nodes_of_frame(const String& p_graph_frame) const;

	void set_panning_scheme(PanningScheme p_scheme);
	PanningScheme get_panning_scheme() const;

	void set_zoom(float p_zoom);
	void set_zoom_custom(float p_zoom, const Vector2& p_center);
	float get_zoom() const;

	void set_zoom_min(float p_zoom_min);
	float get_zoom_min() const;

	void set_zoom_max(float p_zoom_max);
	float get_zoom_max() const;

	void set_zoom_step(float p_zoom_step);
	float get_zoom_step() const;

	void set_minimap_size(Vector2 p_size);
	Vector2 get_minimap_size() const;
	void set_minimap_opacity(float p_opacity);
	float get_minimap_opacity() const;

	void set_minimap_enabled(bool p_enable);
	bool is_minimap_enabled() const;

	bool is_showing_menu() const;
	bool is_showing_zoom_label() const;
	bool is_showing_grid_buttons() const;
	bool is_showing_zoom_buttons() const;
	bool is_showing_minimap_button() const;
	bool is_showing_arrange_button() const;

	Control* get_top_layer() const { return top_layer; }

	GraphEditMinimap* get_minimap() const { return minimap; }

	void override_connections_shader(const Ref<Shader>& p_shader);

	void set_right_disconnects(bool p_enable);
	bool is_right_disconnects_enabled() const;

	void add_valid_right_disconnect_type(int p_type);
	void remove_valid_right_disconnect_type(int p_type);

	void add_valid_left_disconnect_type(int p_type);
	void remove_valid_left_disconnect_type(int p_type);

	void set_scroll_offset(const Vector2& p_ofs);
	Vector2 get_scroll_offset() const;

	void set_selected(Node* p_child);

	void set_snapping_enabled(bool p_enable);
	bool is_snapping_enabled() const;

	void set_snapping_distance(int p_snapping_distance);
	int get_snapping_distance() const;

	void set_show_grid(bool p_enable);
	bool is_showing_grid() const;

	void set_grid_pattern(GridPattern p_pattern);
	GridPattern get_grid_pattern() const;

	void set_connection_lines_curvature(float p_curvature);
	float get_connection_lines_curvature() const;

	void set_connection_lines_thickness(float p_thickness);
	float get_connection_lines_thickness() const;

	void set_connection_lines_antialiased(bool p_antialiased);
	bool is_connection_lines_antialiased() const;

	Control* get_menu_hbox();
	Ref<ViewPanner> get_panner();
	void set_warped_panning(bool p_warped);
	void update_warped_panning();

	void arrange_nodes();

	GraphEdit();
};



// --- End scene/gui/graph_edit.h ---

// --- Folded from scene/gui/color_picker.h ---
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/popup.h"
#include "scene/resources/shader.h"

class ColorMode;
class ColorPickerShape;
class FileDialog;
class Label;
class LineEdit;
class MenuButton;
class OptionButton;
class PopupMenu;
class Slider;
class SpinBox;
class TextureRect;

class ColorPicker : public Control
{
	friend class ColorPickerShape;
	friend class ColorMode;

public:
	enum ColorModeType
	{
		MODE_RGB,
		MODE_HSV,
#ifndef DISABLE_DEPRECATED
		MODE_RAW = 2,
#endif
		MODE_LINEAR = 2,
		MODE_OKHSL,

		MODE_MAX
	};

	enum PickerShapeType
	{
		SHAPE_HSV_RECTANGLE,
		SHAPE_HSV_WHEEL,
		SHAPE_VHS_CIRCLE,
		SHAPE_OKHSL_CIRCLE,
		SHAPE_NONE,
		SHAPE_OK_HS_RECTANGLE,
		SHAPE_OK_HL_RECTANGLE,

		SHAPE_MAX
	};

private:
	inline int get_current_shape_index() { return shape_to_index(current_shape); }

	static inline int shape_to_index(PickerShapeType p_shape)
	{
		if (p_shape == SHAPE_NONE) {
			return -1;
		}
		if (p_shape > SHAPE_NONE) {
			return p_shape - 1;
		}
		return p_shape;
	}

	static inline PickerShapeType index_to_shape(int p_index)
	{
		if (p_index == -1) {
			return SHAPE_NONE;
		}
		if (p_index >= SHAPE_NONE) {
			return (PickerShapeType)(p_index + 1);
		}
		return (PickerShapeType)p_index;
	}

public:
	static const int MODE_SLIDER_COUNT = 3;

	enum SLIDER_EXTRA
	{
		SLIDER_INTENSITY = MODE_SLIDER_COUNT,
		SLIDER_ALPHA,

		SLIDER_MAX
	};

	enum class MenuOption
	{
		MENU_SAVE,
		MENU_SAVE_AS,
		MENU_LOAD,
		MENU_QUICKLOAD,
		MENU_CLEAR,
	};

private:
	static inline List<Color> preset_cache;
	static inline List<Color> recent_preset_cache;

	int current_slider_count = MODE_SLIDER_COUNT;

	const float DEFAULT_GAMEPAD_EVENT_DELAY_MS = 1.0 / 2;
	const float GAMEPAD_EVENT_REPEAT_RATE_MS = 1.0 / 30;
	float gamepad_event_delay_ms = DEFAULT_GAMEPAD_EVENT_DELAY_MS;

	static constexpr int MODE_BUTTON_COUNT = 3;

	bool slider_theme_modified = true;

	LocalVector<ColorMode*> modes;
	LocalVector<ColorPickerShape*> shapes;

	Popup* picker_window = nullptr;
	TextureRect* picker_texture_zoom = nullptr;
	Panel* picker_preview = nullptr;
	Panel* picker_preview_color = nullptr;
	Ref<StyleBox> picker_preview_style_box;
	Ref<StyleBox> picker_preview_style_box_color;

	TextureRect* picker_texture_rect = nullptr;
	Color picker_color;
	FileDialog* file_dialog = nullptr;
	MenuButton* menu_btn = nullptr;
	PopupMenu* options_menu = nullptr;

	Control* internal_margin = nullptr;
	Control* shape_container = nullptr;
	TextureRect* sample = nullptr;
	Control* swatches_vbc = nullptr;
	Control* preset_container = nullptr;
	Control* recent_preset_hbc = nullptr;
	Button* btn_add_preset = nullptr;
	Button* btn_pick = nullptr;
	Label* palette_name = nullptr;
	String palette_path;
	bool presets_just_loaded = false;
	Button* btn_preset = nullptr;
	Button* btn_recent_preset = nullptr;
	PopupMenu* shape_popup = nullptr;
	PopupMenu* mode_popup = nullptr;
	MenuButton* btn_shape = nullptr;
	Control* mode_hbc = nullptr;
	Control* sample_hbc = nullptr;
	Control* slider_gc = nullptr;
	Control* hex_hbc = nullptr;
	Label* hex_label = nullptr;
	MenuButton* btn_mode = nullptr;
	Button* mode_btns[MODE_BUTTON_COUNT];
	Ref<ButtonGroup> mode_group;
	Button* selected_recent_preset = nullptr;
	Ref<ButtonGroup> preset_group;
	Ref<ButtonGroup> recent_preset_group;

	Control* perm_hb = nullptr;
	void _req_permission();

	OptionButton* mode_option_button = nullptr;

	Slider* sliders[SLIDER_MAX];
	SpinBox* values[SLIDER_MAX];
	Label* labels[SLIDER_MAX];

	Button* text_type = nullptr;
	LineEdit* c_text = nullptr;
	Button* text_copy = nullptr;

	Slider* alpha_slider = nullptr;
	SpinBox* alpha_value = nullptr;
	Label* alpha_label = nullptr;

	bool edit_alpha = true;

	Slider* intensity_slider = nullptr;
	SpinBox* intensity_value = nullptr;
	Label* intensity_label = nullptr;

	bool edit_intensity = true;

	Size2i ms;
	bool text_is_constructor = false;
	PickerShapeType current_shape = SHAPE_HSV_RECTANGLE;
	ColorModeType current_mode = MODE_RGB;
	bool colorize_sliders = true;

	const int PRESET_COLUMN_COUNT = 9;
	int prev_preset_size = 0;
	int prev_rencet_preset_size = 0;
	List<Color> presets;
	List<Color> recent_presets;

	Color color;
	Color color_normalized;
	Color old_color;
	Color pre_picking_color;
	bool is_picking_color = false;

	bool display_old_color = false;
	bool deferred_mode_enabled = false;
	bool updating = true;
	bool changing_color = false;
	bool spinning = false;
	bool can_add_swatches = true;
	bool presets_visible = true;
	bool color_modes_visible = true;
	bool sampler_visible = true;
	bool sliders_visible = true;
	bool hex_visible = true;
	bool line_edit_mouse_release = false;
	bool text_changed = false;
	bool currently_dragging = false;

	float h = 0.0;
	float s = 0.0;
	float v = 0.0;

	float ok_hsl_h = 0.0;
	float ok_hsl_s = 0.0;
	float ok_hsl_l = 0.0;

	bool hsv_cached = false;
	bool okhsl_cached = false;

	float intensity = 0.0;

	struct ThemeCache
	{
		float base_scale = 1.0;

		int content_margin = 0;
		int label_width = 0;

		int sv_height = 0;
		int sv_width = 0;
		int h_width = 0;

		bool center_slider_grabbers = true;

		Ref<StyleBox> picker_focus_rectangle;
		Ref<StyleBox> picker_focus_circle;
		Color focused_not_editing_cursor_color;
		Ref<Texture2D> menu_option;
		Ref<Texture2D> screen_picker;
		Ref<Texture2D> expanded_arrow;
		Ref<Texture2D> folded_arrow;
		Ref<Texture2D> add_preset;

		Ref<Texture2D> shape_rect;
		Ref<Texture2D> shape_rect_wheel;
		Ref<Texture2D> shape_circle;

		Ref<Texture2D> bar_arrow;
		Ref<Texture2D> sample_bg;
		Ref<Texture2D> sample_revert;
		Ref<StyleBox> sample_focus;
		Ref<Texture2D> overbright_indicator;
		Ref<Texture2D> picker_cursor;
		Ref<Texture2D> picker_cursor_bg;
		Ref<Texture2D> color_hue;

		Ref<Texture2D> color_script;
		Ref<Texture2D> color_copy;

		Ref<StyleBox> mode_button_normal;
		Ref<StyleBox> mode_button_pressed;
		Ref<StyleBox> mode_button_hover;
		Ref<StyleBox> mode_button_hover_pressed;
	};

	ThemeCache theme_cache;

	void _copy_normalized_to_hsv_okhsl();
	void _copy_hsv_okhsl_to_normalized();

	Color _color_apply_intensity(const Color& col) const;
	void _normalized_apply_intensity_to_color();
	void _copy_color_to_normalized_and_intensity();

	void _reset_sliders_theme();
	void _slider_drag_started();
	void _text_copy_pressed();
	void _sample_draw();
	void _slider_draw(int p_which);
	void _alpha_slider_draw();

	void _line_edit_input(const InputEvent& p_event);
	void _text_changed(const String& p_new_text);
	void _block_input_on_popup_show();
	void _enable_input_on_popup_hide();

	inline int _get_preset_size();
	void _load_palette();

protected:
	virtual void _update_theme_item_cache() override;

public:
	Control* get_slider_container();
	Slider* get_slider(int idx);
	Vector<float> get_active_slider_values();

	void add_mode(ColorMode* p_mode);
	void add_shape(ColorPickerShape* p_shape);

	bool is_editing_alpha() const;

	bool is_editing_intensity() const;

	Color get_pick_color() const;
	void set_old_color(const Color& p_color);

	Color get_old_color() const;

	void set_display_old_color(bool p_enabled);
	bool is_displaying_old_color() const;

	PickerShapeType get_picker_shape() const;

	PackedColorArray get_presets() const;
	PackedColorArray get_recent_presets() const;

	ColorModeType get_color_mode() const;

	void set_colorize_sliders(bool p_colorize_sliders);
	bool is_colorizing_sliders() const;

	void set_deferred_mode(bool p_enabled);
	bool is_deferred_mode() const;

	bool are_swatches_enabled() const;

	bool are_presets_visible() const;

	bool are_modes_visible() const;

	bool is_sampler_visible() const;

	bool are_sliders_visible() const;

	bool is_hex_visible() const;

	void set_focus_on_picker_shape();

	ColorPicker();
	~ColorPicker();
};

class ColorPickerButton : public Control
{
	Button* button = nullptr;
	Popup* popup = nullptr;
	ColorPicker* picker = nullptr;
	Color color;
	bool edit_alpha = true;
	bool edit_intensity = true;
	bool popup_was_open = false;

	struct ThemeCache
	{
		Ref<StyleBox> normal_style;
		Ref<Texture2D> background_icon;
		Ref<Texture2D> overbright_indicator;
	};

	ThemeCache theme_cache;

	void _update_picker();

protected:
	void _notification(int);

public:
	Button* get_button() const { return button; }
	Popup* get_popup();
	ColorPicker* get_picker();

	Color get_pick_color() const;

	bool is_editing_alpha() const;

	bool is_editing_intensity() const;

	virtual Size2 get_minimum_size() const override
	{
		return button ? button->get_minimum_size() : Size2();
	}

	ColorPickerButton(const String& p_text = String());
};

// --- Folded from scene/gui/color_rect.h ---
#include "scene/gui/control.h"

class ColorRect : public Control
{
	Color color = Color(1, 1, 1);

protected:
	void _notification(int p_what);

public:
	static void _color_rect_draw_static(CanvasItem* p_item)
	{
		ColorRect* rect = static_cast<ColorRect*>(p_item);
		rect->_notification(CanvasItem::NOTIFICATION_DRAW);
	}

	ColorRect() { draw_callback = &_color_rect_draw_static; }

	void notification(int p_what);
	void set_color(const Color& p_color);
	Color get_color() const;
	void queue_redraw();
};



// --- End scene/gui/color_rect.h ---

// --- Folded from scene/resources/color_palette.h ---
#include "core/io/resource.h"

class ColorPalette : public Resource
{
private:
	PackedColorArray colors;

protected:

public:
	void set_colors(const PackedColorArray& p_colors);
	PackedColorArray get_colors() const;
};



// --- End scene/resources/color_palette.h ---

// --- End scene/gui/color_picker.h ---

// --- Folded from scene/gui/rich_text_label.h ---
#include "core/templates/rid_owner.h"
#include "scene/gui/control.h"
#include "scene/resources/text_paragraph.h"
#include "scene/resources/text_line.h"

#ifdef TOOLS_ENABLED
#include "editor/themes/editor_scale.h"
#endif

class PopupMenu;
class ScrollBar;
class Texture2D;
class Timer;

class CharFXTransform;
class RichTextEffect;

class RichTextLabel : public Control
{
	enum RTLDrawStep
	{
		DRAW_STEP_BACKGROUND,
		DRAW_STEP_SHADOW_OUTLINE,
		DRAW_STEP_SHADOW,
		DRAW_STEP_OUTLINE,
		DRAW_STEP_TEXT,
		DRAW_STEP_FOREGROUND,
		DRAW_STEP_MAX,
	};

public:
	enum ListType
	{
		LIST_NUMBERS,
		LIST_LETTERS,
		LIST_ROMAN,
		LIST_DOTS
	};

	enum MetaUnderline
	{
		META_UNDERLINE_NEVER,
		META_UNDERLINE_ALWAYS,
		META_UNDERLINE_ON_HOVER,
	};

	enum ItemType
	{
		ITEM_FRAME,
		ITEM_TEXT,
		ITEM_IMAGE,
		ITEM_NEWLINE,
		ITEM_FONT,
		ITEM_FONT_SIZE,
		ITEM_FONT_FEATURES,
		ITEM_COLOR,
		ITEM_OUTLINE_SIZE,
		ITEM_OUTLINE_COLOR,
		ITEM_UNDERLINE,
		ITEM_STRIKETHROUGH,
		ITEM_PARAGRAPH,
		ITEM_INDENT,
		ITEM_LIST,
		ITEM_TABLE,
		ITEM_FADE,
		ITEM_SHAKE,
		ITEM_WAVE,
		ITEM_TORNADO,
		ITEM_RAINBOW,
		ITEM_PULSE,
		ITEM_BGCOLOR,
		ITEM_FGCOLOR,
		ITEM_META,
		ITEM_HINT,
		ITEM_DROPCAP,
		ITEM_CUSTOMFX,
		ITEM_CONTEXT,
		ITEM_LANGUAGE,
	};

	enum MenuItems
	{
		MENU_COPY,
		MENU_SELECT_ALL,
		MENU_MAX
	};

	enum DefaultFont
	{
		RTL_NORMAL_FONT,
		RTL_BOLD_FONT,
		RTL_ITALICS_FONT,
		RTL_BOLD_ITALICS_FONT,
		RTL_MONO_FONT,
		RTL_CUSTOM_FONT,
	};

	enum ImageUpdateMask
	{
		UPDATE_TEXTURE = 1 << 0,
		UPDATE_SIZE = 1 << 1,
		UPDATE_COLOR = 1 << 2,
		UPDATE_ALIGNMENT = 1 << 3,
		UPDATE_REGION = 1 << 4,
		UPDATE_PAD = 1 << 5,
		UPDATE_TOOLTIP = 1 << 6,
		UPDATE_WIDTH_UNIT = 1 << 7,
	};

	enum ImageUnit
	{
		IMAGE_UNIT_PIXEL,
		IMAGE_UNIT_PERCENT,
		IMAGE_UNIT_EM,
	};

protected:
	virtual void _update_theme_item_cache() override;

	void _notification(int p_what);

#ifndef DISABLE_DEPRECATED
	void _push_font_bind_compat_79053(const Ref<Font>& p_font, int p_size);
#endif

private:
	struct Item;

	struct Line
	{						  // Line is a paragraph.
		Item* from = nullptr; // `from` is main if this Line is the first Line in the doc, otherwise
							  // `from` is the previous Item in the doc of any type.

		Ref<TextLine> text_prefix;
		Color prefix_color = Color(0, 0, 0, 0);
		int prefix_outline_size = -1;
		Color prefix_outline_color = Color(0, 0, 0, 0);
		float prefix_width = 0;
		Ref<TextParagraph> text_buf;
		Ref<TextParagraph> text_buf_disp;

		RID accessibility_line_element;
		RID accessibility_text_element;

		Item* dc_item = nullptr;
		Color dc_color;
		int dc_ol_size = 0;
		Color dc_ol_color;

		Vector2 offset;
		float indent = 0.0;
		int char_offset = 0;
		int char_count = 0;

		Line() { text_buf.instantiate(); }

		~Line();

		_FORCE_INLINE_ float get_height(float p_line_separation, float p_paragraph_separation) const
		{
			return offset.y + text_buf->get_size().y +
				   text_buf->get_line_count() * p_line_separation + p_paragraph_separation;
		}
	};

	struct FrameData
	{
		bool cell = false;

		LocalVector<Line> lines;
		std::atomic<int> first_invalid_line;
		std::atomic<int> first_invalid_font_line;
		std::atomic<int> first_resized_line;

		Item* parent_frame = nullptr;

		Color odd_row_bg = Color(0, 0, 0, 0);
		Color even_row_bg = Color(0, 0, 0, 0);
		Color border = Color(0, 0, 0, 0);
		Size2 min_size_over = Size2(-1, -1);
		Size2 max_size_over = Size2(-1, -1);
		Rect2 padding;

		FrameData()
		{
			first_invalid_line.store(0);
			first_invalid_font_line.store(0);
			first_resized_line.store(0);
		}
	};

	struct DropcapData
	{
		String text;
		Ref<Font> font;
		int font_size = 0;
		Color color;
		int ol_size = 0;
		Color ol_color;
		Rect2 dropcap_margins;
	};

	struct ImageData
	{
		Ref<Texture2D> image;
		String alt_text;
		InlineAlignment inline_align = INLINE_ALIGNMENT_CENTER;
		bool pad = false;
		ImageUnit width_unit = IMAGE_UNIT_PIXEL;
		ImageUnit height_unit = IMAGE_UNIT_PIXEL;
		Rect2 region;
		Size2 size;
		Size2 rq_size;
		Color color;
		String tooltip;

		~ImageData();
	};

	struct ParagraphData
	{
		HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;
		String language;
		Control::TextDirection direction = Control::TEXT_DIRECTION_AUTO;
		TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
		uint32_t jst_flags =
			TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA |
			TextServer::JUSTIFICATION_SKIP_LAST_LINE |
			TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
		PackedFloat32Array tab_stops;
	};

	struct TableData
	{
		struct Column
		{
			String name;
			bool expand = false;
			bool shrink = true;
			int expand_ratio = 0;
			int min_width = 0;
			int max_width = 0;
			int width = 0;
		};

		LocalVector<Column> columns;
		LocalVector<float> rows;
		LocalVector<float> rows_baseline;
		String name;

		int align_to_row = -1;
		int total_width = 0;
		int total_height = 0;
		int char_count = 0;
		InlineAlignment inline_align = INLINE_ALIGNMENT_TOP;
	};

	struct FXData
	{
		double elapsed_time = 0.0;
		bool connected = true;

		int strength = 0;
		float rate = 0.0f;
		uint64_t _current_rng = 0;
		uint64_t _previous_rng = 0;
		Vector2 prev_off;

		float frequency = 1.0f;
		float amplitude = 1.0f;
		float radius = 1.0f;
		float saturation = 0.9f;
		float value = 1.0f;
		float speed = 1.0f;
		Color color = Color(1.0, 1.0, 1.0, 0.25);
		float ease = -2.0f;

		Ref<CharFXTransform> char_fx_transform;
		Ref<RichTextEffect> custom_effect;

		explicit FXData(ItemType p_type);
		~FXData();

		void reroll_random()
		{
			_previous_rng = _current_rng;
			_current_rng = Math::rand();
		}

		uint64_t offset_random(int p_index)
		{
			return (_current_rng >> (p_index % 64)) | (_current_rng << (64 - (p_index % 64)));
		}

		uint64_t offset_previous_random(int p_index)
		{
			return (_previous_rng >> (p_index % 64)) | (_previous_rng << (64 - (p_index % 64)));
		}
	};

	struct Item
	{
		int index = 0;
		int char_ofs = 0;
		Item* parent = nullptr; // "parent" means "enclosing item tag", if any. It is an interval
								// predecessor, not a hierarchical parent.
		ItemType type = ITEM_FRAME;
		int line = 0; // `line` is the index number of the paragraph (Line) this item is inside of
					  // (zero if the first paragraph).
		List<Item*> subitems;
		List<Item*>::Element* E = nullptr;
		RID rid;
		RID accessibility_item_element;

		union
		{
			void* extra = nullptr;
			FrameData* frame;
			DropcapData* dropcap;
			ImageData* image;
			ParagraphData* paragraph;
			TableData* table;
			FXData* fx;
		};

		// Lightweight inline fields for frequent leaf/style tags:
		// `text` holds text (ITEM_TEXT), tooltip (ITEM_META), description (ITEM_HINT),
		// language (ITEM_LANGUAGE), or bullet (ITEM_LIST).
		String text;
		Ref<Font> font;
		Color color;

		int font_size = 0;
		int outline_size = 0;
		int level = 0;
		float max_width = 0.0f;
		int starting_index = 0;
		int fade_length = 0;

		DefaultFont def_font = RTL_CUSTOM_FONT;
		MetaUnderline meta_underline = META_UNDERLINE_ALWAYS;
		ListType list_type = LIST_DOTS;
		bool variation = false;
		bool def_size = false;
		bool capitalize = false;

		_FORCE_INLINE_ bool is_fx() const
		{
			return type == ITEM_CUSTOMFX || type == ITEM_SHAKE || type == ITEM_WAVE ||
				   type == ITEM_TORNADO || type == ITEM_RAINBOW || type == ITEM_PULSE;
		}

		void _clear_children();

		explicit Item(ItemType p_type = ITEM_FRAME);
		~Item();

		Item(const Item&) = delete;
		Item& operator=(const Item&) = delete;
	};

	Item* main = nullptr;
	Item* current = nullptr;
	Item* current_frame = nullptr;

	Mutex data_mutex;
	bool threaded = false;
	std::atomic<bool> stop_thread;
	std::atomic<bool> updating;
	std::atomic<bool> validating;
	std::atomic<double> loaded;
	std::atomic<bool> parsing_bbcode;

	uint64_t loading_started = 0;
	int progress_delay = 1000;

	ScrollBar* vscroll = nullptr;

	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_WORD_SMART;
	uint32_t autowrap_flags_trim =
		TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;

	bool scroll_visible = false;
	bool scroll_follow = false;
	bool scroll_follow_visible_characters = false;
	int follow_vc_pos = 0;
	bool scroll_following = false;
	bool scroll_active = true;
	int scroll_w = 0;
	bool scroll_updated = false;
	bool updating_scroll = false;
	int current_idx = 1;
	int current_char_ofs = 0;
	int visible_paragraph_count = 0;
	int visible_line_count = 0;
	Rect2i visible_rect;

	int tab_size = 4;
	bool underline_meta = true;
	bool underline_hint = true;
	bool use_selected_font_color = false;

	HorizontalAlignment default_alignment = HORIZONTAL_ALIGNMENT_LEFT;
	VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_TOP;
	uint32_t default_jst_flags =
		TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA |
		TextServer::JUSTIFICATION_SKIP_LAST_LINE |
		TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	PackedFloat32Array default_tab_stops;

	Item* meta_hovering = nullptr;

	HashMap<RID, Rect2> ac_element_bounds_cache;

	void _update_follow_vc();
	void _invalidate_accessibility();
	void _invalidate_current_line(Item* p_frame);

	void _prepare_scroll_anchor();

	void _thread_function(void* p_userdata);
	void _thread_end();
	bool _validate_line_caches();
	void _process_line_caches();

	void _add_item(Item* p_item, bool p_enter = false, bool p_ensure_newline = false);
	void _remove_frame(HashSet<Item*>& r_erase_list, Item* p_frame, int p_line, bool p_erase,
		int p_char_offset, int p_line_offset);

	void _texture_changed(RID p_item);

	static inline RID_PtrOwner<Item, true> items{65536, 1048576};
	List<String> tag_stack;
	HashSet<RID> hr_list;

	String language;
	TextDirection text_direction = TEXT_DIRECTION_AUTO;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
	struct Selection
	{
		Item* click_frame = nullptr;
		int click_line = 0;
		Item* click_item = nullptr;
		int click_char = 0;

		Item* from_frame = nullptr;
		int from_line = 0;
		Item* from_item = nullptr;
		int from_char = 0;
		mutable bool from_line_found = false;

		Item* to_frame = nullptr;
		int to_line = 0;
		Item* to_item = nullptr;
		int to_char = 0;
		mutable bool to_line_found = false;

		enum SelectionMode
		{
			SINGLE_CLICK,
			DOUBLE_CLICK,
			TRIPLE_CLICK,
		};

		SelectionMode selection_mode = SINGLE_CLICK;
		bool active = false;  // anything selected? i.e. from, to, etc. valid?
		bool enabled = false; // allow selections?
		bool drag_attempt = false;
	};

	Selection selection;
	uint64_t last_double_click = 0;
	Vector2 last_double_click_pos;
	bool deselect_on_focus_loss_enabled = true;
	bool drag_and_drop_selection_enabled = true;

	Item* keyboard_focus_frame = nullptr;
	int keyboard_focus_line = 0;
	Item* keyboard_focus_item = nullptr;
	bool keyboard_focus_on_text = true;

	bool context_menu_enabled = false;
	bool shortcut_keys_enabled = true;

	// Context menu.
	PopupMenu* menu = nullptr;
	void _generate_context_menu();
	Key _get_menu_action_accelerator(const String& p_action);

	int visible_characters = -1;
	float visible_ratio = 1.0;
	TextServer::VisibleCharactersBehavior visible_chars_behavior =
		TextServer::VC_CHARS_BEFORE_SHAPING;

	bool _is_click_inside_selection() const;
	void _find_click(Item* p_frame, const Point2i& p_click,
		Item** r_click_frame = nullptr, int* r_click_line = nullptr,
		Item** r_click_item = nullptr, int* r_click_char = nullptr, bool* r_outside = nullptr,
		bool p_meta = false);

	String _get_line_text(Item* p_frame, int p_line) const;
	bool _search_table_cell(Item* p_table, List<Item*>::Element* p_cell,
		const String& p_string, bool p_reverse_search, int p_from_line);
	bool _search_table(Item* p_table, List<Item*>::Element* p_from, const String& p_string,
		bool p_reverse_search);
	bool _search_line(Item* p_frame, int p_line, const String& p_string, int p_char_idx,
		bool p_reverse_search);

	float _shape_line(Item* p_frame, int p_line, const Ref<Font>& p_base_font,
		int p_base_font_size, int p_width, float p_h, int* r_char_offset);
	float _resize_line(Item* p_frame, int p_line, const Ref<Font>& p_base_font,
		int p_base_font_size, int p_width, float p_h);

	int _get_line_max_width(Item* p_frame, int p_line) const;
	void _update_table_column_width(Item* p_table, int p_available_width);
	void _update_table_size(Item* p_table);

	void _update_line_font(
		Item* p_frame, int p_line, const Ref<Font>& p_base_font, int p_base_font_size);
	int _draw_line(Item* p_frame, int p_line, const Vector2& p_ofs, int p_width, float p_vsep,
		const Color& p_base_color, int p_outline_size, const Color& p_outline_color,
		const Color& p_font_shadow_color, int p_shadow_outline_size, const Point2& p_shadow_ofs,
		int& r_processed_glyphs);
	float _find_click_in_line(Item* p_frame, int p_line, const Vector2& p_ofs, int p_width,
		float p_vsep, const Point2i& p_click, Item** r_click_frame = nullptr,
		int* r_click_line = nullptr, Item** r_click_item = nullptr, int* r_click_char = nullptr,
		bool p_table = false, bool p_meta = false);
	void _accessibility_update_line(
		RID p_id, Item* p_frame, int p_line, const Vector2& p_ofs, int p_width, float p_vsep);

	String _roman(int p_num, bool p_capitalize) const;
	String _letters(int p_num, bool p_capitalize) const;

	Item* _find_indentable(Item* p_item);
	Item* _get_item_at_pos(Item* p_item_from, Item* p_item_to, int p_position);
	void _find_frame(Item* p_item, Item** r_frame, int* r_line);
	Item* _find_font_size(Item* p_item);
	Item* _find_font(Item* p_item);
	int _find_outline_size(Item* p_item, int p_default);
	Item* _find_list_item(Item* p_item);
	Item* _find_dc_item(Item* p_item);
	int _find_list(
		Item* p_item, Vector<int>& r_index, Vector<int>& r_count, Vector<Item*>& r_list);
	int _find_margin(Item* p_item, const Ref<Font>& p_base_font, int p_base_font_size);
	PackedFloat32Array _find_tab_stops(Item* p_item);
	HorizontalAlignment _find_alignment(Item* p_item);
	uint32_t _find_jst_flags(Item* p_item);
	TextServer::Direction _find_direction(Item* p_item);
	TextServer::StructuredTextParser _find_stt(Item* p_item);
	String _find_language(Item* p_item);
	Color _find_color(Item* p_item, const Color& p_default_color);
	Color _find_outline_color(Item* p_item, const Color& p_default_color);
	bool _find_underline(Item* p_item, Color* r_color = nullptr);
	bool _find_strikethrough(Item* p_item, Color* r_color = nullptr);
	bool _find_hint(Item* p_item, String* r_description);
	Color _find_bgcolor(Item* p_item);
	Color _find_fgcolor(Item* p_item);
	bool _find_layout_subitem(Item* from, Item* to);
	void _fetch_item_fx_stack(Item* p_item, Vector<Item*>& r_stack);
	void _normalize_subtags(Vector<String>& subtags);

	void _update_fx(Item* p_frame, double p_delta_time);
	void _scroll_changed(double);
	int _find_first_line(int p_from, int p_to, int p_vofs) const;

#ifdef TOOLS_ENABLED
	const real_t auto_scroll_speed = EDSCALE * 2.0f;
#else
	const real_t auto_scroll_speed = 2.0f;
#endif
	Vector2 last_clamped_mouse_pos;
	Timer* click_select_held = nullptr;
	Vector2 local_mouse_pos;
	bool is_selecting_text = false;
	void _update_selection();

	_FORCE_INLINE_ float _calculate_line_vertical_offset(const Line& line) const;

	Item* _get_next_item(Item* p_item, bool p_free = false) const;
	Item* _get_prev_item(Item* p_item, bool p_free = false) const;

	Rect2 _get_text_rect();
	int _get_wrap_width(const Rect2& p_text_rect) const;

	void _maximum_size_changed();

	Ref<RichTextEffect> _get_custom_effect_by_code(String p_bbcode_identifier);

	void _invalidate_fonts();

	Size2 _get_image_size(const Ref<Texture2D>& p_image, float p_width = 0, float p_height = 0,
		const Rect2& p_region = Rect2());
	Size2 _get_item_image_final_size(Item* p_img, float p_orig_width, float p_base_font_size);

	String _get_prefix(
		Item* p_item, const Vector<int>& p_list_index, const Vector<Item*>& p_list_items);
	void _add_list_prefixes(Item* p_frame, int p_line, Line& r_l);

	static int _find_unquoted(const String& p_src, char32_t p_chr, int p_from);
	static Vector<String> _split_unquoted(const String& p_src, char32_t p_splitter);
	static String _get_tag_value(const String& p_tag);

	bool use_bbcode = false;
	String text;
	void _apply_translation();

	bool internal_stack_editing = false;
	bool stack_externally_modified = false;

	RID accessibility_scroll_element;

	bool fit_content = false;

	struct ThemeCache
	{
		Ref<StyleBox> normal_style;
		Ref<StyleBox> focus_style;
		Ref<StyleBox> progress_bg_style;
		Ref<StyleBox> progress_fg_style;

		Ref<Texture2D> horizontal_rule;

		int line_separation;
		int paragraph_separation;

		Ref<Font> normal_font;
		int normal_font_size;

		Color default_color;
		Color font_selected_color;
		Color selection_color;
		Color font_outline_color;
		Color font_shadow_color;
		int shadow_outline_size;
		int shadow_offset_x;
		int shadow_offset_y;
		int outline_size;
		Color outline_color;

		Ref<Font> bold_font;
		int bold_font_size;
		Ref<Font> bold_italics_font;
		int bold_italics_font_size;
		Ref<Font> italics_font;
		int italics_font_size;
		Ref<Font> mono_font;
		int mono_font_size;

		int text_highlight_h_padding;
		int text_highlight_v_padding;

		int underline_alpha;
		int strikethrough_alpha;

		int table_h_separation;
		int table_v_separation;
		Color table_odd_row_bg;
		Color table_even_row_bg;
		Color table_border;

		float base_scale = 1.0;
	} theme_cache;

public:
	virtual RID get_focused_accessibility_element() const override;
	PackedStringArray get_accessibility_configuration_warnings() const;

	String get_parsed_text() const;
	void add_text(const String& p_text);
	void add_hr(int p_width = 90, int p_height = 2, const Color& p_color = Color(1.0, 1.0, 1.0),
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, bool p_width_in_percent = true,
		bool p_height_in_percent = false);
	void add_newline();
	bool remove_paragraph(int p_paragraph, bool p_no_invalidate = false);
	bool invalidate_paragraph(int p_paragraph);
	void push_dropcap(const String& p_string, const Ref<Font>& p_font, int p_size,
		const Rect2& p_dropcap_margins = Rect2(), const Color& p_color = Color(1, 1, 1),
		int p_ol_size = 0, const Color& p_ol_color = Color(0, 0, 0, 0));
	void _push_def_font(DefaultFont p_def_font);
	void _push_def_font_var(DefaultFont p_def_font, const Ref<Font>& p_font, int p_size = -1);
	void push_font(const Ref<Font>& p_font, int p_size = 0);
	void push_normal();
	void push_bold();
	void push_bold_italics();
	void push_italics();
	void push_mono();
	int get_current_table_column() const;

	void clear();

	void set_offset(int p_pixel);

	void set_meta_underline(bool p_underline);
	bool is_meta_underlined() const;

	void set_hint_underline(bool p_underline);
	bool is_hint_underlined() const;

	void set_scroll_active(bool p_active);
	bool is_scroll_active() const;

	void set_scroll_follow(bool p_follow);
	bool is_scroll_following() const;

	void set_scroll_follow_visible_characters(bool p_follow);
	bool is_scroll_following_visible_characters() const;

	void set_tab_size(int p_spaces);
	int get_tab_size() const;

	void set_context_menu_enabled(bool p_enabled);
	bool is_context_menu_enabled() const;

	void set_shortcut_keys_enabled(bool p_enabled);
	bool is_shortcut_keys_enabled() const;

	void set_fit_content(bool p_enabled);
	bool is_fit_content_enabled() const;

	bool search(
		const String& p_string, bool p_from_selection = false, bool p_search_previous = false);

	void scroll_to_paragraph(int p_paragraph);
	int get_paragraph_count() const;
	int get_visible_paragraph_count() const;

	float get_line_offset(int p_line);
	float get_paragraph_offset(int p_paragraph);

	void scroll_to_line(int p_line);
	int get_line_count() const;
	Vector2i get_line_range(int p_line);
	int get_visible_line_count() const;

	int get_content_height() const;
	int get_content_width() const;

	int get_line_height(int p_line) const;
	int get_line_width(int p_line) const;

	Rect2i get_visible_content_rect() const;

	void scroll_to_selection();

	ScrollBar* get_v_scroll_bar() { return vscroll; }

	virtual CursorShape get_cursor_shape(const Point2& p_pos) const override;

	void set_selection_enabled(bool p_enabled);
	bool is_selection_enabled() const;
	int get_selection_from() const;
	int get_selection_to() const;
	float get_selection_line_offset() const;
	String get_selected_text() const;
	void select_all();

	void set_deselect_on_focus_loss_enabled(const bool p_enabled);
	bool is_deselect_on_focus_loss_enabled() const;

	void set_drag_and_drop_selection_enabled(const bool p_enabled);
	bool is_drag_and_drop_selection_enabled() const;

	void deselect();

	int get_pending_paragraphs() const;
	bool is_finished() const;
	bool is_updating() const;

	void set_threaded(bool p_threaded);
	bool is_threaded() const;

	void set_progress_bar_delay(int p_delay_ms);
	int get_progress_bar_delay() const;

	// Context menu.
	PopupMenu* get_menu() const;
	bool is_menu_visible() const;
	void menu_option(int p_option);

	void parse_bbcode(const String& p_bbcode);
	void append_text(const String& p_bbcode);

	void set_use_bbcode(bool p_enable);
	bool is_using_bbcode() const;

	void set_text(const String& p_bbcode);
	String get_text() const;

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_tab_stops(const PackedFloat32Array& p_tab_stops);
	PackedFloat32Array get_tab_stops() const;

	void set_text_direction(TextDirection p_text_direction);
	TextDirection get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_autowrap_trim_flags(uint32_t p_flags);
	uint32_t get_autowrap_trim_flags() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_visible_characters(int p_visible);
	int get_visible_characters() const;
	int get_character_line(int p_char);
	int get_character_paragraph(int p_char);
	int get_total_character_count() const;
	int get_total_glyph_count() const;

	void set_visible_ratio(float p_ratio);
	float get_visible_ratio() const;

	TextServer::VisibleCharactersBehavior get_visible_characters_behavior() const;
	void set_visible_characters_behavior(TextServer::VisibleCharactersBehavior p_behavior);

	void reload_effects();
	void wait_until_finished();

	virtual Size2 get_minimum_size() const override;

	RichTextLabel(const String& p_text = String());
	~RichTextLabel() = default;
};



// --- End scene/gui/rich_text_label.h ---
