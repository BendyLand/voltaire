/**************************************************************************/
/*  scroll_container.h                                                    */
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

#include "scene/gui/control.h"
#include "scene/gui/scroll_bar.h"

class Panel;
class TextureRect;

class ScrollContainer : public Control
{
public:
	enum ScrollMode
	{
		SCROLL_MODE_DISABLED = 0,
		SCROLL_MODE_AUTO,
		SCROLL_MODE_SHOW_ALWAYS,
		SCROLL_MODE_SHOW_NEVER,
		SCROLL_MODE_RESERVE,
		SCROLL_MODE_MAXIMIZE_FIRST,
	};

	enum ScrollHintMode
	{
		SCROLL_HINT_MODE_DISABLED,
		SCROLL_HINT_MODE_ALL,
		SCROLL_HINT_MODE_TOP_AND_LEFT,
		SCROLL_HINT_MODE_BOTTOM_AND_RIGHT,
	};

private:
	ScrollBar* h_scroll = nullptr;
	ScrollBar* v_scroll = nullptr;
	Panel* focus_panel = nullptr;

	mutable Size2 largest_child_min_size; // The largest one among the min sizes of all available
										  // child controls.

	void _update_scrollbars();

	Vector2 drag_speed;
	Vector2 drag_accum;
	Vector2 drag_from;
	Vector2 last_drag_accum;
	float time_since_motion = 0.0f;
	bool drag_touching = false;
	bool drag_touching_deaccel = false;
	bool beyond_deadzone = false;
	bool scroll_on_drag_hover = false;

	TextureRect* scroll_hint_top_left = nullptr;
	TextureRect* scroll_hint_bottom_right = nullptr;

	ScrollMode horizontal_scroll_mode = SCROLL_MODE_AUTO;
	ScrollMode vertical_scroll_mode = SCROLL_MODE_AUTO;

	int deadzone = 0;
	bool follow_focus = false;
	int scroll_border = 20;
	int scroll_speed = 12;
	bool scroll_horizontal_by_default = false;

	ScrollHintMode scroll_hint_mode = SCROLL_HINT_MODE_DISABLED;
	bool tile_scroll_hint = false;

	bool following = false;
	Vector2 scroll_diff;

	struct ThemeCache
	{
		Ref<StyleBox> panel_style;
		Ref<StyleBox> focus_style;

		Ref<Texture2D> scroll_hint_vertical;
		Ref<Texture2D> scroll_hint_horizontal;

		Color scroll_hint_vertical_color;
		Color scroll_hint_horizontal_color;

		int scrollbar_h_separation = 0;
		int scrollbar_v_separation = 0;
	} theme_cache;

	void _cancel_drag();

	bool _is_h_scroll_visible() const;
	bool _is_v_scroll_visible() const;

	Rect2 _get_margins() const;

	bool draw_focus_border = false;
	bool focus_border_is_drawn = false;
	bool child_has_focus();

	Size2 _get_minimum_size(bool p_use_desired_sizes) const;

	Rect2 _get_local_visible_rect() const;

protected:
	Size2 get_minimum_size() const override;
	Size2 get_desired_size() const override;
	Size2 get_inner_combined_maximum_size() const override;

	void _gui_focus_changed(Control* p_control);
	void _reposition_children();

	void _notification(int p_what);

	bool _updating_scrollbars = false;
	void _update_scrollbar_position();
	void _scroll_moved(float);

public:
	void set_h_scroll(int p_pos);
	int get_h_scroll() const;

	void set_v_scroll(int p_pos);
	int get_v_scroll() const;

	void set_horizontal_custom_step(float p_custom_step);
	float get_horizontal_custom_step() const;

	void set_vertical_custom_step(float p_custom_step);
	float get_vertical_custom_step() const;

	void set_horizontal_scroll_mode(ScrollMode p_mode);
	ScrollMode get_horizontal_scroll_mode() const;

	void set_vertical_scroll_mode(ScrollMode p_mode);
	ScrollMode get_vertical_scroll_mode() const;

	void set_scroll_horizontal_by_default(bool p_enable);
	bool is_scroll_horizontal_by_default() const;

	void set_deadzone(int p_deadzone);
	int get_deadzone() const;

	ScrollHintMode get_scroll_hint_mode() const;

