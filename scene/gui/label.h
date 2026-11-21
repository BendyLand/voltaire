/**************************************************************************/
/*  label.h                                                               */
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
#include "scene/gui/scroll_bar.h"
#include "scene/property_list_helper.h"
#include "scene/resources/text_paragraph.h"

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
