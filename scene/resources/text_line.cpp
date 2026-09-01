/**************************************************************************/
/*  text_line.cpp                                                         */
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

#include "core/object/class_db.h"
#include "text_line.compat.inc"
#include "text_line.h"

void TextLine::_bind_methods() {}

void TextLine::_shape() const
{
	// When a shaped text is invalidated by an external source, we want to reshape it.
	if (!TS->shaped_text_is_ready(rid)) {
		dirty = true;
	}

	if (dirty) {
		if (!tab_stops.is_empty()) {
			TS->shaped_text_tab_align(rid, tab_stops);
		}

		uint32_t overrun_flags = TextServer::OVERRUN_NO_TRIM;
		if (overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
			overrun_flags = TextServer::get_overrun_flags_from_behavior(overrun_behavior);

			if (alignment == HORIZONTAL_ALIGNMENT_FILL) {
				TS->shaped_text_fit_to_width(rid, width, flags);
				overrun_flags.set_flag(TextServer::OVERRUN_JUSTIFICATION_AWARE);
				TS->shaped_text_set_custom_ellipsis(
					rid, (el_char.length() > 0) ? el_char[0] : 0x2026);
				TS->shaped_text_overrun_trim_to_width(rid, width, overrun_flags);
			}
			else {
				TS->shaped_text_set_custom_ellipsis(
					rid, (el_char.length() > 0) ? el_char[0] : 0x2026);
				TS->shaped_text_overrun_trim_to_width(rid, width, overrun_flags);
			}
		}
		else if (alignment == HORIZONTAL_ALIGNMENT_FILL) {
			TS->shaped_text_fit_to_width(rid, width, flags);
		}
		dirty = false;
	}
}

RID TextLine::get_rid() const { return rid; }

void TextLine::clear() { TS->shaped_text_clear(rid); }

Ref<TextLine> TextLine::duplicate() const
{
	Ref<TextLine> copy;
	copy.instantiate();
	if (rid.is_valid()) {
		TS->free_rid(copy->rid);
		copy->rid = TS->shaped_text_duplicate(rid);
	}
	copy->dirty = true;
	copy->width = width;
	copy->flags = flags;
	copy->alignment = alignment;
	copy->el_char = el_char;
	copy->overrun_behavior = overrun_behavior;
	copy->tab_stops = tab_stops;

	return copy;
}

void TextLine::set_preserve_invalid(bool p_enabled)
{
	TS->shaped_text_set_preserve_invalid(rid, p_enabled);
	dirty = true;
}

bool TextLine::get_preserve_invalid() const { return TS->shaped_text_get_preserve_invalid(rid); }

void TextLine::set_preserve_control(bool p_enabled)
{
	TS->shaped_text_set_preserve_control(rid, p_enabled);
	dirty = true;
}

bool TextLine::get_preserve_control() const { return TS->shaped_text_get_preserve_control(rid); }

void TextLine::set_direction(TextServer::Direction p_direction)
{
	TS->shaped_text_set_direction(rid, p_direction);
	dirty = true;
}

TextServer::Direction TextLine::get_direction() const { return TS->shaped_text_get_direction(rid); }

TextServer::Direction TextLine::get_inferred_direction() const
{
	return TS->shaped_text_get_inferred_direction(rid);
}

void TextLine::set_orientation(TextServer::Orientation p_orientation)
{
	TS->shaped_text_set_orientation(rid, p_orientation);
	dirty = true;
}

TextServer::Orientation TextLine::get_orientation() const
{
	return TS->shaped_text_get_orientation(rid);
}

void TextLine::set_bidi_override(const Array& p_override)
{
	TS->shaped_text_set_bidi_override(rid, p_override);
	dirty = true;
}

bool TextLine::add_string(const String& p_text, const Ref<Font>& p_font, int p_font_size,
	const String& p_language, const Variant& p_meta)
{
	ERR_FAIL_COND_V(p_font.is_null(), false);
	bool res = TS->shaped_text_add_string(rid, p_text, p_font->get_rids(), p_font_size,
		p_font->get_opentype_features(), p_language, p_meta);
	dirty = true;
	return res;
}

bool TextLine::add_object(Variant p_key, const Size2& p_size, InlineAlignment p_inline_align,
	int p_length, float p_baseline)
{
	bool res = TS->shaped_text_add_object(rid, p_key, p_size, p_inline_align, p_length, p_baseline);
	dirty = true;
	return res;
}

bool TextLine::resize_object(
	Variant p_key, const Size2& p_size, InlineAlignment p_inline_align, float p_baseline)
{
	_shape();
	return TS->shaped_text_resize_object(rid, p_key, p_size, p_inline_align, p_baseline);
}

bool TextLine::has_object(Variant p_key) const
{
	_shape();
	return TS->shaped_text_has_object(rid, p_key);
}

Array TextLine::get_objects() const { return TS->shaped_text_get_objects(rid); }

Rect2 TextLine::get_object_rect(Variant p_key) const
{
	Vector2 ofs;

	float length = TS->shaped_text_get_width(rid);
	if (width > 0) {
		switch (alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
		case HORIZONTAL_ALIGNMENT_LEFT:
			break;
		case HORIZONTAL_ALIGNMENT_CENTER: {
			if (length <= width) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += Math::floor((width - length) / 2.0);
				}
				else {
					ofs.y += Math::floor((width - length) / 2.0);
				}
			}
			else if (TS->shaped_text_get_inferred_direction(rid) == TextServer::DIRECTION_RTL) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += width - length;
				}
				else {
					ofs.y += width - length;
				}
			}
		} break;
		case HORIZONTAL_ALIGNMENT_RIGHT: {
			if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += width - length;
			}
			else {
				ofs.y += width - length;
			}
		} break;
		}
	}
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(rid);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(rid);
	}

	Rect2 rect = TS->shaped_text_get_object_rect(rid, p_key);
	rect.position += ofs;

	return rect;
}