	void set_tile_scroll_hint(bool p_enable);
	bool is_scroll_hint_tiled();

	bool is_following_focus() const;
	void set_follow_focus(bool p_follow);

	void set_scroll_on_drag_hover(bool p_scroll);

	ScrollBar* get_h_scroll_bar();
	ScrollBar* get_v_scroll_bar();
	void ensure_control_visible(Control* p_control);

	PackedStringArray get_configuration_warnings() const override;

	void set_draw_focus_border(bool p_draw);
	bool get_draw_focus_border();

	ScrollContainer() = default;
};

// --- Folded from scene/gui/foldable_container.h ---
#include "scene/gui/control.h"

class FoldableGroup;
class TextLine;

class FoldableContainer : public Control
{
public:
	enum TitlePosition
	{
		POSITION_TOP,
		POSITION_BOTTOM,
		POSITION_MAX
	};

private:
	bool folded = false;
	String title;
	Ref<FoldableGroup> foldable_group;
	String language;
	TextDirection title_text_direction = TEXT_DIRECTION_AUTO;
	HorizontalAlignment title_alignment = HORIZONTAL_ALIGNMENT_LEFT;
	TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_NO_TRIMMING;
	TitlePosition title_position = POSITION_TOP;

	Ref<TextLine> text_buf;
	bool changing_group = false;
	bool is_hovering = false;
	mutable Vector2 title_minimum_size;

	LocalVector<Control*> title_controls;

	struct ThemeCache
	{
		Ref<StyleBox> title_style;
		Ref<StyleBox> title_hover_style;
		Ref<StyleBox> title_collapsed_style;
		Ref<StyleBox> title_collapsed_hover_style;
		Ref<StyleBox> panel_style;
		Ref<StyleBox> focus_style;

		Color title_font_color;
		Color title_hovered_font_color;
		Color title_collapsed_font_color;
		Color title_font_outline_color;

		Ref<Font> title_font;
		int title_font_size = 0;
		int title_font_outline_size = 0;

		Ref<Texture2D> expanded_arrow;
		Ref<Texture2D> expanded_arrow_mirrored;
		Ref<Texture2D> folded_arrow;
		Ref<Texture2D> folded_arrow_mirrored;

		int h_separation = 0;
	} theme_cache;

	Ref<StyleBox> _get_title_style() const;
	Ref<Texture2D> _get_title_icon() const;
	Rect2 _get_title_rect() const;

	int _get_h_separation() const { return MAX(theme_cache.h_separation, 0); }

	real_t _get_title_controls_width() const;

	void _update_title_min_size() const;
	void _shape();
	HorizontalAlignment _get_actual_alignment() const;
	void _update_group();
	void _draw_flippable_stylebox(const Ref<StyleBox> p_stylebox, const Rect2& p_rect);

protected:
	virtual bool has_point(const Point2& p_point) const override;
	virtual void gui_input(const InputEvent& p_event);
	virtual String get_tooltip(const Point2& p_pos) const;
	void _notification(int p_what);

public:
	void fold();
	void expand();

	void set_folded(bool p_folded);
	bool is_folded() const;

	void set_foldable_group(const Ref<FoldableGroup>& p_group);
	Ref<FoldableGroup> get_foldable_group() const;

	void set_title(const String& p_text);
	String get_title() const;

	void set_title_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_title_alignment() const;

	void set_title_text_direction(TextDirection p_text_direction);
	TextDirection get_title_text_direction() const;

	void set_title_text_overrun_behavior(TextServer::OverrunBehavior p_overrun_behavior);
	TextServer::OverrunBehavior get_title_text_overrun_behavior() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_title_position(TitlePosition p_title_position);
	TitlePosition get_title_position() const;

	void add_title_bar_control(Control* p_control);
	void remove_title_bar_control(Control* p_control);

	virtual Size2 get_minimum_size() const override;
	virtual Size2 get_desired_size() const override;
	virtual Size2 get_inner_combined_maximum_size() const override;

	virtual Vector<int> get_allowed_size_flags_horizontal() const override
	{
		return {SIZE_FILL, SIZE_SHRINK_BEGIN, SIZE_SHRINK_CENTER, SIZE_SHRINK_END};
	}

