/**************************************************************************/
/*  status_indicator.cpp                                                  */
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

#include "scene/gui/popup_menu.h"
#include "servers/display/display_server.h"
#include "status_indicator.h"

void StatusIndicator::set_icon(const Ref<Texture2D>& p_icon)
{
	ERR_MAIN_THREAD_GUARD;
	icon = p_icon;
	if (iid != DisplayServerEnums::INVALID_INDICATOR_ID) {
		DisplayServer::get_singleton()->status_indicator_set_icon(iid, icon);
	}
}

Ref<Texture2D> StatusIndicator::get_icon() const { return icon; }

void StatusIndicator::set_tooltip(const String& p_tooltip)
{
	ERR_MAIN_THREAD_GUARD;
	tooltip = p_tooltip;
	if (iid != DisplayServerEnums::INVALID_INDICATOR_ID) {
		DisplayServer::get_singleton()->status_indicator_set_tooltip(iid, tooltip);
	}
}

String StatusIndicator::get_tooltip() const { return tooltip; }

NodePath StatusIndicator::get_menu() const { return menu; }

bool StatusIndicator::is_visible() const { return visible; }

Rect2 StatusIndicator::get_rect() const
{
	if (iid == DisplayServerEnums::INVALID_INDICATOR_ID) {
		return Rect2();
	}
	return DisplayServer::get_singleton()->status_indicator_get_rect(iid);
}


