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

// --- Inlined/Folded from scene/gui/button.h ---
#include "core/input/input_event.h"
#include "scene/gui/control.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/text_line.h"
#include "scene/resources/text_line.h"

class Button;
class ButtonGroup;

struct ButtonBehavior
{
	enum ActionMode
	{
		ACTION_MODE_BUTTON_PRESS,
		ACTION_MODE_BUTTON_RELEASE,
	};

	enum DrawMode
	{
		DRAW_NORMAL,
		DRAW_PRESSED,
		DRAW_HOVER,
		DRAW_DISABLED,
		DRAW_HOVER_PRESSED,
	};

	uint32_t button_mask = 0;
	int click_margin = 0;
	int touch_index = -1;
	ActionMode action_mode = ACTION_MODE_BUTTON_RELEASE;

	bool toggle_mode = false;
	bool shortcut_in_tooltip = true;
	bool was_mouse_pressed = false;
	bool keep_pressed_outside = false;
	bool shortcut_feedback = true;
	bool in_shortcut_feedback = false;

	bool pressed = false;
	bool hovering = false;
	bool press_attempt = false;
	bool pressing_inside = false;
	bool pressed_down_with_focus = false;
	bool disabled = false;

	Ref<Shortcut> shortcut;
	Ref<ButtonGroup> button_group;

	bool is_disabled() const { return disabled; }
	bool is_pressing() const { return press_attempt; }
	bool is_pressed() const { return toggle_mode ? pressed : press_attempt; }
	bool is_hovered() const { return hovering; }

	DrawMode get_draw_mode() const;
};

class Button : public Control
{
public:
	using ActionMode = ButtonBehavior::ActionMode;
	using DrawMode = ButtonBehavior::DrawMode;

	static constexpr ActionMode ACTION_MODE_BUTTON_PRESS = ButtonBehavior::ACTION_MODE_BUTTON_PRESS;
	static constexpr ActionMode ACTION_MODE_BUTTON_RELEASE = ButtonBehavior::ACTION_MODE_BUTTON_RELEASE;

	static constexpr DrawMode DRAW_NORMAL = ButtonBehavior::DRAW_NORMAL;
	static constexpr DrawMode DRAW_PRESSED = ButtonBehavior::DRAW_PRESSED;
	static constexpr DrawMode DRAW_HOVER = ButtonBehavior::DRAW_HOVER;
	static constexpr DrawMode DRAW_DISABLED = ButtonBehavior::DRAW_DISABLED;
	static constexpr DrawMode DRAW_HOVER_PRESSED = ButtonBehavior::DRAW_HOVER_PRESSED;

	enum Style
	{
		STYLE_STANDARD,
		STYLE_CHECK_BOX,
		STYLE_CHECK_BUTTON,
		STYLE_LINK,
		STYLE_TEXTURE,
		STYLE_COLOR_SWATCH,
	};

	enum UnderlineMode
	{
		UNDERLINE_MODE_ALWAYS,
		UNDERLINE_MODE_ON_HOVER,
		UNDERLINE_MODE_NEVER,
	};

	enum StretchMode
	{
		STRETCH_SCALE,
		STRETCH_TILE,
		STRETCH_KEEP,
		STRETCH_KEEP_CENTERED,
		STRETCH_KEEP_ASPECT,
		STRETCH_KEEP_ASPECT_CENTERED,
		STRETCH_KEEP_ASPECT_COVERED,
	};

	struct TextureSet
	{
		Ref<Texture2D> normal;
		Ref<Texture2D> pressed;
		Ref<Texture2D> hover;
		Ref<Texture2D> disabled;
		Ref<Texture2D> focused;
		Ref<BitMap> click_mask;
		StretchMode stretch_mode = STRETCH_KEEP;
		Rect2 texture_region;
		Rect2 position_rect;
		bool ignore_texture_size = false;
		bool tile = false;
		bool hflip = false;
		bool vflip = false;
	};

	ButtonBehavior behavior;
	TextureSet textures;

private:
	Style style = STYLE_STANDARD;
	bool flat = false;
	String text;
	String xl_text;
	Ref<TextParagraph> text_buf;
	Ref<TextLine> link_text_buf;

	UnderlineMode underline_mode = UNDERLINE_MODE_ALWAYS;
	String uri;
	Color swatch_color;

	String language;
	TextDirection text_direction = TEXT_DIRECTION_AUTO;
	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
	uint32_t autowrap_flags_trim = TextServer::BREAK_TRIM_END_EDGE_SPACES;
	TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_NO_TRIMMING;

	Ref<Texture2D> icon;
	bool expand_icon = false;
	bool clip_text = false;
	HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_CENTER;
	HorizontalAlignment horizontal_icon_alignment = HORIZONTAL_ALIGNMENT_LEFT;
	VerticalAlignment vertical_icon_alignment = VERTICAL_ALIGNMENT_CENTER;
	float _internal_margin[4] = {};

	struct ThemeCache
	{
		Ref<StyleBox> normal;
		Ref<StyleBox> normal_mirrored;
		Ref<StyleBox> pressed;
		Ref<StyleBox> pressed_mirrored;
		Ref<StyleBox> hover;
		Ref<StyleBox> hover_mirrored;
		Ref<StyleBox> hover_pressed;
		Ref<StyleBox> hover_pressed_mirrored;
		Ref<StyleBox> disabled;
		Ref<StyleBox> disabled_mirrored;
		Ref<StyleBox> focus;

		Size2 max_style_size;
		float style_margin_left = 0;
		float style_margin_right = 0;
		float style_margin_top = 0;
		float style_margin_bottom = 0;

		bool align_to_largest_stylebox = false;

		Color font_color;
		Color font_focus_color;
		Color font_pressed_color;
		Color font_hover_color;
		Color font_hover_pressed_color;
		Color font_disabled_color;

		Ref<Font> font;
		int font_size = 0;
		int outline_size = 0;
		Color font_outline_color;

		Color icon_normal_color;
		Color icon_focus_color;
		Color icon_pressed_color;
		Color icon_hover_color;
		Color icon_hover_pressed_color;
		Color icon_disabled_color;

		Ref<Texture2D> icon;

		Ref<Texture2D> checked;
		Ref<Texture2D> unchecked;
		Ref<Texture2D> checked_disabled;
		Ref<Texture2D> unchecked_disabled;
		Ref<Texture2D> checked_mirrored;
		Ref<Texture2D> unchecked_mirrored;
		Ref<Texture2D> checked_disabled_mirrored;
		Ref<Texture2D> unchecked_disabled_mirrored;
		Ref<Texture2D> radio_checked;
		Ref<Texture2D> radio_unchecked;
		Ref<Texture2D> radio_checked_disabled;
		Ref<Texture2D> radio_unchecked_disabled;

		Color check_checked_color = Color(1, 1, 1);
		Color check_unchecked_color = Color(1, 1, 1);

		int h_separation = 0;
		int check_v_offset = 0;
		int underline_spacing = 0;
		int icon_max_width = 0;
		int line_spacing = 0;
	} theme_cache;

	void _shape(Ref<TextParagraph> p_paragraph = Ref<TextParagraph>(), String p_text = "") const;
	void _update_style_margins(const Ref<StyleBox>& p_stylebox);
	Size2 _get_check_icon_size() const;

protected:
	virtual void _update_theme_item_cache() override;

	void _set_internal_margin(Side p_side, float p_value);
	virtual void _queue_update_size_cache();
	virtual String _get_translated_text(const String& p_text) const;

	Size2 _fit_icon_size(const Size2& p_size) const;
	Ref<StyleBox> _get_current_stylebox() const;
	Size2 _get_largest_stylebox_size() const;
	void _notification(int p_what);

	virtual String _get_accessibility_name() const override;

public:
	Style get_style() const { return style; }
	void set_style(Style p_style);

	DrawMode get_draw_mode() const { return behavior.get_draw_mode(); }
	bool is_pressed() const { return behavior.is_pressed(); }
	bool is_pressing() const { return behavior.is_pressing(); }
	bool is_hovered() const { return behavior.is_hovered(); }
	bool is_disabled() const { return behavior.is_disabled(); }
	bool is_radio() const { return behavior.button_group.is_valid(); }

	void set_pressed(bool p_pressed);
	void set_pressed_no_signal(bool p_pressed);
	void set_toggle_mode(bool p_on);
	bool is_toggle_mode() const { return behavior.toggle_mode; }
	void set_disabled(bool p_disabled);

	void set_shortcut_in_tooltip(bool p_on);
	bool is_shortcut_in_tooltip_enabled() const { return behavior.shortcut_in_tooltip; }

	void set_action_mode(ActionMode p_mode) { behavior.action_mode = p_mode; }
	ActionMode get_action_mode() const { return behavior.action_mode; }

	void set_keep_pressed_outside(bool p_on) { behavior.keep_pressed_outside = p_on; }
	bool is_keep_pressed_outside() const { return behavior.keep_pressed_outside; }

	void set_shortcut_feedback(bool p_enable) { behavior.shortcut_feedback = p_enable; }
	bool is_shortcut_feedback() const { return behavior.shortcut_feedback; }

	void set_button_mask(uint32_t p_mask) { behavior.button_mask = p_mask; }
	uint32_t get_button_mask() const { return behavior.button_mask; }

	void set_shortcut(const Ref<Shortcut>& p_shortcut);
	Ref<Shortcut> get_shortcut() const { return behavior.shortcut; }

	void set_button_group(const Ref<ButtonGroup>& p_group);
	Ref<ButtonGroup> get_button_group() const { return behavior.button_group; }

	virtual void pressed();
	virtual void toggled(bool p_pressed);

	virtual bool has_point(const Point2& p_point) const override;
	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2()) const override;
	virtual Size2 get_minimum_size() const override;
	PackedStringArray get_configuration_warnings() const override;

	Size2 get_minimum_size_for_text_and_icon(const String& p_text, Ref<Texture2D> p_icon) const;

	void set_text(const String& p_text);
	String get_text() const;

	void set_uri(const String& p_uri);
	String get_uri() const { return uri; }
	void set_underline_mode(UnderlineMode p_mode) { underline_mode = p_mode; }
	UnderlineMode get_underline_mode() const { return underline_mode; }

	void set_swatch_color(const Color& p_color);
	Color get_swatch_color() const { return swatch_color; }

	void set_texture_normal(const Ref<Texture2D>& p_normal) { textures.normal = p_normal; }
	void set_texture_pressed(const Ref<Texture2D>& p_pressed) { textures.pressed = p_pressed; }
	void set_texture_hover(const Ref<Texture2D>& p_hover) { textures.hover = p_hover; }
	void set_texture_disabled(const Ref<Texture2D>& p_disabled) { textures.disabled = p_disabled; }
	void set_texture_focused(const Ref<Texture2D>& p_focused) { textures.focused = p_focused; }
	void set_click_mask(const Ref<BitMap>& p_click_mask) { textures.click_mask = p_click_mask; }

	Ref<Texture2D> get_texture_normal() const { return textures.normal; }
	Ref<Texture2D> get_texture_pressed() const { return textures.pressed; }
	Ref<Texture2D> get_texture_hover() const { return textures.hover; }
	Ref<Texture2D> get_texture_disabled() const { return textures.disabled; }
	Ref<Texture2D> get_texture_focused() const { return textures.focused; }
	Ref<BitMap> get_click_mask() const { return textures.click_mask; }

	void set_ignore_texture_size(bool p_ignore) { textures.ignore_texture_size = p_ignore; }
	bool get_ignore_texture_size() const { return textures.ignore_texture_size; }
	void set_stretch_mode(StretchMode p_mode) { textures.stretch_mode = p_mode; }
	StretchMode get_stretch_mode() const { return textures.stretch_mode; }
	void set_flip_h(bool p_flip) { textures.hflip = p_flip; }
	bool is_flipped_h() const { return textures.hflip; }
	void set_flip_v(bool p_flip) { textures.vflip = p_flip; }
	bool is_flipped_v() const { return textures.vflip; }

	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_autowrap_trim_flags(uint32_t p_flags);
	uint32_t get_autowrap_trim_flags() const;

	void set_text_direction(TextDirection p_text_direction);
	TextDirection get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_button_icon(const Ref<Texture2D>& p_icon) { icon = p_icon; }
	Ref<Texture2D> get_button_icon() const;

	void set_expand_icon(bool p_enabled);
	bool is_expand_icon() const;

	void set_flat(bool p_enabled);
	bool is_flat() const;

	void set_clip_text(bool p_enabled);
	bool get_clip_text() const;

	void set_text_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_text_alignment() const;

	void set_icon_alignment(HorizontalAlignment p_alignment);
	void set_vertical_icon_alignment(VerticalAlignment p_alignment);
	HorizontalAlignment get_icon_alignment() const;
	VerticalAlignment get_vertical_icon_alignment() const;

	Button(const String& p_text = String(), Style p_style = STYLE_STANDARD);
	~Button();
};

class ButtonGroup : public Resource
{
	friend class Button;
	HashSet<Button*> buttons;
	bool allow_unpress = false;

public:
	Button* get_pressed_button();
	void get_buttons(List<Button*>* r_buttons);
	void set_allow_unpress(bool p_enabled) { allow_unpress = p_enabled; }
	bool is_allow_unpress() const { return allow_unpress; }
	ButtonGroup();
};

// --- Folded from scene/gui/menu_bar.h ---
// --- Inlined/Folded from scene/gui/popup_menu.h ---
#include "core/input/input_event.h"
// --- Inlined/Folded from scene/gui/popup.h ---
#include "core/templates/local_vector.h"
#include "scene/main/window.h"

class Panel;

struct PopupTracker
{
	enum HideReason
	{
		HIDE_REASON_NONE,
		HIDE_REASON_CANCELED,  // E.g., because of rupture of UI flow (app unfocused). Includes
							   // closed programmatically.
		HIDE_REASON_UNFOCUSED, // E.g., user clicked outside.
	};

	LocalVector<Window*> visible_parents;
	bool popped_up = false;
	HideReason hide_reason = HIDE_REASON_NONE;

	void configure_window(Window* p_window);
	void initialize_visible_parents(Window* p_window);
	void deinitialize_visible_parents(Window* p_window);
	void post_popup();
	bool handle_cancel_input(const Window* p_window, const InputEvent& p_event);
	bool handle_parent_focused(const Window* p_window);
	void close_pressed(Window* p_window);
	static Rect2i adjust_popup_rect(const Window* p_window);
};

class Popup : public Window
{
public:
	using HideReason = PopupTracker::HideReason;
	static constexpr HideReason HIDE_REASON_NONE = PopupTracker::HIDE_REASON_NONE;
	static constexpr HideReason HIDE_REASON_CANCELED = PopupTracker::HIDE_REASON_CANCELED;
	static constexpr HideReason HIDE_REASON_UNFOCUSED = PopupTracker::HIDE_REASON_UNFOCUSED;

private:
	PopupTracker tracker;
	bool is_panel = false;
	Panel* panel = nullptr;
	bool close_on_accept = false;

	struct ThemeCache
	{
		Ref<StyleBox> panel_style;
	} theme_cache;

	mutable Rect2i pre_popup_rect;

	void _update_shadow_offsets() const;
	void _update_child_rects() const;

protected:
	void _close_pressed();
	void _parent_focused();
	virtual Rect2i _popup_adjust_rect() const override;
	virtual void _input_from_window(const InputEvent& p_event) override;
	virtual void _post_popup() override;
	virtual Size2 _get_contents_minimum_size() const override;

	void _notification(int p_what);

public:
	HideReason get_hide_reason() const { return tracker.hide_reason; }

	void set_panel_enabled(bool p_enabled);
	bool is_panel_enabled() const { return is_panel; }
	Panel* get_panel() const { return panel; }

	void set_close_on_accept(bool p_enable) { close_on_accept = p_enable; }
	bool is_closing_on_accept() const { return close_on_accept; }

#ifdef TOOLS_ENABLED
	PackedStringArray get_configuration_warnings() const override;
#endif

	Popup(bool p_panel = false);
	~Popup();
};
#include "scene/gui/scroll_container.h"
#include "scene/property_list_helper.h"
#include "scene/resources/text_line.h"
#include "servers/display/native_menu.h"

class Panel;
class LineEdit;
class Timer;
class PopupMenuItems;

class PopupMenu : public Window
{
	friend class PopupMenuItems;

public:
	using HideReason = PopupTracker::HideReason;
	static constexpr HideReason HIDE_REASON_NONE = PopupTracker::HIDE_REASON_NONE;
	static constexpr HideReason HIDE_REASON_CANCELED = PopupTracker::HIDE_REASON_CANCELED;
	static constexpr HideReason HIDE_REASON_UNFOCUSED = PopupTracker::HIDE_REASON_UNFOCUSED;

private:
	PopupTracker tracker;

	static HashMap<NativeMenu::SystemMenus, PopupMenu*> system_menus;

	struct Item
	{
		mutable RID accessibility_item_element;
		mutable bool accessibility_item_dirty = true;

		Ref<Texture2D> icon;
		int icon_max_width = 0;
		Color icon_modulate = Color(1, 1, 1, 1);
		String text;
		String xl_text;
		Ref<TextLine> text_buf;
		Ref<TextLine> accel_text_buf;

		String language;
		Control::TextDirection text_direction = Control::TEXT_DIRECTION_AUTO;
		AutoTranslateMode auto_translate_mode = AUTO_TRANSLATE_MODE_INHERIT;

		bool checked = false;
		bool visible = true;

		enum
		{
			CHECKABLE_TYPE_NONE,
			CHECKABLE_TYPE_CHECK_BOX,
			CHECKABLE_TYPE_RADIO_BUTTON,
		} checkable_type = CHECKABLE_TYPE_NONE;

		int max_states = 0;
		int state = 0;
		bool separator = false;
		bool disabled = false;
		bool indeterminate = false;
		bool dirty = true;
		int id = 0;
		String submenu_name; // Compatibility.
		PopupMenu* submenu = nullptr;
		String tooltip;
		Key accel = Key::NONE;
		int _ofs_cache = 0;
		int _height_cache = 0;
		int indent = 0;
		Ref<Shortcut> shortcut;
		bool shortcut_is_global = false;
		bool shortcut_is_disabled = false;
		bool allow_echo = false;
		bool submenu_bound = false;

		// Returns (0,0) if icon is null.
		Size2 get_icon_size() const { return icon.is_null() ? Size2() : icon->get_size(); }

		Item()
		{
			text_buf.instantiate();
			accel_text_buf.instantiate();
			checkable_type = CHECKABLE_TYPE_NONE;
		}

		Item(bool p_dummy) {}
	};

	RID accessibility_scroll_element;

	mutable Rect2i pre_popup_rect;
	void _update_shadow_offsets() const;

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	// To make Item available.
	friend class OptionButton;
	friend class MenuButton;

	RID global_menu;
	RID system_menu;
	NativeMenu::SystemMenus system_menu_id = NativeMenu::INVALID_MENU_ID;
	bool prefer_native = false;

	bool activated_by_keyboard = false;

	Timer* close_suspended_timer = nullptr;
	bool close_was_suspended = false;
	Timer* submenu_timer = nullptr;
	List<Rect2> autohide_areas;
	mutable Vector<Item> items;
	uint32_t initial_button_mask = 0;
	bool during_grabbed_click = false;
	bool is_scrolling = false;
	int mouse_over = -1;
	int prev_mouse_over = -1;
	int submenu_over = -1;
	int this_submenu_index =
		-1; // Always -1 for the parent popup, and always a positive int for every open submenu.
	int active_submenu_index = -1; // A positive int for the parent popup if any submenu is open.
	bool is_active_submenu_left = false;
	Vector<Point2> active_submenu_target_line;
	Point2 last_submenu_mouse_position;
	int submenu_mouse_exited_ticks_msec = -1;
	bool mouse_movement_was_tested = false;
	Point2 panel_offset_start;
	float submenu_timer_popup_delay = 0.2;
	const float CLOSE_SUSPENDED_TIMER_DELAY = 0.5;
	String _get_accel_text(const Item& p_item) const;
	int _get_mouse_over(const Point2& p_over) const;
	void _mouse_over_update(const Point2& p_over);
	virtual Size2 _get_contents_minimum_size() const override;

	int _get_item_height(int p_idx) const;
	int _get_items_total_height() const;
	Size2 _get_item_icon_size(int p_idx) const;

	void _shape_item(int p_idx) const;

	void _activate_submenu(int p_over, bool p_by_keyboard = false);
	void _submenu_timeout();
	bool _is_mouse_moving_toward_submenu(const Vector2& p_relative, bool p_is_submenu_left,
		const Vector2& p_mouse_position, const Vector<Point2>& p_active_submenu_target_line) const;
	void _close_or_suspend();
	void _close_suspended_timeout();

	uint64_t popup_time_msec = 0;
	bool hide_on_item_selection = true;
	bool hide_on_checkable_item_selection = true;
	bool hide_on_multistate_item_selection = false;
	Vector2 moved;

	HashMap<Ref<Shortcut>, int> shortcut_refcount;

