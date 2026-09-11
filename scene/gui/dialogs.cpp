/**************************************************************************/
/*  dialogs.cpp                                                           */
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
#include "dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

void AcceptDialog::_input_from_window(const Ref<InputEvent>& p_event)
{
	if (close_on_escape && p_event->is_action_pressed(SNAME("ui_close_dialog"), false, true)) {
		_cancel_pressed();
	}
	Window::_input_from_window(p_event);
}

void AcceptDialog::_parent_focused()
{
	if (popped_up && !is_exclusive() && get_flag(FLAG_POPUP)) {
		_cancel_pressed();
	}
}

void AcceptDialog::_text_submitted(const String& p_text)
{
	if (get_ok_button() && get_ok_button()->is_disabled()) {
		return; // Do not allow submission if OK button is disabled.
	}
	_ok_pressed();
}

void AcceptDialog::_post_popup()
{
	Window::_post_popup();
	popped_up = true;
}

String AcceptDialog::get_text() const { return message_label->get_text(); }

void AcceptDialog::set_text(String p_text)
{
	if (message_label->get_text() == p_text) {
		return;
	}

	message_label->set_text(p_text);

	child_controls_changed();
	if (is_visible()) {
		_update_child_rects();
	}
}

void AcceptDialog::set_hide_on_ok(bool p_hide) { hide_on_ok = p_hide; }

bool AcceptDialog::get_hide_on_ok() const { return hide_on_ok; }

void AcceptDialog::set_close_on_escape(bool p_hide) { close_on_escape = p_hide; }

bool AcceptDialog::get_close_on_escape() const { return close_on_escape; }

void AcceptDialog::set_autowrap(bool p_autowrap)
{
	message_label->set_autowrap_mode(
		p_autowrap ? TextServer::AUTOWRAP_WORD : TextServer::AUTOWRAP_OFF);
}

bool AcceptDialog::has_autowrap()
{
	return message_label->get_autowrap_mode() != TextServer::AUTOWRAP_OFF;
}

void AcceptDialog::set_ok_button_text(String p_ok_button_text)
{
	ok_text = p_ok_button_text;
	_update_ok_text();
}

String AcceptDialog::get_ok_button_text() const { return ok_text; }

void AcceptDialog::_update_ok_text()
{
	String prev_text = ok_button->get_text();
	String new_text = default_ok_text;

	if (!ok_text.is_empty()) {
		new_text = ok_text;
	}

	if (new_text == prev_text) {
		return;
	}
	ok_button->set_text(new_text);

	child_controls_changed();
	if (is_visible()) {
		_update_child_rects();
	}
}

void AcceptDialog::set_swap_cancel_ok(bool p_swap) { swap_cancel_ok = p_swap; }

AcceptDialog::~AcceptDialog() {}

void ConfirmationDialog::set_cancel_button_text(String p_cancel_button_text)
{
	cancel->set_text(p_cancel_button_text);
}

String ConfirmationDialog::get_cancel_button_text() const { return cancel->get_text(); }

Button* ConfirmationDialog::get_cancel_button() { return cancel; }


