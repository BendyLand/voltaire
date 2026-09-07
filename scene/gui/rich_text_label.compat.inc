/**************************************************************************/
/*  rich_text_label.compat.inc                                            */
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

#include "rich_text_label.h"

void RichTextLabel::_push_font_bind_compat_79053(const Ref<Font>& p_font, int p_size)
{
	push_font(p_font, p_size);
}

void RichTextLabel::_set_table_column_expand_bind_compat_79053(
	int p_column, bool p_expand, int p_ratio)
{
	set_table_column_expand(p_column, p_expand, p_ratio, true);
}

void RichTextLabel::_set_table_column_expand_bind_compat_101482(
	int p_column, bool p_expand, int p_ratio)
{
	set_table_column_expand(p_column, p_expand, p_ratio, true);
}

void RichTextLabel::_push_table_bind_compat_76829(
	int p_columns, InlineAlignment p_alignment, int p_align_to_row)
{
	push_table(p_columns, p_alignment, p_align_to_row, String());
}

bool RichTextLabel::_remove_paragraph_bind_compat_91098(int p_paragraph)
{
	return remove_paragraph(p_paragraph, false);
}

void RichTextLabel::_push_underline_bind_compat_106300() { push_underline(Color(0, 0, 0, 0)); }

void RichTextLabel::_push_strikethrough_bind_compat_106300()
{
	push_strikethrough(Color(0, 0, 0, 0));
}

#endif // DISABLE_DEPRECATED