	void _ref_shortcut(Ref<Shortcut> p_sc);
	void _unref_shortcut(Ref<Shortcut> p_sc);

	void _shortcut_changed();

	bool allow_search = true;
	uint64_t search_time_msec = 0;
	String search_string = "";

	bool search_bar_enabled = false;
	int search_bar_min_item_count = 0;
	bool search_bar_fuzzy_search_enabled = true;
	int search_bar_fuzzy_search_max_misses = 2;
	Panel* panel = nullptr;
	Control* vbox_container = nullptr;
	LineEdit* search_bar = nullptr;
	ScrollContainer* scroll_container = nullptr;
	PopupMenuItems* control = nullptr;

	const float DEFAULT_GAMEPAD_EVENT_DELAY_MS = 0.5;
	const float GAMEPAD_EVENT_REPEAT_RATE_MS = 1.0 / 20;
	float gamepad_event_delay_ms = DEFAULT_GAMEPAD_EVENT_DELAY_MS;
	bool joypad_event_process = false;

	struct ThemeCache
	{
		Ref<StyleBox> panel_style;
		Ref<StyleBox> hover_style;

		Ref<StyleBox> separator_style;
		Ref<StyleBox> labeled_separator_left;
		Ref<StyleBox> labeled_separator_right;

		int v_separation = 0;
		int h_separation = 0;
		int search_bar_separation = 0;
		int indent = 0;
		int item_start_padding = 0;
		int item_end_padding = 0;
		int icon_max_width = 0;
		int gutter_compact = 0;

		Ref<Texture2D> checked;
		Ref<Texture2D> checked_disabled;
		Ref<Texture2D> unchecked;
		Ref<Texture2D> unchecked_disabled;
		Ref<Texture2D> indeterminate;
		Ref<Texture2D> indeterminate_disabled;
		Ref<Texture2D> radio_checked;
		Ref<Texture2D> radio_checked_disabled;
		Ref<Texture2D> radio_unchecked;
		Ref<Texture2D> radio_unchecked_disabled;

		Ref<Texture2D> search;
		Ref<Texture2D> submenu;
		Ref<Texture2D> submenu_mirrored;

		Ref<Font> font;
		int font_size = 0;
		Ref<Font> font_separator;
		int font_separator_size = 0;

		Color font_color;
		Color font_hover_color;
		Color font_disabled_color;
		Color font_accelerator_color;
		int font_outline_size = 0;
		Color font_outline_color;

		Color font_separator_color;
		int font_separator_outline_size = 0;
		Color font_separator_outline_color;
	} theme_cache;

	void _draw_items();
	void _update_search_bar_visibility();
	void _items_focus_entered();
	void _search_bar_text_changed(const String& p_new_text);
	void _search_bar_focus_entered();
	void _filter_items(const String& p_query);

	void _close_pressed();
	void _menu_changed();
	void _input_from_window_internal(const InputEvent& p_event);
	bool _set_item_accelerator(int p_index, const InputEvent& p_ie);
	void _set_item_checkable_type(int p_index, int p_checkable_type);
	int _get_item_checkable_type(int p_index) const;
	void _native_popup(const Rect2i& p_rect);
	String _atr(int p_idx, const String& p_text) const;
	void _submenu_hidden();

	bool shrink_height = true;
	bool shrink_width = true;

protected:
	virtual void _pre_popup(const Size2i& p_size) override;
	virtual void _post_popup() override;
	virtual Rect2i _popup_adjust_rect() const override;

	virtual void add_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;
	virtual void _input_from_window(const InputEvent& p_event) override;

	virtual void _popup_base(const Rect2i& p_bounds = Rect2i()) override;
	void _notification(int p_what);

	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}

	virtual String _get_accessibility_name() const override;

#ifndef DISABLE_DEPRECATED
	void _add_shortcut_bind_compat_36493(
		const Ref<Shortcut>& p_shortcut, int p_id = -1, bool p_global = false);
	void _add_icon_shortcut_bind_compat_36493(const Ref<Texture2D>& p_icon,
		const Ref<Shortcut>& p_shortcut, int p_id = -1, bool p_global = false);
	void _clear_bind_compat_79965();

	void _set_system_menu_root_bind_compat_87452(const String& p_special);
	String _get_system_menu_root_bind_compat_87452() const;

	static void _bind_compatibility_methods();
#endif

public:
	// ATTENTION: This is used by the POT generator's scene parser. If the number of properties
	// returned by `_get_items()` ever changes, this value should be updated to reflect the new
	// size.
	static const int ITEM_PROPERTY_SIZE = 10;

	virtual RID get_focused_accessibility_element() const override;

	HideReason get_hide_reason() const { return tracker.hide_reason; }

	void _parent_focused();

	RID bind_global_menu();
	void unbind_global_menu();
	bool is_system_menu() const;

	void set_system_menu(NativeMenu::SystemMenus p_system_menu_id);
	NativeMenu::SystemMenus get_system_menu() const;

	void add_item(const String& p_label, int p_id = -1, Key p_accel = Key::NONE);
	void add_icon_item(const Ref<Texture2D>& p_icon, const String& p_label, int p_id = -1,
		Key p_accel = Key::NONE);
	void add_check_item(const String& p_label, int p_id = -1, Key p_accel = Key::NONE);
	void add_icon_check_item(const Ref<Texture2D>& p_icon, const String& p_label, int p_id = -1,
		Key p_accel = Key::NONE);
	void add_radio_check_item(const String& p_label, int p_id = -1, Key p_accel = Key::NONE);
	void add_icon_radio_check_item(const Ref<Texture2D>& p_icon, const String& p_label,
		int p_id = -1, Key p_accel = Key::NONE);

	void add_multistate_item(const String& p_label, int p_max_states, int p_default_state = 0,
		int p_id = -1, Key p_accel = Key::NONE);

	void add_shortcut(const Ref<Shortcut>& p_shortcut, int p_id = -1, bool p_global = false,
		bool p_allow_echo = false);
	void add_icon_shortcut(const Ref<Texture2D>& p_icon, const Ref<Shortcut>& p_shortcut,
		int p_id = -1, bool p_global = false, bool p_allow_echo = false);
	void add_check_shortcut(const Ref<Shortcut>& p_shortcut, int p_id = -1, bool p_global = false);
	void add_icon_check_shortcut(const Ref<Texture2D>& p_icon, const Ref<Shortcut>& p_shortcut,
		int p_id = -1, bool p_global = false);
	void add_radio_check_shortcut(
		const Ref<Shortcut>& p_shortcut, int p_id = -1, bool p_global = false);
	void add_icon_radio_check_shortcut(const Ref<Texture2D>& p_icon,
		const Ref<Shortcut>& p_shortcut, int p_id = -1, bool p_global = false);

	void add_submenu_item(const String& p_label, const String& p_submenu, int p_id = -1);
	void add_submenu_node_item(const String& p_label, PopupMenu* p_submenu, int p_id = -1);

	void set_item_text(int p_idx, const String& p_text);

	void set_item_text_direction(int p_idx, Control::TextDirection p_text_direction);
	void set_item_language(int p_idx, const String& p_language);
	void set_item_auto_translate_mode(int p_idx, AutoTranslateMode p_mode);
	void set_item_icon(int p_idx, const Ref<Texture2D>& p_icon);
	void set_item_icon_max_width(int p_idx, int p_width);
	void set_item_icon_modulate(int p_idx, const Color& p_modulate);
	void set_item_checked(int p_idx, bool p_checked);
	void set_item_indeterminate(int p_idx, bool p_indeterminate);
	void set_item_id(int p_idx, int p_id);
	void set_item_accelerator(int p_idx, Key p_accel);
	void set_item_disabled(int p_idx, bool p_disabled);
	void set_item_submenu(int p_idx, const String& p_submenu);
	void set_item_submenu_node(int p_idx, PopupMenu* p_submenu);
	void set_item_as_separator(int p_idx, bool p_separator);
	void set_item_as_checkable(int p_idx, bool p_checkable);
	void set_item_as_radio_checkable(int p_idx, bool p_radio_checkable);
	void set_item_tooltip(int p_idx, const String& p_tooltip);
	void set_item_shortcut(int p_idx, const Ref<Shortcut>& p_shortcut, bool p_global = false);
	void set_item_indent(int p_idx, int p_indent);
	void set_item_max_states(int p_idx, int p_max_states);
	void set_item_multistate(int p_idx, int p_state);
	void toggle_item_multistate(int p_idx);
	void set_item_shortcut_disabled(int p_idx, bool p_disabled);
	void set_item_index(int p_idx, int p_target_idx);

	void toggle_item_checked(int p_idx);

	String get_item_text(int p_idx) const;
	String get_item_xl_text(int p_idx) const;
	Control::TextDirection get_item_text_direction(int p_idx) const;
	String get_item_language(int p_idx) const;
	AutoTranslateMode get_item_auto_translate_mode(int p_idx) const;
	int get_item_idx_from_text(const String& text) const;
	Ref<Texture2D> get_item_icon(int p_idx) const;
	int get_item_icon_max_width(int p_idx) const;
	Color get_item_icon_modulate(int p_idx) const;
	bool is_item_checked(int p_idx) const;
	bool is_item_indeterminate(int p_idx) const;
	int get_item_id(int p_idx) const;
	int get_item_index(int p_id) const;
	Key get_item_accelerator(int p_idx) const;
	bool is_item_disabled(int p_idx) const;
	String get_item_submenu(int p_idx) const;
	PopupMenu* get_item_submenu_node(int p_idx) const;
	bool is_item_separator(int p_idx) const;
	bool is_item_checkable(int p_idx) const;
	bool is_item_radio_checkable(int p_idx) const;
	bool is_item_shortcut_disabled(int p_idx) const;
	bool is_item_shortcut_global(int p_idx) const;
	String get_item_tooltip(int p_idx) const;
	Ref<Shortcut> get_item_shortcut(int p_idx) const;
	int get_item_indent(int p_idx) const;
	int get_item_max_states(int p_idx) const;
	int get_item_state(int p_idx) const;

	void set_focused_item(int p_idx);
	int get_focused_item() const;

	void set_item_count(int p_count);
	int get_item_count() const;

	void set_prefer_native_menu(bool p_enabled);
	bool is_prefer_native_menu() const;

	void set_search_bar_enabled(bool p_enabled);
	bool is_search_bar_enabled() const;

	void set_search_bar_min_item_count(int p_count);
	int get_search_bar_min_item_count() const;

	void set_search_bar_fuzzy_search_enabled(bool p_enabled);
	bool is_search_bar_fuzzy_search_enabled() const;

	void set_search_bar_fuzzy_search_max_misses(int p_max_misses);
	int get_search_bar_fuzzy_search_max_misses() const;

	bool is_native_menu() const;

	void scroll_to_item(int p_idx);

	bool activate_item_by_event(const InputEvent& p_event, bool p_for_global_only = false);
	void activate_item(int p_idx);

	void _about_to_popup();
	void _about_to_close();

	void remove_item(int p_idx);

	void add_separator(const String& p_text = String(), int p_id = -1);

	void clear(bool p_free_submenus = true);

#ifdef TOOLS_ENABLED
	PackedStringArray get_configuration_warnings() const override;
#endif

	void add_autohide_area(const Rect2& p_area);
	void clear_autohide_areas();

	void set_hide_on_item_selection(bool p_enabled);
	bool is_hide_on_item_selection() const;

	void set_hide_on_checkable_item_selection(bool p_enabled);
	bool is_hide_on_checkable_item_selection() const;

	void set_hide_on_multistate_item_selection(bool p_enabled);
	bool is_hide_on_multistate_item_selection() const;

	void set_submenu_popup_delay(float p_time);
	float get_submenu_popup_delay() const;

	void set_allow_search(bool p_allow);
	bool get_allow_search() const;

	void set_shrink_height(bool p_shrink);
	bool get_shrink_height() const;

	void set_shrink_width(bool p_shrink);
	bool get_shrink_width() const;

	virtual void set_visible(bool p_visible) override;

	PopupMenu();
	~PopupMenu();
};

class PopupMenuItems : public Control
{
	PopupMenu* popup = nullptr;

public:
	PopupMenuItems(PopupMenu* p_popup) : popup(p_popup) {}

	virtual RID get_focused_accessibility_element() const override;
};

class MenuBar : public Control
{
	Mutex mutex;

	bool switch_on_hover = true;
	bool disable_shortcuts = false;
	bool prefer_native = true;
	bool flat = false;
	int start_index = -1;

	String language;
	TextDirection text_direction = TEXT_DIRECTION_AUTO;

	struct Menu
	{
		String name;
		String tooltip;

		Ref<TextLine> text_buf;
		bool hidden = false;
		bool disabled = false;
		RID submenu_rid;
		NativeMenu::SystemMenus sysmenu_id = NativeMenu::INVALID_MENU_ID;

		Menu(const String& p_name)
		{
			name = p_name;
			text_buf.instantiate();
		}

		Menu() { text_buf.instantiate(); }
	};

	Vector<Menu> menu_cache;

	int focused_menu = -1;
	int selected_menu = -1;
	int active_menu = -1;

	Vector2i old_mouse_pos;

	struct ThemeCache
	{
		Ref<StyleBox> normal;
		Ref<StyleBox> normal_mirrored;
		Ref<StyleBox> disabled;
		Ref<StyleBox> disabled_mirrored;
		Ref<StyleBox> pressed;
		Ref<StyleBox> pressed_mirrored;
		Ref<StyleBox> hover;
		Ref<StyleBox> hover_mirrored;
		Ref<StyleBox> hover_pressed;
		Ref<StyleBox> hover_pressed_mirrored;

		Ref<Font> font;
		int font_size = 0;
		int outline_size = 0;
		Color font_outline_color;

		Color font_color;
		Color font_disabled_color;
		Color font_pressed_color;
		Color font_hover_color;
		Color font_hover_pressed_color;
		Color font_focus_color;

		int h_separation = 0;
	} theme_cache;

	int _get_index_at_point(const Point2& p_point) const;
	Rect2 _get_menu_item_rect(int p_index) const;
	void _draw_menu_item(int p_index);

	void shape(Menu& p_menu);
	void _refresh_menu_names();
	Vector<PopupMenu*> _get_popups() const;
	int get_menu_idx_from_control(PopupMenu* p_child) const;

	void _open_popup(int p_index, bool p_focus_item = false);
	void _popup_visibility_changed(bool p_visible);

	String global_menu_tag;

	void bind_global_menu();
	void unbind_global_menu();

protected:
	void _notification(int p_what);
	virtual void add_child_notify(Node* p_child) override;
	virtual void move_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

public:
	void set_switch_on_hover(bool p_enabled);
	bool is_switch_on_hover();
	void set_disable_shortcuts(bool p_disabled);

	void set_prefer_global_menu(bool p_enabled);
	bool is_prefer_global_menu() const;

	bool is_native_menu() const;

	virtual Size2 get_minimum_size() const override;

	int get_menu_count() const;

	void set_text_direction(TextDirection p_text_direction);
	TextDirection get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_start_index(int p_index);
	int get_start_index() const;

	void set_flat(bool p_enabled);
	bool is_flat() const;

	void set_menu_title(int p_menu, const String& p_title);
	String get_menu_title(int p_menu) const;

	void set_menu_tooltip(int p_menu, const String& p_tooltip);
	String get_menu_tooltip(int p_menu) const;

	void set_menu_disabled(int p_menu, bool p_disabled);
	bool is_menu_disabled(int p_menu) const;

	void set_menu_hidden(int p_menu, bool p_hidden);
	bool is_menu_hidden(int p_menu) const;

	PopupMenu* get_menu_popup(int p_menu) const;

	virtual String get_tooltip(const Point2& p_pos) const;

	MenuBar();
	~MenuBar() = default;
};



// --- End scene/gui/menu_bar.h ---

// --- Folded from scene/gui/menu_button.h ---

#include "scene/gui/control.h"

#include "scene/property_list_helper.h"

class MenuButton : public Control
{
	bool switch_on_hover = false;
	bool disable_shortcuts = false;
	Button* button = nullptr;
	PopupMenu* popup = nullptr;

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

protected:
	void _notification(int p_what);

	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}

public:
	void pressed();

	Button* get_button() const { return button; }
	PopupMenu* get_popup() const;
	void show_popup();

	void set_text(const String& p_text)
	{
		if (button) {
			button->set_text(p_text);
		}
	}
	String get_text() const { return button ? button->get_text() : String(); }
	void set_flat(bool p_flat)
	{
		if (button) {
			button->set_flat(p_flat);
		}
	}
	bool is_flat() const { return button ? button->is_flat() : true; }
	void set_disabled(bool p_disabled)
	{
		if (button) {
			button->set_disabled(p_disabled);
		}
	}
	bool is_disabled() const { return button ? button->is_disabled() : false; }

	void set_switch_on_hover(bool p_enabled);
	bool is_switch_on_hover();
	void set_disable_shortcuts(bool p_disabled);

	void set_item_count(int p_count);
	int get_item_count() const;

	virtual Size2 get_minimum_size() const override
	{
		return button ? button->get_minimum_size() : Size2();
	}

#ifdef TOOLS_ENABLED
	PackedStringArray get_configuration_warnings() const override;
#endif

	MenuButton(const String& p_text = String());
	~MenuButton();
};

// --- End scene/gui/menu_button.h ---

// --- Folded from scene/gui/option_button.h ---

#include "scene/gui/control.h"

#include "scene/property_list_helper.h"

class OptionButton : public Control
{
	bool disable_shortcuts = false;
	Button* button = nullptr;
	PopupMenu* popup = nullptr;
	int current = -1;
	bool fit_to_longest_item = true;
	Vector2 _cached_size;
	bool cache_refresh_pending = false;
	bool allow_reselect = false;
	bool initialized = false;
	int queued_current = -1;

	struct ThemeCache
	{
		Ref<StyleBox> normal;

		Color font_color;
		Color font_focus_color;
		Color font_pressed_color;
		Color font_hover_color;
		Color font_hover_pressed_color;
		Color font_disabled_color;

		int h_separation = 0;

		Ref<Texture2D> arrow_icon;
		int arrow_margin = 0;
		int modulate_arrow = 0;
	};

	ThemeCache theme_cache;

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	void _focused(int p_which);
	void _selected(int p_which);
	void _select(int p_which, bool p_emit = false);
	void _select_int(int p_which);
	void _refresh_size_cache();

	void _dummy_setter() {}

	void pressed();

protected:
	Size2 get_minimum_size() const override;
	void _queue_update_size_cache();
	String _get_translated_text(const String& p_text) const;

	void _notification(int p_what);

	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}

public:
	static const int ITEM_PROPERTY_SIZE = 5;

	Button* get_button() const { return button; }
	void set_text(const String& p_text)
	{
		if (button) {
			button->set_text(p_text);
		}
	}
	String get_text() const { return button ? button->get_text() : String(); }
	void set_disabled(bool p_disabled)
	{
		if (button) {
			button->set_disabled(p_disabled);
		}
	}
	bool is_disabled() const { return button ? button->is_disabled() : false; }

	void add_icon_item(const Ref<Texture2D>& p_icon, const String& p_label, int p_id = -1);
	void add_item(const String& p_label, int p_id = -1);

	void set_item_text(int p_idx, const String& p_text);
	void set_item_icon(int p_idx, const Ref<Texture2D>& p_icon);
	void set_item_id(int p_idx, int p_id);
	void set_item_tooltip(int p_idx, const String& p_tooltip);
	void set_item_auto_translate_mode(int p_idx, AutoTranslateMode p_mode);

	String get_item_text(int p_idx) const;
	Ref<Texture2D> get_item_icon(int p_idx) const;
	int get_item_id(int p_idx) const;
	int get_item_index(int p_id) const;
	bool is_item_disabled(int p_idx) const;
	bool is_item_separator(int p_idx) const;
	String get_item_tooltip(int p_idx) const;
	AutoTranslateMode get_item_auto_translate_mode(int p_idx) const;

	bool is_search_bar_enabled() const;

	int get_search_bar_min_item_count() const;

	void set_search_bar_fuzzy_search_enabled(bool p_enabled);
	bool is_search_bar_fuzzy_search_enabled() const;

	void set_search_bar_fuzzy_search_max_misses(int p_max_misses);
	int get_search_bar_fuzzy_search_max_misses() const;

	bool has_selectable_items() const;
	int get_selectable_item(bool p_from_last = false) const;

	void set_item_count(int p_count);
	int get_item_count() const;
	void set_fit_to_longest_item(bool p_fit);
	bool is_fit_to_longest_item() const;

	void set_allow_reselect(bool p_allow);
	bool get_allow_reselect() const;

	void add_separator(const String& p_text = "");

	void clear();

	void select(int p_idx);
	int get_selected() const;
	int get_selected_id() const;

	void remove_item(int p_idx);

	PopupMenu* get_popup() const;
	void show_popup();

	void set_disable_shortcuts(bool p_disabled);

#ifdef TOOLS_ENABLED
	PackedStringArray get_configuration_warnings() const override;
#endif

	OptionButton(const String& p_text = String());
	~OptionButton();
};

// --- End scene/gui/option_button.h ---

