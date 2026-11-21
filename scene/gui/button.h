/**************************************************************************/
/*  button.h                                                              */
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

#include "core/input/shortcut.h"
#include "scene/gui/control.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/text_line.h"
#include "scene/resources/text_paragraph.h"

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
#include "scene/gui/popup_menu.h"

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
#include "scene/gui/popup_menu.h"
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
#include "scene/gui/popup_menu.h"
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
