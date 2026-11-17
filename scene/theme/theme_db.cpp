/**************************************************************************/
/*  theme_db.cpp                                                          */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "scene/gui/control.h"
#include "scene/main/node.h"
#include "scene/main/window.h"
#include "scene/resources/font.h"
#include "scene/resources/style_box.h"
#include "scene/resources/texture.h"
#include "scene/theme/default_theme.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#include "theme_db.h"

void ThemeDB::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void ThemeDB::finalize()
{
	if (!data) {
		return;
	}
	_finalize_theme_contexts();
	data->default_theme.unref();
	data->project_theme.unref();
	data->fallback_font.unref();
	data->fallback_icon.unref();
	data->fallback_stylebox.unref();
	memdelete(data);
	data = nullptr;
}

void ThemeDB::initialize_theme() {}

void ThemeDB::initialize_theme_noproject()
{
	if (!data) {
		return;
	}
	if (Renderer::data) {
		make_default_theme(1.0, Ref<Font>());
	}
	_init_default_theme_context();
}

void ThemeDB::finalize_theme()
{
	if (!data) {
		return;
	}
	if (!Renderer::data) {
		WARN_PRINT("Finalizing theme when there is no Renderer is an error; check the order "
				   "of operations.");
	}
	_finalize_theme_contexts();
	data->default_theme.unref();
	data->fallback_font.unref();
	data->fallback_icon.unref();
	data->fallback_stylebox.unref();
}

// Global Theme resources.

void ThemeDB::set_default_theme(const Ref<Theme>& p_default)
{
	if (data) {
		data->default_theme = p_default;
	}
}

Ref<Theme> ThemeDB::get_default_theme() { return data ? data->default_theme : Ref<Theme>(); }

void ThemeDB::set_project_theme(const Ref<Theme>& p_project_default)
{
	if (data) {
		data->project_theme = p_project_default;
	}
}

Ref<Theme> ThemeDB::get_project_theme() { return data ? data->project_theme : Ref<Theme>(); }

void ThemeDB::set_fallback_base_scale(float p_base_scale) { (void)p_base_scale; }

float ThemeDB::get_fallback_base_scale() { return data ? data->fallback_base_scale : 1.0f; }

void ThemeDB::set_fallback_font(const Ref<Font>& p_font) { (void)p_font; }

Ref<Font> ThemeDB::get_fallback_font() { return data ? data->fallback_font : Ref<Font>(); }

void ThemeDB::set_fallback_font_size(int p_font_size) { (void)p_font_size; }

int ThemeDB::get_fallback_font_size() { return data ? data->fallback_font_size : 16; }

void ThemeDB::set_fallback_icon(const Ref<Texture2D>& p_icon) { (void)p_icon; }

Ref<Texture2D> ThemeDB::get_fallback_icon()
{
	return data ? data->fallback_icon : Ref<Texture2D>();
}

void ThemeDB::set_fallback_stylebox(const Ref<StyleBox>& p_stylebox) { (void)p_stylebox; }

Ref<StyleBox> ThemeDB::get_fallback_stylebox()
{
	return data ? data->fallback_stylebox : Ref<StyleBox>();
}

void ThemeDB::get_native_type_dependencies(const String& p_base_type, Vector<String>& r_result)
{
	(void)r_result;
	if (p_base_type == String()) {
		return;
	}
}

void ThemeDB::_init_default_theme_context()
{
	if (!data) {
		return;
	}
	if (data->default_theme_context) {
		memdelete(data->default_theme_context);
	}
	data->default_theme_context = memnew(ThemeContext);
	Vector<Ref<Theme>> themes;

#ifdef TOOLS_ENABLED
	if (!Engine::is_editor_hint()) {
		themes.push_back(data->project_theme);
	}
#else
	themes.push_back(data->project_theme);
#endif
	themes.push_back(data->default_theme);
	data->default_theme_context->set_themes(themes);
}