// --- Folded from scene/gui/texture_rect.h ---
#include "scene/gui/control.h"

class TextureRect : public Control
{
public:
	enum ExpandMode
	{
		EXPAND_KEEP_SIZE,
		EXPAND_IGNORE_SIZE,
		EXPAND_FIT_WIDTH,
		EXPAND_FIT_WIDTH_PROPORTIONAL,
		EXPAND_FIT_HEIGHT,
		EXPAND_FIT_HEIGHT_PROPORTIONAL,
	};

	enum StretchMode
	{
		STRETCH_SCALE,
		STRETCH_TILE,
		STRETCH_KEEP,
		STRETCH_KEEP_CENTERED,
		STRETCH_KEEP_ASPECT,
		STRETCH_KEEP_ASPECT_CENTERED,
		STRETCH_KEEP_ASPECT_COVERED,
	};

private:
	bool hflip = false;
	bool vflip = false;
	Ref<Texture2D> texture;
	ExpandMode expand_mode = EXPAND_KEEP_SIZE;
	StretchMode stretch_mode = STRETCH_SCALE;

	void _texture_changed();

protected:
	void _notification(int p_what);
	virtual Size2 get_minimum_size() const override;

public:
	void set_texture(const Ref<Texture2D>& p_tex);
	Ref<Texture2D> get_texture() const;

	void set_expand_mode(ExpandMode p_mode);
	ExpandMode get_expand_mode() const;

	void set_stretch_mode(StretchMode p_mode);
	StretchMode get_stretch_mode() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	PackedStringArray get_configuration_warnings() const override;

	TextureRect();
	~TextureRect();
};



// --- End scene/gui/texture_rect.h ---
#include "scene/gui/control.h"
// --- Inlined/Folded from scene/gui/label.h ---
#include "scene/gui/control.h"
// --- Folded from scene/resources/label_settings.h ---
#include "core/io/resource.h"
#include "scene/property_list_helper.h"
#include "scene/resources/font.h"

/*************************************************************************/

class LabelSettings : public Resource
{
public:
	struct StackedOutlineData
	{
		int32_t size = 0;
		Color color;
	};

	struct StackedShadowData
	{
		Vector2 offset = Vector2(1, 1);
		Color color;
		int32_t outline_size = 0;
	};

private:
	real_t line_spacing = 3;
	real_t paragraph_spacing = 0;

	Ref<Font> font;
	int font_size = Font::DEFAULT_FONT_SIZE;
	Color font_color = Color(1, 1, 1);

	int outline_size = 0;
	Color outline_color = Color(1, 1, 1);

	int shadow_size = 1;
	Color shadow_color = Color(0, 0, 0, 0);
	Vector2 shadow_offset = Vector2(1, 1);

	Vector<StackedOutlineData> stacked_outline_data;
	Vector<StackedShadowData> stacked_shadow_data;

	static inline PropertyListHelper stacked_outline_base_property_helper;
	static inline PropertyListHelper stacked_shadow_base_property_helper;
	PropertyListHelper stacked_outline_property_helper;
	PropertyListHelper stacked_shadow_property_helper;

	void _font_changed();

protected:

	bool _property_can_revert(const String& p_name) const
	{
		return stacked_outline_property_helper.property_can_revert(p_name) ||
			   stacked_shadow_property_helper.property_can_revert(p_name);
	}

public:
	void set_line_spacing(real_t p_spacing);
	real_t get_line_spacing() const;

	void set_paragraph_spacing(real_t p_spacing);
	real_t get_paragraph_spacing() const;

	void set_font(const Ref<Font>& p_font);
	Ref<Font> get_font() const;

	void set_font_size(int p_size);
	int get_font_size() const;

	void set_font_color(const Color& p_color);
	Color get_font_color() const;

	void set_outline_size(int p_size);
	int get_outline_size() const;

	void set_outline_color(const Color& p_color);
	Color get_outline_color() const;

	void set_shadow_size(int p_size);
	int get_shadow_size() const;

	void set_shadow_color(const Color& p_color);
	Color get_shadow_color() const;

	void set_shadow_offset(const Vector2& p_offset);
	Vector2 get_shadow_offset() const;

	Vector<StackedOutlineData> get_stacked_outline_data() const;
	int get_stacked_outline_count() const;
	void set_stacked_outline_count(int p_count);
	void add_stacked_outline(int p_index = -1);
	void move_stacked_outline(int p_from_index, int p_to_position);
	void remove_stacked_outline(int p_index);
	void set_stacked_outline_size(int p_index, int p_size);
	int get_stacked_outline_size(int p_index) const;
	void set_stacked_outline_color(int p_index, const Color& p_color);
	Color get_stacked_outline_color(int p_index) const;

	Vector<StackedShadowData> get_stacked_shadow_data() const;
	int get_stacked_shadow_count() const;
	void set_stacked_shadow_count(int p_count);
	void add_stacked_shadow(int p_index = -1);
	void move_stacked_shadow(int p_from_index, int p_to_position);
	void remove_stacked_shadow(int p_index);
	void set_stacked_shadow_offset(int p_index, const Vector2& p_offset);
	Vector2 get_stacked_shadow_offset(int p_index) const;
	void set_stacked_shadow_color(int p_index, const Color& p_color);
	Color get_stacked_shadow_color(int p_index) const;
	void set_stacked_shadow_outline_size(int p_index, int p_size);
	int get_stacked_shadow_outline_size(int p_index) const;

	LabelSettings() {}
};



// --- End scene/resources/label_settings.h ---

class Label : public Control
{
private:
	HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT;
	VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_TOP;
	String text;
	String xl_text;
	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
	uint32_t autowrap_flags_trim =
		TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	uint32_t jst_flags =
		TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA |
		TextServer::JUSTIFICATION_SKIP_LAST_LINE |
		TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	bool clip = false;
	String el_char = U"…";
	TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_NO_TRIMMING;
	mutable Size2 minsize;
	bool uppercase = false;

	struct Paragraph
	{
		bool lines_dirty = true;
		bool dirty = true;
		int start = 0;

		String text;
		RID text_rid;
		Vector<RID> lines_rid;
	};

	mutable bool dirty = true;
	mutable bool font_dirty = true;
	mutable bool text_dirty = true;
	mutable Vector<Paragraph> paragraphs;
	mutable int total_line_count = 0;
	String paragraph_separator = "\\n";

	String language;
	TextDirection text_direction = TEXT_DIRECTION_AUTO;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
	TextServer::VisibleCharactersBehavior visible_chars_behavior =
		TextServer::VC_CHARS_BEFORE_SHAPING;
	int visible_chars = -1;
	float visible_ratio = 1.0;
	int lines_skipped = 0;
	int max_lines_visible = -1;
	PackedFloat32Array tab_stops;

	Ref<LabelSettings> settings;

	struct ThemeCache
	{
		Ref<StyleBox> normal_style;
		Ref<StyleBox> focus_style;
		Ref<Font> font;

		int font_size = 0;
		int line_spacing = 0;
		int paragraph_spacing = 0;
		Color font_color;
		Color font_shadow_color;
		Point2 font_shadow_offset;
		Color font_outline_color;
		int font_outline_size;
		int font_shadow_outline_size;
	};

	ThemeCache theme_cache;

	Rect2 _get_line_rect(int p_para, int p_line) const;
	void _ensure_shaped() const;
	void _update_visible() const;
	void _shape() const;
	void _invalidate();
	void _maximum_size_changed();

protected:
	RID get_line_rid(int p_line) const;
	Rect2 get_line_rect(int p_line) const;
	int get_layout_data(Vector2& r_offset, int& r_last_line, int& r_line_spacing) const;

	void _notification(int p_what);

public:
	virtual Size2 get_minimum_size() const override;
	virtual Size2 get_desired_size() const override;
	virtual PackedStringArray get_configuration_warnings() const override;

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_text(const String& p_string);
	String get_text() const;

	void set_label_settings(const Ref<LabelSettings>& p_settings);
	Ref<LabelSettings> get_label_settings() const;

	void set_text_direction(TextDirection p_text_direction);
	TextDirection get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_paragraph_separator(const String& p_paragraph_separator);
	String get_paragraph_separator() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_autowrap_trim_flags(uint32_t p_flags);
	uint32_t get_autowrap_trim_flags() const;

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_visible_characters_behavior(TextServer::VisibleCharactersBehavior p_behavior);
	TextServer::VisibleCharactersBehavior get_visible_characters_behavior() const;

	void set_visible_characters(int p_amount);
	int get_visible_characters() const;
	int get_total_character_count() const;

	void set_visible_ratio(float p_ratio);
	float get_visible_ratio() const;

	void set_clip_text(bool p_clip);
	bool is_clipping_text() const;

	void set_tab_stops(const PackedFloat32Array& p_tab_stops);
	PackedFloat32Array get_tab_stops() const;

	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior() const;

	void set_ellipsis_char(const String& p_char);
	String get_ellipsis_char() const;

	void set_lines_skipped(int p_lines);
	int get_lines_skipped() const;

	void set_max_lines_visible(int p_lines);
	int get_max_lines_visible() const;

	int get_line_height(int p_line = -1) const;
	int get_line_count() const;
	int get_visible_line_count() const;

	Rect2 get_character_bounds(int p_pos) const;

	Label(const String& p_text = String()) : text(p_text) {}
	~Label();

	template <typename... VarArgsFunc, typename... VarArgs>
	void draw_text(bool p_rtl, int p_ellipsis_pos, int p_ellipsis_gl_size,
		const Glyph* p_ellipsis_glyphs, bool p_trim_chars, int p_para_start, int p_visible_chars,
		bool p_trim_glyphs_ltr, int& p_processed_glyphs_step, int p_processed_glyphs,
		int p_visible_glyphs, bool p_trim_glyphs_rtl, int p_total_glyphs, const RID& p_ci,
		const Vector2& p_ofs, int p_gl_size, int p_trim_pos, const Glyph* p_glyphs,
		const Color& p_color,
		void (*p_draw_func)(const Glyph& p_gl, const RID& p_canvas,
			const Color& p_font_outline_color, const Vector2& p_ofs, VarArgsFunc... p_args),
		VarArgs&&... p_args)
	{
		p_processed_glyphs_step = p_processed_glyphs;
		Vector2 offset_step = p_ofs; /* Draw RTL ellipsis string when necessary. */
		if (p_rtl && p_ellipsis_pos >= 0) {
			for (int gl_idx = p_ellipsis_gl_size - 1; gl_idx >= 0; gl_idx--) {
				for (int j = 0; j < p_ellipsis_glyphs[gl_idx].repeat; j++) {
					bool skip =
						(p_trim_chars &&
							p_ellipsis_glyphs[gl_idx].end + p_para_start > p_visible_chars) ||
						(p_trim_glyphs_ltr && (p_processed_glyphs_step >= p_visible_glyphs)) ||
						(p_trim_glyphs_rtl &&
							(p_processed_glyphs_step < p_total_glyphs - p_visible_glyphs));
					if (!skip) {
						p_draw_func(p_ellipsis_glyphs[gl_idx], p_ci, p_color, offset_step,
							std::forward<VarArgs>(p_args)...);
					}
					p_processed_glyphs_step++;
					offset_step.x += p_ellipsis_glyphs[gl_idx].advance;
				}
			}
		}									  /* Draw main text. */
		for (int j = 0; j < p_gl_size; j++) { /* Trim when necessary. */
			if (p_trim_pos >= 0) {
				if (p_rtl) {
					if (j < p_trim_pos) {
						continue;
					}
				}
				else {
					if (j >= p_trim_pos) {
						break;
					}
				}
			}
			for (int k = 0; k < p_glyphs[j].repeat; k++) {
				bool skip = (p_trim_chars && p_glyphs[j].end + p_para_start > p_visible_chars) ||
							(p_trim_glyphs_ltr && (p_processed_glyphs_step >= p_visible_glyphs)) ||
							(p_trim_glyphs_rtl &&
								(p_processed_glyphs_step < p_total_glyphs - p_visible_glyphs));
				if (!skip) {
					p_draw_func(
						p_glyphs[j], p_ci, p_color, offset_step, std::forward<VarArgs>(p_args)...);
				}
				p_processed_glyphs_step++;
				offset_step.x += p_glyphs[j].advance;
			}
		} /* Draw LTR ellipsis string when necessary. */
		if (!p_rtl && p_ellipsis_pos >= 0) {
			for (int gl_idx = 0; gl_idx < p_ellipsis_gl_size; gl_idx++) {
				for (int j = 0; j < p_ellipsis_glyphs[gl_idx].repeat; j++) {
					bool skip =
						(p_trim_chars &&
							p_ellipsis_glyphs[gl_idx].end + p_para_start > p_visible_chars) ||
						(p_trim_glyphs_ltr && (p_processed_glyphs_step >= p_visible_glyphs)) ||
						(p_trim_glyphs_rtl &&
							(p_processed_glyphs_step < p_total_glyphs - p_visible_glyphs));
					if (!skip) {
						p_draw_func(p_ellipsis_glyphs[gl_idx], p_ci, p_color, offset_step,
							std::forward<VarArgs>(p_args)...);
					}
					p_processed_glyphs_step++;
					offset_step.x += p_ellipsis_glyphs[gl_idx].advance;
				}
			}
		}
	}
};

// --- Folded from scene/gui/item_list.h ---
#include "scene/gui/control.h"
#include "scene/gui/scroll_container.h"
#include "scene/property_list_helper.h"
#include "scene/resources/text_line.h"

class ItemList : public Control
{
public:
	enum IconMode
	{
		ICON_MODE_TOP,
		ICON_MODE_LEFT
	};

	enum SelectMode
	{
		SELECT_SINGLE,
		SELECT_MULTI,
		SELECT_TOGGLE,
	};

	enum ScrollHintMode
	{
		SCROLL_HINT_MODE_DISABLED,
		SCROLL_HINT_MODE_BOTH,
		SCROLL_HINT_MODE_TOP,
		SCROLL_HINT_MODE_BOTTOM,
	};

private:
	struct Item
	{
		mutable RID accessibility_item_element;
		mutable bool accessibility_item_dirty = true;

		Ref<Texture2D> icon;
		bool icon_transposed = false;
		Rect2i icon_region;
		Color icon_modulate = Color(1, 1, 1, 1);
		Ref<Texture2D> tag_icon;
		String text;
		String xl_text;
		Ref<TextParagraph> text_buf;
		String language;
		TextDirection text_direction = TEXT_DIRECTION_AUTO;
		AutoTranslateMode auto_translate_mode = AUTO_TRANSLATE_MODE_INHERIT;

		bool selectable = true;
		bool selected = false;
		bool disabled = false;
		bool tooltip_enabled = true;
		String tooltip;
		Color custom_fg;
		Color custom_bg = Color(0.0, 0.0, 0.0, 0.0);

		int column = 0;
		Rect2 rect_cache;
		Rect2 min_rect_cache;

		Size2 get_icon_size() const;

		bool operator<(const Item& p_another) const { return text < p_another.text; }

		Item() { text_buf.instantiate(); }

		Item(bool p_dummy) {}
	};

	RID accessibility_scroll_element;

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	int current = -1;
	int hovered = -1;
	int prev_hovered = -1;
	int shift_anchor = -1;

	bool shape_changed = true;

	bool ensure_selected_visible = false;
	bool same_column_width = false;
	bool allow_search = true;

	bool auto_width = false;
	float auto_width_value = 0.0;

	bool auto_height = false;
	float auto_height_value = 0.0;

	bool wraparound_items = true;

	Vector<Item> items;
	Vector<int> separators;

	SelectMode select_mode = SELECT_SINGLE;
	IconMode icon_mode = ICON_MODE_LEFT;
	ScrollBar* scroll_bar_v = nullptr;
	ScrollBar* scroll_bar_h = nullptr;
	TextServer::OverrunBehavior text_overrun_behavior = TextServer::OVERRUN_TRIM_ELLIPSIS;

	ScrollHintMode scroll_hint_mode = SCROLL_HINT_MODE_DISABLED;
	bool tile_scroll_hint = false;

	uint64_t search_time_msec = 0;
	String search_string;

	int current_columns = 1;
	int fixed_column_width = 0;
	int max_text_lines = 1;
	int max_columns = 1;

	Size2 fixed_icon_size;
	Size2 fixed_tag_icon_size;

	int defer_select_single = -1;
	bool allow_rmb_select = false;
	bool allow_reselect = false;

	real_t icon_scale = 1.0;

	bool do_autoscroll_to_bottom = false;

	void _scroll_changed(double);
	void _mouse_exited();
	void _shift_range_select(int p_from, int p_to);

protected:
	struct ThemeCache
	{
		int h_separation = 0;
		int v_separation = 0;
		int scroll_bar_h_separation = 0;

		Ref<StyleBox> panel_style;
		Ref<StyleBox> focus_style;

		Ref<Font> font;
		int font_size = 0;
		Color font_color;
		Color font_hovered_color;
		Color font_hovered_selected_color;
		Color font_selected_color;
		int font_outline_size = 0;
		Color font_outline_color;
		Color font_disabled_color;
		Color font_disabled_hovered_color;

		int line_separation = 0;
		int icon_margin = 0;
		Ref<StyleBox> hovered_style;
		Ref<StyleBox> hovered_selected_style;
		Ref<StyleBox> hovered_selected_focus_style;
		Ref<StyleBox> selected_style;
		Ref<StyleBox> selected_focus_style;
		Ref<StyleBox> cursor_style;
		Ref<StyleBox> cursor_focus_style;
		Ref<StyleBox> disabled_style;
		Ref<StyleBox> disabled_hovered_style;
		Color guide_color;

		Ref<Texture2D> scroll_hint;
		Color scroll_hint_color;
	} theme_cache;

	void _notification(int p_what);

	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}

public:
	virtual RID get_focused_accessibility_element() const override;

	int add_item(const String& p_item, const Ref<Texture2D>& p_texture = Ref<Texture2D>(),
		bool p_selectable = true);
	int add_icon_item(const Ref<Texture2D>& p_item, bool p_selectable = true);

	void set_item_text(int p_idx, const String& p_text);
	String get_item_text(int p_idx) const;

	void set_item_text_direction(int p_idx, TextDirection p_text_direction);
	TextDirection get_item_text_direction(int p_idx) const;

	void set_item_language(int p_idx, const String& p_language);
	String get_item_language(int p_idx) const;

	void set_item_auto_translate_mode(int p_idx, AutoTranslateMode p_mode);
	AutoTranslateMode get_item_auto_translate_mode(int p_idx) const;

	void set_item_icon(int p_idx, const Ref<Texture2D>& p_icon);
	Ref<Texture2D> get_item_icon(int p_idx) const;

	void set_item_icon_transposed(int p_idx, const bool transposed);
	bool is_item_icon_transposed(int p_idx) const;

	void set_item_icon_region(int p_idx, const Rect2& p_region);
	Rect2 get_item_icon_region(int p_idx) const;

	void set_item_icon_modulate(int p_idx, const Color& p_modulate);
	Color get_item_icon_modulate(int p_idx) const;

	void set_item_selectable(int p_idx, bool p_selectable);
	bool is_item_selectable(int p_idx) const;

	void set_item_disabled(int p_idx, bool p_disabled);
	bool is_item_disabled(int p_idx) const;

	void set_item_tag_icon(int p_idx, const Ref<Texture2D>& p_tag_icon);

	void set_item_tooltip_enabled(int p_idx, const bool p_enabled);
	bool is_item_tooltip_enabled(int p_idx) const;

	void set_item_tooltip(int p_idx, const String& p_tooltip);
	String get_item_tooltip(int p_idx) const;

	void set_item_custom_bg_color(int p_idx, const Color& p_custom_bg_color);
	Color get_item_custom_bg_color(int p_idx) const;

	void set_item_custom_fg_color(int p_idx, const Color& p_custom_fg_color);
	Color get_item_custom_fg_color(int p_idx) const;

	Rect2 get_item_rect(int p_idx, bool p_expand = true) const;

	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior() const;

	void select(int p_idx, bool p_single = true);
	void deselect(int p_idx);
	void deselect_all();
	bool is_selected(int p_idx) const;
	Vector<int> get_selected_items();
	bool is_anything_selected();

	void set_current(int p_current);
	int get_current() const;

	void move_item(int p_from_idx, int p_to_idx);

	void set_item_count(int p_count);
	int get_item_count() const;
	void remove_item(int p_idx);

	void clear();

	void set_fixed_column_width(int p_size);
	int get_fixed_column_width() const;

	void set_same_column_width(bool p_enable);
	bool is_same_column_width() const;

	void set_max_text_lines(int p_lines);
	int get_max_text_lines() const;

	void set_max_columns(int p_amount);
	int get_max_columns() const;

	void set_select_mode(SelectMode p_mode);
	SelectMode get_select_mode() const;

	void set_icon_mode(IconMode p_mode);
	IconMode get_icon_mode() const;

	void set_fixed_icon_size(const Size2i& p_size);
	Size2i get_fixed_icon_size() const;

	void set_fixed_tag_icon_size(const Size2i& p_size);

	void set_allow_rmb_select(bool p_allow);
	bool get_allow_rmb_select() const;

	void set_allow_reselect(bool p_allow);
	bool get_allow_reselect() const;

	void set_allow_search(bool p_allow);
	bool get_allow_search() const;

	void ensure_current_is_visible();
	void center_on_current(bool p_center_verically = true, bool p_center_horizontally = true);

	void sort_items_by_text();

	virtual AutoTranslateMode get_tooltip_auto_translate_mode_at(const Point2& p_at) const override;
	int get_item_at_position(const Point2& p_pos, bool p_exact = false) const;
	bool is_pos_at_end_of_items(const Point2& p_pos) const;

	void set_icon_scale(real_t p_scale);
	real_t get_icon_scale() const;

	void set_auto_width(bool p_enable);
	bool has_auto_width() const;

	void set_auto_height(bool p_enable);
	bool has_auto_height() const;

	void set_wraparound_items(bool p_enable);
	bool has_wraparound_items() const;

	Size2 get_minimum_size() const override;

	void set_autoscroll_to_bottom(const bool p_enable);

	void force_update_list_size();

	ScrollBar* get_v_scroll_bar() { return scroll_bar_v; }

	ScrollBar* get_h_scroll_bar() { return scroll_bar_h; }

	void set_scroll_hint_mode(ScrollHintMode p_mode);
	ScrollHintMode get_scroll_hint_mode() const;

	void set_tile_scroll_hint(bool p_enable);
	bool is_scroll_hint_tiled();

	ItemList();
	~ItemList();
};