	virtual Vector<int> get_allowed_size_flags_vertical() const override
	{
		return {SIZE_FILL, SIZE_SHRINK_BEGIN, SIZE_SHRINK_CENTER, SIZE_SHRINK_END};
	}

	FoldableContainer(const String& p_text = String());
	~FoldableContainer();
};

class FoldableGroup : public Resource
{
	friend class FoldableContainer;

	HashSet<FoldableContainer*> containers;
	bool allow_folding_all = false;
	bool updating_group = false;

protected:

public:
	FoldableContainer* get_expanded_container() const;

	void get_containers(List<FoldableContainer*>* r_containers) const;

	void set_allow_folding_all(bool p_enabled);
	bool is_allow_folding_all() const;

	FoldableGroup();
};



// --- End scene/gui/foldable_container.h ---

// --- Folded from scene/gui/separator.h ---
#include "scene/gui/control.h"

class Separator : public Control
{
	struct ThemeCache
	{
		int separation = 0;
		Ref<StyleBox> separator_style;
	} theme_cache;

protected:
	Orientation orientation = Orientation::HORIZONTAL;

	void _notification(int p_what);

public:
	Orientation get_orientation() const { return orientation; }
	void set_orientation(Orientation p_orientation)
	{
		if (orientation != p_orientation) {
			orientation = p_orientation;
			update_minimum_size();
			queue_redraw();
		}
	}

	virtual Size2 get_minimum_size() const override;

	Separator(Orientation p_orientation = HORIZONTAL);
	~Separator();
};

// --- End scene/gui/separator.h ---

// --- Folded from scene/gui/split_container.h ---
#include "scene/gui/control.h"

class TextureRect;
class SplitContainer;

class SplitContainerMultiDragger : public Control
{
	bool dragging = false;
	int drag_from = 0;
	int start_drag_split_offset = 0;

protected:
	void _notification(int p_what);

public:
	int dragger_index = -1;
	SplitContainer* split_container = nullptr;

	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	void update_position();
	void stop_dragging();

	SplitContainerMultiDragger();
};

class SplitContainerDragger : public Control
{
	friend class SplitContainer;

	bool dragging = false;
	int drag_from = 0;
	int start_drag_split_offset = 0;
	bool mouse_inside = false;

	Rect2 split_bar_rect;
	TextureRect* touch_dragger = nullptr;

	void _touch_dragger_mouse_exited();
	void _touch_dragger_gui_input(const InputEvent& p_event);

protected:
	void _notification(int p_what);
	virtual String _get_accessibility_name() const override;

public:
	int dragger_index = -1;

	virtual CursorShape get_cursor_shape(const Point2& p_pos = Point2i()) const override;

	void set_touch_dragger_enabled(bool p_enabled);
	void update_touch_dragger();
	void stop_dragging();

	SplitContainerDragger();
};

class SplitContainer : public Control
{
	friend class SplitContainerDragger;
	friend class SplitContainerMultiDragger;

public:
	enum DraggerVisibility
	{
		DRAGGER_VISIBLE,
		DRAGGER_HIDDEN,
		DRAGGER_HIDDEN_COLLAPSED
	};

private:
	bool show_drag_area = false;
	int drag_area_margin_begin = 0;
	int drag_area_margin_end = 0;
	int drag_area_offset = 0;

	PackedInt32Array split_offsets;
	LocalVector<int> default_dragger_positions;
	LocalVector<int> dragger_positions;
	LocalVector<Control*> valid_children;
	LocalVector<SplitContainerDragger*> dragging_area_controls;

	bool vertical = false;
	bool collapsed = false;
	DraggerVisibility dragger_visibility = DRAGGER_VISIBLE;
	bool dragging_enabled = true;
	bool split_offset_pending = false;
	bool can_use_desired_sizes = false;
	bool initialized = false;

	bool touch_dragger_enabled = false;
	bool drag_nested_intersections = false;
	int force_show_grabber_icon = -1;

	struct ThemeCache
	{
		Color touch_dragger_color;
		Color touch_dragger_pressed_color;
		Color touch_dragger_hover_color;
		int separation = 0;
		int minimum_grab_thickness = 0;
		bool autohide = false;
		Ref<Texture2D> touch_dragger_icon;
		Ref<Texture2D> touch_dragger_icon_h;
		Ref<Texture2D> touch_dragger_icon_v;
		Ref<Texture2D> grabber_icon;
		Ref<Texture2D> grabber_icon_h;
		Ref<Texture2D> grabber_icon_v;
		float base_scale = 1.0;
		Ref<StyleBox> split_bar_background;
	} theme_cache;

