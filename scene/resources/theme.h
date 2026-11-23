/**************************************************************************/
/*  theme.h                                                               */
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

#include "core/io/resource.h"
#include "scene/resources/font.h"
// --- Folded from scene/resources/style_box.h ---
#include "core/io/resource.h"
#include "scene/resources/texture.h"

class CanvasItem;

class StyleBox : public Resource
{
public:
	enum StyleType
	{
		STYLE_EMPTY,
		STYLE_FLAT,
		STYLE_TEXTURE,
		STYLE_LINE,
	};

	enum AxisStretchMode
	{
		AXIS_STRETCH_MODE_STRETCH,
		AXIS_STRETCH_MODE_TILE,
		AXIS_STRETCH_MODE_TILE_FIT,
	};

private:
	StyleType style_type = STYLE_EMPTY;
	float content_margin[4];

	// Shared by STYLE_FLAT and STYLE_TEXTURE.
	real_t expand_margin[4] = {};
	bool draw_center = true;

	// STYLE_FLAT fields.
	Color bg_color = Color(0.6, 0.6, 0.6);
	Color shadow_color = Color(0, 0, 0, 0.6);
	Color border_color = Color(0.8, 0.8, 0.8);
	real_t border_width[4] = {};
	real_t corner_radius[4] = {};
	bool blend_border = false;
	Vector2 skew;
	bool anti_aliased = true;
	int corner_detail = 8;
	int shadow_size = 0;
	Point2 shadow_offset;
	real_t aa_size = 1;

	// STYLE_TEXTURE fields.
	float texture_margin[4] = {};
	Rect2 region_rect;
	Ref<Texture2D> texture;
	Color modulate = Color(1, 1, 1, 1);
	AxisStretchMode axis_h = AXIS_STRETCH_MODE_STRETCH;
	AxisStretchMode axis_v = AXIS_STRETCH_MODE_STRETCH;

	// STYLE_LINE fields.
	Color color;
	int thickness = 1;
	bool vertical = false;
	float grow_begin = 1.0;
	float grow_end = 1.0;

	void _draw_flat(RID p_canvas_item, const Rect2& p_rect) const;
	void _draw_texture(RID p_canvas_item, const Rect2& p_rect) const;
	void _draw_line(RID p_canvas_item, const Rect2& p_rect) const;

protected:
	float get_style_margin(Side p_side) const;

public:
	void set_style_type(StyleType p_type);
	StyleType get_style_type() const { return style_type; }

	Size2 get_minimum_size() const;

	void set_content_margin(Side p_side, float p_value);
	void set_content_margin_all(float p_value);
	void set_content_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_content_margin(Side p_side) const;

	float get_margin(Side p_side) const;
	Point2 get_offset() const;

	void draw(RID p_canvas_item, const Rect2& p_rect) const;
	Rect2 get_draw_rect(const Rect2& p_rect) const;

	CanvasItem* get_current_item_drawn() const;

	bool test_mask(const Point2& p_point, const Rect2& p_rect) const;

	// Shared STYLE_FLAT / STYLE_TEXTURE properties.
	void set_expand_margin(Side p_expand_side, float p_size);
	void set_expand_margin_all(float p_expand_margin_size);
	void set_expand_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_expand_margin(Side p_expand_side) const;

	void set_draw_center(bool p_enabled);
	bool is_draw_center_enabled() const;

	// STYLE_FLAT properties.
	void set_bg_color(const Color& p_color);
	Color get_bg_color() const;

	void set_border_color(const Color& p_color);
	Color get_border_color() const;

	void set_border_width_all(int p_size);
	int get_border_width_min() const;

	void set_border_width(Side p_side, int p_width);
	int get_border_width(Side p_side) const;

	void set_border_blend(bool p_blend);
	bool get_border_blend() const;

	void set_corner_radius_all(int radius);
	void set_corner_radius_individual(const int radius_top_left, const int radius_top_right,
		const int radius_bottom_right, const int radius_bottom_left);
	void set_corner_radius(Corner p_corner, const int radius);
	int get_corner_radius(Corner p_corner) const;

