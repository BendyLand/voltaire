/**************************************************************************/
/*  theme_db.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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
#include "servers/rendering/rendering_server.h"
#include "servers/text/text_server.h"
#include "theme_db.h"

void ThemeDB::initialize_theme_noproject()
{
	if (RenderingServer::get_singleton()) {
		make_default_theme(1.0, Ref<Font>());
	}
	_init_default_theme_context();
}

void ThemeDB::finalize_theme()
{
	if (!RenderingServer::get_singleton()) {
		WARN_PRINT("Finalizing theme when there is no RenderingServer is an error; check the order "
				   "of operations.");
	}
	_finalize_theme_contexts();
	default_theme.unref();
	fallback_font.unref();
	fallback_icon.unref();
	fallback_stylebox.unref();
}

// Global Theme resources.

void ThemeDB::set_default_theme(const Ref<Theme>& p_default) { default_theme = p_default; }

Ref<Theme> ThemeDB::get_default_theme() { return default_theme; }

void ThemeDB::set_project_theme(const Ref<Theme>& p_project_default)
{
	project_theme = p_project_default;
}

Ref<Theme> ThemeDB::get_project_theme() { return project_theme; }

float ThemeDB::get_fallback_base_scale() { return fallback_base_scale; }

Ref<Font> ThemeDB::get_fallback_font() { return fallback_font; }

int ThemeDB::get_fallback_font_size() { return fallback_font_size; }

Ref<Texture2D> ThemeDB::get_fallback_icon() { return fallback_icon; }

Ref<StyleBox> ThemeDB::get_fallback_stylebox() { return fallback_stylebox; }

void ThemeDB::get_native_type_dependencies(
	const StringName& p_base_type, Vector<StringName>& r_result)
{
	if (p_base_type == StringName()) {
		return;
	}
}

void ThemeDB::_init_default_theme_context()
{
	default_theme_context = memnew(ThemeContext);
	Vector<Ref<Theme>> themes;

	// Only add the project theme to the default context when running projects.

#ifdef TOOLS_ENABLED
	if (!Engine::get_singleton()->is_editor_hint()) {
		themes.push_back(project_theme);
	}
#else
	themes.push_back(project_theme);
#endif
	themes.push_back(default_theme);
	default_theme_context->set_themes(themes);
}

void ThemeDB::_finalize_theme_contexts()
{
	if (default_theme_context) {
		memdelete(default_theme_context);
		default_theme_context = nullptr;
	}
	while (theme_contexts.size()) {
		HashMap<Node*, ThemeContext*>::Iterator E = theme_contexts.begin();
		memdelete(E->value);
		theme_contexts.remove(E);
	}
}

ThemeContext* ThemeDB::get_theme_context(Node* p_node) const
{
	if (!theme_contexts.has(p_node)) {
		return nullptr;
	}
	return theme_contexts[p_node];
}

ThemeContext* ThemeDB::get_default_theme_context() const { return default_theme_context; }

ThemeContext* ThemeDB::get_nearest_theme_context(Node* p_for_node) const
{
	ERR_FAIL_COND_V(!p_for_node->is_inside_tree(), nullptr);
	Node* parent_node = p_for_node->get_parent();
	while (parent_node) {
		if (theme_contexts.has(parent_node)) {
			return theme_contexts[parent_node];
		}
		parent_node = parent_node->get_parent();
	}
	return nullptr;
}

// Theme item binding.

void ThemeDB::bind_class_item(Theme::DataType p_data_type, const StringName& p_class_name,
	const StringName& p_prop_name, const StringName& p_item_name, ThemeItemSetter p_setter)
{
	ERR_FAIL_COND_MSG(theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.setter = p_setter;
	theme_item_binds[p_class_name][p_prop_name] = bind;
	theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::bind_class_external_item(Theme::DataType p_data_type, const StringName& p_class_name,
	const StringName& p_prop_name, const StringName& p_item_name, const StringName& p_type_name,
	ThemeItemSetter p_setter)
{
	ERR_FAIL_COND_MSG(theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.type_name = p_type_name;
	bind.external = true;
	bind.setter = p_setter;
	theme_item_binds[p_class_name][p_prop_name] = bind;
	theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::get_class_items(const StringName& p_class_name, List<ThemeItemBind>* r_list,
	bool p_include_inherited, Theme::DataType p_filter_type)
{
	List<StringName> class_hierarchy;
	StringName class_name = p_class_name;
	while (class_name != StringName()) {
		class_hierarchy.push_front(class_name); // Put parent classes in front.
	}
	HashSet<StringName> inherited_props;
	for (const StringName& theme_type : class_hierarchy) {
		HashMap<StringName, List<ThemeItemBind>>::Iterator E =
			theme_item_binds_list.find(theme_type);
		if (E) {
			for (const ThemeItemBind& F : E->value) {
				if (p_filter_type != Theme::DATA_TYPE_MAX && F.data_type != p_filter_type) {
					continue;
				}
				if (inherited_props.has(F.item_name)) {
					continue; // Skip inherited properties.
				}
				if (F.external || F.class_name != p_class_name) {
					inherited_props.insert(F.item_name);
					if (!p_include_inherited) {
						continue; // Track properties defined in parent classes, and skip them.
					}
				}
				r_list->push_back(F);
			}
		}
	}
}

void ThemeDB::_sort_theme_items()
{
	for (KeyValue<StringName, List<ThemeDB::ThemeItemBind>>& E : theme_item_binds_list) {
		E.value.sort_custom<ThemeItemBind::SortByType>();
	}
}

ThemeDB* ThemeDB::singleton = nullptr;

ThemeDB* ThemeDB::get_singleton() { return singleton; }

ThemeDB::~ThemeDB()
{
	// For technical reasons unit tests recreate and destroy the default
	// theme over and over again. Make sure that finalize_theme() also
	// frees any objects that can be recreated by initialize_theme*().
	_finalize_theme_contexts();
	default_theme.unref();
	project_theme.unref();
	fallback_font.unref();
	fallback_icon.unref();
	fallback_stylebox.unref();
	singleton = nullptr;
}

const Vector<Ref<Theme>> ThemeContext::get_themes() const { return themes; }

Ref<Theme> ThemeContext::get_fallback_theme() const
{
	// We expect all contexts to be valid and non-empty, but just in case...
	if (themes.is_empty()) {
		return ThemeDB::get_singleton()->get_default_theme();
	}
	return themes[themes.size() - 1];
}


