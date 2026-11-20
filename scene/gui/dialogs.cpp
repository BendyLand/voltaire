/**************************************************************************/
/*  dialogs.cpp                                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "dialogs.h"

#include "core/config/engine.h"
#include "scene/gui/line_edit.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

void DialogChrome::init_controls(Window* p_owner)
{
	p_owner->set_wrap_controls(true);
	p_owner->set_visible(false);
	p_owner->set_transient(true);
	p_owner->set_exclusive(true);
	p_owner->set_clamp_to_embedder(true);
	p_owner->set_keep_title_visible(true);

	bg_panel = memnew(Panel);
	p_owner->add_child(bg_panel, false, Node::INTERNAL_MODE_FRONT);

	message_label = memnew(Label);
	message_label->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	p_owner->add_child(message_label, false, Node::INTERNAL_MODE_FRONT);

	buttons_hbox = Control::create_hbox();
	p_owner->add_child(buttons_hbox, false, Node::INTERNAL_MODE_FRONT);

	ok_button = memnew(Button);
	ok_button->set_text(default_ok_text);
	buttons_hbox->add_child(ok_button);

	if (has_cancel_button) {
		cancel = memnew(Button);
		cancel->set_text("Cancel");
		buttons_hbox->add_child(cancel);
		if (swap_cancel_ok) {
			buttons_hbox->move_child(cancel, 0);
		}
	}
}

bool DialogChrome::handle_escape_input(const InputEvent& p_event) const
{
	return close_on_escape && p_event.is_action_pressed(SNAME("ui_close_dialog"), false, true);
}

bool DialogChrome::should_cancel_on_parent_focus(const Window* p_owner) const
{
	return popped_up && !p_owner->is_exclusive() && p_owner->get_flag(Window::FLAG_POPUP);
}

void DialogChrome::update_child_rects(Window*) {}

void DialogChrome::update_ok_text(Window* p_owner)
{
	if (!ok_button) {
		return;
	}
	String prev_text = ok_button->get_text();
	String new_text = ok_text.is_empty() ? default_ok_text : ok_text;

	if (new_text == prev_text) {
		return;
	}
	ok_button->set_text(new_text);

	p_owner->child_controls_changed();
	if (p_owner->is_visible()) {
		update_child_rects(p_owner);
	}
}

void DialogChrome::set_default_ok_text(Window* p_owner, const String& p_text)
{
	default_ok_text = p_text;
	update_ok_text(p_owner);
}

void DialogChrome::set_cancel_button_text(const String& p_cancel_button_text)
{
	if (cancel) {
		cancel->set_text(p_cancel_button_text);
	}
}

String DialogChrome::get_cancel_button_text() const
{
	return cancel ? cancel->get_text() : String();
}

void DialogChrome::set_text(Window* p_owner, const String& p_text)
{
	if (!message_label || message_label->get_text() == p_text) {
		return;
	}

	message_label->set_text(p_text);

	p_owner->child_controls_changed();
	if (p_owner->is_visible()) {
		update_child_rects(p_owner);
	}
}

String DialogChrome::get_text() const
{
	return message_label ? message_label->get_text() : String();
}

void DialogChrome::set_autowrap(bool p_autowrap)
{
	if (message_label) {
		message_label->set_autowrap_mode(
			p_autowrap ? TextServer::AUTOWRAP_WORD : TextServer::AUTOWRAP_OFF);
	}
}

bool DialogChrome::has_autowrap() const
{
	return message_label && message_label->get_autowrap_mode() != TextServer::AUTOWRAP_OFF;
}

void DialogChrome::set_ok_button_text(Window* p_owner, const String& p_ok_button_text)
{
	ok_text = p_ok_button_text;
	update_ok_text(p_owner);
}

Size2 Dialog::_get_contents_minimum_size() const { return Size2(); }

void Dialog::_input_from_window(const InputEvent& p_event)
{
	if (chrome.handle_escape_input(p_event)) {
		_cancel_pressed();
	}
	Window::_input_from_window(p_event);
}

void Dialog::_parent_focused()
{
	if (chrome.should_cancel_on_parent_focus(this)) {
		_cancel_pressed();
	}
}

void Dialog::_text_submitted(const String& p_text)
{
	if (get_ok_button() && get_ok_button()->is_disabled()) {
		return;
	}
	_ok_pressed();
}

void Dialog::_post_popup()
{
	Window::_post_popup();
	chrome.post_popup();
}

void Dialog::_notification(int p_what)
{
	if (p_what == NOTIFICATION_VISIBILITY_CHANGED && !is_visible()) {
		chrome.popped_up = false;
	}
}

void Dialog::set_swap_cancel_ok(bool p_swap) { DialogChrome::swap_cancel_ok = p_swap; }

void Dialog::_cancel_pressed()
{
	hide();
}

void Dialog::_ok_pressed()
{
	if (chrome.get_hide_on_ok()) {
		hide();
	}
}

Dialog::Dialog(bool p_with_cancel) : chrome(p_with_cancel)
{
	chrome.init_controls(this);
}