void TextLine::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	if (alignment != p_alignment) {
		if (alignment == HORIZONTAL_ALIGNMENT_FILL || p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			alignment = p_alignment;
			dirty = true;
		}
		else {
			alignment = p_alignment;
		}
	}
}

HorizontalAlignment TextLine::get_horizontal_alignment() const { return alignment; }

void TextLine::tab_align(const Vector<float>& p_tab_stops)
{
	tab_stops = p_tab_stops;
	dirty = true;
}

void TextLine::set_flags(uint32_t p_flags)
{
	if (flags != p_flags) {
		flags = p_flags;
		dirty = true;
	}
}

uint32_t TextLine::get_flags() const { return flags; }

void TextLine::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	if (overrun_behavior != p_behavior) {
		overrun_behavior = p_behavior;
		dirty = true;
	}
}

TextServer::OverrunBehavior TextLine::get_text_overrun_behavior() const { return overrun_behavior; }

void TextLine::set_ellipsis_char(const String& p_char)
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
	dirty = true;
}

String TextLine::get_ellipsis_char() const { return el_char; }

void TextLine::set_width(float p_width)
{
	if (width == p_width) {
		return;
	}
	width = p_width;
	if (alignment == HORIZONTAL_ALIGNMENT_FILL ||
		overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		dirty = true;
	}
}

float TextLine::get_width() const { return width; }

Size2 TextLine::get_size() const
{
	_shape();
	return TS->shaped_text_get_size(rid);
}

float TextLine::get_line_ascent() const
{
	_shape();
	return TS->shaped_text_get_ascent(rid);
}

float TextLine::get_line_descent() const
{
	_shape();
	return TS->shaped_text_get_descent(rid);
}

float TextLine::get_line_width() const
{
	_shape();
	return TS->shaped_text_get_width(rid);
}

float TextLine::get_line_underline_position() const
{
	_shape();
	return TS->shaped_text_get_underline_position(rid);
}

float TextLine::get_line_underline_thickness() const
{
	_shape();
	return TS->shaped_text_get_underline_thickness(rid);
}

void TextLine::draw(
	RID p_canvas, const Vector2& p_pos, const Color& p_color, float p_oversampling) const
{
	_shape();

	Vector2 ofs = p_pos;

	float length = TS->shaped_text_get_width(rid);
	if (width > 0) {
		switch (alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
		case HORIZONTAL_ALIGNMENT_LEFT:
			break;
		case HORIZONTAL_ALIGNMENT_CENTER: {
			if (length <= width) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += Math::floor((width - length) / 2.0);
				}
				else {
					ofs.y += Math::floor((width - length) / 2.0);
				}
			}
			else if (TS->shaped_text_get_inferred_direction(rid) == TextServer::DIRECTION_RTL) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += width - length;
				}
				else {
					ofs.y += width - length;
				}
			}
		} break;
		case HORIZONTAL_ALIGNMENT_RIGHT: {
			if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += width - length;
			}
			else {
				ofs.y += width - length;
			}
		} break;
		}
	}

	float clip_l;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.x - ofs.x);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.y - ofs.y);
	}
	return TS->shaped_text_draw(
		rid, p_canvas, ofs, clip_l, clip_l + width, p_color, p_oversampling);
}

void TextLine::draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, float p_oversampling) const
{
	_shape();

	Vector2 ofs = p_pos;

	float length = TS->shaped_text_get_width(rid);
	if (width > 0) {
		switch (alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
		case HORIZONTAL_ALIGNMENT_LEFT:
			break;
		case HORIZONTAL_ALIGNMENT_CENTER: {
			if (length <= width) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += Math::floor((width - length) / 2.0);
				}
				else {
					ofs.y += Math::floor((width - length) / 2.0);
				}
			}
			else if (TS->shaped_text_get_inferred_direction(rid) == TextServer::DIRECTION_RTL) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += width - length;
				}
				else {
					ofs.y += width - length;
				}
			}
		} break;
		case HORIZONTAL_ALIGNMENT_RIGHT: {
			if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += width - length;
			}
			else {
				ofs.y += width - length;
			}
		} break;
		}
	}

	float clip_l;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.x - ofs.x);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.y - ofs.y);
	}
	return TS->shaped_text_draw_outline(
		rid, p_canvas, ofs, clip_l, clip_l + width, p_outline_size, p_color, p_oversampling);
}

int TextLine::hit_test(float p_coords) const
{
	_shape();

	return TS->shaped_text_hit_test_position(rid, p_coords);
}

TextLine::TextLine(const String& p_text, const Ref<Font>& p_font, int p_font_size,
	const String& p_language, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation)
{
	rid = TS->create_shaped_text(p_direction, p_orientation);
	if (p_font.is_valid()) {
		TS->shaped_text_add_string(rid, p_text, p_font->get_rids(), p_font_size,
			p_font->get_opentype_features(), p_language);
	}
}

TextLine::TextLine() { rid = TS->create_shaped_text(); }

TextLine::~TextLine() { TS->free_rid(rid); }