	void set_corner_detail(const int& p_corner_detail);
	int get_corner_detail() const;

	void set_skew(Vector2 p_skew);
	Vector2 get_skew() const;

	void set_shadow_color(const Color& p_color);
	Color get_shadow_color() const;

	void set_shadow_size(const int& p_size);
	int get_shadow_size() const;

	void set_shadow_offset(const Point2& p_offset);
	Point2 get_shadow_offset() const;

	void set_anti_aliased(const bool& p_anti_aliased);
	bool is_anti_aliased() const;
	void set_aa_size(const real_t p_aa_size);
	real_t get_aa_size() const;

	// STYLE_TEXTURE properties.
	void set_texture(Ref<Texture2D> p_texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_margin(Side p_side, float p_size);
	void set_texture_margin_all(float p_size);
	void set_texture_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_texture_margin(Side p_side) const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_h_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_h_axis_stretch_mode() const;

	void set_v_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_v_axis_stretch_mode() const;

	void set_modulate(const Color& p_modulate);
	Color get_modulate() const;

	// STYLE_LINE properties.
	void set_color(const Color& p_color);
	Color get_color() const;

	void set_thickness(int p_thickness);
	int get_thickness() const;

	void set_vertical(bool p_vertical);
	bool is_vertical() const;

	void set_grow_begin(float p_grow);
	float get_grow_begin() const;

	void set_grow_end(float p_grow);
	float get_grow_end() const;

	StyleBox(StyleType p_type = STYLE_EMPTY);
};
// --- End scene/resources/style_box.h ---
#include "scene/resources/texture.h"

class Theme : public Resource
{
#ifdef TOOLS_ENABLED
	friend class ThemeItemImportTree;
	friend class ThemeItemEditorDialog;
	friend class ThemeTypeEditor;
#endif

public:
	using ThemeIconMap = HashMap<String, Ref<Texture2D>>;
	using ThemeStyleMap = HashMap<String, Ref<StyleBox>>;
	using ThemeFontMap = HashMap<String, Ref<Font>>;
	using ThemeFontSizeMap = HashMap<String, int>;
	using ThemeColorMap = HashMap<String, Color>;
	using ThemeConstantMap = HashMap<String, int>;

	enum DataType
	{
		DATA_TYPE_COLOR,
		DATA_TYPE_CONSTANT,
		DATA_TYPE_FONT,
		DATA_TYPE_FONT_SIZE,
		DATA_TYPE_ICON,
		DATA_TYPE_STYLEBOX,
		DATA_TYPE_MAX
	};

private:
	bool no_change_propagation = false;

	void _emit_theme_changed(bool p_notify_list_changed = false);

	Vector<String> _get_icon_list(const String& p_theme_type) const;
	Vector<String> _get_icon_type_list() const;
	Vector<String> _get_stylebox_list(const String& p_theme_type) const;
	Vector<String> _get_stylebox_type_list() const;
	Vector<String> _get_font_list(const String& p_theme_type) const;
	Vector<String> _get_font_type_list() const;
	Vector<String> _get_font_size_list(const String& p_theme_type) const;
	Vector<String> _get_font_size_type_list() const;
	Vector<String> _get_color_list(const String& p_theme_type) const;
	Vector<String> _get_color_type_list() const;
	Vector<String> _get_constant_list(const String& p_theme_type) const;
	Vector<String> _get_constant_type_list() const;

	Vector<String> _get_theme_item_list(DataType p_data_type, const String& p_theme_type) const;
	Vector<String> _get_theme_item_type_list(DataType p_data_type) const;

	Vector<String> _get_type_variation_list(const String& p_theme_type) const;
	Vector<String> _get_type_list() const;

protected:
	// Default values configurable for each individual theme.
	float default_base_scale = 0.0;
	Ref<Font> default_font;
	int default_font_size = -1;