	Ref<Texture2D> _get_grabber_icon() const;
	Ref<Texture2D> _get_touch_dragger_icon() const;
	Point2i _get_valid_range(int p_dragger_index) const;

	PackedInt32Array _get_desired_sizes() const;
	void _set_desired_sizes(const PackedInt32Array& p_desired_sizes, int p_priority_index = -1);

	void _update_default_dragger_positions();
	void _update_dragger_positions(int p_clamp_index = -1);
	int _get_separation() const;
	void _resort();
	void _update_draggers();
	void _on_child_visibility_changed(Control* p_control);
	void _add_valid_child(Control* p_control);
	void _remove_valid_child(Control* p_control);

	void _remove_nested_descendent(SplitContainer* p_nested_sc);
	void _update_nested_descendent(SplitContainer* p_nested_sc, Control* p_direct_child);
	void _update_nested_ancestors(bool p_remove = false);
	void _update_all_nested_descendents(Control* p_control, Control* p_first_child = nullptr);

	Size2 _get_minimum_size(bool p_use_desired_sizes) const;

protected:
	bool is_fixed = false;

	void _notification(int p_what);

	virtual void add_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;
	virtual void move_child_notify(Node* p_child) override;

#ifndef DISABLE_DEPRECATED
	void _clamp_split_offset_bind_compat_90411();
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	void set_split_offset(int p_offset, int p_index = 0);
	int get_split_offset(int p_index = 0) const;

	void set_split_offsets(const PackedInt32Array& p_offsets);
	PackedInt32Array get_split_offsets() const;

	void clamp_split_offset(int p_priority_index = 0);

	void set_collapsed(bool p_collapsed);
	bool is_collapsed() const;

	void set_dragger_visibility(DraggerVisibility p_visibility);
	DraggerVisibility get_dragger_visibility() const;

	void set_vertical(bool p_vertical);
	bool is_vertical() const;

	void set_dragging_enabled(bool p_enabled);
	bool is_dragging_enabled() const;

	virtual Size2 get_minimum_size() const override;
	virtual Size2 get_desired_size() const override;

	virtual Vector<int> get_allowed_size_flags_horizontal() const override;
	virtual Vector<int> get_allowed_size_flags_vertical() const override;

	void set_drag_area_margin_begin(int p_margin);
	int get_drag_area_margin_begin() const;

	void set_drag_area_margin_end(int p_margin);
	int get_drag_area_margin_end() const;

	void set_drag_area_offset(int p_offset);
	int get_drag_area_offset() const;

	void set_show_drag_area_enabled(bool p_enabled);
	bool is_show_drag_area_enabled() const;

	void set_touch_dragger_enabled(bool p_enabled);
	bool is_touch_dragger_enabled() const;

	void show_grabber_icon(int p_index);

	void set_drag_nested_intersections(bool p_enabled);
	bool is_dragging_nested_intersections() const;

#ifndef DISABLE_DEPRECATED
	Control* get_drag_area_control() { return dragging_area_controls[0]; }

	void _set_split_offset_first(int p_offset) { set_split_offset(p_offset); }

	int _get_split_offset_first() const { return get_split_offset(); }
#endif

	SplitContainer(bool p_vertical = false);
};



// --- End scene/gui/split_container.h ---

// --- Folded from scene/gui/subviewport_container.h ---
#include "scene/gui/control.h"

class SubViewportContainer : public Control
{
	bool stretch = false;
	int shrink = 1;
	bool mouse_target = false;

	void _notify_viewports(int p_notification);
	bool _is_propagated_in_gui_input(const InputEvent& p_event);
	void _send_event_to_viewports(const InputEvent& p_event);
	void _propagate_nonpositional_event(const InputEvent& p_event);

protected:
	void _notification(int p_what);

	virtual void add_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

public:
	void set_stretch(bool p_enable);
	bool is_stretch_enabled() const;