// --- End scene/gui/item_list.h ---

// --- Folded from scene/gui/nine_patch_rect.h ---
#include "scene/gui/control.h"

class NinePatchRect : public Control
{
public:
	enum AxisStretchMode
	{
		AXIS_STRETCH_MODE_STRETCH,
		AXIS_STRETCH_MODE_TILE,
		AXIS_STRETCH_MODE_TILE_FIT,
	};

	bool draw_center = true;
	int margin[4] = {};
	Rect2 region_rect;
	Ref<Texture2D> texture;

	AxisStretchMode axis_h = AXIS_STRETCH_MODE_STRETCH;
	AxisStretchMode axis_v = AXIS_STRETCH_MODE_STRETCH;

	void _texture_changed();

protected:
	void _notification(int p_what);
	virtual Size2 get_minimum_size() const override;

public:
	void set_texture(const Ref<Texture2D>& p_tex);
	Ref<Texture2D> get_texture() const;

	void set_patch_margin(Side p_side, int p_size);
	int get_patch_margin(Side p_side) const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_draw_center(bool p_enabled);
	bool is_draw_center_enabled() const;

	void set_h_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_h_axis_stretch_mode() const;

	void set_v_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_v_axis_stretch_mode() const;

	NinePatchRect();
	~NinePatchRect();
};



// --- End scene/gui/nine_patch_rect.h ---

// --- Folded from scene/gui/reference_rect.h ---
#include "scene/gui/control.h"

class ReferenceRect : public Control {

	Color border_color = Color(1, 0, 0);
	float border_width = 1.0;
	bool editor_only = true;

protected:
	void _notification(int p_what);

public:
	void set_border_color(const Color &p_color);
	Color get_border_color() const;

	void set_border_width(float p_width);
	float get_border_width() const;

	void set_editor_only(const bool &p_enabled);
	bool get_editor_only() const;
};

// --- End scene/gui/reference_rect.h ---
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

#include "scene/gui/control.h"

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
#include "scene/resources/text_line.h"
#include "scene/resources/text_line.h"

#ifdef TOOLS_ENABLED
#include "editor/file_system/editor_paths.h"
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

// --- Folded from scene/gui/line_edit.h ---
#include "scene/gui/control.h"


class LineEdit : public Control
{
public:
	enum MenuItems
	{
		MENU_CUT,
		MENU_COPY,
		MENU_PASTE,
		MENU_CLEAR,
		MENU_SELECT_ALL,
		MENU_UNDO,
		MENU_REDO,
		MENU_SUBMENU_TEXT_DIR,
		MENU_DIR_INHERITED,
		MENU_DIR_AUTO,
		MENU_DIR_LTR,
		MENU_DIR_RTL,
		MENU_DISPLAY_UCC,
		MENU_SUBMENU_INSERT_UCC,
		MENU_INSERT_LRM,
		MENU_INSERT_RLM,
		MENU_INSERT_LRE,
		MENU_INSERT_RLE,
		MENU_INSERT_LRO,
		MENU_INSERT_RLO,
		MENU_INSERT_PDF,
		MENU_INSERT_ALM,
		MENU_INSERT_LRI,
		MENU_INSERT_RLI,
		MENU_INSERT_FSI,
		MENU_INSERT_PDI,
		MENU_INSERT_ZWJ,
		MENU_INSERT_ZWNJ,
		MENU_INSERT_WJ,
		MENU_INSERT_SHY,
		MENU_EMOJI_AND_SYMBOL,
		MENU_MAX
	};

	// Keep synced with DisplayServerEnums::VirtualKeyboardType enum.
	enum VirtualKeyboardType
	{
		KEYBOARD_TYPE_DEFAULT,
		KEYBOARD_TYPE_MULTILINE,
		KEYBOARD_TYPE_NUMBER,
		KEYBOARD_TYPE_NUMBER_DECIMAL,
		KEYBOARD_TYPE_PHONE,
		KEYBOARD_TYPE_EMAIL_ADDRESS,
		KEYBOARD_TYPE_PASSWORD,
		KEYBOARD_TYPE_URL
	};

	enum ExpandMode
	{
		EXPAND_MODE_ORIGINAL_SIZE,
		EXPAND_MODE_FIT_TO_TEXT,
		EXPAND_MODE_FIT_TO_LINE_EDIT,
	};

private:
	HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;

	bool editing = false;
	bool keep_editing_on_text_submit = false;
	bool editable = false;
	bool pass = false;
	bool text_changed_dirty = false;
	ExpandMode icon_expand_mode = EXPAND_MODE_ORIGINAL_SIZE;

	enum AltInputMode
	{
		ALT_INPUT_NONE,
		ALT_INPUT_UNICODE,
		ALT_INPUT_OEM,
		ALT_INPUT_WIN,
	};

	AltInputMode alt_mode = ALT_INPUT_NONE;
	bool alt_start = false;
	bool alt_start_no_hold = false;
	uint32_t alt_code = 0;

	String undo_text;
	String text;
	String placeholder;
	String placeholder_translated;
	String secret_character = U"•";
	String ime_text;
	Point2 ime_selection;

	RID text_rid;
	RID accessibility_text_root_element;
	float full_width = 0.0;

	bool selecting_enabled = true;
	bool deselect_on_focus_loss_enabled = true;
	bool drag_and_drop_selection_enabled = true;

	bool context_menu_enabled = true;
	bool emoji_menu_enabled = true;
	bool backspace_deletes_composite_character_enabled = false;
	PopupMenu* menu = nullptr;
	PopupMenu* menu_dir = nullptr;
	PopupMenu* menu_ctl = nullptr;

	bool caret_mid_grapheme_enabled = false;

	int caret_column = 0;
	float scroll_offset = 0.0;
	int max_length = 0; // 0 for no maximum.

	String language;
	TextDirection text_direction = TEXT_DIRECTION_AUTO;
	TextDirection input_direction = TEXT_DIRECTION_LTR;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
	bool draw_control_chars = false;

	bool expand_to_text_length = false;
	bool window_has_focus = true;

	bool clear_button_enabled = false;

	bool shortcut_keys_enabled = true;

	bool virtual_keyboard_enabled = true;
	bool virtual_keyboard_show_on_focus = true;
	VirtualKeyboardType virtual_keyboard_type = KEYBOARD_TYPE_DEFAULT;

	bool middle_mouse_paste_enabled = true;

	bool drag_action = false;
	bool drag_caret_force_displayed = false;

	Ref<Texture2D> right_icon;
	bool flat = false;
	bool spin_box_mode = false;
	float right_icon_scale = 1.0;

	struct Selection
	{
		int begin = 0;
		int end = 0;
		int start_column = 0;
		bool enabled = false;
		bool creating = false;
		bool double_click = false;
		bool drag_attempt = false;
	};

	Selection selection;

	struct TextOperation
	{
		int caret_column = 0;
		float scroll_offset = 0.0;
		String text;
	};

	List<TextOperation> undo_stack;
	List<TextOperation>::Element* undo_stack_pos = nullptr;

	struct ClearButtonStatus
	{
		bool press_attempt = false;
		bool pressing_inside = false;
	} clear_button_status;

	uint64_t last_dblclk = 0;
	Vector2 last_dblclk_pos;

	bool caret_blink_enabled = false;
	bool caret_force_displayed = false;
	bool draw_caret = true;
	float caret_blink_interval = 0.65;
	double caret_blink_timer = 0.0;
	bool caret_can_draw = false;

	bool pending_select_all_on_focus = false;
	bool select_all_on_focus = false;

	struct ThemeCache
	{
		Ref<StyleBox> normal;
		Ref<StyleBox> read_only;
		Ref<StyleBox> focus;

		Ref<Font> font;
		int font_size = 0;
		Color font_color;
		Color font_uneditable_color;
		Color font_selected_color;
		int font_outline_size;
		Color font_outline_color;
		Color font_placeholder_color;
		int caret_width = 0;
		Color caret_color;
		int minimum_character_width = 0;
		Color selection_color;

		Ref<Texture2D> clear_icon;
		Color clear_button_color;
		Color clear_button_color_pressed;

		float base_scale = 1.0;
	};

	ThemeCache theme_cache;

	void _close_ime_window();
	void _update_ime_window_position();

	void _clear_undo_stack();
	void _clear_redo();
	void _create_undo_state();

	Key _get_menu_action_accelerator(const String& p_action);
	void _generate_context_menu();
	void _update_context_menu();

	void _shape();
	void _fit_to_width();
	void _text_changed();
	void _emit_text_change();

	void shift_selection_check_pre(bool);
	void shift_selection_check_post(bool);

	void selection_fill_at_caret();
	void set_scroll_offset(float p_pos);
	float get_scroll_offset() const;

	void set_caret_at_pixel_pos(int p_x);
	Vector2 get_caret_pixel_pos();

	void _reset_caret_blink_timer();
	void _toggle_draw_caret();
	void _validate_caret_can_draw();

	void clear_internal();

	void _editor_settings_changed();

	void _swap_current_input_direction();
	void _move_caret_left(bool p_select, bool p_move_by_word = false);
	void _move_caret_right(bool p_select, bool p_move_by_word = false);
	void _move_caret_start(bool p_select);
	void _move_caret_end(bool p_select);
	void _backspace(bool p_word = false, bool p_all_to_left = false);
	void _delete(bool p_word = false, bool p_all_to_right = false);
	void _texture_changed();

	void _edit(bool p_show_virtual_keyboard = true, bool p_hide_focus = false);
	Point2 _get_right_icon_size(Ref<Texture2D> p_right_icon) const;

protected:
	bool _is_over_clear_button(const Point2& p_pos) const;

	virtual void _update_theme_item_cache() override;

	void _notification(int p_what);
	static void _bind_methods();

#ifndef DISABLE_DEPRECATED
	void _edit_bind_compat_111117();
	static void _bind_compatibility_methods();
#endif

	virtual void unhandled_key_input(const InputEvent& p_event) override;
	virtual String _get_accessibility_name() const override;

public:
	void edit(bool p_hide_focus = false);
	void unedit();
	bool is_editing() const;
	void set_keep_editing_on_text_submit(bool p_enabled);
	bool is_editing_kept_on_text_submit() const;

	bool has_ime_text() const;
	void cancel_ime();
	void apply_ime();

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	virtual CursorShape get_cursor_shape(const Point2& p_pos) const override;

	void menu_option(int p_option);
	void set_context_menu_enabled(bool p_enable);
	bool is_context_menu_enabled();
	PopupMenu* get_menu() const;
	bool is_menu_visible() const;

	void show_emoji_and_symbol_picker();

	void set_emoji_menu_enabled(bool p_enabled);
	bool is_emoji_menu_enabled() const;

	void set_backspace_deletes_composite_character_enabled(bool p_enabled);
	bool is_backspace_deletes_composite_character_enabled() const;

	void select(int p_from = 0, int p_to = -1);
	void select_all();
	void selection_delete();
	void deselect();
	bool has_selection() const;
	String get_selected_text();
	int get_selection_from_column() const;
	int get_selection_to_column() const;

	void delete_char();
	void delete_text(int p_from_column, int p_to_column);

	void _set_text(String p_text, bool p_emit_signal = false);
	void set_text(String p_text);
	String get_text() const;
	void set_text_with_selection(const String& p_text); // Set text, while preserving selection.

	void set_text_direction(TextDirection p_text_direction);
	TextDirection get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_draw_control_chars(bool p_draw_control_chars);
	bool get_draw_control_chars() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_placeholder(String p_text);
	String get_placeholder() const;

	void set_caret_column(int p_column);
	int get_caret_column() const;
	int get_next_composite_character_column(int p_column) const;
	int get_previous_composite_character_column(int p_column) const;

	void set_max_length(int p_max_length);
	int get_max_length() const;

	void insert_text_at_caret(String p_text);
	void clear();

	void set_caret_mid_grapheme_enabled(const bool p_enabled);
	bool is_caret_mid_grapheme_enabled() const;

	bool is_caret_blink_enabled() const;
	void set_caret_blink_enabled(const bool p_enabled);

	float get_caret_blink_interval() const;
	void set_caret_blink_interval(const float p_interval);

	void set_caret_force_displayed(const bool p_enabled);
	bool is_caret_force_displayed() const;

	void copy_text();
	void cut_text();
	void paste_text();
	bool has_undo() const;
	bool has_redo() const;
	void undo();
	void redo();

	void set_editable(bool p_editable);
	bool is_editable() const;

	void set_secret(bool p_secret);
	bool is_secret() const;

	void set_secret_character(const String& p_string);
	String get_secret_character() const;

	virtual Size2 get_minimum_size() const override;

	void set_expand_to_text_length_enabled(bool p_enabled);
	bool is_expand_to_text_length_enabled() const;

	void set_clear_button_enabled(bool p_enabled);
	bool is_clear_button_enabled() const;

	void set_shortcut_keys_enabled(bool p_enabled);
	bool is_shortcut_keys_enabled() const;

	void set_virtual_keyboard_enabled(bool p_enable);
	bool is_virtual_keyboard_enabled() const;

	void set_virtual_keyboard_show_on_focus(bool p_show_on_focus);
	bool get_virtual_keyboard_show_on_focus() const;

	void set_virtual_keyboard_type(VirtualKeyboardType p_type);
	VirtualKeyboardType get_virtual_keyboard_type() const;

	void set_middle_mouse_paste_enabled(bool p_enabled);
	bool is_middle_mouse_paste_enabled() const;

	void set_selecting_enabled(bool p_enabled);
	bool is_selecting_enabled() const;

	void set_deselect_on_focus_loss_enabled(const bool p_enabled);
	bool is_deselect_on_focus_loss_enabled() const;

	void set_drag_and_drop_selection_enabled(const bool p_enabled);
	bool is_drag_and_drop_selection_enabled() const;

	void set_right_icon(const Ref<Texture2D>& p_icon);
	Ref<Texture2D> get_right_icon();

	void set_icon_expand_mode(ExpandMode p_mode);
	ExpandMode get_icon_expand_mode() const;

	void set_right_icon_scale(float p_scale);
	float get_right_icon_scale() const;

	void set_flat(bool p_enabled);
	bool is_flat() const;

	void set_spin_box_mode(bool p_enabled) { spin_box_mode = p_enabled; }
	bool is_spin_box_mode() const { return spin_box_mode; }

	void set_select_all_on_focus(bool p_enabled);
	bool is_select_all_on_focus() const;
	void clear_pending_select_all_on_focus(); // For other controls, e.g. SpinBox.

	virtual bool is_text_field() const override;

	PackedStringArray get_configuration_warnings() const override;

	void show_virtual_keyboard();

	LineEdit(const String& p_placeholder = String());
	~LineEdit();
};

// --- Folded from scene/gui/spin_box.h ---
#include "scene/gui/control.h"

#include "scene/gui/scroll_container.h"
#include "scene/main/scene_tree.h"

class SpinBox : public Control
{
	LineEdit* line_edit = nullptr;
	bool update_on_text_changed = false;
	bool accepted = true;

	struct SizingCache
	{
		int buttons_block_width = 0;
		int buttons_width = 0;
		int buttons_vertical_separation = 0;
		int buttons_left = 0;
		int button_up_height = 0;
		int button_down_height = 0;
		int second_button_top = 0;
		int buttons_separator_top = 0;
		int field_and_buttons_separator_left = 0;
		int field_and_buttons_separator_width = 0;
	} sizing_cache;

	Timer* range_click_timer = nullptr;
	void _release_mouse_from_drag_mode();
	void _arrow_clicked(bool p_up);

	void _update_text(bool p_only_update_if_value_changed = false);
	void _text_submitted(const String& p_string);
	void _text_changed(const String& p_string);

	String prefix;
	String suffix;
	String last_text_value;
	double custom_arrow_step = 0.0;
	bool custom_arrow_round = false;

	void _line_edit_input(const InputEvent& p_event);

	struct Drag
	{
		double base_val = 0.0;
		bool allowed = false;
		bool enabled = false;
		Vector2 capture_pos;
		double diff_y = 0.0;
	} drag;

	struct StateCache
	{
		bool up_button_hovered = false;
		bool up_button_pressed = false;
		bool up_button_disabled = false;
		bool down_button_hovered = false;
		bool down_button_pressed = false;
		bool down_button_disabled = false;
	} state_cache;

	void _line_edit_editing_toggled(bool p_toggled_on);

	inline void _compute_sizes();
	inline int _get_widest_button_icon_width();

	struct ThemeCache
	{
		Ref<Texture2D> up_icon;
		Ref<Texture2D> up_hover_icon;
		Ref<Texture2D> up_pressed_icon;
		Ref<Texture2D> up_disabled_icon;
		Ref<Texture2D> down_icon;
		Ref<Texture2D> down_hover_icon;
		Ref<Texture2D> down_pressed_icon;
		Ref<Texture2D> down_disabled_icon;

		Ref<StyleBox> up_base_stylebox;
		Ref<StyleBox> up_hover_stylebox;
		Ref<StyleBox> up_pressed_stylebox;
		Ref<StyleBox> up_disabled_stylebox;
		Ref<StyleBox> down_base_stylebox;
		Ref<StyleBox> down_hover_stylebox;
		Ref<StyleBox> down_pressed_stylebox;
		Ref<StyleBox> down_disabled_stylebox;

		Color up_icon_modulate;
		Color up_hover_icon_modulate;
		Color up_pressed_icon_modulate;
		Color up_disabled_icon_modulate;
		Color down_icon_modulate;
		Color down_hover_icon_modulate;
		Color down_pressed_icon_modulate;
		Color down_disabled_icon_modulate;

		Ref<StyleBox> field_and_buttons_separator;
		Ref<StyleBox> up_down_buttons_separator;

		int buttons_vertical_separation = 0;
		int field_and_buttons_separation = 0;
		int buttons_width = 0;
#ifndef DISABLE_DEPRECATED
		Ref<Texture2D> updown_icon;
		bool is_updown_assigned = false;
		bool set_min_buttons_width_from_icons = false;
#endif
	} theme_cache;

	void _mouse_exited();
	void _update_buttons_state_for_current_value();

protected:
	void _value_changed(double p_value);

	void _notification(int p_what);

public:
	NumericRange range;

	double get_value() const { return range.value; }
	double get_min() const { return range.min; }
	double get_max() const { return range.max; }
	double get_step() const { return range.step; }
	double get_page() const { return range.page; }
	double get_as_ratio() const { return range.get_as_ratio(); }

	void set_value(double p_val)
	{
		if (range.set_value(p_val)) {
			_value_changed(range.value);
			queue_redraw();
		}
		queue_accessibility_update();
	}

	void set_value_no_signal(double p_val)
	{
		if (range.set_value(p_val)) {
			_value_changed(range.value);
			queue_redraw();
		}
	}

	void set_min(double p_min)
	{
		if (range.set_min(p_min)) {
			_value_changed(range.value);
			queue_redraw();
			queue_accessibility_update();
		}
	}

	void set_max(double p_max)
	{
		if (range.set_max(p_max)) {
			_value_changed(range.value);
			queue_redraw();
			queue_accessibility_update();
		}
	}

	void set_step(double p_step)
	{
		if (range.set_step(p_step)) {
			queue_accessibility_update();
		}
	}

	void set_page(double p_page)
	{
		if (range.set_page(p_page)) {
			_value_changed(range.value);
			queue_redraw();
			queue_accessibility_update();
		}
	}

	void set_as_ratio(double p_value)
	{
		if (range.set_as_ratio(p_value)) {
			_value_changed(range.value);
			queue_redraw();
		}
		queue_accessibility_update();
	}