void ThemeDB::_finalize_theme_contexts()
{
	if (!data) {
		return;
	}
	if (data->default_theme_context) {
		memdelete(data->default_theme_context);
		data->default_theme_context = nullptr;
	}
	while (data->theme_contexts.size()) {
		HashMap<Node*, ThemeContext*>::Iterator E = data->theme_contexts.begin();
		memdelete(E->value);
		data->theme_contexts.remove(E);
	}
}

ThemeContext* ThemeDB::create_theme_context(Node* p_node, Vector<Ref<Theme>>& p_themes)
{
	(void)p_node;
	(void)p_themes;
	return nullptr;
}

void ThemeDB::destroy_theme_context(Node* p_node)
{
	if (!data || !data->theme_contexts.has(p_node)) {
		return;
	}
	memdelete(data->theme_contexts[p_node]);
	data->theme_contexts.erase(p_node);
}

ThemeContext* ThemeDB::get_theme_context(Node* p_node)
{
	if (!data || !data->theme_contexts.has(p_node)) {
		return nullptr;
	}
	return data->theme_contexts[p_node];
}

ThemeContext* ThemeDB::get_default_theme_context()
{
	return data ? data->default_theme_context : nullptr;
}

ThemeContext* ThemeDB::get_nearest_theme_context(Node* p_for_node)
{
	if (!data || !p_for_node) {
		return nullptr;
	}
	ERR_FAIL_COND_V(!p_for_node->is_inside_tree(), nullptr);
	Node* parent_node = p_for_node->get_parent();
	while (parent_node) {
		if (data->theme_contexts.has(parent_node)) {
			return data->theme_contexts[parent_node];
		}
		parent_node = parent_node->get_parent();
	}
	return nullptr;
}

// Theme item binding.

void ThemeDB::bind_class_item(Theme::DataType p_data_type, const String& p_class_name,
	const String& p_prop_name, const String& p_item_name, ThemeItemSetter p_setter)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.setter = p_setter;
	data->theme_item_binds[p_class_name][p_prop_name] = bind;
	data->theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::bind_class_external_item(Theme::DataType p_data_type, const String& p_class_name,
	const String& p_prop_name, const String& p_item_name, const String& p_type_name,
	ThemeItemSetter p_setter)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.type_name = p_type_name;
	bind.external = true;
	bind.setter = p_setter;
	data->theme_item_binds[p_class_name][p_prop_name] = bind;
	data->theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::update_class_instance_items(Node* p_instance) { (void)p_instance; }

void ThemeDB::get_class_items(const String& p_class_name, List<ThemeItemBind>* r_list,
	bool p_include_inherited, Theme::DataType p_filter_type)
{
	if (!data) {
		return;
	}
	List<String> class_hierarchy;
	String class_name = p_class_name;
	while (class_name != String()) {
		class_hierarchy.push_front(class_name);
	}
	HashSet<String> inherited_props;
	for (const String& theme_type : class_hierarchy) {
		HashMap<String, List<ThemeItemBind>>::Iterator E =
			data->theme_item_binds_list.find(theme_type);
		if (E) {
			for (const ThemeItemBind& F : E->value) {
				if (p_filter_type != Theme::DATA_TYPE_MAX && F.data_type != p_filter_type) {
					continue;
				}
				if (inherited_props.has(F.item_name)) {
					continue;
				}
				if (F.external || F.class_name != p_class_name) {
					inherited_props.insert(F.item_name);
					if (!p_include_inherited) {
						continue;
					}
				}
				r_list->push_back(F);
			}
		}
	}
}

void ThemeDB::_sort_theme_items()
{
	if (!data) {
		return;
	}
	for (KeyValue<String, List<ThemeDB::ThemeItemBind>>& E : data->theme_item_binds_list) {
		E.value.sort_custom<ThemeItemBind::SortByType>();
	}
}

const Vector<Ref<Theme>> ThemeContext::get_themes() const { return themes; }

Ref<Theme> ThemeContext::get_fallback_theme() const
{
	if (themes.is_empty()) {
		return ThemeDB::get_default_theme();
	}
	return themes[themes.size() - 1];
}