	virtual void input(const InputEvent& p_event) override;
	virtual void unhandled_input(const InputEvent& p_event) override;
	virtual void gui_input(const InputEvent& p_event);
	void set_stretch_shrink(int p_shrink);
	int get_stretch_shrink() const;
	void recalc_force_viewport_sizes();

	void set_mouse_target(bool p_enable);
	bool is_mouse_target_enabled();

	virtual Size2 get_minimum_size() const override;

	virtual Vector<int> get_allowed_size_flags_horizontal() const override;
	virtual Vector<int> get_allowed_size_flags_vertical() const override;

	PackedStringArray get_configuration_warnings() const override;

	SubViewportContainer();
};



// --- End scene/gui/subviewport_container.h ---

// --- Folded from scene/gui/tab_bar.h ---
#include "scene/gui/control.h"
#include "scene/property_list_helper.h"
#include "scene/resources/text_line.h"

class Timer;

class TabBar : public Control
{
public:
	enum AlignmentMode
	{
		ALIGNMENT_LEFT,
		ALIGNMENT_CENTER,
		ALIGNMENT_RIGHT,
		ALIGNMENT_MAX,
	};

	enum CloseButtonDisplayPolicy
	{
		CLOSE_BUTTON_SHOW_NEVER,
		CLOSE_BUTTON_SHOW_ACTIVE_ONLY,
		CLOSE_BUTTON_SHOW_ALWAYS,
		CLOSE_BUTTON_MAX
	};

	enum DrawMode
	{
		DRAW_NORMAL,
		DRAW_PRESSED,
		DRAW_HOVER,
		DRAW_DISABLED,
		DRAW_MAX,
	};

private:
	struct Tab
	{
		mutable RID accessibility_item_element;
		mutable bool accessibility_item_dirty = true;

		// Corresponds to color overrides for the DrawMode enum
		Color font_color_overrides[DrawMode::DRAW_MAX] = {
			Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0)};

		String text;
		String tooltip;

		String language;
		Control::TextDirection text_direction = Control::TEXT_DIRECTION_INHERITED;

		Ref<TextLine> text_buf;
		Ref<Texture2D> icon;
		int icon_max_width = 0;

		bool disabled = false;
		bool hidden = false;
		bool truncated = false;

		int ofs_cache = 0;
		int size_cache = 0;
		int size_text = 0;

		Ref<Texture2D> right_button;
		Rect2 rb_rect;
		Rect2 cb_rect;

		Tab() { text_buf.instantiate(); }

		Tab(bool p_dummy) {}
	};

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	int offset = 0;
	int max_drawn_tab = 0;
	int highlight_arrow = -1;
	bool buttons_visible = false;
	bool missing_right = false;
	Vector<Tab> tabs;
	int current = -1;
	int previous = -1;
	AlignmentMode tab_alignment = ALIGNMENT_LEFT;
	bool clip_tabs = true;
	int rb_hover = -1;
	bool rb_pressing = false;
	bool tab_style_v_flip = false;

	bool select_with_rmb = false;
	bool deselect_enabled = false;
	bool switch_on_release = false;

	int cb_hover = -1;
	bool cb_pressing = false;
	CloseButtonDisplayPolicy cb_displaypolicy = CLOSE_BUTTON_SHOW_NEVER;
	bool close_with_middle_mouse = true;

	int hover = -1; // Hovered tab.
	int max_width = 0;
	bool scrolling_enabled = true;
	bool drag_to_rearrange_enabled = false;
	bool dragging_valid_tab = false;
	bool scroll_to_selected = true;
	int tabs_rearrange_group = -1;
	bool switch_on_drag_hover = true;

	static const int CURRENT_TAB_UNINITIALIZED = -2;
	bool initialized = false;
	int queued_current = CURRENT_TAB_UNINITIALIZED;

	const float DEFAULT_GAMEPAD_EVENT_DELAY_MS = 0.5;
	const float GAMEPAD_EVENT_REPEAT_RATE_MS = 1.0 / 20;
	float gamepad_event_delay_ms = DEFAULT_GAMEPAD_EVENT_DELAY_MS;

	struct ThemeCache
	{
		int h_separation = 0;
		int tab_separation = 0;
		int icon_max_width = 0;
		int hover_switch_wait_msec = 500;

		Ref<StyleBox> tab_unselected_style;
		Ref<StyleBox> tab_hovered_style;
		Ref<StyleBox> tab_selected_style;
		Ref<StyleBox> tab_disabled_style;
		Ref<StyleBox> tab_focus_style;

		Ref<Texture2D> increment_icon;
		Ref<Texture2D> increment_hl_icon;
		Ref<Texture2D> decrement_icon;
		Ref<Texture2D> decrement_hl_icon;
		Ref<Texture2D> drop_mark_icon;
		Color drop_mark_color;

		Ref<Font> font;
		int font_size;
		int outline_size = 0;

		Color font_selected_color;
		Color font_hovered_color;
		Color font_unselected_color;
		Color font_disabled_color;
		Color font_outline_color;

		Color icon_selected_color;
		Color icon_hovered_color;
		Color icon_unselected_color;
		Color icon_disabled_color;

		Ref<Texture2D> close_icon;
		Ref<StyleBox> button_pressed_style;
		Ref<StyleBox> button_hl_style;
	} theme_cache;

	Timer* hover_switch_delay = nullptr;

	int get_tab_width(int p_idx) const;
	Size2 _get_tab_icon_size(int p_idx) const;
	void _ensure_no_over_offset();
	bool _can_deselect() const;

	void _update_hover();
	void _update_cache(bool p_update_hover = true);
	void _hover_switch_timeout();

	void _on_mouse_exited();

	void _shape(int p_tab);
	void _draw_tab(Ref<StyleBox>& p_tab_style, const Color& p_font_color, const Color& p_icon_color,
		int p_index, float p_x, bool p_focus);


