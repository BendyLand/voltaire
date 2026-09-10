/**************************************************************************/
/*  graph_element.cpp                                                     */
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
#include "graph_element.h"
#include "scene/gui/graph_edit.h"
#include "scene/theme/theme_db.h"

void GraphElement::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_SORT_CHILDREN: {
		_resort();
	} break;
	}
}

Vector2 GraphElement::get_position_offset() const { return position_offset; }

bool GraphElement::is_selected() { return selected; }

Vector2 GraphElement::get_drag_from() { return drag_from; }

bool GraphElement::is_resizable() const { return resizable; }

void GraphElement::set_draggable(bool p_draggable) { draggable = p_draggable; }

bool GraphElement::is_draggable() { return draggable; }

void GraphElement::set_selectable(bool p_selectable)
{
	if (!p_selectable) {
		set_selected(false);
	}
	selectable = p_selectable;
}

bool GraphElement::is_selectable() { return selectable; }

void GraphElement::set_scaling_menus(bool p_scaling_menus) { scaling_menus = p_scaling_menus; }

bool GraphElement::is_scaling_menus() const { return scaling_menus; }


