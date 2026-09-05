/**************************************************************************/
/*  label_settings.cpp                                                    */
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

#include "label_settings.h"

void LabelSettings::_font_changed() { emit_changed(); }

void LabelSettings::_bind_methods() {}

void LabelSettings::set_line_spacing(real_t p_spacing)
{
	if (line_spacing != p_spacing) {
		line_spacing = p_spacing;
		emit_changed();
	}
}

real_t LabelSettings::get_line_spacing() const { return line_spacing; }

void LabelSettings::set_paragraph_spacing(real_t p_spacing)
{
	if (paragraph_spacing != p_spacing) {
		paragraph_spacing = p_spacing;
		emit_changed();
	}
}

real_t LabelSettings::get_paragraph_spacing() const { return paragraph_spacing; }

void LabelSettings::set_font(const Ref<Font>& p_font)
{
	if (font != p_font) {
		if (font.is_valid()) {
			font->disconnect_changed(callable_mp(this, &LabelSettings::_font_changed));
		}
		font = p_font;
		if (font.is_valid()) {
			font->connect_changed(callable_mp(this, &LabelSettings::_font_changed),
				Object::CONNECT_REFERENCE_COUNTED);
		}
		emit_changed();
	}
}

Ref<Font> LabelSettings::get_font() const { return font; }

void LabelSettings::set_font_size(int p_size)
{
	if (font_size != p_size) {
		font_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_font_size() const { return font_size; }

void LabelSettings::set_font_color(const Color& p_color)
{
	if (font_color != p_color) {
		font_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_font_color() const { return font_color; }

void LabelSettings::set_outline_size(int p_size)
{
	if (outline_size != p_size) {
		outline_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_outline_size() const { return outline_size; }

void LabelSettings::set_outline_color(const Color& p_color)
{
	if (outline_color != p_color) {
		outline_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_outline_color() const { return outline_color; }

void LabelSettings::set_shadow_size(int p_size)
{
	if (shadow_size != p_size) {
		shadow_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_shadow_size() const { return shadow_size; }

void LabelSettings::set_shadow_color(const Color& p_color)
{
	if (shadow_color != p_color) {
		shadow_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_shadow_color() const { return shadow_color; }

void LabelSettings::set_shadow_offset(const Vector2& p_offset)
{
	if (shadow_offset != p_offset) {
		shadow_offset = p_offset;
		emit_changed();
	}
}

Vector2 LabelSettings::get_shadow_offset() const { return shadow_offset; }

Vector<LabelSettings::StackedOutlineData> LabelSettings::get_stacked_outline_data() const
{
	return stacked_outline_data;
}

int LabelSettings::get_stacked_outline_count() const { return stacked_outline_data.size(); }

void LabelSettings::set_stacked_outline_count(int p_count)
{
	ERR_FAIL_COND(p_count < 0);
	if (stacked_outline_data.size() != p_count) {
		stacked_outline_data.resize(p_count);
		this->obj->notify_property_list_changed();
		emit_changed();
	}
}

void LabelSettings::add_stacked_outline(int p_index)
{
	if (p_index < 0) {
		p_index = stacked_outline_data.size();
	}
	ERR_FAIL_INDEX(p_index, stacked_outline_data.size() + 1);
	stacked_outline_data.insert(p_index, StackedOutlineData());
	this->obj->notify_property_list_changed();
	emit_changed();
}

void LabelSettings::move_stacked_outline(int p_from_index, int p_to_position)
{
	ERR_FAIL_INDEX(p_from_index, stacked_outline_data.size());
	ERR_FAIL_INDEX(p_to_position, stacked_outline_data.size() + 1);
	stacked_outline_data.insert(p_to_position, stacked_outline_data[p_from_index]);
	stacked_outline_data.remove_at(p_to_position < p_from_index ? p_from_index + 1 : p_from_index);
	this->obj->notify_property_list_changed();
	emit_changed();
}

void LabelSettings::remove_stacked_outline(int p_index)
{
	ERR_FAIL_INDEX(p_index, stacked_outline_data.size());
	stacked_outline_data.remove_at(p_index);
	this->obj->notify_property_list_changed();
	emit_changed();
}

void LabelSettings::set_stacked_outline_size(int p_index, int p_size)
{
	ERR_FAIL_INDEX(p_index, stacked_outline_data.size());
	if (stacked_outline_data[p_index].size != p_size) {
		stacked_outline_data.write[p_index].size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_stacked_outline_size(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_outline_data.size(), 0);
	return stacked_outline_data[p_index].size;
}

void LabelSettings::set_stacked_outline_color(int p_index, const Color& p_color)
{
	ERR_FAIL_INDEX(p_index, stacked_outline_data.size());
	if (stacked_outline_data[p_index].color != p_color) {
		stacked_outline_data.write[p_index].color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_stacked_outline_color(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_outline_data.size(), Color());
	return stacked_outline_data[p_index].color;
}

Vector<LabelSettings::StackedShadowData> LabelSettings::get_stacked_shadow_data() const
{
	return stacked_shadow_data;
}

int LabelSettings::get_stacked_shadow_count() const { return stacked_shadow_data.size(); }

void LabelSettings::set_stacked_shadow_count(int p_count)
{
	ERR_FAIL_COND(p_count < 0);
	if (stacked_shadow_data.size() != p_count) {
		stacked_shadow_data.resize(p_count);
		this->obj->notify_property_list_changed();
		emit_changed();
	}
}

void LabelSettings::add_stacked_shadow(int p_index)
{
	if (p_index < 0) {
		p_index = stacked_shadow_data.size();
	}
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size() + 1);
	stacked_shadow_data.insert(p_index, StackedShadowData());
	this->obj->notify_property_list_changed();
	emit_changed();
}

void LabelSettings::move_stacked_shadow(int p_from_index, int p_to_position)
{
	ERR_FAIL_INDEX(p_from_index, stacked_shadow_data.size());
	ERR_FAIL_INDEX(p_to_position, stacked_shadow_data.size() + 1);
	stacked_shadow_data.insert(p_to_position, stacked_shadow_data[p_from_index]);
	stacked_shadow_data.remove_at(p_to_position < p_from_index ? p_from_index + 1 : p_from_index);
	this->obj->notify_property_list_changed();
	emit_changed();
}

void LabelSettings::remove_stacked_shadow(int p_index)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	stacked_shadow_data.remove_at(p_index);
	this->obj->notify_property_list_changed();
	emit_changed();
}

void LabelSettings::set_stacked_shadow_offset(int p_index, const Vector2& p_offset)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].offset != p_offset) {
		stacked_shadow_data.write[p_index].offset = p_offset;
		emit_changed();
	}
}

Vector2 LabelSettings::get_stacked_shadow_offset(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), Vector2());
	return stacked_shadow_data[p_index].offset;
}

void LabelSettings::set_stacked_shadow_color(int p_index, const Color& p_color)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].color != p_color) {
		stacked_shadow_data.write[p_index].color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_stacked_shadow_color(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), Color());
	return stacked_shadow_data[p_index].color;
}

void LabelSettings::set_stacked_shadow_outline_size(int p_index, int p_size)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].outline_size != p_size) {
		stacked_shadow_data.write[p_index].outline_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_stacked_shadow_outline_size(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), 0);
	return stacked_shadow_data[p_index].outline_size;
}