protected:
	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}


	void _notification(int p_what);

	void _move_tab_from(TabBar* p_from_tabbar, int p_from_index, int p_to_index);

public:
	RID get_tab_accessibility_element(int p_tab) const;
	virtual RID get_focused_accessibility_element() const override;

	void _draw_tab_drop(RID p_canvas_item);

	void add_tab(const String& p_str = "", const Ref<Texture2D>& p_icon = Ref<Texture2D>());

	void set_tab_title(int p_tab, const String& p_title);
	String get_tab_title(int p_tab) const;

	void set_tab_tooltip(int p_tab, const String& p_tooltip);
	String get_tab_tooltip(int p_tab) const;

	void set_tab_text_direction(int p_tab, TextDirection p_text_direction);
	TextDirection get_tab_text_direction(int p_tab) const;

	void set_tab_language(int p_tab, const String& p_language);
	String get_tab_language(int p_tab) const;

	void set_tab_icon(int p_tab, const Ref<Texture2D>& p_icon);
	Ref<Texture2D> get_tab_icon(int p_tab) const;

	void set_tab_icon_max_width(int p_tab, int p_width);
	int get_tab_icon_max_width(int p_tab) const;

	void set_font_color_override_all(int p_tab, const Color& p_color);
	void set_font_color_override(int p_tab, DrawMode p_draw_mode, const Color& p_color);
	Color get_font_color_override(int p_tab, DrawMode p_draw_mode) const;

	void set_tab_disabled(int p_tab, bool p_disabled);
	bool is_tab_disabled(int p_tab) const;

	void set_tab_hidden(int p_tab, bool p_hidden);
	bool is_tab_hidden(int p_tab) const;

	void set_tab_button_icon(int p_tab, const Ref<Texture2D>& p_icon);
	Ref<Texture2D> get_tab_button_icon(int p_tab) const;

	int get_tab_idx_at_point(const Point2& p_point) const;
	int get_closest_tab_idx_to_point(const Point2& p_point) const;

	void set_tab_alignment(AlignmentMode p_alignment);
	AlignmentMode get_tab_alignment() const;

	void set_clip_tabs(bool p_clip_tabs);
	bool get_clip_tabs() const;

	void set_tab_style_v_flip(bool p_tab_style_v_flip);

	void move_tab(int p_from, int p_to);

	void set_tab_close_display_policy(CloseButtonDisplayPolicy p_policy);
	CloseButtonDisplayPolicy get_tab_close_display_policy() const;

	void set_close_with_middle_mouse(bool p_scroll_close);
	bool get_close_with_middle_mouse() const;

	void set_tab_count(int p_count);
	int get_tab_count() const;

	void set_current_tab(int p_current);
	int get_current_tab() const;
	int get_previous_tab() const;
	int get_hovered_tab() const;

	int get_previous_available(int p_idx = -1) const;
	int get_next_available(int p_idx = -1) const;

	bool select_previous_available();
	bool select_next_available();

	void set_tab_offset(int p_offset);
	int get_tab_offset() const;
	bool get_offset_buttons_visible() const;

	void remove_tab(int p_idx);

	void clear_tabs();

	void set_scrolling_enabled(bool p_enabled);
	bool get_scrolling_enabled() const;

	void set_drag_to_rearrange_enabled(bool p_enabled);
	bool get_drag_to_rearrange_enabled() const;
	void set_tabs_rearrange_group(int p_group_id);
	int get_tabs_rearrange_group() const;

	void set_scroll_to_selected(bool p_enabled);
	bool get_scroll_to_selected() const;

	void set_switch_on_drag_hover(bool p_enabled);
	bool get_switch_on_drag_hover() const;

	void set_select_with_rmb(bool p_enabled);
	bool get_select_with_rmb() const;

	void set_deselect_enabled(bool p_enabled);
	bool get_deselect_enabled() const;

	void ensure_tab_visible(int p_idx);

	void set_max_tab_width(int p_width);
	int get_max_tab_width() const;

	void set_switch_on_release(bool p_switch) { switch_on_release = p_switch; }

	Rect2 get_tab_rect(int p_tab) const;
	Size2 get_minimum_size() const override;
	Size2 get_desired_size() const override;

	TabBar();
};



