/**************************************************************************/
/*  editor_variant_type_selectors.cpp                                     */
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

#include "editor_variant_type_selectors.h"

struct CompareVariantTypeNames
{
	bool operator()(const String& p_lhs, const String& p_rhs) const
	{
		// Variant type names should not be empty, but just in case.
		DEV_ASSERT(!p_lhs.is_empty() && !p_rhs.is_empty());

		// Variant type names are ascii strings.
		const bool lhs_lower = is_ascii_lower_case(p_lhs[0]);
		const bool rhs_lower = is_ascii_lower_case(p_rhs[0]);
		if (lhs_lower != rhs_lower) {
			// Lowercase types like `int` and `float` come first.
			return lhs_lower > rhs_lower;
		}

		return p_lhs < p_rhs;
	}
};

void EditorVariantTypePopupMenu::_popup_base(const Rect2i& p_bounds)
{
	if (icons_dirty) {
		_update_menu_icons();
		icons_dirty = false;
	}
	PopupMenu::_popup_base(p_bounds);
}

EditorVariantTypePopupMenu::EditorVariantTypePopupMenu(bool p_remove_item)
{
	remove_item = p_remove_item;
	set_search_bar_enabled(true);
	set_search_bar_min_item_count(10);
}


