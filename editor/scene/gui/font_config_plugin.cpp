/**************************************************************************/
/*  font_config_plugin.cpp                                                */
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

#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "editor/import/dynamic_font_import_settings.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "font_config_plugin.h"
#include "scene/gui/margin_container.h"

/*************************************************************************/
/*  EditorPropertyFontMetaObject                                         */
/*************************************************************************/

/*************************************************************************/
/*  EditorPropertyFontOTObject                                           */
/*************************************************************************/

/*************************************************************************/
/* EditorPropertyFontMetaOverride                                        */
/*************************************************************************/

void EditorPropertyFontMetaOverride::_add_menu()
{
	if (script_editor) {
		Size2 size = get_size();
		menu->set_position(
			get_screen_position() + Size2(0, size.height * get_global_transform().get_scale().y));
		menu->reset_size();
		menu->popup();
	}
	else {
		locale_select->popup_locale_dialog();
	}
}

void EditorPropertyFontMetaOverride::_page_changed(int p_page)
{
	if (updating) {
		return;
	}
	page_index = p_page;
	update_property();
}

/*************************************************************************/
/* EditorPropertyOTVariation                                             */
/*************************************************************************/

void EditorPropertyOTVariation::_page_changed(int p_page)
{
	if (updating) {
		return;
	}
	page_index = p_page;
	update_property();
}

/*************************************************************************/
/* EditorPropertyOTFeatures                                              */
/*************************************************************************/

void EditorPropertyOTFeatures::_add_menu()
{
	Size2 size = get_size();
	menu->set_position(
		get_screen_position() + Size2(0, size.height * get_global_transform().get_scale().y));
	menu->reset_size();
	menu->popup();
}

void EditorPropertyOTFeatures::_page_changed(int p_page)
{
	if (updating) {
		return;
	}
	page_index = p_page;
	update_property();
}

Size2 FontPreview::get_minimum_size() const { return Vector2(64, 64) * EDSCALE; }

void FontPreview::_preview_changed() { queue_redraw(); }

void EditorPropertyFontNamesArray::_add_element()
{
	Size2 size = get_size();
	menu->set_position(
		get_screen_position() + Size2(0, size.height * get_global_transform().get_scale().y));
	menu->reset_size();
	menu->popup();
}

EditorPropertyFontNamesArray::EditorPropertyFontNamesArray()
{
	menu = memnew(PopupMenu);
	menu->add_item("Sans-Serif", 0);
	menu->add_item("Serif", 1);
	menu->add_item("Monospace", 2);
	menu->add_item("Fantasy", 3);
	menu->add_item("Cursive", 4);

	menu->add_separator();

	if (OS::get_singleton()) {
		Vector<String> fonts = OS::get_singleton()->get_system_fonts();
		fonts.sort();
		for (int i = 0; i < fonts.size(); i++) {
			menu->add_item(fonts[i], i + 6);
		}
	}
	add_child(menu);
}


