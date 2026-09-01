/**************************************************************************/
/*  theme_owner.cpp                                                       */
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

#include "scene/gui/control.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "theme_owner.h"

ThemeContext* ThemeOwner::_get_active_owner_context() const
{
	if (owner_context) {
		return owner_context;
	}

	return ThemeDB::get_singleton()->get_default_theme_context();
}

bool ThemeOwner::has_theme_item_in_types(
	Theme::DataType p_data_type, const StringName& p_name, const Vector<StringName>& p_theme_types)
{
	ERR_FAIL_COND_V_MSG(
		p_theme_types.is_empty(), false, "At least one theme type must be specified.");

	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered.
	Node* current_owner = owner_node;

	while (current_owner) {
		// For each theme resource check the theme types provided and see if p_name exists with any
		// of them.
		for (const StringName& E : p_theme_types) {
			Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

			if (owner_theme.is_valid() && owner_theme->has_theme_item(p_data_type, p_name, E)) {
				return true;
			}
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			for (const StringName& E : p_theme_types) {
				if (theme->has_theme_item(p_data_type, p_name, E)) {
					return true;
				}
			}
		}
	}

	// Finally, if no match exists, return false.
	return false;
}

float ThemeOwner::get_theme_default_base_scale()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_base_scale()) {
			return owner_theme->get_default_base_scale();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_base_scale()) {
				return theme->get_default_base_scale();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_singleton()->get_fallback_base_scale();
}

Ref<Font> ThemeOwner::get_theme_default_font()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_font()) {
			return owner_theme->get_default_font();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_font()) {
				return theme->get_default_font();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_singleton()->get_fallback_font();
}

int ThemeOwner::get_theme_default_font_size()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_font_size()) {
			return owner_theme->get_default_font_size();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_font_size()) {
				return theme->get_default_font_size();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_singleton()->get_fallback_font_size();
}


