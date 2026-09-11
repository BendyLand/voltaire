/**************************************************************************/
/*  text_paragraph.cpp                                                    */
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

#include "text_paragraph.compat.inc"
#include "text_paragraph.h"

RID TextParagraph::get_rid() const { return rid; }

RID TextParagraph::get_line_rid(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), RID());
	return lines_rid[p_line];
}

RID TextParagraph::get_dropcap_rid() const { return dropcap_rid; }

void TextParagraph::clear()
{
	_THREAD_SAFE_METHOD_

	for (const RID& line_rid : lines_rid) {
		TS->free_rid(line_rid);
	}
	lines_rid.clear();
	TS->shaped_text_clear(rid);
	TS->shaped_text_clear(dropcap_rid);
}

Ref<TextParagraph> TextParagraph::duplicate() const
{
	Ref<TextParagraph> copy;
	copy.instantiate();
	if (dropcap_rid.is_valid()) {
		TS->free_rid(copy->dropcap_rid);
		copy->dropcap_rid = TS->shaped_text_duplicate(dropcap_rid);
	}
	copy->dropcap_lines = dropcap_lines;
	copy->dropcap_margins = dropcap_margins;
	if (rid.is_valid()) {
		TS->free_rid(copy->rid);
		copy->rid = TS->shaped_text_duplicate(rid);
	}
	copy->lines_dirty = true;
	copy->line_spacing = line_spacing;
	copy->width = width;
	copy->max_lines_visible = max_lines_visible;
	copy->brk_flags = brk_flags;
	copy->jst_flags = jst_flags;
	copy->el_char = el_char;
	copy->overrun_behavior = overrun_behavior;
	copy->alignment = alignment;
	copy->tab_stops = tab_stops;

	return copy;
}

void TextParagraph::set_preserve_invalid(bool p_enabled)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_preserve_invalid(rid, p_enabled);
	TS->shaped_text_set_preserve_invalid(dropcap_rid, p_enabled);
	lines_dirty = true;
}

bool TextParagraph::get_preserve_invalid() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_preserve_invalid(rid);
}

void TextParagraph::set_preserve_control(bool p_enabled)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_preserve_control(rid, p_enabled);
	TS->shaped_text_set_preserve_control(dropcap_rid, p_enabled);
	lines_dirty = true;
}

bool TextParagraph::get_preserve_control() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_preserve_control(rid);
}

void TextParagraph::set_direction(TextServer::Direction p_direction)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_direction(rid, p_direction);
	TS->shaped_text_set_direction(dropcap_rid, p_direction);
	lines_dirty = true;
}

TextServer::Direction TextParagraph::get_direction() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_direction(rid);
}

TextServer::Direction TextParagraph::get_inferred_direction() const
{
	_THREAD_SAFE_METHOD_

	const_cast<TextParagraph*>(this)->_shape_lines();
	return TS->shaped_text_get_inferred_direction(rid);
}

void TextParagraph::set_custom_punctuation(const String& p_punct)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_custom_punctuation(rid, p_punct);
	lines_dirty = true;
}

String TextParagraph::get_custom_punctuation() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_custom_punctuation(rid);
}

void TextParagraph::set_orientation(TextServer::Orientation p_orientation)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_orientation(rid, p_orientation);
	TS->shaped_text_set_orientation(dropcap_rid, p_orientation);
	lines_dirty = true;
}

TextServer::Orientation TextParagraph::get_orientation() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_orientation(rid);
}

void TextParagraph::clear_dropcap()
{
	_THREAD_SAFE_METHOD_
	dropcap_margins = Rect2();
	TS->shaped_text_clear(dropcap_rid);
	lines_dirty = true;
}

void TextParagraph::set_alignment(HorizontalAlignment p_alignment)
{
	_THREAD_SAFE_METHOD_

	if (alignment != p_alignment) {
		if (alignment == HORIZONTAL_ALIGNMENT_FILL || p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			alignment = p_alignment;
			lines_dirty = true;
		}
		else {
			alignment = p_alignment;
		}
	}
}

