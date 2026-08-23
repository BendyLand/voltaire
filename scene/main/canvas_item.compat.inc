/**************************************************************************/
/*  canvas_item.compat.inc                                                */
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

#ifndef DISABLE_DEPRECATED

#include "canvas_item.h"
#include "core/object/class_db.h"
#include "scene/resources/font.h"

void CanvasItem::_draw_string_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
	const String& p_text, HorizontalAlignment p_alignment, float p_width, int p_font_size,
	const Color& p_modulate, BitField<TextServer::JustificationFlag> p_jst_flags,
	TextServer::Direction p_direction, TextServer::Orientation p_orientation) const
{
	draw_string(p_font.ptr(), p_pos, p_text, p_alignment, p_width, p_font_size, p_modulate, p_jst_flags,
		p_direction, p_orientation, 0.0);
}

void CanvasItem::_draw_multiline_string_bind_compat_104872(const Ref<Font>& p_font,
	const Point2& p_pos, const String& p_text, HorizontalAlignment p_alignment, float p_width,
	int p_font_size, int p_max_lines, const Color& p_modulate,
	BitField<TextServer::LineBreakFlag> p_brk_flags,
	BitField<TextServer::JustificationFlag> p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation) const
{
	draw_multiline_string(p_font.ptr(), p_pos, p_text, p_alignment, p_width, p_font_size, p_max_lines,
		p_modulate, p_brk_flags, p_jst_flags, p_direction, p_orientation, 0.0);
}

void CanvasItem::_draw_string_outline_bind_compat_104872(const Ref<Font>& p_font,
	const Point2& p_pos, const String& p_text, HorizontalAlignment p_alignment, float p_width,
	int p_font_size, int p_size, const Color& p_modulate,
	BitField<TextServer::JustificationFlag> p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation) const
{
	draw_string_outline(p_font.ptr(), p_pos, p_text, p_alignment, p_width, p_font_size, p_size,
		p_modulate, p_jst_flags, p_direction, p_orientation, 0.0);
}

void CanvasItem::_draw_multiline_string_outline_bind_compat_104872(const Ref<Font>& p_font,
	const Point2& p_pos, const String& p_text, HorizontalAlignment p_alignment, float p_width,
	int p_font_size, int p_max_lines, int p_size, const Color& p_modulate,
	BitField<TextServer::LineBreakFlag> p_brk_flags,
	BitField<TextServer::JustificationFlag> p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation) const
{
	draw_multiline_string_outline(p_font.ptr(), p_pos, p_text, p_alignment, p_width, p_font_size,
		p_max_lines, p_size, p_modulate, p_brk_flags, p_jst_flags, p_direction, p_orientation, 0.0);
}

void CanvasItem::_draw_char_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
	const String& p_char, int p_font_size, const Color& p_modulate) const
{
	draw_char(p_font.ptr(), p_pos, p_char, p_font_size, p_modulate, 0.0);
}

void CanvasItem::_draw_char_outline_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
	const String& p_char, int p_font_size, int p_size, const Color& p_modulate) const
{
	draw_char_outline(p_font.ptr(), p_pos, p_char, p_font_size, p_size, p_modulate, 0.0);
}

void CanvasItem::_draw_circle_bind_compat_84472(
	const Point2& p_pos, real_t p_radius, const Color& p_color)
{
	draw_circle(p_pos, p_radius, p_color, true, -1.0, false);
}

void CanvasItem::_draw_rect_bind_compat_84523(
	const Rect2& p_rect, const Color& p_color, bool p_filled, real_t p_width)
{
	draw_rect(p_rect, p_color, p_filled, p_width, false);
}

void CanvasItem::_draw_dashed_line_bind_compat_84523(const Point2& p_from, const Point2& p_to,
	const Color& p_color, real_t p_width, real_t p_dash, bool p_aligned)
{
	draw_dashed_line(p_from, p_to, p_color, p_width, p_dash, p_aligned, false);
}

void CanvasItem::_draw_multiline_bind_compat_84523(
	const Vector<Point2>& p_points, const Color& p_color, real_t p_width)
{
	draw_multiline(p_points, p_color, p_width, false);
}

void CanvasItem::_draw_multiline_colors_bind_compat_84523(
	const Vector<Point2>& p_points, const Vector<Color>& p_colors, real_t p_width)
{
	draw_multiline_colors(p_points, p_colors, p_width, false);
}

void CanvasItem::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


