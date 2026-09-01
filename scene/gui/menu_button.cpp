/**************************************************************************/
/*  menu_button.cpp                                                       */
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

#include "menu_button.h"
#include "scene/main/window.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

void MenuButton::_popup_visibility_changed(bool p_visible)
{
	set_pressed(p_visible);

	if (!p_visible) {
		set_process_internal(false);
		return;
	}

	if (switch_on_hover) {
		set_process_internal(true);
	}
}

void MenuButton::pressed()
{
	if (popup->is_visible()) {
		popup->hide();
		return;
	}

	show_popup();
}

PopupMenu* MenuButton::get_popup() const { return popup; }

void MenuButton::set_switch_on_hover(bool p_enabled) { switch_on_hover = p_enabled; }

bool MenuButton::is_switch_on_hover() { return switch_on_hover; }

int MenuButton::get_item_count() const { return popup->get_item_count(); }

void MenuButton::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

#ifdef TOOLS_ENABLED
PackedStringArray MenuButton::get_configuration_warnings() const
{
	PackedStringArray warnings = Button::get_configuration_warnings();
	warnings.append_array(popup->get_configuration_warnings());
	return warnings;
}
#endif

MenuButton::~MenuButton() {}