HorizontalAlignment TextParagraph::get_alignment() const { return alignment; }

void TextParagraph::tab_align(const Vector<float>& p_tab_stops)
{
	_THREAD_SAFE_METHOD_

	tab_stops = p_tab_stops;
	lines_dirty = true;
}

void TextParagraph::set_justification_flags(uint32_t p_flags)
{
	_THREAD_SAFE_METHOD_

	if (jst_flags != p_flags) {
		jst_flags = p_flags;
		lines_dirty = true;
	}
}

uint32_t TextParagraph::get_justification_flags() const { return jst_flags; }

void TextParagraph::set_break_flags(uint32_t p_flags)
{
	_THREAD_SAFE_METHOD_

	if (brk_flags != p_flags) {
		brk_flags = p_flags;
		lines_dirty = true;
	}
}

uint32_t TextParagraph::get_break_flags() const { return brk_flags; }

void TextParagraph::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	_THREAD_SAFE_METHOD_

	if (overrun_behavior != p_behavior) {
		overrun_behavior = p_behavior;
		lines_dirty = true;
	}
}

TextServer::OverrunBehavior TextParagraph::get_text_overrun_behavior() const
{
	return overrun_behavior;
}

void TextParagraph::set_ellipsis_char(const String& p_char)
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
	lines_dirty = true;
}

String TextParagraph::get_ellipsis_char() const { return el_char; }

void TextParagraph::set_width(float p_width)
{
	_THREAD_SAFE_METHOD_

	if (width != p_width) {
		width = p_width;
		lines_dirty = true;
	}
}

float TextParagraph::get_width() const { return width; }

Size2 TextParagraph::get_non_wrapped_size() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_size(rid);
}

Size2 TextParagraph::get_size() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();

	float h_offset = 0.f;
	float v_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
		v_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
		v_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}

	Size2 size;
	int visible_lines = (max_lines_visible >= 0) ? MIN(max_lines_visible, (int)lines_rid.size())
												 : (int)lines_rid.size();
	for (int i = 0; i < visible_lines; i++) {
		Size2 lsize = TS->shaped_text_get_size(lines_rid[i]);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			if (h_offset > 0 && i <= dropcap_lines) {
				lsize.x += h_offset;
			}
			size.x = MAX(size.x, lsize.x);
			size.y += lsize.y;
			if (i != visible_lines - 1) {
				size.y += line_spacing;
			}
		}
		else {
			if (h_offset > 0 && i <= dropcap_lines) {
				lsize.y += h_offset;
			}
			size.x += lsize.x;
			size.y = MAX(size.y, lsize.y);
			if (i != visible_lines - 1) {
				size.x += line_spacing;
			}
		}
	}
	if (h_offset > 0) {
		if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
			size.y = MAX(size.y, v_offset);
		}
		else {
			size.x = MAX(size.x, v_offset);
		}
	}
	return size;
}

Vector2i TextParagraph::get_range() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_range(rid);
}

int TextParagraph::get_line_count() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return (int)lines_rid.size();
}

void TextParagraph::set_max_lines_visible(int p_lines)
{
	_THREAD_SAFE_METHOD_

	if (p_lines != max_lines_visible) {
		max_lines_visible = p_lines;
		lines_dirty = true;
	}
}

int TextParagraph::get_max_lines_visible() const { return max_lines_visible; }

void TextParagraph::set_line_spacing(float p_spacing)
{
	_THREAD_SAFE_METHOD_

	if (line_spacing != p_spacing) {
		line_spacing = p_spacing;
		lines_dirty = true;
	}
}

float TextParagraph::get_line_spacing() const { return line_spacing; }

Size2 TextParagraph::get_line_size(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), Size2());
	return TS->shaped_text_get_size(lines_rid[p_line]);
}

