/**************************************************************************/
/*  editor_sectioned_inspector.cpp                                        */
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

#include "core/templates/mem_unique_ptr.h"
#include "editor/editor_string_names.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/themes/editor_scale.h"
#include "editor_sectioned_inspector.h"
#include "scene/gui/check_button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/tree.h"

static bool _property_path_matches(const String& p_property_path, const String& p_filter,
	EditorPropertyNameProcessor::Style p_style)
{
	if (p_property_path.containsn(p_filter)) {
		return true;
	}

	const Vector<String> sections = p_property_path.split("/");
	for (int i = 0; i < sections.size(); i++) {
		if (p_filter.is_subsequence_ofn(EditorPropertyNameProcessor::get_singleton()->process_name(
				sections[i], p_style, p_property_path))) {
			return true;
		}
	}
	return false;
}

void SectionedInspector::set_current_section(const String& p_section)
{
	if (section_map.has(p_section)) {
		TreeItem* item = section_map[p_section];
		item->select(0);
		sections->scroll_to_item(item);
	}
}

String SectionedInspector::get_full_item_path(const String& p_item)
{
	String base = get_current_section();

	if (!base.is_empty()) {
		return base + "/" + p_item;
	}
	else {
		return p_item;
	}
}

void SectionedInspector::_search_changed(const String& p_what)
{
	if (advanced_toggle) {
		if (p_what.is_empty()) {
			advanced_toggle->set_pressed_no_signal(!restrict_to_basic);
			advanced_toggle->set_disabled(false);
			advanced_toggle->set_tooltip_text(String());
		}
		else {
			advanced_toggle->set_pressed_no_signal(true);
			advanced_toggle->set_disabled(true);
			advanced_toggle->set_tooltip_text(
				TTRC("Advanced settings are always shown when searching."));
		}
	}
	update_category_list();
}

void SectionedInspector::_advanced_toggled(bool p_toggled_on)
{
	restrict_to_basic = !p_toggled_on;
	update_category_list();
	inspector->set_restrict_to_basic_settings(restrict_to_basic);
}

EditorInspector* SectionedInspector::get_inspector() { return inspector; }

SectionedInspector::~SectionedInspector() {}