	void set_allow_greater(bool p_allow) { range.allow_greater = p_allow; }
	bool is_greater_allowed() const { return range.allow_greater; }
	void set_allow_lesser(bool p_allow) { range.allow_lesser = p_allow; }
	bool is_lesser_allowed() const { return range.allow_lesser; }
	void set_use_rounded_values(bool p_enable) { range.rounded_values = p_enable; }
	bool is_using_rounded_values() const { return range.rounded_values; }
	bool is_ratio_exp() const { return range.exp_ratio; }

	LineEdit* get_line_edit();

	virtual Size2 get_minimum_size() const override;
	PackedStringArray get_configuration_warnings() const override
	{
		PackedStringArray warnings = Control::get_configuration_warnings();
		range.append_configuration_warnings(warnings);
		return warnings;
	}

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_editable(bool p_enabled);
	bool is_editable() const;

	void set_suffix(const String& p_suffix);
	String get_suffix() const;

	void set_prefix(const String& p_prefix);
	String get_prefix() const;

	void set_update_on_text_changed(bool p_enabled);
	bool get_update_on_text_changed() const;

	void set_select_all_on_focus(bool p_enabled);
	bool is_select_all_on_focus() const;

	void apply();
	void set_custom_arrow_step(const double p_custom_arrow_step);
	double get_custom_arrow_step() const;

	void set_custom_arrow_round(bool p_round);
	bool is_custom_arrow_rounding() const;

	SpinBox();
};

// --- End scene/gui/spin_box.h ---

// --- Folded from scene/gui/text_edit.h ---
#include <type_traits>

#include "scene/gui/control.h"

#include "scene/gui/scroll_container.h"
#include "scene/main/scene_tree.h"
// --- Folded from scene/resources/syntax_highlighter.h ---
#include "core/io/resource.h"
#include "core/templates/rb_map.h"

class TextEdit;

class CodeHighlighter : public Resource
{
private:
	struct ColorRegion
	{
		Color color;
		String start_key;
		String end_key;
		bool line_only = false;
	};

	TextEdit* text_edit = nullptr;

	Vector<ColorRegion> color_regions;
	HashMap<int, int> color_region_cache;

	Color font_color;
	Color member_color;
	Color function_color;
	Color symbol_color;
	Color number_color;

	bool uint_suffix_enabled = false;

	void _lines_edited_from(int p_from_line, int p_to_line);

public:
	void clear_highlighting_cache();
	void _clear_highlighting_cache();

	void update_cache();
	void _update_cache();

	void set_text_edit(TextEdit* p_text_edit);
	TextEdit* get_text_edit() const;

	void add_keyword_color(const String& p_keyword, const Color& p_color);
	void remove_keyword_color(const String& p_keyword);
	bool has_keyword_color(const String& p_keyword) const;
	Color get_keyword_color(const String& p_keyword) const;

	void clear_keyword_colors();

	void add_member_keyword_color(const String& p_member_keyword, const Color& p_color);
	void remove_member_keyword_color(const String& p_member_keyword);
	bool has_member_keyword_color(const String& p_member_keyword) const;
	Color get_member_keyword_color(const String& p_member_keyword) const;

	void clear_member_keyword_colors();

	void add_color_region(const String& p_start_key, const String& p_end_key, const Color& p_color,
		bool p_line_only = false);
	void remove_color_region(const String& p_start_key);
	bool has_color_region(const String& p_start_key) const;

	void clear_color_regions();

	void set_number_color(Color p_color);
	Color get_number_color() const;

	void set_symbol_color(Color p_color);
	Color get_symbol_color() const;

	void set_function_color(Color p_color);
	Color get_function_color() const;

	void set_member_variable_color(Color p_color);
	Color get_member_variable_color() const;

	void set_uint_suffix_enabled(bool p_enabled);
};

// --- End scene/resources/syntax_highlighter.h ---
#include "scene/resources/text_line.h"

class TextEdit : public Control
{
public:
	/* Edit Actions. */
	enum EditAction
	{
		ACTION_NONE,
		ACTION_TYPING,
		ACTION_BACKSPACE,
		ACTION_DELETE,
	};

	/* Caret. */
	enum CaretType
	{
		CARET_TYPE_LINE,
		CARET_TYPE_BLOCK
	};

	/* Selection */
	enum SelectionMode
	{
		SELECTION_MODE_NONE,
		SELECTION_MODE_SHIFT,
		SELECTION_MODE_POINTER,
		SELECTION_MODE_WORD,
		SELECTION_MODE_LINE
	};

	/* Line Wrapping.*/
	enum LineWrappingMode
	{
		LINE_WRAPPING_NONE,
		LINE_WRAPPING_BOUNDARY
	};

	/* Gutters. */
	enum GutterType
	{
		GUTTER_TYPE_STRING,
		GUTTER_TYPE_ICON,
		GUTTER_TYPE_CUSTOM
	};

	/* Context Menu. */
	enum MenuItems
	{
		MENU_CUT,
		MENU_COPY,
		MENU_PASTE,
		MENU_CLEAR,
		MENU_SELECT_ALL,
		MENU_UNDO,
		MENU_REDO,
		MENU_SUBMENU_TEXT_DIR,
		MENU_DIR_INHERITED,
		MENU_DIR_AUTO,
		MENU_DIR_LTR,
		MENU_DIR_RTL,
		MENU_DISPLAY_UCC,
		MENU_SUBMENU_INSERT_UCC,
		MENU_INSERT_LRM,
		MENU_INSERT_RLM,
		MENU_INSERT_LRE,
		MENU_INSERT_RLE,
		MENU_INSERT_LRO,
		MENU_INSERT_RLO,
		MENU_INSERT_PDF,
		MENU_INSERT_ALM,
		MENU_INSERT_LRI,
		MENU_INSERT_RLI,
		MENU_INSERT_FSI,
		MENU_INSERT_PDI,
		MENU_INSERT_ZWJ,
		MENU_INSERT_ZWNJ,
		MENU_INSERT_WJ,
		MENU_INSERT_SHY,
		MENU_EMOJI_AND_SYMBOL,
		MENU_MAX

	};

	/* Search. */
	enum SearchFlags
	{
		SEARCH_MATCH_CASE = 1,
		SEARCH_WHOLE_WORDS = 2,
		SEARCH_BACKWARDS = 4
	};

	/* Code Completion. */
	enum CodeCompletionKind
	{
		KIND_CLASS,
		KIND_FUNCTION,
		KIND_SIGNAL,
		KIND_VARIABLE,
		KIND_MEMBER,
		KIND_ENUM,
		KIND_CONSTANT,
		KIND_NODE_PATH,
		KIND_FILE_PATH,
		KIND_PLAIN_TEXT,
		KIND_KEYWORD,
	};

	enum CodeCompletionLocation
	{
		LOCATION_LOCAL = 0,
		LOCATION_PARENT_MASK = 1 << 8,
		LOCATION_OTHER_USER_CODE = 1 << 9,
		LOCATION_OTHER = 1 << 10,
	};

private:
	struct GutterInfo
	{
		GutterType type = GutterType::GUTTER_TYPE_STRING;
		String name = "";
		int width = 24;
		bool draw = true;
		bool clickable = false;
		bool overwritable = false;
	};

	class Text
	{
	public:
		struct Gutter
		{
			bool clickable = false;

			Ref<Texture2D> icon;
			String text = "";
			Color color = Color(1, 1, 1);
		};

		struct Line
		{
			Vector<Gutter> gutters;

			String data;
			Ref<TextParagraph> data_buf;
			Vector<RID> accessibility_text_root_element;

			String ime_data;

			Color background_color = Color(0, 0, 0, 0);
			bool hidden = false;
			int line_count = 0;
			int height = 0;
			int width = 0;
			float indent_ofs = -1.0;

			Line() { data_buf.instantiate(); }
		};

	private:
		bool is_dirty = false;
		bool tab_size_dirty = false;

		mutable Vector<Line> text;
		Ref<Font> font;
		int font_size = -1;
		int font_height = 0;

		String language;
		TextServer::Direction direction = TextServer::DIRECTION_AUTO;
		uint32_t brk_flags = TextServer::BREAK_MANDATORY;
		bool draw_control_chars = false;
		String custom_word_separators;
		bool use_default_word_separators = true;
		bool use_custom_word_separators = false;

		mutable bool max_line_width_dirty = true;
		mutable bool max_line_height_dirty = true;
		mutable int max_line_width = 0;
		mutable int max_line_height = 0;
		mutable int total_visible_line_count = 0;
		int width = -1;

		int tab_size = 4;
		int gutter_count = 0;
		bool indent_wrapped_lines = false;

	public:
		void set_tab_size(int p_tab_size);
		int get_tab_size() const;
		void set_indent_wrapped_lines(bool p_enabled);
		bool is_indent_wrapped_lines() const;

		void set_font(const Ref<Font>& p_font);
		void set_font_size(int p_font_size);
		void set_direction_and_language(
			TextServer::Direction p_direction, const String& p_language);
		void set_draw_control_chars(bool p_enabled);

		int get_line_height() const;
		int get_line_width(int p_line, int p_wrap_index = -1) const;
		int get_max_width() const;
		int get_total_visible_line_count() const;

		void set_use_default_word_separators(bool p_enabled);
		bool is_default_word_separators_enabled() const;

		void set_use_custom_word_separators(bool p_enabled);
		bool is_custom_word_separators_enabled() const;

		void set_custom_word_separators(const String& p_separators);
		String get_enabled_word_separators() const;
		String get_custom_word_separators() const;
		String get_default_word_separators() const;

		void set_width(float p_width);
		float get_width() const;
		void set_brk_flags(uint32_t p_flags);
		uint32_t get_brk_flags() const;
		int get_line_wrap_amount(int p_line) const;

		const Vector<RID> get_accessibility_elements(int p_line);
		void update_accessibility(int p_line, RID p_root);

		void clear_accessibility()
		{
			for (int i = 0; i < text.size(); i++) {
				text.write[i].accessibility_text_root_element.clear();
			}
		}

		Vector<Vector2i> get_line_wrap_ranges(int p_line) const;
		const Ref<TextParagraph> get_line_data(int p_line) const;
		float get_indent_offset(int p_line, bool p_rtl) const;

		void set_hidden(int p_line, bool p_hidden);
		bool is_hidden(int p_line) const;
		void remove_range(int p_from_line, int p_to_line);

		int size() const { return text.size(); }

		void clear();

		void invalidate_cache(int p_line, bool p_text_changed = false);
		void invalidate_font();
		void invalidate_all();
		void invalidate_all_lines();

		_FORCE_INLINE_ const String& operator[](int p_line) const;
		_FORCE_INLINE_ const String& get_text_with_ime(int p_line) const;

		/* Gutters. */
		void add_gutter(int p_at);
		void remove_gutter(int p_gutter);
		void move_gutters(int p_from_line, int p_to_line);

		void set_line_gutter_text(int p_line, int p_gutter, const String& p_text)
		{
			text.write[p_line].gutters.write[p_gutter].text = p_text;
		}

		const String& get_line_gutter_text(int p_line, int p_gutter) const
		{
			return text[p_line].gutters[p_gutter].text;
		}

		void set_line_gutter_icon(int p_line, int p_gutter, const Ref<Texture2D>& p_icon)
		{
			text.write[p_line].gutters.write[p_gutter].icon = p_icon;
		}

		const Ref<Texture2D>& get_line_gutter_icon(int p_line, int p_gutter) const
		{
			return text[p_line].gutters[p_gutter].icon;
		}

		void set_line_gutter_item_color(int p_line, int p_gutter, const Color& p_color)
		{
			text.write[p_line].gutters.write[p_gutter].color = p_color;
		}

		const Color& get_line_gutter_item_color(int p_line, int p_gutter) const
		{
			return text[p_line].gutters[p_gutter].color;
		}

		void set_line_gutter_clickable(int p_line, int p_gutter, bool p_clickable)
		{
			text.write[p_line].gutters.write[p_gutter].clickable = p_clickable;
		}

		bool is_line_gutter_clickable(int p_line, int p_gutter) const
		{
			return text[p_line].gutters[p_gutter].clickable;
		}

		/* Line style. */
		void set_line_background_color(int p_line, const Color& p_color)
		{
			text.write[p_line].background_color = p_color;
		}

		const Color get_line_background_color(int p_line) const
		{
			return text[p_line].background_color;
		}
	};

	/* Text */
	Text text;
	RID text_ci;
	bool setting_text = false;

	enum AltInputMode
	{
		ALT_INPUT_NONE,
		ALT_INPUT_UNICODE,
		ALT_INPUT_OEM,
		ALT_INPUT_WIN,
	};

	AltInputMode alt_mode = ALT_INPUT_NONE;
	bool alt_start = false;
	bool alt_start_no_hold = false;
	uint32_t alt_code = 0;

	bool tab_input_mode = true;

	// Text properties.
	String ime_text = "";
	Point2 ime_selection;

	// Placeholder
	String placeholder_text = "";
	Ref<TextParagraph> placeholder_data_buf;
	int placeholder_line_height = -1;
	int placeholder_max_width = -1;

	Vector<String> placeholder_wrapped_rows;

	void _update_placeholder();
	bool _using_placeholder() const;

	/* Initialize to opposite first, so we get past the early-out in set_editable. */
	bool editable = false;

	TextDirection text_direction = TEXT_DIRECTION_AUTO;
	TextDirection input_direction = TEXT_DIRECTION_LTR;

	String language = "";

	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;

	void _clear();
	void _update_caches(bool p_invalidate_all = false);

	void _close_ime_window();
	void _update_ime_window_position();
	void _update_ime_text();

	// User control.
	bool overtype_mode = false;
	bool context_menu_enabled = true;
	bool emoji_menu_enabled = true;
	bool backspace_deletes_composite_character_enabled = false;
	bool shortcut_keys_enabled = true;
	bool virtual_keyboard_enabled = true;
	bool virtual_keyboard_show_on_focus = true;
	bool middle_mouse_paste_enabled = true;
	bool empty_selection_clipboard_enabled = true;

	// Overridable actions.
	String cut_copy_line = "";

	// Context menu.
	PopupMenu* menu = nullptr;
	PopupMenu* menu_dir = nullptr;
	PopupMenu* menu_ctl = nullptr;

	Key _get_menu_action_accelerator(const String& p_action);
	void _generate_context_menu();

	/* Versioning */
	struct Caret;

	struct TextOperation
	{
		enum Type
		{
			TYPE_NONE,
			TYPE_INSERT,
			TYPE_REMOVE
		};

		Vector<Caret> start_carets;
		Vector<Caret> end_carets;

		Type type = TYPE_NONE;
		int from_line = 0;
		int from_column = 0;
		int to_line = 0;
		int to_column = 0;
		String text;
		uint32_t prev_version = 0;
		uint32_t version = 0;
		bool chain_forward = false;
		bool chain_backward = false;
	};

	bool undo_enabled = true;
	int undo_stack_max_size = 50;

	EditAction current_action = EditAction::ACTION_NONE;
	bool pending_action_end = false;
	bool in_action = false;

	int complex_operation_count = 0;
	bool next_operation_is_complex = false;

	TextOperation current_op;
	List<TextOperation> undo_stack;
	List<TextOperation>::Element* undo_stack_pos = nullptr;

	Timer* idle_detect = nullptr;

	uint32_t version = 0;
	uint32_t saved_version = 0;

	void _push_current_op();
	void _do_text_op(const TextOperation& p_op, bool p_reverse);
	void _clear_redo();

	/* Search */
	String search_text = "";
	uint32_t search_flags = 0;

	int _get_column_pos_of_word(const String& p_key, const String& p_search,
		uint32_t p_search_flags, int p_from_column) const;

	/* Mouse */
	struct LineDrawingCache
	{
		int y_offset = 0;
		Vector<int> first_visible_chars;
		Vector<int> last_visible_chars;
	};

	HashMap<int, LineDrawingCache> line_drawing_cache;

	int _get_char_pos_for_line(int p_px, int p_line, int p_wrap_index = 0) const;

	/* Caret. */
	struct Selection
	{
		bool active = false;

		int origin_line = 0;
		int origin_column = 0;
		int origin_last_fit_x = 0;
		int word_begin_column = 0;
		int word_end_column = 0;
	};

	struct Caret
	{
		Selection selection;

		Point2 draw_pos;
		bool visible = false;
		int last_fit_x = 0;
		int line = 0;
		int column = 0;
	};

	// Vector containing all the carets, index '0' is the "main caret" and should never be removed.
	Vector<Caret> carets;

	bool setting_caret_line = false;
	bool caret_pos_dirty = false;
	void _set_caret_pos_dirty(bool p_dirty);

	int multicaret_edit_count = 0;
	bool multicaret_edit_merge_queued = false;
	HashSet<int> multicaret_edit_ignore_carets;

	CaretType caret_type = CaretType::CARET_TYPE_LINE;

	bool draw_caret = true;
	bool draw_caret_when_editable_disabled = false;

	bool caret_blink_enabled = false;
	Timer* caret_blink_timer = nullptr;

	bool move_caret_on_right_click = true;

	bool caret_mid_grapheme_enabled = false;

	bool multi_carets_enabled = true;

	bool drag_action = false;
	bool drag_caret_force_displayed = false;

	void _caret_changed(int p_caret = -1);
	void _emit_caret_changed();

	void _show_virtual_keyboard();
	void _reset_caret_blink_timer();
	void _toggle_draw_caret();

	int _get_column_x_offset_for_line(int p_char, int p_line, int p_column) const;
	bool _is_line_col_in_range(int p_line, int p_column, int p_from_line, int p_from_column,
		int p_to_line, int p_to_column, bool p_include_edges = true) const;

	void _offset_carets_after(int p_old_line, int p_old_column, int p_new_line, int p_new_column,
		bool p_include_selection_begin = true, bool p_include_selection_end = true);

	void _cancel_drag_and_drop_text();

	/* Selection. */
	SelectionMode selecting_mode = SelectionMode::SELECTION_MODE_NONE;

	bool selecting_enabled = true;
	bool deselect_on_focus_loss_enabled = true;
	bool drag_and_drop_selection_enabled = true;

	bool use_selected_font_color = false;

	bool selection_drag_attempt = false;
	bool dragging_selection = false;
	int drag_and_drop_origin_caret_index = -1;
	int drag_caret_index = -1;

	Timer* click_select_held = nullptr;
	uint64_t last_dblclk = 0;
	Vector2 last_dblclk_pos;

	bool touch_dragging_starting = false;
	bool touch_dragging_in_progress = false;
	bool touch_dragging_deaccel = false;
	Vector2 drag_accum;
	Vector2 drag_from;
	Vector2 drag_speed;
	Vector2 last_drag_accum;
	double time_since_motion = 0.0;
	bool pan_gesture_performed = false;

	enum SelectionHandleDragType
	{
		SELECTION_HANDLE_NONE,
		SELECTION_HANDLE_START,
		SELECTION_HANDLE_END,
	};

	SelectionHandleDragType selection_handle_drag_type = SELECTION_HANDLE_NONE;
	float selection_handle_radius;
	int dragging_caret_index = -1;
	Vector2 selection_handle_drag_offset;
	bool show_selection_handle = false;
	bool selection_handle_enabled = true;
	Vector<Point2i> _get_selection_handles_pos(int p_cursor) const;
	bool _is_first_column(int p_line, int p_column) const;
	void _draw_selection_handle(Vector2 p_pos) const;

	void _cancel_inertial_scroll();

	void _on_drag_or_mouse_motion_event(Vector2i p_event_position, bool p_is_left_click_or_drag);

	void _selection_changed(int p_caret = -1);
	void _click_selection_held();

	void _pre_shift_selection(int p_caret);

	bool _selection_contains(int p_caret, int p_line, int p_column, bool p_include_edges = true,
		bool p_only_selections = true) const;

	/* Line wrapping. */
	LineWrappingMode line_wrapping_mode = LineWrappingMode::LINE_WRAPPING_NONE;
	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_WORD_SMART;

	int wrap_at_column = 0;

	void _update_wrap_at_column(bool p_force = false);

	/* Viewport. */
	ScrollBar* h_scroll = nullptr;
	ScrollBar* v_scroll = nullptr;

	Vector2i content_size_cache;
	bool fit_content_height = false;
	bool fit_content_width = false;
	bool scroll_past_end_of_file_enabled = false;

	// Smooth scrolling.
	bool smooth_scroll_enabled = false;
	float target_v_scroll = 0.0;
	float v_scroll_speed = 80.0;

	// Scrolling.
	int first_visible_line = 0;
	int first_visible_line_wrap_ofs = 0;
	int first_visible_col = 0;

	bool scrolling = false;
	bool updating_scrolls = false;

	void _update_scrollbars();
	int _get_control_height() const;

	void _v_scroll_input();
	void _scroll_moved(double p_to_val);

	double _get_visible_lines_offset() const;
	double _get_v_scroll_offset() const;

	void _scroll_up(real_t p_delta, bool p_animate);
	void _scroll_down(real_t p_delta, bool p_animate);

	void _scroll_lines_up();
	void _scroll_lines_down();