// --- End scene/gui/tab_bar.h ---

// --- Folded from scene/gui/tab_container.h ---
#include "scene/gui/control.h"

#include "scene/property_list_helper.h"

class Button;
class Popup;

class TabContainer : public Control
{
public:
	enum TabPosition
	{
		POSITION_TOP,
		POSITION_BOTTOM,
		POSITION_MAX,
	};

private:
	Control* internal_container = nullptr;
	TabBar* tab_bar = nullptr;
	Button* popup_button = nullptr;

	bool tabs_visible = true;
	TabPosition tabs_position = POSITION_TOP;
	bool use_hidden_tabs_for_min_size = false;
	bool theme_changing = false;
	Vector<Control*> children_removing;
	bool drag_to_rearrange_enabled = false;
	// Set the default setup current tab to be an invalid index.
	int setup_current_tab = -2;
	bool updating_visibility = false;

	struct ThemeCache
	{
		int side_margin = 0;

		Ref<StyleBox> panel_style;
		Ref<StyleBox> tabbar_style;

		Ref<Texture2D> menu_icon;
		Ref<Texture2D> menu_hl_icon;

		// TabBar overrides.
		int icon_separation = 0;
		int tab_separation = 0;
		int icon_max_width = 0;
		int outline_size = 0;

		Ref<StyleBox> tab_unselected_style;
		Ref<StyleBox> tab_hovered_style;
		Ref<StyleBox> tab_selected_style;
		Ref<StyleBox> tab_disabled_style;
		Ref<StyleBox> tab_focus_style;

		Ref<Texture2D> increment_icon;
		Ref<Texture2D> increment_hl_icon;
		Ref<Texture2D> decrement_icon;
		Ref<Texture2D> decrement_hl_icon;
		Ref<Texture2D> drop_mark_icon;
		Color drop_mark_color;

		Color font_selected_color;
		Color font_hovered_color;
		Color font_unselected_color;
		Color font_disabled_color;
		Color font_outline_color;

		Color icon_selected_color;
		Color icon_hovered_color;
		Color icon_unselected_color;
		Color icon_disabled_color;

		Ref<Font> tab_font;
		int tab_font_size;
	} theme_cache;

	struct CachedTab
	{
		bool has_title = false;
		String title;
		Ref<Texture2D> icon;
		bool disabled = false;
		bool hidden = false;
	};

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	mutable Vector<CachedTab> pending_tabs;
	CachedTab* get_pending_tab(int p_idx) const;

	HashMap<Node*, RID> tab_panels;

