/**************************************************************************/
/*  button.cpp                                                            */
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

#include "button.h"
#include "scene/gui/dialogs.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

Size2 Button::get_minimum_size() const
{
	Ref<Texture2D> _icon = icon;
	if (_icon.is_null() && has_theme_icon(SNAME("icon"))) {
		_icon = theme_cache.icon;
	}

	return get_minimum_size_for_text_and_icon("", _icon);
}

void Button::_set_internal_margin(Side p_side, float p_value)
{
	_internal_margin[p_side] = p_value;
}

void Button::_queue_update_size_cache() {}

void Button::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.max_style_size = Vector2();
	theme_cache.style_margin_left = 0;
	theme_cache.style_margin_right = 0;
	theme_cache.style_margin_top = 0;
	theme_cache.style_margin_bottom = 0;

	const bool rtl = is_layout_rtl();
	if (rtl && has_theme_stylebox(SNAME("normal_mirrored"))) {
		_update_style_margins(theme_cache.normal_mirrored);
	}
	else {
		_update_style_margins(theme_cache.normal);
	}
	if (has_theme_stylebox("hover_pressed")) {
		if (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored"))) {
			_update_style_margins(theme_cache.hover_pressed_mirrored);
		}
		else {
			_update_style_margins(theme_cache.hover_pressed);
		}
	}
	if (rtl && has_theme_stylebox(SNAME("pressed_mirrored"))) {
		_update_style_margins(theme_cache.pressed_mirrored);
	}
	else {
		_update_style_margins(theme_cache.pressed);
	}
	if (rtl && has_theme_stylebox(SNAME("hover_mirrored"))) {
		_update_style_margins(theme_cache.hover_mirrored);
	}
	else {
		_update_style_margins(theme_cache.hover);
	}
	if (rtl && has_theme_stylebox(SNAME("disabled_mirrored"))) {
		_update_style_margins(theme_cache.disabled_mirrored);
	}
	else {
		_update_style_margins(theme_cache.disabled);
	}
	theme_cache.max_style_size = theme_cache.max_style_size.max(
		Vector2(theme_cache.style_margin_left + theme_cache.style_margin_right,
			theme_cache.style_margin_top + theme_cache.style_margin_bottom));
}

Size2 Button::_get_largest_stylebox_size() const { return theme_cache.max_style_size; }

Ref<StyleBox> Button::_get_current_stylebox() const
{
	Ref<StyleBox> stylebox = theme_cache.normal;
	const bool rtl = is_layout_rtl();

	switch (get_draw_mode()) {
	case DRAW_NORMAL: {
		if (rtl && has_theme_stylebox(SNAME("normal_mirrored"))) {
			stylebox = theme_cache.normal_mirrored;
		}
		else {
			stylebox = theme_cache.normal;
		}
	} break;

	case DRAW_HOVER_PRESSED: {
		// Edge case for CheckButton and CheckBox.
		if (has_theme_stylebox("hover_pressed")) {
			if (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored"))) {
				stylebox = theme_cache.hover_pressed_mirrored;
			}
			else {
				stylebox = theme_cache.hover_pressed;
			}
			break;
		}
	}
		[[fallthrough]];
	case DRAW_PRESSED: {
		if (rtl && has_theme_stylebox(SNAME("pressed_mirrored"))) {
			stylebox = theme_cache.pressed_mirrored;
		}
		else {
			stylebox = theme_cache.pressed;
		}
	} break;

	case DRAW_HOVER: {
		if (rtl && has_theme_stylebox(SNAME("hover_mirrored"))) {
			stylebox = theme_cache.hover_mirrored;
		}
		else {
			stylebox = theme_cache.hover;
		}
	} break;

	case DRAW_DISABLED: {
		if (rtl && has_theme_stylebox(SNAME("disabled_mirrored"))) {
			stylebox = theme_cache.disabled_mirrored;
		}
		else {
			stylebox = theme_cache.disabled;
		}
	} break;
	}

	return stylebox;
}

String Button::_get_accessibility_name() const
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