	void _adjust_viewport_to_caret_horizontally(int p_caret = 0, bool p_maximize_selection = true);

	// Minimap.
	bool draw_minimap = false;

	int minimap_width = 80;
	Point2 minimap_char_size = Point2(1, 2);
	int minimap_line_spacing = 1;

	// Minimap scroll.
	bool minimap_clicked = false;
	bool hovering_minimap = false;
	bool dragging_minimap = false;
	bool can_drag_minimap = false;

	double minimap_scroll_ratio = 0.0;
	double minimap_scroll_click_pos = 0.0;

	void _update_minimap_hover();
	void _update_minimap_click();
	void _update_minimap_drag();

	/* Gutters. */
	Vector<GutterInfo> gutters;
	int gutters_width = 0;
	int gutter_padding = 0;
	Vector2i hovered_gutter = Vector2i(-1, -1); // X = gutter index, Y = row.

	void _update_gutter_width();
	Vector2i _get_hovered_gutter(const Point2& p_mouse_pos) const;

	/* Syntax highlighting. */
	Ref<CodeHighlighter> syntax_highlighter;
	HashMap<int, Vector<Pair<int64_t, Color>>> syntax_highlighting_cache;

	Vector<Pair<int64_t, Color>> _get_line_syntax_highlighting(int p_line);
	void _clear_syntax_highlighting_cache();
	void _syntax_highlighter_changed();

	/* Code editing features (composed). */
	enum MainGutterType
	{
		MAIN_GUTTER_BREAKPOINT = 0x01,
		MAIN_GUTTER_BOOKMARK = 0x02,
		MAIN_GUTTER_EXECUTING = 0x04
	};

	enum DelimiterType
	{
		TYPE_STRING,
		TYPE_COMMENT,
	};

	struct BracePair
	{
		String open_key = "";
		String close_key = "";
	};

	struct Delimiter
	{
		DelimiterType type;
		String start_key = "";
		String end_key = "";
		bool line_only = true;
	};

	struct CodeFeatures
	{
		/* Indent management */
		int indent_size = 4;
		String indent_text = "\t";
		bool auto_indent = false;
		HashSet<char32_t> auto_indent_prefixes;
		bool indent_using_spaces = false;

		/* Auto brace completion */
		bool auto_brace_completion_enabled = false;
		Vector<BracePair> auto_brace_completion_pairs;

		/* Main Gutter */
		int main_gutter = -1;
		HashMap<int, bool> breakpointed_lines;
		HashMap<int, bool> bookmarked_lines;
		HashMap<int, bool> executing_lines;
		bool draw_breakpoints = false;
		bool draw_bookmarks = false;
		bool draw_executing_lines = false;

		/* Line numbers */
		int line_number_gutter = -1;
		int line_number_digits = 1;
		int line_numbers_min_digits = 3;
		String line_number_padding = " ";
		HashMap<int, RID> line_number_text_cache;

		/* Fold Gutter */
		int fold_gutter = -1;
		bool draw_fold_gutter = false;

		/* Line Folding */
		bool line_folding_enabled = false;
		String code_region_start_string;
		String code_region_end_string;
		String code_region_start_tag = "region";
		String code_region_end_tag = "endregion";

		/* Delimiters */
		bool setting_delimiters = false;
		Vector<Delimiter> delimiters;
		Vector<RBMap<int, int>> delimiter_cache;

		/* Code Hint */
		String code_hint = "";
		bool code_hint_draw_below = true;
		int code_hint_xpos = -0xFFFF;

		/* Code Completion */
		bool code_completion_enabled = false;
		bool code_completion_forced = false;
		Vector2 completion_touch_drag_accum;
		bool code_completion_active = false;
		bool is_code_completion_scroll_hovered = false;
		bool is_code_completion_scroll_pressed = false;
		bool is_code_completion_drag_started = false;
		Vector<RID> code_completion_ac_items;
		RID code_completion_ac_scroll_element;
		RID code_completion_ac_root_element;
		int code_completion_line_ofs = 0;
		int code_completion_current_selected = 0;
		int code_completion_force_item_center = -1;
		int code_completion_longest_line = 0;
		Rect2i code_completion_rect;
		Rect2i code_completion_scroll_rect;
		float code_completion_pan_offset = 0.0f;
		HashSet<char32_t> code_completion_prefixes;
		String code_completion_base;
		String code_completion_line;
		int code_completion_caret_line = 0;
		int code_completion_caret_column = 0;

		/* Line length guidelines */
		Vector<int> line_length_guideline_columns;

		/* Symbol lookup */
		bool symbol_lookup_on_click_enabled = false;
		Point2i symbol_lookup_pos;
		String symbol_lookup_new_word;
		String symbol_lookup_word;

		/* Symbol tooltip */
		bool symbol_tooltip_on_hover_enabled = false;
		Point2i symbol_tooltip_pos;
		String symbol_tooltip_word;
		Timer* symbol_tooltip_timer = nullptr;

		/* Callbacks */
		int lines_edited_changed = 0;
		int lines_edited_from = -1;
		int lines_edited_to = -1;
	};

	bool code_mode = false;
	CodeFeatures code;

	int _calculate_spaces_till_next_left_indent(int p_column) const;
	int _calculate_spaces_till_next_right_indent(int p_column) const;
	int _get_auto_brace_pair_open_at_pos(int p_line, int p_col);
	int _get_auto_brace_pair_close_at_pos(int p_line, int p_col);

	_FORCE_INLINE_ bool _is_bookmark_only() const
	{
		return code.draw_bookmarks && !code.draw_executing_lines && !code.draw_breakpoints;
	}

	void _ensure_code_gutters();
	void _update_draw_main_gutter();
	void _main_gutter_draw_callback(int p_line, int p_gutter, const Rect2& p_region);
	void _clear_line_number_text_cache();
	void _update_line_number_gutter_width();
	void _line_number_draw_callback(int p_line, int p_gutter, const Rect2& p_region);
	void _fold_gutter_draw_callback(int p_line, int p_gutter, Rect2 p_region);
	void _gutter_clicked(int p_line, int p_gutter);
	void _update_gutter_indexes();
	void _update_code_region_tags();
	bool _fold_line(int p_line);
	bool _unfold_line(int p_line);

	void _update_delimiter_cache(int p_from_line = 0, int p_to_line = -1);
	int _is_in_delimiter(int p_line, int p_column, DelimiterType p_type) const;
	void _add_delimiter(
		const String& p_start_key, const String& p_end_key, bool p_line_only, DelimiterType p_type);
	void _remove_delimiter(const String& p_start_key, DelimiterType p_type);
	bool _has_delimiter(const String& p_start_key, DelimiterType p_type) const;
	void _set_delimiters(const Vector<String>& p_delimiters, DelimiterType p_type);
	void _clear_delimiters(DelimiterType p_type);
	Vector<String> _get_delimiters(DelimiterType p_type) const;

	void _update_scroll_selected_line(float p_mouse_y);
	void _filter_code_completion_candidates_impl();
	void _on_symbol_tooltip_timer_timeout();

	void _code_lines_edited_from(int p_from_line, int p_to_line);
	void _code_text_set();
	void _code_line_col_changed();
	void _code_text_changed();

	/* Visual. */
	struct ThemeCache
	{
		float base_scale = 1.0;

		/* Search */
		Color search_result_color = Color(1, 1, 1);
		Color search_result_border_color = Color(1, 1, 1);

		/* Caret */
		int caret_width = 1;
		Color caret_color = Color(1, 1, 1);
		Color caret_background_color = Color(0, 0, 0);

		/* Selection */
		Color font_selected_color = Color(0, 0, 0, 0);
		Color selection_color = Color(1, 1, 1);

		/* Other visuals */
		Ref<StyleBox> style_normal;
		Ref<StyleBox> style_focus;
		Ref<StyleBox> style_readonly;

		Ref<Texture2D> tab_icon;
		Ref<Texture2D> space_icon;

		Ref<Font> font;
		int font_size = 16;
		Color font_color = Color(1, 1, 1);
		Color font_readonly_color = Color(1, 1, 1);
		Color font_placeholder_color = Color(1, 1, 1, 0.6);

		int outline_size = 0;
		Color outline_color = Color(1, 1, 1);

		int line_spacing = 1;
		int wrap_offset = 10;

		Color current_line_color = Color(1, 1, 1);
		Color word_highlighted_color = Color(1, 1, 1);

		/* Code mode gutters */
		Color code_folding_color = Color(1, 1, 1);
		Color folded_code_region_color = Color(1, 1, 1);
		Ref<Texture2D> can_fold_icon;
		Ref<Texture2D> folded_icon;
		Ref<Texture2D> can_fold_code_region_icon;
		Ref<Texture2D> folded_code_region_icon;
		Ref<Texture2D> folded_eol_icon;
		Ref<Texture2D> completion_color_bg;

		Color breakpoint_color = Color(1, 1, 1);
		Ref<Texture2D> breakpoint_icon;

		Color bookmark_color = Color(1, 1, 1);
		Ref<Texture2D> bookmark_icon;

		Color executing_line_color = Color(1, 1, 1);
		Ref<Texture2D> executing_line_icon;

		Color line_number_color = Color(1, 1, 1);

		/* Code Completion */
		Ref<StyleBox> code_completion_style;
		int code_completion_icon_separation = 0;

		int code_completion_max_width = 0;
		int code_completion_max_lines = 7;
		int code_completion_scroll_width = 0;
		Color code_completion_scroll_color = Color(0, 0, 0, 0);
		Color code_completion_scroll_hovered_color = Color(0, 0, 0, 0);
		Color code_completion_background_color = Color(0, 0, 0, 0);
		Color code_completion_selected_color = Color(0, 0, 0, 0);
		Color code_completion_existing_color = Color(0, 0, 0, 0);

		/* Code hint */
		Ref<StyleBox> code_hint_style;
		Color code_hint_color;

		/* Line length guideline & brace mismatch */
		Color line_length_guideline_color;
		Color brace_mismatch_color;

#ifndef DISABLE_DEPRECATED
		Color background_color = Color(1, 1, 1);
#endif // DISABLE_DEPRECATED
	} theme_cache;

	bool window_has_focus = true;
	bool first_draw = true;

	bool highlight_current_line = false;
	bool highlight_all_occurrences = false;
	bool draw_control_chars = false;
	bool draw_tabs = false;
	bool draw_spaces = false;

	// FIXME: Helper method to draw unfilled rects, should be moved to Renderer.
	void _draw_rect_unfilled(RID p_canvas_item, const Rect2& p_rect, const Color& p_color,
		real_t p_width = -1.0, bool p_antialiased = false) const;

	/* Theme. */
	Ref<StyleBox> _get_current_stylebox() const;

	/*** Super internal Core API. Everything builds on it. ***/
	bool text_changed_dirty = false;
	void _text_changed();
	void _emit_text_changed();

	void _base_insert_text(
		int p_line, int p_char, const String& p_text, int& r_end_line, int& r_end_column);
	String _base_get_text(int p_from_line, int p_from_column, int p_to_line, int p_to_column) const;
	void _base_remove_text(int p_from_line, int p_from_column, int p_to_line, int p_to_column);

	/* Input actions. */
	void _swap_current_input_direction();
	void _move_caret_left(bool p_select, bool p_move_by_word = false);
	void _move_caret_right(bool p_select, bool p_move_by_word = false);
	void _move_caret_up(bool p_select);
	void _move_caret_down(bool p_select);
	void _move_caret_to_line_start(bool p_select);
	void _move_caret_to_line_end(bool p_select);
	void _move_caret_page_up(bool p_select);
	void _move_caret_page_down(bool p_select);
	void _move_caret_document_start(bool p_select);
	void _move_caret_document_end(bool p_select);
	bool _clear_carets_and_selection();

protected:
	void _notification(int p_what);

#ifndef DISABLE_DEPRECATED
	void _set_selection_mode_bind_compat_86978(
		SelectionMode p_mode, int p_line = -1, int p_column = -1, int p_caret = 0);
	Point2i _get_line_column_at_pos_bind_compat_100913(
		const Point2i& p_pos, bool p_allow_out_of_bounds = true) const;
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

	void _draw_guidelines();

	virtual void _update_theme_item_cache() override;

	virtual String _get_accessibility_name() const override;

	// Brace matching.
	struct BraceMatchingData
	{
		int open_match_line = -1;
		int open_match_column = -1;
		bool open_matching = false;
		bool open_mismatch = false;
		int close_match_line = -1;
		int close_match_column = -1;
		bool close_matching = false;
		bool close_mismatch = false;
	};

	bool highlight_matching_braces_enabled = false;

	// Line hiding.
	bool hiding_enabled = false;

	void _set_hiding_enabled(bool p_enabled);
	bool _is_hiding_enabled() const;

	void _set_line_as_hidden(int p_line, bool p_hidden);
	bool _is_line_hidden(int p_line) const;

	void _unhide_all_lines();
	void _unhide_carets();

	int _get_wrapped_indent_level(int p_line, int& r_first_wrap) const;
	float _get_wrap_indent_offset(int p_line, int p_wrap_index, bool p_rtl) const;

	// Symbol lookup.
	String lookup_symbol_word;
	void _set_symbol_lookup_word(const String& p_symbol);

	/* Text manipulation */
	void _copy_internal(int p_caret);

public:
	/* General overrides. */
	virtual Size2 get_minimum_size() const override;
	virtual bool is_text_field() const override;
	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	/* Text */
	// Text properties.
	RID get_text_canvas_item() const;

	bool has_ime_text() const;
	void cancel_ime();

	void set_editable(bool p_editable);
	bool is_editable() const;

	void set_text_direction(TextDirection p_text_direction);
	TextDirection get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_tab_size(const int p_size);
	int get_tab_size() const;

	void set_indent_wrapped_lines(bool p_enabled);
	bool is_indent_wrapped_lines() const;

	void set_tab_input_mode(bool p_enabled);
	bool get_tab_input_mode() const;

	// User controls
	void set_overtype_mode_enabled(bool p_enabled);
	bool is_overtype_mode_enabled() const;

	void set_context_menu_enabled(bool p_enabled);
	bool is_context_menu_enabled() const;

	void show_emoji_and_symbol_picker();

	void set_emoji_menu_enabled(bool p_enabled);
	bool is_emoji_menu_enabled() const;

	void set_backspace_deletes_composite_character_enabled(bool p_enabled);
	bool is_backspace_deletes_composite_character_enabled() const;

	void set_shortcut_keys_enabled(bool p_enabled);
	bool is_shortcut_keys_enabled() const;

	void set_virtual_keyboard_enabled(bool p_enabled);
	bool is_virtual_keyboard_enabled() const;

	void set_virtual_keyboard_show_on_focus(bool p_show_on_focus);
	bool get_virtual_keyboard_show_on_focus() const;

	void set_middle_mouse_paste_enabled(bool p_enabled);
	bool is_middle_mouse_paste_enabled() const;

	void set_empty_selection_clipboard_enabled(bool p_enabled);
	bool is_empty_selection_clipboard_enabled() const;

	// Text manipulation
	void clear();

	void _set_text(const String& p_text, bool p_emit_signal = false);
	void set_text(const String& p_text);
	String get_text() const;

	int get_line_count() const;

	void set_placeholder(const String& p_text);
	String get_placeholder() const;

	String get_line(int p_line) const;
	String get_line_with_ime(int p_line) const;

	int get_line_width(int p_line, int p_wrap_index = -1) const;
	int get_line_height() const;

	int get_indent_level(int p_line) const;
	int get_first_non_whitespace_column(int p_line) const;

	void remove_line_at(int p_line, bool p_move_carets_down = true);

	int get_last_unhidden_line() const;
	int get_next_visible_line_offset_from(int p_line_from, int p_visible_amount) const;
	Point2i get_next_visible_line_index_offset_from(
		int p_line_from, int p_wrap_index_from, int p_visible_amount) const;

	// Overridable actions
	void copy(int p_caret = -1);

	// Context menu.
	PopupMenu* get_menu() const;
	bool is_menu_visible() const;
	void menu_option(int p_option);

	/* Versioning */
	void start_action(EditAction p_action);
	void end_action();
	EditAction get_current_action() const;

	void begin_complex_operation();
	void end_complex_operation();

	bool has_undo() const;
	bool has_redo() const;
	void undo();
	void redo();
	void clear_undo_history();

	bool is_insert_text_operation() const;

	void tag_saved_version();

	uint32_t get_version() const;
	uint32_t get_saved_version() const;

	/* Search */
	void set_search_text(const String& p_search_text);
	void set_search_flags(uint32_t p_flags);

	Point2i search(
		const String& p_key, uint32_t p_search_flags, int p_from_line, int p_from_column) const;

	/* Mouse */
	Point2 get_local_mouse_pos() const;

	String get_word_at_pos(const Vector2& p_pos) const;
	String get_word(int p_line, int p_column) const;

	Point2i get_line_column_at_pos(
		const Point2i& p_pos, bool p_clamp_line = true, bool p_clamp_column = true) const;
	Point2i get_pos_at_line_column(int p_line, int p_column) const;
	Rect2i get_rect_at_line_column(int p_line, int p_column) const;
	int get_line_start_margin() const;

	int get_minimap_line_at_pos(const Point2i& p_pos) const;

	bool is_dragging_cursor() const;
	bool is_mouse_over_selection(bool p_edges = true, int p_caret = -1) const;

	/* Caret */
	void set_caret_type(CaretType p_type);
	CaretType get_caret_type() const;

	bool is_caret_blink_enabled() const;

	float get_caret_blink_interval() const;

	void set_draw_caret_when_editable_disabled(bool p_enable);
	bool is_drawing_caret_when_editable_disabled() const;

	void set_move_caret_on_right_click_enabled(bool p_enabled);
	bool is_move_caret_on_right_click_enabled() const;

	void set_caret_mid_grapheme_enabled(bool p_enabled);
	bool is_caret_mid_grapheme_enabled() const;

	void set_multiple_carets_enabled(bool p_enabled);
	bool is_multiple_carets_enabled() const;

	int add_caret(int p_line, int p_column);
	void remove_caret(int p_caret);
	void remove_drag_caret();
	void remove_secondary_carets();
	int get_caret_count() const;
	void add_caret_at_carets(bool p_below);

	Vector<int> get_sorted_carets(bool p_include_ignored_carets = false) const;
	void collapse_carets(int p_from_line, int p_from_column, int p_to_line, int p_to_column,
		bool p_inclusive = false);

	void merge_overlapping_carets();
	void begin_multicaret_edit();
	void end_multicaret_edit();
	bool is_in_mulitcaret_edit() const;
	bool multicaret_edit_ignore_caret(int p_caret) const;

	bool is_caret_visible(int p_caret = 0) const;
	Point2 get_caret_draw_pos(int p_caret = 0) const;

	void set_caret_line(int p_line, bool p_adjust_viewport = true, bool p_can_be_hidden = true,
		int p_wrap_index = 0, int p_caret = 0);
	int get_caret_line(int p_caret = 0) const;

	void set_caret_column(int p_column, bool p_adjust_viewport = true, int p_caret = 0);
	int get_caret_column(int p_caret = 0) const;
	int get_next_composite_character_column(int p_line, int p_column) const;
	int get_previous_composite_character_column(int p_line, int p_column) const;

	int get_caret_wrap_index(int p_caret = 0) const;

	String get_word_under_caret(int p_caret = -1) const;

	/* Selection. */
	void set_selecting_enabled(bool p_enabled);
	bool is_selecting_enabled() const;

	void set_deselect_on_focus_loss_enabled(bool p_enabled);
	bool is_deselect_on_focus_loss_enabled() const;

	void set_drag_and_drop_selection_enabled(bool p_enabled);
	bool is_drag_and_drop_selection_enabled() const;

	void set_selection_mode(SelectionMode p_mode);
	SelectionMode get_selection_mode() const;

	void select_all();
	void select_word_under_caret(int p_caret = -1);
	void add_selection_for_next_occurrence();
	void skip_selection_for_next_occurrence();
	void select(int p_origin_line, int p_origin_column, int p_caret_line, int p_caret_column,
		int p_caret = 0);

	bool has_selection(int p_caret = -1) const;

	String get_selected_text(int p_caret = -1);
	int get_selection_at_line_column(
		int p_line, int p_column, bool p_include_edges = true, bool p_only_selections = true) const;
	Vector<Point2i> get_line_ranges_from_carets(
		bool p_only_selections = false, bool p_merge_adjacent = true) const;

	void set_selection_origin_line(
		int p_line, bool p_can_be_hidden = true, int p_wrap_index = -1, int p_caret = 0);
	void set_selection_origin_column(int p_column, int p_caret = 0);
	int get_selection_origin_line(int p_caret = 0) const;
	int get_selection_origin_column(int p_caret = 0) const;

	int get_selection_from_line(int p_caret = 0) const;
	int get_selection_from_column(int p_caret = 0) const;
	int get_selection_to_line(int p_caret = 0) const;
	int get_selection_to_column(int p_caret = 0) const;

	bool is_caret_after_selection_origin(int p_caret = 0) const;

	void deselect(int p_caret = -1);