	int _get_tab_height() const;
	Control* _as_tab_control(Node* p_child) const;
	Vector<Control*> _get_tab_controls() const;
	void _on_theme_changed();
	void _repaint_call_deferred();
	void _repaint();
	void _repaint_internal();
	void _refresh_tab_indices();
	void _refresh_tab_names();
	void _update_margins();
	void _on_tab_changed(int p_tab);
	void _on_tab_clicked(int p_tab);
	void _on_tab_hovered(int p_tab);
	void _on_tab_selected(int p_tab);
	void _on_tab_button_pressed(int p_tab);
	void _on_active_tab_rearranged(int p_tab);

	void _drag_move_tab(int p_from_index, int p_to_index);
	void _drag_move_tab_from(TabBar* p_from_tabbar, int p_from_index, int p_to_index);

	void _popup_button_hovered(bool p_hover);
	void _popup_button_pressed();

	Size2 _get_minimum_size(bool p_use_desired_sizes) const;

protected:
	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}

	void _notification(int p_what);
	virtual void add_child_notify(Node* p_child) override;
	virtual void move_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

public:
	virtual bool accessibility_override_tree_hierarchy() const override { return true; }

	Control* get_internal_container() { return internal_container; }

	TabBar* get_tab_bar() const;

	int get_tab_idx_at_point(const Point2& p_point) const;
	int get_tab_idx_from_control(Control* p_child) const;

	void set_tab_alignment(TabBar::AlignmentMode p_alignment);
	TabBar::AlignmentMode get_tab_alignment() const;

	void set_tabs_position(TabPosition p_tab_position);
	TabPosition get_tabs_position() const;

	void set_tab_focus_mode(FocusMode p_focus_mode);
	FocusMode get_tab_focus_mode() const;

	void set_clip_tabs(bool p_clip_tabs);
	bool get_clip_tabs() const;

	void set_tabs_visible(bool p_visible);
	bool are_tabs_visible() const;

#ifndef DISABLE_DEPRECATED
	void set_all_tabs_in_front(bool p_is_front);
	bool is_all_tabs_in_front() const;
#endif

	void set_tab_title(int p_tab, const String& p_title);
	String get_tab_title(int p_tab) const;

	void set_tab_tooltip(int p_tab, const String& p_tooltip);
	String get_tab_tooltip(int p_tab) const;

	void set_tab_icon(int p_tab, const Ref<Texture2D>& p_icon);
	Ref<Texture2D> get_tab_icon(int p_tab) const;

	void set_tab_icon_max_width(int p_tab, int p_width);
	int get_tab_icon_max_width(int p_tab) const;

	void set_tab_disabled(int p_tab, bool p_disabled);
	bool is_tab_disabled(int p_tab) const;

	void set_tab_hidden(int p_tab, bool p_hidden);
	bool is_tab_hidden(int p_tab) const;


	void set_tab_button_icon(int p_tab, const Ref<Texture2D>& p_icon);
	Ref<Texture2D> get_tab_button_icon(int p_tab) const;

	int get_tab_count() const;
	void set_current_tab(int p_current);
	int get_current_tab() const;
	int get_previous_tab() const;

	bool select_previous_available();
	bool select_next_available();

	void set_deselect_enabled(bool p_enabled);
	bool get_deselect_enabled() const;

	Control* get_tab_control(int p_idx) const;
	Control* get_current_tab_control() const;

	virtual Size2 get_minimum_size() const override;
	virtual Size2 get_inner_combined_maximum_size() const override;
	virtual Size2 get_desired_size() const override;

	void set_popup(Node* p_popup);
	Popup* get_popup() const;

	void move_tab_from_tab_container(TabContainer* p_from, int p_from_index, int p_to_index = -1);

	void set_switch_on_drag_hover(bool p_enabled);
	bool get_switch_on_drag_hover() const;
	void set_drag_to_rearrange_enabled(bool p_enabled);
	bool get_drag_to_rearrange_enabled() const;
	void set_tabs_rearrange_group(int p_group_id);
	int get_tabs_rearrange_group() const;
	void set_use_hidden_tabs_for_min_size(bool p_use_hidden_tabs);
	bool get_use_hidden_tabs_for_min_size() const;

	virtual Vector<int> get_allowed_size_flags_horizontal() const override;
	virtual Vector<int> get_allowed_size_flags_vertical() const override;

	TabContainer();
};



// --- End scene/gui/tab_container.h ---
