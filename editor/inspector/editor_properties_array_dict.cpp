/**************************************************************************/
/*  editor_properties_array_dict.cpp                                      */
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

#include "core/input/input.h"
#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_icon_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/gui/editor_variant_type_selectors.h"
#include "editor/inspector/editor_properties.h"
#include "editor/inspector/editor_properties_vector.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_properties_array_dict.h"
#include "scene/gui/button.h"
#include "scene/gui/margin_container.h"
#include "scene/main/scene_tree.h"

///////////////////

String EditorPropertyDictionaryObject::get_property_name_for_index(int p_index)
{
	switch (p_index) {
	case NEW_KEY_INDEX:
		return "new_item_key";
	case NEW_VALUE_INDEX:
		return "new_item_value";
	default:
		return "indices/" + itos(p_index);
	}
}

String EditorPropertyDictionaryObject::get_key_name_for_index(int p_index)
{
	switch (p_index) {
	case NEW_KEY_INDEX:
		return "new_item_key_name";
	case NEW_VALUE_INDEX:
		return "new_item_value_name";
	default:
		return "keys/" + itos(p_index);
	}
}

///////////////////// ARRAY ///////////////////////////

void EditorPropertyArray::_update_slots_size()
{
	float name_size = 0;
	for (Slot& slot : slots) {
		if (name_size == 0) {
			int max_index = (page_index + 1) * page_length - 1;
			const String ms = String("M").repeat(itos(max_index).length());

			Ref<Font> font = theme_cache.font;
			int font_size = theme_cache.font_size;
			int half_padding = theme_cache.padding / 2;
			name_size = slots[0].reorder_button->get_minimum_size().x +
						theme_cache.horizontal_separation + half_padding +
						font->get_string_size(ms, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
		}
		slot.prop->set_name_fixed_size(name_size);
	}
}

void EditorPropertyArray::set_preview_value(bool p_preview_value)
{
	preview_value = p_preview_value;
}

void EditorPropertyArray::_button_draw()
{
	if (dropping) {
		Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		edit->draw_rect(Rect2(Point2(), edit->get_size()), color, false);
	}
}

void EditorPropertyArray::_button_add_item_draw()
{
	if (dropping) {
		Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		button_add_item->draw_rect(Rect2(Point2(), button_add_item->get_size()), color, false);
	}
}

void EditorPropertyArray::_page_changed(int p_page)
{
	if (updating) {
		return;
	}
	page_index = p_page;
	int i = p_page * page_length;

	if (reorder_slot.index < 0) {
		for (Slot& slot : slots) {
			slot.set_index(i);
			i++;
		}
	}
	else {
		int reorder_from_page = reorder_slot.index / page_length;
		if (reorder_from_page < p_page) {
			i++;
		}
		for (Slot& slot : slots) {
			if (slot.index != reorder_slot.index) {
				slot.set_index(i);
				i++;
			}
			else if (i == reorder_slot.index) {
				i++;
			}
		}
	}
	update_property();
}

void EditorPropertyArray::_reorder_button_down(int p_slot_index)
{
	if (is_read_only()) {
		return;
	}
	reorder_slot = slots[p_slot_index];
	reorder_to_index = reorder_slot.index;
	// Ideally it'd to be able to show the mouse but I had issues with
	// Control's `mouse_exit()`/`mouse_entered()` signals not getting called.
	Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_CAPTURED);
}

bool EditorPropertyArray::is_colored(ColorationMode p_mode)
{
	return p_mode == COLORATION_CONTAINER_RESOURCE;
}

///////////////////// DICTIONARY ///////////////////////////

void EditorPropertyDictionary::set_preview_value(bool p_preview_value)
{
	preview_value = p_preview_value;
}

void EditorPropertyDictionary::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		if (button_add_item) {
			add_panel->add_theme_style_override(
				SceneStringName(panel), get_theme_stylebox(SNAME("DictionaryAddItem")).ptr());
		}
	} break;
	}
}

bool EditorPropertyDictionary::is_colored(ColorationMode p_mode)
{
	return p_mode == COLORATION_CONTAINER_RESOURCE;
}

///////////////////// LOCALIZABLE STRING ///////////////////////////

void EditorPropertyLocalizableString::_add_locale_popup() { locale_select->popup_locale_dialog(); }

void EditorPropertyLocalizableString::_page_changed(int p_page)
{
	if (updating) {
		return;
	}
	page_index = p_page;
	update_property();
}