	void set_selection_handle_enabled(bool p_enabled);
	bool is_selection_handle_enabled() const;

	/* Line wrapping. */
	void set_line_wrapping_mode(LineWrappingMode p_wrapping_mode);
	LineWrappingMode get_line_wrapping_mode() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	bool is_line_wrapped(int p_line) const;
	int get_line_wrap_count(int p_line) const;
	int get_line_wrap_index_at_column(int p_line, int p_column) const;

	Vector<String> get_line_wrapped_text(int p_line) const;

	/* Viewport. */
	// Scrolling.
	void set_smooth_scroll_enabled(bool p_enabled);
	bool is_smooth_scroll_enabled() const;

	void set_scroll_past_end_of_file_enabled(bool p_enabled);
	bool is_scroll_past_end_of_file_enabled() const;

	ScrollBar* get_v_scroll_bar() const;
	ScrollBar* get_h_scroll_bar() const;

	void set_v_scroll(double p_scroll);
	double get_v_scroll() const;

	void set_h_scroll(int p_scroll);
	int get_h_scroll() const;

	void set_v_scroll_speed(float p_speed);
	float get_v_scroll_speed() const;

	void set_fit_content_height_enabled(bool p_enabled);
	bool is_fit_content_height_enabled() const;

	void set_fit_content_width_enabled(bool p_enabled);
	bool is_fit_content_width_enabled() const;

	double get_scroll_pos_for_line(int p_line, int p_wrap_index = 0) const;

	// Visible lines.
	void set_line_as_first_visible(int p_line, int p_wrap_index = 0);
	int get_first_visible_line() const;

	void set_line_as_center_visible(int p_line, int p_wrap_index = 0);

	void set_line_as_last_visible(int p_line, int p_wrap_index = 0);
	int get_last_full_visible_line() const;
	int get_last_full_visible_line_wrap_index() const;

	int get_visible_line_count() const;
	int get_visible_line_count_in_range(int p_from, int p_to) const;
	int get_total_visible_line_count() const;

	// Auto Adjust
	bool is_line_in_viewport(int p_line) const;
	void adjust_viewport_to_caret(int p_caret = 0);
	void center_viewport_to_caret(int p_caret = 0);

	// Minimap
	void set_draw_minimap(bool p_enabled);
	bool is_drawing_minimap() const;

	void set_minimap_width(int p_minimap_width);
	int get_minimap_width() const;

	int get_minimap_visible_lines() const;

	/* Gutters. */
	void add_gutter(int p_at = -1);
	void remove_gutter(int p_gutter);
	int get_gutter_count() const;

	Vector2i get_hovered_gutter() const { return hovered_gutter; }

	void set_gutter_name(int p_gutter, const String& p_name);
	String get_gutter_name(int p_gutter) const;

	void set_gutter_type(int p_gutter, GutterType p_type);
	GutterType get_gutter_type(int p_gutter) const;

	void set_gutter_width(int p_gutter, int p_width);
	int get_gutter_width(int p_gutter) const;
	int get_total_gutter_width() const;

	void set_gutter_draw(int p_gutter, bool p_draw);
	bool is_gutter_drawn(int p_gutter) const;

	void set_gutter_clickable(int p_gutter, bool p_clickable);
	bool is_gutter_clickable(int p_gutter) const;

	void set_gutter_overwritable(int p_gutter, bool p_overwritable);
	bool is_gutter_overwritable(int p_gutter) const;

	void merge_gutters(int p_from_line, int p_to_line);

	void set_line_gutter_text(int p_line, int p_gutter, const String& p_text);
	String get_line_gutter_text(int p_line, int p_gutter) const;

	void set_line_gutter_icon(int p_line, int p_gutter, const Ref<Texture2D>& p_icon);
	Ref<Texture2D> get_line_gutter_icon(int p_line, int p_gutter) const;

	void set_line_gutter_item_color(int p_line, int p_gutter, const Color& p_color);
	Color get_line_gutter_item_color(int p_line, int p_gutter) const;

	void set_line_gutter_clickable(int p_line, int p_gutter, bool p_clickable);
	bool is_line_gutter_clickable(int p_line, int p_gutter) const;

	// Line style
	void set_line_background_color(int p_line, const Color& p_color);
	Color get_line_background_color(int p_line) const;

	/* Syntax Highlighting. */
	void set_syntax_highlighter(Ref<CodeHighlighter> p_syntax_highlighter);
	Ref<CodeHighlighter> get_syntax_highlighter() const;

	/* Code Mode & Code Features. */
	void set_code_mode(bool p_enabled);
	bool is_code_mode() const { return code_mode; }

	// Indent management
	void set_indent_size(const int p_size);
	int get_indent_size() const;

	void set_indent_using_spaces(const bool p_use_spaces);
	bool is_indent_using_spaces() const;

	void set_auto_indent_enabled(bool p_enabled);
	bool is_auto_indent_enabled() const;

	void set_auto_indent_prefixes(const Vector<String>& p_prefixes);
	Vector<String> get_auto_indent_prefixes() const;

	// Auto brace completion
	void set_auto_brace_completion_enabled(bool p_enabled);
	bool is_auto_brace_completion_enabled() const;

	void set_highlight_matching_braces_enabled(bool p_enabled);
	bool is_highlight_matching_braces_enabled() const;

	void add_auto_brace_completion_pair(const String& p_open_key, const String& p_close_key);

	bool has_auto_brace_completion_open_key(const String& p_open_key) const;
	bool has_auto_brace_completion_close_key(const String& p_close_key) const;

	String get_auto_brace_completion_close_key(const String& p_open_key) const;

	// Main Gutter
	void set_draw_breakpoints_gutter(bool p_draw);
	bool is_drawing_breakpoints_gutter() const;

	void set_draw_bookmarks_gutter(bool p_draw);
	bool is_drawing_bookmarks_gutter() const;

	void set_draw_executing_lines_gutter(bool p_draw);
	bool is_drawing_executing_lines_gutter() const;

	void set_line_as_breakpoint(int p_line, bool p_breakpointed);
	bool is_line_breakpointed(int p_line) const;
	void clear_breakpointed_lines();
	PackedInt32Array get_breakpointed_lines() const;

	void set_line_as_bookmarked(int p_line, bool p_bookmarked);
	bool is_line_bookmarked(int p_line) const;
	void clear_bookmarked_lines();
	PackedInt32Array get_bookmarked_lines() const;

	void set_line_as_executing(int p_line, bool p_executing);
	bool is_line_executing(int p_line) const;
	void clear_executing_lines();
	PackedInt32Array get_executing_lines() const;

	// Line numbers
	void set_draw_line_numbers(bool p_draw);
	bool is_draw_line_numbers_enabled() const;
	void set_line_numbers_zero_padded(bool p_zero_padded);
	bool is_line_numbers_zero_padded() const;
	void set_line_numbers_min_digits(int p_count);
	int get_line_numbers_min_digits() const;

	// Fold gutter
	void set_draw_fold_gutter(bool p_draw);
	bool is_drawing_fold_gutter() const;

	// Line Folding
	void set_line_folding_enabled(bool p_enabled);
	bool is_line_folding_enabled() const;

	bool can_fold_line(int p_line) const;

	void fold_line(int p_line);
	void unfold_line(int p_line);
	void fold_all_lines();
	void unfold_all_lines();
	void toggle_foldable_line(int p_line);
	void toggle_foldable_lines_at_carets();

	int get_folded_line_header(int p_line) const;
	bool is_line_folded(int p_line) const;
	Vector<int> get_folded_lines_bind() const;
	PackedInt32Array get_folded_lines() const;

	// Code region
	void create_code_region();
	String get_code_region_start_tag() const;
	String get_code_region_end_tag() const;
	void set_code_region_tags(const String& p_start = "region", const String& p_end = "endregion");
	bool is_line_code_region_start(int p_line) const;
	bool is_line_code_region_end(int p_line) const;

	// Delimiters
	void add_string_delimiter(
		const String& p_start_key, const String& p_end_key, bool p_line_only = false);
	void remove_string_delimiter(const String& p_start_key);
	bool has_string_delimiter(const String& p_start_key) const;

	void set_string_delimiters(const Vector<String>& p_string_delimiters);
	void clear_string_delimiters();
	Vector<String> get_string_delimiters() const;

	int is_in_string(int p_line, int p_column = -1) const;

	void add_comment_delimiter(
		const String& p_start_key, const String& p_end_key, bool p_line_only = false);
	void remove_comment_delimiter(const String& p_start_key);
	bool has_comment_delimiter(const String& p_start_key) const;

	void set_comment_delimiters(const Vector<String>& p_comment_delimiters);
	void clear_comment_delimiters();
	Vector<String> get_comment_delimiters() const;

	int is_in_comment(int p_line, int p_column = -1) const;

	String get_delimiter_start_key(int p_delimiter_idx) const;
	String get_delimiter_end_key(int p_delimiter_idx) const;

	Point2 get_delimiter_start_position(int p_line, int p_column) const;
	Point2 get_delimiter_end_position(int p_line, int p_column) const;

	// Code hint
	void set_code_hint(const String& p_hint);
	void set_code_hint_draw_below(bool p_below);

	// Code Completion
	void set_code_completion_enabled(bool p_enable);
	bool is_code_completion_enabled() const;

	void set_code_completion_prefixes(const Vector<String>& p_prefixes);
	Vector<String> get_code_completion_prefixes() const;

	String get_text_for_code_completion() const;

	void request_code_completion(bool p_force = false);
	void update_code_completion_options(bool p_forced = false);

	int get_code_completion_selected_index() const;
	void set_code_completion_selected_index(int p_index);

	void confirm_code_completion(bool p_replace = false);
	void cancel_code_completion();

	// Line length guidelines
	void set_line_length_guidelines(Vector<int> p_guideline_columns);
	Vector<int> get_line_length_guidelines() const;

	// Symbol lookup
	void set_symbol_lookup_on_click_enabled(bool p_enabled);
	bool is_symbol_lookup_on_click_enabled() const;

	String get_text_for_symbol_lookup() const;
	String get_text_with_cursor_char(int p_line, int p_column) const;
	String get_lookup_word(int p_line, int p_column) const;

	void set_symbol_lookup_word_as_valid(bool p_valid);

	// Symbol tooltip
	void set_symbol_tooltip_on_hover_enabled(bool p_enabled);
	bool is_symbol_tooltip_on_hover_enabled() const;

	/* Visual. */
	void set_highlight_current_line(bool p_enabled);
	bool is_highlight_current_line_enabled() const;

	void set_highlight_all_occurrences(bool p_enabled);
	bool is_highlight_all_occurrences_enabled() const;

	void set_draw_control_chars(bool p_enabled);
	bool get_draw_control_chars() const;

	void set_draw_tabs(bool p_enabled);
	bool is_drawing_tabs() const;

	void set_draw_spaces(bool p_enabled);
	bool is_drawing_spaces() const;

	Color get_font_color() const;

	/* Behavior */

	String get_default_word_separators() const;

	void set_use_default_word_separators(bool p_enabled);
	bool is_default_word_separators_enabled() const;

	void set_custom_word_separators(const String& p_separators);
	void set_use_custom_word_separators(bool p_enabled);
	bool is_custom_word_separators_enabled() const;

	String get_custom_word_separators() const;

	/* Deprecated. */
#ifndef DISABLE_DEPRECATED
	Vector<int> get_caret_index_edit_order();
	void adjust_carets_after_edit(
		int p_caret, int p_from_line, int p_from_col, int p_to_line, int p_to_col);

	int get_selection_line(int p_caret = 0) const;
	int get_selection_column(int p_caret = 0) const;
#endif

	TextEdit(const String& p_placeholder = String());
	template <typename T, typename = std::enable_if_t<std::is_same_v<T, bool>>>
	explicit TextEdit(T p_code_mode) : TextEdit(String())
	{
		if (p_code_mode) {
			set_code_mode(true);
		}
	}
	~TextEdit();
};

// --- Folded from scene/gui/tree.h ---
#include "scene/gui/control.h"
#include "scene/resources/text_line.h"
#include "servers/display/accessibility_server.h"

class LineEdit;
class Popup;
class PopupMenu;
class ScrollBar;
class Slider;
class TextEdit;
class Timer;
class Tree;

class TreeItem
{
public:
	enum TreeCellMode
	{
		CELL_MODE_STRING,
		CELL_MODE_CHECK,
		CELL_MODE_RANGE,
		CELL_MODE_ICON,
		CELL_MODE_CUSTOM,
	};

private:
	friend class Tree;

	struct Cell
	{
		mutable RID accessibility_cell_element;
		TreeCellMode mode = TreeItem::CELL_MODE_STRING;

		Ref<Texture2D> icon;
		Ref<Texture2D> icon_overlay;
		Rect2i icon_region;
		String text;
		String xl_text;
		Node::AutoTranslateMode auto_translate_mode = Node::AUTO_TRANSLATE_MODE_INHERIT;
		String description;
		bool edit_multiline = false;
		String suffix;
		Ref<TextParagraph> text_buf;
		String language;
		TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
		Control::TextDirection text_direction = Control::TEXT_DIRECTION_INHERITED;
		TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
		uint32_t autowrap_trim_flags =
			TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
		bool dirty = true;
		double min = 0.0;
		double max = 100.0;
		double step = 1.0;
		double val = 0.0;
		int icon_max_w = 0;
		bool expr = false;
		bool checked = false;
		bool indeterminate = false;
		bool editable = false;
		bool selected = false;
		bool selectable = true;
		bool custom_color = false;
		Color color;
		bool custom_bg_color = false;
		bool custom_bg_outline = false;
		Color bg_color;
		bool custom_button = false;
		bool expand_right = false;
		Color icon_color = Color(1, 1, 1);
		Ref<StyleBox> custom_stylebox;

		Rect2 focus_rect;
		Size2i cached_minimum_size;
		bool cached_minimum_size_dirty = true;

		HorizontalAlignment text_alignment = HORIZONTAL_ALIGNMENT_LEFT;

		String tooltip;

		struct Button
		{
			mutable RID accessibility_button_element;
			int id = 0;
			bool disabled = false;
			Ref<Texture2D> texture;
			Color color = Color(1, 1, 1, 1);
			String tooltip;
			String description;
		};

		Vector<Button> buttons;

		Ref<Font> custom_font;
		int custom_font_size = -1;

		Cell()
		{
			text_buf.instantiate();
			text_buf->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		}

		Size2 get_icon_size() const;
		void draw_icon(const RID& p_where, const Point2& p_pos, const Size2& p_size = Size2(),
			const Rect2i& p_region = Rect2i(), const Color& p_color = Color()) const;
	};

	mutable RID accessibility_row_element;
	mutable bool accessibility_row_dirty = true;

	Vector<Cell> cells;

	Rect2 focus_rect;
	bool collapsed = false; // Won't show children.
	bool visible = true;
	bool parent_visible_in_tree = true;
	bool disable_folding = false;
	bool accepts_children = true;
	int custom_min_height = 0;

	TreeItem* parent = nullptr; // Parent item.
	TreeItem* prev = nullptr;	// Previous in list.
	TreeItem* next = nullptr;	// Next in list.
	TreeItem* first_child = nullptr;
	TreeItem* last_child = nullptr;

	int cached_start = 0;
	int cached_label_height = 0;
	int cached_total_end = 0;
	Point2 sticky_offset;

	LocalVector<TreeItem*> children_cache;
	bool is_root = false; // For tree root.
	Tree* tree = nullptr; // Tree (for reference).

	void _changed_notify(int p_cell);
	void _changed_notify();
	void _cell_selected(int p_cell, bool p_set_as_cursor);
	void _cell_deselected(int p_cell);
	void _handle_visibility_changed(bool p_visible);
	void _propagate_visibility_changed(bool p_parent_visible_in_tree);

	void _change_tree(Tree* p_tree);

	_FORCE_INLINE_ void _create_children_cache()
	{
		if (children_cache.is_empty()) {
			TreeItem* c = first_child;
			while (c) {
				children_cache.push_back(c);
				c = c->next;
			}
		}
	}

	_FORCE_INLINE_ void _unlink_from_tree()
	{
		if (accessibility_row_element.is_valid()) {
			AccessibilityServer::free_element(accessibility_row_element);
			accessibility_row_element = RID();
		}
		for (Cell& cell : cells) {
			if (cell.accessibility_cell_element.is_valid()) {
				AccessibilityServer::free_element(cell.accessibility_cell_element);
				cell.accessibility_cell_element = RID();
			}
			for (Cell::Button& btn : cell.buttons) {
				if (btn.accessibility_button_element.is_valid()) {
					AccessibilityServer::free_element(
						btn.accessibility_button_element);
					btn.accessibility_button_element = RID();
				}
			}
		}
		TreeItem* p = get_prev();
		if (p) {
			p->next = next;
		}
		if (next) {
			next->prev = p;
		}
		if (parent) {
			if (!parent->children_cache.is_empty()) {
				parent->children_cache.erase(this);
			}
			if (parent->first_child == this) {
				parent->first_child = next;
			}
			if (parent->last_child == this) {
				parent->last_child = prev;
			}
		}
	}

	bool _is_any_collapsed(bool p_only_visible);

protected:
#ifndef DISABLE_DEPRECATED
	void _select_bind_compat_119367(int p_column);
	static void _bind_compatibility_methods();
#endif

public:
	TreeItem(Tree* p_tree);
	// Cell mode.
	void set_cell_mode(int p_column, TreeCellMode p_mode);
	TreeCellMode get_cell_mode(int p_column) const;

	// Auto translate mode.
	void set_auto_translate_mode(int p_column, Node::AutoTranslateMode p_mode);
	Node::AutoTranslateMode get_auto_translate_mode(int p_column) const;

	// Multiline editable.
	void set_edit_multiline(int p_column, bool p_multiline);
	bool is_edit_multiline(int p_column) const;

	// Check mode.
	void set_checked(int p_column, bool p_checked);
	void set_indeterminate(int p_column, bool p_indeterminate);
	bool is_checked(int p_column) const;
	bool is_indeterminate(int p_column) const;

	void propagate_check(int p_column, bool p_emit_signal = true);

	String atr(int p_column, const String& p_text) const;

private:
	// Check helpers.
	void _propagate_check_through_children(int p_column, bool p_checked, bool p_emit_signal);
	void _propagate_check_through_parents(int p_column, bool p_emit_signal);

	TreeItem* _get_prev_in_tree(bool p_wrap = false, bool p_include_invisible = false);
	TreeItem* _get_next_in_tree(bool p_wrap = false, bool p_include_invisible = false);

public:
	String get_text(int p_column) const;

	String get_description(int p_column) const;

	void set_text_direction(int p_column, Control::TextDirection p_text_direction);
	Control::TextDirection get_text_direction(int p_column) const;