	HashMap<String, ThemeIconMap> icon_map;
	HashMap<String, ThemeStyleMap> style_map;
	HashMap<String, ThemeFontMap> font_map;
	HashMap<String, ThemeFontSizeMap> font_size_map;
	HashMap<String, ThemeColorMap> color_map;
	HashMap<String, ThemeConstantMap> constant_map;
	HashMap<String, String> variation_map;
	HashMap<String, List<String>> variation_base_map;


	void _freeze_change_propagation();
	void _unfreeze_and_propagate_changes();

	virtual void reset_state() override;

public:
	static bool is_valid_type_name(const String& p_name);
	static bool is_valid_item_name(const String& p_name);
	static String validate_type_name(const String& p_name);

	void set_default_base_scale(float p_base_scale);
	float get_default_base_scale() const;
	bool has_default_base_scale() const;

	void set_default_font(const Ref<Font>& p_default_font);
	Ref<Font> get_default_font() const;
	bool has_default_font() const;

	void set_default_font_size(int p_font_size);
	int get_default_font_size() const;
	bool has_default_font_size() const;

	void set_icon(
		const String& p_name, const String& p_theme_type, const Ref<Texture2D>& p_icon);
	virtual Ref<Texture2D> get_icon(const String& p_name, const String& p_theme_type) const;
	bool has_icon(const String& p_name, const String& p_theme_type) const;
	bool has_icon_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_icon(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_icon(const String& p_name, const String& p_theme_type);
	void get_icon_list(const String& p_theme_type, List<String>* p_list) const;
	void add_icon_type(const String& p_theme_type);
	void remove_icon_type(const String& p_theme_type);
	void rename_icon_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_icon_type_list(List<String>* p_list) const;

	virtual Ref<StyleBox> get_stylebox(
		const String& p_name, const String& p_theme_type) const;
	bool has_stylebox(const String& p_name, const String& p_theme_type) const;
	bool has_stylebox_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_stylebox(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_stylebox(const String& p_name, const String& p_theme_type);
	void get_stylebox_list(const String& p_theme_type, List<String>* p_list) const;
	void add_stylebox_type(const String& p_theme_type);
	void remove_stylebox_type(const String& p_theme_type);
	void rename_stylebox_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_stylebox_type_list(List<String>* p_list) const;

	void set_font(
		const String& p_name, const String& p_theme_type, const Ref<Font>& p_font);
	virtual Ref<Font> get_font(const String& p_name, const String& p_theme_type) const;
	bool has_font(const String& p_name, const String& p_theme_type) const;
	bool has_font_no_default(const String& p_name, const String& p_theme_type) const;
	bool has_font_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_font(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_font(const String& p_name, const String& p_theme_type);
	void get_font_list(const String& p_theme_type, List<String>* p_list) const;
	void add_font_type(const String& p_theme_type);
	void remove_font_type(const String& p_theme_type);
	void rename_font_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_font_type_list(List<String>* p_list) const;

	void set_font_size(const String& p_name, const String& p_theme_type, int p_font_size);
	virtual int get_font_size(const String& p_name, const String& p_theme_type) const;
	bool has_font_size(const String& p_name, const String& p_theme_type) const;
	bool has_font_size_no_default(const String& p_name, const String& p_theme_type) const;
	bool has_font_size_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_font_size(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_font_size(const String& p_name, const String& p_theme_type);
	void get_font_size_list(const String& p_theme_type, List<String>* p_list) const;
	void add_font_size_type(const String& p_theme_type);
	void remove_font_size_type(const String& p_theme_type);
	void rename_font_size_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_font_size_type_list(List<String>* p_list) const;

	void set_color(const String& p_name, const String& p_theme_type, const Color& p_color);
	virtual Color get_color(const String& p_name, const String& p_theme_type) const;
	bool has_color(const String& p_name, const String& p_theme_type) const;
	bool has_color_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_color(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_color(const String& p_name, const String& p_theme_type);
	void get_color_list(const String& p_theme_type, List<String>* p_list) const;
	void add_color_type(const String& p_theme_type);
	void remove_color_type(const String& p_theme_type);
	void rename_color_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_color_type_list(List<String>* p_list) const;

	void set_constant(const String& p_name, const String& p_theme_type, int p_constant);
	virtual int get_constant(const String& p_name, const String& p_theme_type) const;
	bool has_constant(const String& p_name, const String& p_theme_type) const;
	bool has_constant_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_constant(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_constant(const String& p_name, const String& p_theme_type);
	void get_constant_list(const String& p_theme_type, List<String>* p_list) const;
	void add_constant_type(const String& p_theme_type);
	void remove_constant_type(const String& p_theme_type);
	void rename_constant_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_constant_type_list(List<String>* p_list) const;

	bool has_theme_item(
		DataType p_data_type, const String& p_name, const String& p_theme_type) const;
	bool has_theme_item_nocheck(
		DataType p_data_type, const String& p_name, const String& p_theme_type) const;
	void rename_theme_item(DataType p_data_type, const String& p_old_name,
		const String& p_name, const String& p_theme_type);
	void clear_theme_item(
		DataType p_data_type, const String& p_name, const String& p_theme_type);
	void get_theme_item_list(
		DataType p_data_type, const String& p_theme_type, List<String>* p_list) const;
	void add_theme_item_type(DataType p_data_type, const String& p_theme_type);
	void remove_theme_item_type(DataType p_data_type,
const String& p_theme_type);
	void rename_theme_item_type(
		DataType p_data_type, const String& p_old_theme_type, const String& p_theme_type);
	void get_theme_item_type_list(DataType p_data_type, List<String>* p_list) const;

	void set_type_variation(const String& p_theme_type, const String& p_base_type);
	bool is_type_variation(const String& p_theme_type, const String& p_base_type) const;
	void clear_type_variation(const String& p_theme_type);
	String get_type_variation_base(const String& p_theme_type) const;
	void get_type_variation_list(const String& p_base_type, List<String>* p_list) const;

	void add_type(const String& p_theme_type);
	void remove_type(const String& p_theme_type);
	void rename_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_type_list(List<String>* p_list) const;
	void get_type_dependencies(const String& p_base_type, const String& p_type_variant,
		Vector<String>& r_result);

	void merge_with(const Ref<Theme>& p_other);
	void clear();

	Theme();
	~Theme();
};

// --- Folded from scene/theme/theme_db.h ---
#include <functional>


class Font;
class Node;
class StyleBox;
class Texture2D;
class ThemeContext;

class ThemeDB final
{
public:
	typedef std::function<void(Node*, const String&, const String&)> ThemeItemSetter;

	struct ThemeItemBind
	{
		Theme::DataType data_type;
		String class_name;
		String item_name;
		String type_name;
		bool external = false;

		ThemeItemSetter setter;

		struct SortByType
		{
			_FORCE_INLINE_ bool operator()(const ThemeItemBind& l, const ThemeItemBind& r) const
			{
				return l.data_type < r.data_type;
			}
		};
	};

	struct Data
	{
		// Global Theme resources used by the default theme context.
		Ref<Theme> default_theme;
		Ref<Theme> project_theme;

		// Universal default values, final fallback for every theme.
		float fallback_base_scale = 1.0f;
		Ref<Font> fallback_font;
		int fallback_font_size = 16;
		Ref<Texture2D> fallback_icon;
		Ref<StyleBox> fallback_stylebox;

		// Global theme contexts used to scope global Theme resources.
		ThemeContext* default_theme_context = nullptr;
		HashMap<Node*, ThemeContext*> theme_contexts;

		HashMap<String, HashMap<String, ThemeItemBind>> theme_item_binds;
		HashMap<String, List<ThemeItemBind>> theme_item_binds_list;
	};

	static inline Data* data = nullptr;

private:
	static void _propagate_theme_context(Node* p_from_node, ThemeContext* p_context);
	static void _init_default_theme_context();
	static void _finalize_theme_contexts();
	static void _sort_theme_items();

public:
	ThemeDB() = delete;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void initialize_theme();
	static void initialize_theme_noproject();
	static void finalize_theme();

	// Global Theme resources.
	static void set_default_theme(const Ref<Theme>& p_default);
	static Ref<Theme> get_default_theme();

	static void set_project_theme(const Ref<Theme>& p_project_default);
	static Ref<Theme> get_project_theme();

	// Universal fallback values.
	static void set_fallback_base_scale(float p_base_scale);
	static float get_fallback_base_scale();

	static void set_fallback_font(const Ref<Font>& p_font);
	static Ref<Font> get_fallback_font();

	static void set_fallback_font_size(int p_font_size);
	static int get_fallback_font_size();

	static void set_fallback_icon(const Ref<Texture2D>& p_icon);
	static Ref<Texture2D> get_fallback_icon();

	static void set_fallback_stylebox(const Ref<StyleBox>& p_stylebox);
	static Ref<StyleBox> get_fallback_stylebox();

	static void get_native_type_dependencies(const String& p_base_type, Vector<String>& r_result);

	// Global theme contexts.
	static ThemeContext* create_theme_context(Node* p_node, Vector<Ref<Theme>>& p_themes);
	static void destroy_theme_context(Node* p_node);

	static ThemeContext* get_theme_context(Node* p_node);
	static ThemeContext* get_default_theme_context();
	static ThemeContext* get_nearest_theme_context(Node* p_for_node);

	// Theme item binding.
	static void bind_class_item(Theme::DataType p_data_type, const String& p_class_name,
		const String& p_prop_name, const String& p_item_name, ThemeItemSetter p_setter);
	static void bind_class_external_item(Theme::DataType p_data_type, const String& p_class_name,
		const String& p_prop_name, const String& p_item_name, const String& p_type_name,
		ThemeItemSetter p_setter);
	static void update_class_instance_items(Node* p_instance);

	static void get_class_items(const String& p_class_name, List<ThemeItemBind>* r_list,
		bool p_include_inherited = false, Theme::DataType p_filter_type = Theme::DATA_TYPE_MAX);
};

class ThemeContext
{
	friend class ThemeDB;

	Node* node = nullptr;
	ThemeContext* parent = nullptr;

	// Themes are stacked in the order of relevance, for easy iteration.
	// This means that the first theme is the one you should check first,
	// and the last theme is the fallback theme where every lookup ends.
	Vector<Ref<Theme>> themes;

	void _emit_changed();

public:
	void set_themes(Vector<Ref<Theme>>& p_themes);
	const Vector<Ref<Theme>> get_themes() const;
	Ref<Theme> get_fallback_theme() const;
};

// --- Folded from scene/theme/theme_owner.h ---


class Control;
class Node;
class ThemeContext;
class Window;

class ThemeOwner
{
	Node* holder = nullptr;

	Node* owner_node = nullptr;
	ThemeContext* owner_context = nullptr;

	void _owner_context_changed();
	ThemeContext* _get_active_owner_context() const;

	Node* _get_next_owner_node(Node* p_from_node) const;
	Ref<Theme> _get_owner_node_theme(Node* p_owner_node) const;

public:
	// Theme owner node.
	void set_owner_node(Node* p_node);

	Node* get_owner_node() const { return owner_node; }

	bool has_owner_node() const { return owner_node != nullptr; }

	void set_owner_context(ThemeContext* p_context, bool p_propagate = true);

	// Theme propagation.

	void assign_theme_on_parented(Node* p_for_node);
	void clear_theme_on_unparented(Node* p_for_node);
	void propagate_theme_changed(Node* p_to_node, Node* p_owner_node, bool p_notify, bool p_assign);

	// Theme lookup.

	void get_theme_type_dependencies(
		const Node* p_for_node, const String& p_theme_type, Vector<String>& r_result) const;

	bool has_theme_item_in_types(Theme::DataType p_data_type, const String& p_name,
		const Vector<String>& p_theme_types);

	float get_theme_default_base_scale();
	Ref<Font> get_theme_default_font();
	int get_theme_default_font_size();

	ThemeOwner(Node* p_holder) { holder = p_holder; }
};



// --- End scene/theme/theme_owner.h ---
// --- End scene/theme/theme_db.h ---