Size2 Button::_fit_icon_size(const Size2& p_size) const
{
	int max_width = theme_cache.icon_max_width;
	Size2 icon_size = p_size;

	if (max_width > 0 && icon_size.width > max_width) {
		icon_size.height = icon_size.height * max_width / icon_size.width;
		icon_size.width = max_width;
	}

	return icon_size;
}

Size2 Button::get_minimum_size_for_text_and_icon(const String& p_text, Ref<Texture2D> p_icon) const
{
	// Do not include `_internal_margin`, it's already added in the `get_minimum_size` overrides.

	Ref<TextParagraph> paragraph;
	if (p_text.is_empty()) {
		paragraph = text_buf;
	}
	else {
		paragraph.instantiate();
		_shape(paragraph, p_text);
	}

	Size2 minsize = paragraph->get_size();
	if (clip_text || overrun_behavior != TextServer::OVERRUN_NO_TRIMMING ||
		autowrap_mode != TextServer::AUTOWRAP_OFF) {
		minsize.width = 0;
	}

	if (!expand_icon && p_icon.is_valid()) {
		Size2 icon_size = _fit_icon_size(p_icon->get_size());
		if (vertical_icon_alignment == VERTICAL_ALIGNMENT_CENTER) {
			minsize.height = MAX(minsize.height, icon_size.height);
		}
		else {
			minsize.height += icon_size.height;
		}

		if (horizontal_icon_alignment != HORIZONTAL_ALIGNMENT_CENTER) {
			minsize.width += icon_size.width;
			if (!xl_text.is_empty() || !p_text.is_empty()) {
				minsize.width += MAX(0, theme_cache.h_separation);
			}
		}
		else {
			minsize.width = MAX(minsize.width, icon_size.width);
		}
	}

	if (!xl_text.is_empty() || !p_text.is_empty()) {
		Ref<Font> font = theme_cache.font;
		float font_height = font->get_height(theme_cache.font_size);
		if (vertical_icon_alignment == VERTICAL_ALIGNMENT_CENTER) {
			minsize.height = MAX(font_height, minsize.height);
		}
		else {
			minsize.height += font_height;
		}
	}

	return (theme_cache.align_to_largest_stylebox ? _get_largest_stylebox_size()
												  : _get_current_stylebox()->get_minimum_size()) +
		   minsize;
}

TextServer::OverrunBehavior Button::get_text_overrun_behavior() const { return overrun_behavior; }

String Button::get_text() const { return text; }

TextServer::AutowrapMode Button::get_autowrap_mode() const { return autowrap_mode; }

uint32_t Button::get_autowrap_trim_flags() const { return autowrap_flags_trim; }

Control::TextDirection Button::get_text_direction() const { return text_direction; }

String Button::get_language() const { return language; }

void Button::_update_style_margins(const Ref<StyleBox>& p_stylebox)
{
	theme_cache.max_style_size = theme_cache.max_style_size.max(p_stylebox->get_minimum_size());
	theme_cache.style_margin_left =
		MAX(theme_cache.style_margin_left, p_stylebox->get_margin(SIDE_LEFT));
	theme_cache.style_margin_right =
		MAX(theme_cache.style_margin_right, p_stylebox->get_margin(SIDE_RIGHT));
	theme_cache.style_margin_top =
		MAX(theme_cache.style_margin_top, p_stylebox->get_margin(SIDE_TOP));
	theme_cache.style_margin_bottom =
		MAX(theme_cache.style_margin_bottom, p_stylebox->get_margin(SIDE_BOTTOM));
}

Ref<Texture2D> Button::get_button_icon() const { return icon; }

bool Button::is_expand_icon() const { return expand_icon; }

bool Button::is_flat() const { return flat; }

bool Button::get_clip_text() const { return clip_text; }

HorizontalAlignment Button::get_text_alignment() const { return alignment; }

HorizontalAlignment Button::get_icon_alignment() const { return horizontal_icon_alignment; }

VerticalAlignment Button::get_vertical_icon_alignment() const { return vertical_icon_alignment; }

Button::Button(const String& p_text)
{
	text_buf.instantiate();
	text_buf->set_break_flags(TextServer::BREAK_MANDATORY | autowrap_flags_trim);
	set_mouse_filter(MOUSE_FILTER_STOP);

	set_text(p_text);
}

Button::~Button() {}