	void set_autowrap_mode(int p_column, TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode(int p_column) const;

	void set_autowrap_trim_flags(int p_column, uint32_t p_flags);
	uint32_t get_autowrap_trim_flags(int p_column) const;

	void set_text_overrun_behavior(int p_column, TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior(int p_column) const;

	void set_structured_text_bidi_override(int p_column, TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override(int p_column) const;

	void set_language(int p_column, const String& p_language);
	String get_language(int p_column) const;

	void set_suffix(int p_column, String p_suffix);
	String get_suffix(int p_column) const;

	void set_icon(int p_column, const Ref<Texture2D>& p_icon);
	Ref<Texture2D> get_icon(int p_column) const;

	void set_icon_overlay(int p_column, const Ref<Texture2D>& p_icon_overlay);
	Ref<Texture2D> get_icon_overlay(int p_column) const;

	void set_icon_region(int p_column, const Rect2& p_icon_region);
	Rect2 get_icon_region(int p_column) const;

	void set_icon_modulate(int p_column, const Color& p_modulate);
	Color get_icon_modulate(int p_column) const;

	void set_icon_max_width(int p_column, int p_max);
	int get_icon_max_width(int p_column) const;

	void clear_buttons();
	int get_button_count(int p_column) const;
	String get_button_tooltip_text(int p_column, int p_index) const;
	Ref<Texture2D> get_button(int p_column, int p_index) const;
	int get_button_id(int p_column, int p_index) const;
	int get_button_by_id(int p_column, int p_id) const;
	Color get_button_color(int p_column, int p_index) const;
	void set_button_tooltip_text(int p_column, int p_index, const String& p_tooltip);
	void set_button(int p_column, int p_index, const Ref<Texture2D>& p_button);
	void set_button_color(int p_column, int p_index, const Color& p_color);
	void set_button_disabled(int p_column, int p_index, bool p_disabled);
	bool is_button_disabled(int p_column, int p_index) const;

	// Range works for mode number or mode combo.
	void set_range(int p_column, double p_value);
	double get_range(int p_column) const;

	void set_range_config(
		int p_column, double p_min, double p_max, double p_step, bool p_exp = false);
	void get_range_config(int p_column, double& r_min, double& r_max, double& r_step) const;
	bool is_range_exponential(int p_column) const;

	void set_collapsed(bool p_collapsed);
	bool is_collapsed();

	void set_collapsed_recursive(bool p_collapsed);
	bool is_any_collapsed(bool p_only_visible = false);

	void set_visible(bool p_visible);
	bool is_visible();
	bool is_visible_in_tree() const;

	void uncollapse_tree();

	void set_custom_minimum_height(int p_height);
	int get_custom_minimum_height() const;

	void set_custom_stylebox(int p_column, const Ref<StyleBox>& p_stylebox);
	Ref<StyleBox> get_custom_stylebox(int p_column) const;

	void set_selectable(int p_column, bool p_selectable);
	bool is_selectable(int p_column) const;

	bool is_selected(int p_column);
	bool is_any_column_selected() const;
	void select(int p_column, bool p_set_as_cursor = true);
	void deselect(int p_column);
	void set_as_cursor(int p_column);

	void set_editable(int p_column, bool p_editable);
	bool is_editable(int p_column);

	void set_custom_color(int p_column, const Color& p_color);
	Color get_custom_color(int p_column) const;
	void clear_custom_color(int p_column);

	void set_custom_font(int p_column, const Ref<Font>& p_font);
	Ref<Font> get_custom_font(int p_column) const;

	void set_custom_font_size(int p_column, int p_font_size);
	int get_custom_font_size(int p_column) const;

	void set_custom_bg_color(int p_column, const Color& p_color, bool p_bg_outline = false);
	void clear_custom_bg_color(int p_column);
	Color get_custom_bg_color(int p_column) const;

	void set_custom_as_button(int p_column, bool p_button);
	bool is_custom_set_as_button(int p_column) const;

	void set_tooltip_text(int p_column, const String& p_tooltip);
	String get_tooltip_text(int p_column) const;

	void set_text_alignment(int p_column, HorizontalAlignment p_alignment);
	HorizontalAlignment get_text_alignment(int p_column) const;

	void set_expand_right(int p_column, bool p_enable);
	bool get_expand_right(int p_column) const;

	void set_disable_folding(bool p_disable);
	bool is_folding_disabled() const;

	void set_accept_children(bool p_allowed);
	bool is_accepting_children() const;

	Size2 get_minimum_size(int p_column);

	// Item manipulation.
	TreeItem* create_child(int p_index = -1);
	void add_child(TreeItem* p_item);
	void remove_child(TreeItem* p_item);

	Tree* get_tree() const;

	TreeItem* get_prev();
	TreeItem* get_next() const;
	TreeItem* get_parent() const;
	TreeItem* get_first_child() const;
	TreeItem* get_last_child() const;

	TreeItem* get_prev_in_tree(bool p_wrap = false);
	TreeItem* get_next_in_tree(bool p_wrap = false);

	TreeItem* get_prev_visible(bool p_wrap = false);
	TreeItem* get_next_visible(bool p_wrap = false);

	TreeItem* get_child(int p_index);
	int get_visible_child_count();
	int get_child_count();
	Vector<TreeItem*> get_children();
	void clear_children();
	int get_index();

#ifdef DEV_ENABLED
	// This debugging code can be removed once the current refactoring of this class is complete.
	void validate_cache() const;
#else
	void validate_cache() const {}
#endif

	void move_before(TreeItem* p_item);
	void move_after(TreeItem* p_item);

	~TreeItem();
};

class Tree : public Control
{
public:
	enum SelectMode
	{
		SELECT_SINGLE,
		SELECT_ROW,
		SELECT_MULTI
	};

	enum DropModeFlags
	{
		DROP_MODE_DISABLED = 0,
		DROP_MODE_ON_ITEM = 1,
		DROP_MODE_INBETWEEN = 2
	};

	enum ScrollHintMode
	{
		SCROLL_HINT_MODE_DISABLED,
		SCROLL_HINT_MODE_BOTH,
		SCROLL_HINT_MODE_TOP,
		SCROLL_HINT_MODE_BOTTOM,
	};

private:
	friend class TreeItem;

	TreeItem* root = nullptr;
	TreeItem* popup_edited_item = nullptr;
	TreeItem* selected_item = nullptr;
	TreeItem* edited_item = nullptr;
	TreeItem* shift_anchor = nullptr;

	TreeItem* popup_pressing_edited_item = nullptr; // Candidate.
	int popup_pressing_edited_item_column = -1;

	TreeItem* drop_mode_over = nullptr;
	int drop_mode_section = 0;
	bool drop_mode_unchanged = false;
	bool dragging_within_self = false;

	TreeItem* single_select_defer = nullptr;
	int single_select_defer_column = 0;

	int pressed_button = -1;
	bool pressing_for_editor = false;
	Vector2 pressing_pos;

	Vector2 hovered_pos = Vector2(-1.0, -1.0);
	bool is_mouse_hovering = false;

	float range_drag_base = 0.0;
	bool range_drag_enabled = false;
	Vector2 range_drag_capture_pos;

	bool propagate_mouse_activated = false;
	float content_scale_factor = 0.0;

	int sticky_stack_end = 0;
	LocalVector<TreeItem*> sticky_candidates;
	LocalVector<TreeItem*> sticky_list;

	Rect2 custom_popup_rect;
	int edited_col = -1;
	int selected_col = -1;
	int selected_button = -1;
	int popup_edited_item_col = -1;
	bool hide_root = false;
	SelectMode select_mode = SELECT_SINGLE;

	int blocked = 0;

	int drop_mode_flags = 0;
	static constexpr int COLUMN_NOT_FOUND = -100;

	struct ColumnInfo
	{
		mutable RID accessibility_col_element;
		int custom_min_width = 0;
		int expand_ratio = 1;
		bool expand = true;
		bool clip_content = false;
		String title_tooltip;
		String title;
		String xl_title;
		HorizontalAlignment title_alignment = HORIZONTAL_ALIGNMENT_CENTER;
		Ref<TextParagraph> text_buf;
		String language;
		Control::TextDirection text_direction = Control::TEXT_DIRECTION_INHERITED;

		mutable int cached_minimum_width = 0;
		mutable bool cached_minimum_width_dirty = true;

		ColumnInfo() { text_buf.instantiate(); }
	};

	bool show_column_titles = false;

	bool popup_edit_committed = true;
	int scroll_pending = 0;
	RID accessibility_scroll_element;
	RID stylebox_ci;	// Separate canvas item for drawing native styleboxes.
	RID custom_ci;		// Separate canvas item for drawing custom content.
	RID header_ci;		// Separate canvas item for drawing column headers.
	RID content_ci;		// Separate canvas item for drawing tree rows.
	RID last_sticky_ci; // Separate canvas item for drawing the last sticky item, which can
						// partially hide underneath other sticky items.
	RID drop_indicator_ci;

	Control* popup_editor_vb = nullptr;
	Popup* popup_editor = nullptr;
	LineEdit* line_editor = nullptr;
	TextEdit* text_editor = nullptr;
	Slider* value_editor = nullptr;
	bool updating_value_editor = false;
	uint64_t focus_in_id = 0;
	PopupMenu* popup_menu = nullptr;

	Vector<ColumnInfo> columns;

	Timer* range_click_timer = nullptr;
	TreeItem* range_item_last = nullptr;
	bool range_up_last = false;
	void _range_click_timeout();

	int compute_item_height(TreeItem* p_item) const;
	int get_item_height(TreeItem* p_item) const;
	Point2i convert_rtl_position(const Point2i& p_pos, int p_width = 0) const;
	Point2 convert_rtl_position(const Point2& p_pos, int p_width = 0) const;
	Rect2i convert_rtl_rect(const Rect2i& p_rect) const;
	void _update_all();
	void update_column(int p_col);
	void update_item_cell(TreeItem* p_item, int p_col) const;
	void update_item_cache(TreeItem* p_item) const;
	void draw_item_rect(const TreeItem::Cell& p_cell, const Rect2i& p_rect, const Color& p_color,
		const Color& p_icon_color, int p_ol_size, const Color& p_ol_color, const RID& p_ci) const;
	int draw_item(const Point2i& p_pos, const Point2& p_draw_ofs, const Size2& p_draw_size,
		TreeItem* p_item, int& r_self_height, const RID& p_ci);
	void select_single_item(TreeItem* p_selected, TreeItem* p_current, int p_col,
		TreeItem* p_prev = nullptr, bool* r_in_range = nullptr, bool p_force_deselect = false);
	int propagate_mouse_event(const Point2i& p_pos, int x_ofs, int y_ofs, int x_limit,
		bool p_double_click, TreeItem* p_item, MouseButton p_button, const InputEvent& p_mod,
		bool p_first_call = true, bool p_skip_children = false);
	void _line_editor_submit(String p_text);
	void _apply_multiline_edit(bool p_hide_focus = false);
	void _text_editor_popup_modal_close();
	void _text_editor_gui_input(const InputEvent& p_event);
	void value_editor_changed(double p_value);
	void _update_popup_menu(const TreeItem::Cell& p_cell);

	void popup_select(int p_option);

	void item_edited(
		int p_column, TreeItem* p_item, MouseButton p_custom_mouse_index = MouseButton::NONE);
	void item_changed(int p_column, TreeItem* p_item);
	void item_selected(int p_column, TreeItem* p_item, bool p_set_as_cursor);
	void item_deselected(int p_column, TreeItem* p_item);
	void update_min_size_for_item_change();

	void propagate_set_columns(TreeItem* p_item);

	struct ThemeCache
	{
		Ref<StyleBox> panel_style;
		Ref<StyleBox> focus_style;

		Ref<Font> font;
		Ref<Font> tb_font;
		int font_size = 0;
		int tb_font_size = 0;

		Ref<StyleBox> hovered;
		Ref<StyleBox> hovered_dimmed;
		Ref<StyleBox> hovered_selected;
		Ref<StyleBox> hovered_selected_focus;
		Ref<StyleBox> selected;
		Ref<StyleBox> selected_focus;
		Ref<StyleBox> cursor;
		Ref<StyleBox> cursor_unfocus;
		Ref<StyleBox> button_hover;
		Ref<StyleBox> button_pressed;
		Ref<StyleBox> title_button;
		Ref<StyleBox> title_button_hover;
		Ref<StyleBox> title_button_pressed;
		Ref<StyleBox> custom_button;
		Ref<StyleBox> custom_button_hover;
		Ref<StyleBox> custom_button_pressed;

		Color title_button_color;

		Ref<Texture2D> checked;
		Ref<Texture2D> unchecked;
		Ref<Texture2D> checked_disabled;
		Ref<Texture2D> unchecked_disabled;
		Ref<Texture2D> indeterminate;
		Ref<Texture2D> indeterminate_disabled;
		Ref<Texture2D> arrow;
		Ref<Texture2D> arrow_collapsed;
		Ref<Texture2D> arrow_collapsed_mirrored;
		Ref<Texture2D> select_arrow;
		Ref<Texture2D> updown;
		Ref<Texture2D> scroll_hint;

		Color font_color;
		Color font_hovered_color;
		Color font_hovered_dimmed_color;
		Color font_hovered_selected_color;
		Color font_selected_color;
		Color font_disabled_color;
		Color guide_color;
		Color drop_on_item_color;
		Color drop_position_color;
		Color relationship_line_color;
		Color parent_hl_line_color;
		Color children_hl_line_color;
		Color custom_button_font_highlight;
		Color font_outline_color;
		Color scroll_hint_color;

		float base_scale = 1.0;
		int font_outline_size = 0;

		int h_separation = 0;
		int v_separation = 0;
		int inner_item_margin_bottom = 0;
		int inner_item_margin_left = 0;
		int inner_item_margin_right = 0;
		int inner_item_margin_top = 0;
		int item_margin = 0;
		int check_h_separation = 0;
		int icon_h_separation = 0;
		int button_margin = 0;
		int icon_max_width = 0;
		Point2 offset;

		int draw_relationship_lines = 0;
		int relationship_line_width = 0;
		int parent_hl_line_width = 0;
		int children_hl_line_width = 0;
		int parent_hl_line_margin = 0;
		int draw_guides = 0;

		int dragging_unfold_wait_msec = 500;

		int scroll_border = 0;
		int scroll_speed = 0;
		int scroll_max_sticky_items = 5;

		int scrollbar_margin_top = -1;
		int scrollbar_margin_right = -1;
		int scrollbar_margin_bottom = -1;
		int scrollbar_margin_left = -1;
		int scrollbar_h_separation = 0;
		int scrollbar_v_separation = 0;
	} theme_cache;

	struct Cache
	{
		enum ClickType
		{
			CLICK_NONE,
			CLICK_TITLE,
			CLICK_BUTTON,
		};

		ClickType click_type = Cache::CLICK_NONE;
		int click_index = -1;
		int click_id = -1;
		TreeItem* click_item = nullptr;
		int click_column = 0;
		int hover_header_column = -1;
		bool hover_header_row = false;
		Point2 click_pos;

		TreeItem* hover_item = nullptr;
		int hover_column = -1;
		int hover_button_index_in_column = -1;

		bool rtl = false;
		int font_height = -1;
	} cache;

	int _get_title_button_height() const;
	Size2 _get_cell_icon_size(const TreeItem::Cell& p_cell) const;

	void _scroll_moved(float p_value);
	ScrollBar* h_scroll = nullptr;
	ScrollBar* v_scroll = nullptr;

	bool h_scroll_enabled = true;
	bool v_scroll_enabled = true;

	Size2 get_internal_min_size() const;

	Rect2 search_item_rect(TreeItem* p_from, TreeItem* p_item);
	uint64_t last_keypress = 0;
	String incr_search;
	bool cursor_can_exit_tree = true;
	void _do_incr_search(const String& p_add);
	void _incr_search_as_needed(const InputEvent& p_event_key);

	TreeItem* _search_item_text(TreeItem* p_at, const String& p_find, int* r_col, bool p_selectable,
		bool p_backwards = false);

	TreeItem* _find_item_at_pos(TreeItem* p_item, const Point2& p_pos, int& r_column, int& r_height,
		int& r_section, bool p_sticky_check = true) const;

	void _find_button_at_pos(
		const Point2& p_pos, TreeItem*& r_item, int& r_column, int& r_index, int& r_section) const;

	struct FindColumnButtonResult
	{
		int column_index = -1;
		int button_index = -1;
		int column_width = -1;
		int column_offset = -1;
		int pos_x = -1;
	};

	FindColumnButtonResult _find_column_and_button_at_pos(
		int p_x, const TreeItem* p_item, int p_x_ofs, int p_x_limit) const;

	float drag_speed = 0.0;
	float drag_from = 0.0;
	float drag_accum = 0.0;
	bool drag_touching = false;
	bool drag_touching_deaccel = false;
	bool click_handled = false;
	bool allow_rmb_select = false;
	bool scrolling = false;
	bool using_native_touch = true;

	ScrollHintMode scroll_hint_mode = SCROLL_HINT_MODE_DISABLED;
	bool tile_scroll_hint = false;

	bool allow_reselect = false;
	bool allow_search = true;

	bool force_edit_checkbox_only_on_checkbox = false;

	bool hide_folding = false;

	bool enable_recursive_folding = true;

	bool enable_drag_unfolding = true;
	Timer* dropping_unfold_timer = nullptr;
	void _on_dropping_unfold_timer_timeout();
	void _reset_drop_mode_over();

	bool enable_auto_tooltip = true;

	bool hovered_update_queued = false;
	void _determine_hovered_item();
	void _queue_update_hovered_item();

	int _count_selected_items(TreeItem* p_from) const;
	bool _is_branch_selected(TreeItem* p_from) const;
	bool _is_sibling_branch_selected(TreeItem* p_from) const;
	void _go_left();
	void _go_right();
	void _go_down();
	void _go_up();
	void _shift_select_range(TreeItem* new_item);

	bool _scroll(bool p_horizontal, float p_pages);

	Rect2 _get_scrollbar_layout_rect() const;
	Rect2 _get_content_rect() const; // Considering the background stylebox and scrollbars.
	Rect2 _get_item_focus_rect(const TreeItem* p_item) const;

	void _check_item_accessibility(
		TreeItem* p_item, PackedStringArray& r_warnings, int& r_row) const;

	void _accessibility_clean_info(TreeItem* p_item);
	void _accessibility_update_item(Point2& r_ofs, TreeItem* p_item, int& r_row, int p_level);

protected:
	virtual void _update_theme_item_cache() override;

	void _notification(int p_what);

public:
	PackedStringArray get_accessibility_configuration_warnings() const;
	virtual RID get_focused_accessibility_element() const override;

	virtual void set_self_modulate(const Color& p_self_modulate) override;

	virtual AutoTranslateMode get_tooltip_auto_translate_mode_at(const Point2& p_at) const override;

	TreeItem* get_item_at_position(const Point2& p_pos) const;
	int get_column_at_position(const Point2& p_pos) const;
	int get_drop_section_at_position(const Point2& p_pos) const;
	int get_button_id_at_position(const Point2& p_pos) const;

	void clear();

	TreeItem* create_item(TreeItem* p_parent = nullptr, int p_index = -1);
	TreeItem* get_root() const;
	TreeItem* get_last_item() const;

	void set_column_custom_minimum_width(int p_column, int p_min_width);
	void set_column_expand(int p_column, bool p_expand);
	void set_column_expand_ratio(int p_column, int p_ratio);
	void set_column_clip_content(int p_column, bool p_fit);
	int get_column_minimum_width(int p_column) const;
	int get_column_width(int p_column) const;
	int get_column_expand_ratio(int p_column) const;

	bool is_column_expanding(int p_column) const;
	bool is_column_clipping_content(int p_column) const;

	void set_hide_root(bool p_enabled);
	bool is_root_hidden() const;
	TreeItem* get_next_selected(TreeItem* p_item);
	TreeItem* get_selected() const;
	void set_selected(TreeItem* p_item, int p_column = 0);
	int get_selected_column() const;
	int get_pressed_button() const;
	void set_select_mode(SelectMode p_mode);
	SelectMode get_select_mode() const;
	void deselect_all();
	bool is_anything_selected();

	void set_columns(int p_columns);
	int get_columns() const;

	void set_column_title(int p_column, const String& p_title);
	String get_column_title(int p_column) const;

	void set_column_title_tooltip_text(int p_column, const String& p_tooltip);
	String get_column_title_tooltip_text(int p_column) const;

	void set_column_title_alignment(int p_column, HorizontalAlignment p_alignment);
	HorizontalAlignment get_column_title_alignment(int p_column) const;

	void set_column_title_direction(int p_column, Control::TextDirection p_text_direction);
	Control::TextDirection get_column_title_direction(int p_column) const;

	void set_column_title_language(int p_column, const String& p_language);
	String get_column_title_language(int p_column) const;

	void set_column_titles_visible(bool p_show);
	bool are_column_titles_visible() const;

	RID get_custom_drawing_canvas_item() const { return custom_ci; }

	TreeItem* get_edited() const;
	int get_edited_column() const;

	void ensure_cursor_is_visible();

	Rect2 get_custom_popup_rect() const;

	int get_item_offset(TreeItem* p_item) const;
	Rect2 get_item_rect(TreeItem* p_item, int p_column = -1, int p_button = -1) const;
	bool edit_selected(bool p_force_edit = false);
	bool is_editing();
	void set_editor_selection(int p_from_line, int p_to_line, int p_from_column = -1,
		int p_to_column = -1, int p_caret = 0);

	// First item that starts with the text, from the current focused item down and wraps around.
	TreeItem* search_item_text(
		const String& p_find, int* r_col = nullptr, bool p_selectable = false);
	// First item that matches the whole text, from the first item down.
	TreeItem* get_item_with_text(const String& p_find) const;

	Point2 get_scroll() const;
	void set_h_scroll_enabled(bool p_enable);
	bool is_h_scroll_enabled() const;
	void set_v_scroll_enabled(bool p_enable);
	bool is_v_scroll_enabled() const;

	void set_scroll_hint_mode(ScrollHintMode p_mode);
	ScrollHintMode get_scroll_hint_mode() const;

	void set_tile_scroll_hint(bool p_enable);
	bool is_scroll_hint_tiled();

	void set_cursor_can_exit_tree(bool p_enable);

	ScrollBar* get_vscroll_bar() { return v_scroll; }

	void set_hide_folding(bool p_hide);
	bool is_folding_hidden() const;

	void set_enable_recursive_folding(bool p_enable);
	bool is_recursive_folding_enabled() const;

	void set_enable_drag_unfolding(bool p_enable);
	bool is_drag_unfolding_enabled() const;

	void set_drop_mode_flags(int p_flags);
	int get_drop_mode_flags() const;

	void set_edit_checkbox_cell_only_when_checkbox_is_pressed(bool p_enable);
	bool get_edit_checkbox_cell_only_when_checkbox_is_pressed() const;

	void set_allow_rmb_select(bool p_allow);
	bool get_allow_rmb_select() const;

	void set_allow_reselect(bool p_allow);
	bool get_allow_reselect() const;

	void set_allow_search(bool p_allow);
	bool get_allow_search() const;

	void set_auto_tooltip(bool p_enable);
	bool is_auto_tooltip_enabled() const;

	Size2 get_minimum_size() const override;

	Tree() = default;
	~Tree();
};
