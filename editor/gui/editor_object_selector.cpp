/**************************************************************************/
/*  editor_object_selector.cpp                                            */
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

#include "editor/editor_data.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor_object_selector.h"
#include "scene/gui/box_container.h"
#include "scene/gui/margin_container.h"

Size2 EditorObjectSelector::get_minimum_size() const
{
	Ref<Font> font = get_theme_font(SceneStringName(font));
	int font_size = get_theme_font_size(SceneStringName(font_size));
	return Button::get_minimum_size() + Size2(0, font->get_height(font_size));
}

void EditorObjectSelector::_show_popup()
{
	if (sub_objects_menu->is_visible()) {
		sub_objects_menu->hide();
		return;
	}

	sub_objects_menu->clear();

	Rect2 rect = get_screen_rect();
	rect.position.y += rect.size.height;
	rect.size.height = 0;

	sub_objects_menu->set_min_size(Size2(0, 0));
	sub_objects_menu->popup(rect);
}

void EditorObjectSelector::clear_path()
{
	set_disabled(true);
	set_tooltip_text("");

	current_object_label->set_text("");
	current_object_icon->set_texture(nullptr);
	sub_objects_icon->hide();
}

void EditorObjectSelector::enable_path()
{
	set_disabled(false);
	sub_objects_icon->show();
}


