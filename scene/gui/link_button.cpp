/**************************************************************************/
/*  link_button.cpp                                                       */
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
#include "link_button.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

String LinkButton::get_text() const { return text; }

void LinkButton::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	if (overrun_behavior != p_behavior) {
		overrun_behavior = p_behavior;
		_shape();
		update_minimum_size();
		queue_redraw();
	}
}

TextServer::OverrunBehavior LinkButton::get_text_overrun_behavior() const
{
	return overrun_behavior;
}

void LinkButton::set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser)
{
	if (st_parser != p_parser) {
		st_parser = p_parser;
		_shape();
		queue_redraw();
	}
}

void LinkButton::set_ellipsis_char(const String& p_char)
{
	String c = p_char;
	if (c.length() > 1) {
		WARN_PRINT("Ellipsis must be exactly one character long (" + itos(c.length()) +
				   " characters given).");
		c = c.left(1);
	}

	if (el_char == c) {
		return;
	}
	el_char = c;

	if (overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		_shape();
		queue_redraw();
		update_minimum_size();
	}
}

String LinkButton::get_ellipsis_char() const { return el_char; }

TextServer::StructuredTextParser LinkButton::get_structured_text_bidi_override() const
{
	return st_parser;
}

void LinkButton::set_text_direction(Control::TextDirection p_text_direction)
{
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (text_direction != p_text_direction) {
		text_direction = p_text_direction;
		_shape();
		queue_redraw();
	}
}

Control::TextDirection LinkButton::get_text_direction() const { return text_direction; }

void LinkButton::set_language(const String& p_language)
{
	if (language != p_language) {
		language = p_language;
		_shape();
		queue_redraw();
	}
}

String LinkButton::get_language() const { return language; }

void LinkButton::set_uri(const String& p_uri)
{
	if (uri != p_uri) {
		uri = p_uri;
		queue_accessibility_update();
	}
}

String LinkButton::get_uri() const { return uri; }

void LinkButton::set_underline_mode(UnderlineMode p_underline_mode)
{
	if (underline_mode == p_underline_mode) {
		return;
	}

	underline_mode = p_underline_mode;
	queue_redraw();
}

LinkButton::UnderlineMode LinkButton::get_underline_mode() const { return underline_mode; }

Ref<Font> LinkButton::get_button_font() const { return theme_cache.font; }

int LinkButton::get_button_font_size() const { return theme_cache.font_size; }

void LinkButton::pressed()
{
	if (uri.is_empty()) {
		return;
	}

	OS::get_singleton()->shell_open(uri);
}

Size2 LinkButton::get_minimum_size() const
{
	Size2 minsize = text_buf->get_size();
	if (overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		minsize.width = 0;
	}

	return minsize;
}

Control::CursorShape LinkButton::get_cursor_shape(const Point2& p_pos) const
{
	return is_disabled() ? CURSOR_ARROW : get_default_cursor_shape();
}

String LinkButton::_get_accessibility_name() const
{
	const String& ac_name = Control::_get_accessibility_name();
	if (!xl_text.is_empty() && ac_name.is_empty()) {
		return xl_text;
	}
	else if (!xl_text.is_empty() && !ac_name.is_empty() && ac_name != xl_text) {
		return ac_name + ": " + xl_text;
	}
	else if (xl_text.is_empty() && ac_name.is_empty() && !get_tooltip_text().is_empty()) {
		return get_tooltip_text(); // Fall back to tooltip.
	}
	else {
		return ac_name;
	}
}

LinkButton::LinkButton(const String& p_text)
{
	text_buf.instantiate();
	set_focus_mode(FOCUS_ACCESSIBILITY);
	set_default_cursor_shape(CURSOR_POINTING_HAND);

	set_text(p_text);
}


