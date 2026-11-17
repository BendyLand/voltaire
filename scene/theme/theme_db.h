/**************************************************************************/
/*  theme_db.h                                                            */
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

#include <functional>
#include "scene/resources/theme.h"

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
