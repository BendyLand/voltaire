/**************************************************************************/
/*  syntax_highlighter.cpp                                                */
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

#include "scene/gui/text_edit.h"
#include "syntax_highlighter.h"

void SyntaxHighlighter::update_cache()
{
	clear_highlighting_cache();

	if (text_edit == nullptr) {
		return;
	}
	_update_cache();
}

TextEdit* SyntaxHighlighter::get_text_edit() const { return text_edit; }

void CodeHighlighter::_clear_highlighting_cache() { color_region_cache.clear(); }

void CodeHighlighter::_update_cache() { font_color = text_edit->get_font_color(); }

void CodeHighlighter::add_color_region(
	const String& p_start_key, const String& p_end_key, const Color& p_color, bool p_line_only)
{
	for (int i = 0; i < p_start_key.length(); i++) {
		ERR_FAIL_COND_MSG(!is_symbol(p_start_key[i]), "color regions must start with a symbol");
	}

	if (p_end_key.length() > 0) {
		for (int i = 0; i < p_end_key.length(); i++) {
			ERR_FAIL_COND_MSG(!is_symbol(p_end_key[i]), "color regions must end with a symbol");
		}
	}

	int at = 0;
	for (int i = 0; i < color_regions.size(); i++) {
		ERR_FAIL_COND_MSG(color_regions[i].start_key == p_start_key,
			"color region with start key '" + p_start_key + "' already exists.");
		if (p_start_key.length() < color_regions[i].start_key.length()) {
			at++;
		}
	}

	ColorRegion color_region;
	color_region.color = p_color;
	color_region.start_key = p_start_key;
	color_region.end_key = p_end_key;
	color_region.line_only = p_line_only || p_end_key.is_empty();
	color_regions.insert(at, color_region);
	clear_highlighting_cache();
}

void CodeHighlighter::remove_color_region(const String& p_start_key)
{
	for (int i = 0; i < color_regions.size(); i++) {
		if (color_regions[i].start_key == p_start_key) {
			color_regions.remove_at(i);
			break;
		}
	}
	clear_highlighting_cache();
}

bool CodeHighlighter::has_color_region(const String& p_start_key) const
{
	for (int i = 0; i < color_regions.size(); i++) {
		if (color_regions[i].start_key == p_start_key) {
			return true;
		}
	}
	return false;
}

void CodeHighlighter::clear_color_regions()
{
	color_regions.clear();
	clear_highlighting_cache();
}

void CodeHighlighter::set_uint_suffix_enabled(bool p_enabled) { uint_suffix_enabled = p_enabled; }

void CodeHighlighter::set_number_color(Color p_color)
{
	number_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_number_color() const { return number_color; }

void CodeHighlighter::set_symbol_color(Color p_color)
{
	symbol_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_symbol_color() const { return symbol_color; }

void CodeHighlighter::set_function_color(Color p_color)
{
	function_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_function_color() const { return function_color; }

void CodeHighlighter::set_member_variable_color(Color p_color)
{
	member_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_member_variable_color() const { return member_color; }