Vector2i TextParagraph::get_line_range(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), Vector2i());
	return TS->shaped_text_get_range(lines_rid[p_line]);
}

float TextParagraph::get_line_ascent(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_ascent(lines_rid[p_line]);
}

float TextParagraph::get_line_descent(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_descent(lines_rid[p_line]);
}

float TextParagraph::get_line_width(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_width(lines_rid[p_line]);
}

float TextParagraph::get_line_underline_position(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_underline_position(lines_rid[p_line]);
}

float TextParagraph::get_line_underline_thickness(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_underline_thickness(lines_rid[p_line]);
}

Size2 TextParagraph::get_dropcap_size() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_size(dropcap_rid) + dropcap_margins.size + dropcap_margins.position;
}

int TextParagraph::get_dropcap_lines() const { return dropcap_lines; }

void TextParagraph::draw(RID p_canvas, const Vector2& p_pos, const Color& p_color,
	const Color& p_dc_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		Vector2 dc_off = ofs;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				dc_off.x += l_width - h_offset;
			}
			else {
				dc_off.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw(dropcap_rid, p_canvas,
			dc_off + Vector2(0, TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.size.y +
									dropcap_margins.position.y / 2),
			-1, -1, p_dc_color, p_oversampling);
	}

	int lines_visible = (max_lines_visible >= 0) ? MIN(max_lines_visible, (int)lines_rid.size())
												 : (int)lines_rid.size();

	for (int i = 0; i < lines_visible; i++) {
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.x -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.y -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		float line_width = TS->shaped_text_get_width(lines_rid[i]);
		if (l_width > 0) {
			switch (alignment) {
			case HORIZONTAL_ALIGNMENT_FILL:
				if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
					TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - line_width;
					}
					else {
						ofs.y += l_width - line_width;
					}
				}
				break;
			case HORIZONTAL_ALIGNMENT_LEFT:
				break;
			case HORIZONTAL_ALIGNMENT_CENTER: {
				if (line_width <= l_width) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += Math::floor((l_width - line_width) / 2.0);
					}
					else {
						ofs.y += Math::floor((l_width - line_width) / 2.0);
					}
				}
				else if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
						   TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - line_width;
					}
					else {
						ofs.y += l_width - line_width;
					}
				}
			} break;
			case HORIZONTAL_ALIGNMENT_RIGHT: {
				if (TS->shaped_text_get_orientation(lines_rid[i]) ==
					TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += l_width - line_width;
				}
				else {
					ofs.y += l_width - line_width;
				}
			} break;
			}
		}
		float clip_l;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			clip_l = MAX(0, p_pos.x - ofs.x);
		}
		else {
			clip_l = MAX(0, p_pos.y - ofs.y);
		}
		TS->shaped_text_draw(
			lines_rid[i], p_canvas, ofs, clip_l, clip_l + l_width, p_color, p_oversampling);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
	}
}

void TextParagraph::draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, const Color& p_dc_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs = p_pos;

	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		Vector2 dc_off = ofs;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				dc_off.x += l_width - h_offset;
			}
			else {
				dc_off.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw_outline(dropcap_rid, p_canvas,
			dc_off + Vector2(dropcap_margins.position.x,
						 TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_outline_size, p_dc_color, p_oversampling);
	}

	for (int i = 0; i < (int)lines_rid.size(); i++) {
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.x -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.y -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		float length = TS->shaped_text_get_width(lines_rid[i]);
		if (l_width > 0) {
			switch (alignment) {
			case HORIZONTAL_ALIGNMENT_FILL:
				if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
					TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - length;
					}
					else {
						ofs.y += l_width - length;
					}
				}
				break;
			case HORIZONTAL_ALIGNMENT_LEFT:
				break;
			case HORIZONTAL_ALIGNMENT_CENTER: {
				if (length <= l_width) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += Math::floor((l_width - length) / 2.0);
					}
					else {
						ofs.y += Math::floor((l_width - length) / 2.0);
					}
				}
				else if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
						   TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - length;
					}
					else {
						ofs.y += l_width - length;
					}
				}
			} break;
			case HORIZONTAL_ALIGNMENT_RIGHT: {
				if (TS->shaped_text_get_orientation(lines_rid[i]) ==
					TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += l_width - length;
				}
				else {
					ofs.y += l_width - length;
				}
			} break;
			}
		}
		float clip_l;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			clip_l = MAX(0, p_pos.x - ofs.x);
		}
		else {
			clip_l = MAX(0, p_pos.y - ofs.y);
		}
		TS->shaped_text_draw_outline(lines_rid[i], p_canvas, ofs, clip_l, clip_l + l_width,
			p_outline_size, p_color, p_oversampling);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
	}
}

int TextParagraph::hit_test(const Point2& p_coords) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		if (ofs.y < 0) {
			return 0;
		}
	}
	else {
		if (ofs.x < 0) {
			return 0;
		}
	}
	for (const RID& line_rid : lines_rid) {
		if (TS->shaped_text_get_orientation(line_rid) == TextServer::ORIENTATION_HORIZONTAL) {
			if ((p_coords.y >= ofs.y) &&
				(p_coords.y <= ofs.y + TS->shaped_text_get_size(line_rid).y)) {
				return TS->shaped_text_hit_test_position(line_rid, p_coords.x);
			}
			ofs.y += TS->shaped_text_get_size(line_rid).y + line_spacing;
		}
		else {
			if ((p_coords.x >= ofs.x) &&
				(p_coords.x <= ofs.x + TS->shaped_text_get_size(line_rid).x)) {
				return TS->shaped_text_hit_test_position(line_rid, p_coords.y);
			}
			ofs.y += TS->shaped_text_get_size(line_rid).x + line_spacing;
		}
	}
	return TS->shaped_text_get_range(rid).y;
}

bool TextParagraph::is_dirty() { return lines_dirty; }

void TextParagraph::draw_dropcap(
	RID p_canvas, const Vector2& p_pos, const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += l_width - h_offset;
			}
			else {
				ofs.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw(dropcap_rid, p_canvas,
			ofs + Vector2(dropcap_margins.position.x,
					  TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_color, p_oversampling);
	}
}

void TextParagraph::draw_dropcap_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += l_width - h_offset;
			}
			else {
				ofs.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw_outline(dropcap_rid, p_canvas,
			ofs + Vector2(dropcap_margins.position.x,
					  TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_outline_size, p_color, p_oversampling);
	}
}

void TextParagraph::draw_line(RID p_canvas, const Vector2& p_pos, int p_line, const Color& p_color,
	float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND(p_line < 0 || p_line >= (int)lines_rid.size());

	Vector2 ofs = p_pos;

	if (TS->shaped_text_get_orientation(lines_rid[p_line]) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	return TS->shaped_text_draw(lines_rid[p_line], p_canvas, ofs, -1, -1, p_color, p_oversampling);
}

void TextParagraph::draw_line_outline(RID p_canvas, const Vector2& p_pos, int p_line,
	int p_outline_size, const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND(p_line < 0 || p_line >= (int)lines_rid.size());

	Vector2 ofs = p_pos;
	if (TS->shaped_text_get_orientation(lines_rid[p_line]) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	return TS->shaped_text_draw_outline(
		lines_rid[p_line], p_canvas, ofs, -1, -1, p_outline_size, p_color, p_oversampling);
}

TextParagraph::TextParagraph()
{
	rid = TS->create_shaped_text();
	dropcap_rid = TS->create_shaped_text();
}

TextParagraph::~TextParagraph()
{
	for (const RID& line_rid : lines_rid) {
		TS->free_rid(line_rid);
	}
	lines_rid.clear();
	TS->free_rid(rid);
	TS->free_rid(dropcap_rid);
}


