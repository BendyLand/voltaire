/**************************************************************************/
/*  theme.cpp                                                             */
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

#include "scene/theme/theme_db.h"
#include "theme.h"

bool Theme::is_valid_type_name(const String& p_name)
{
	int len = p_name.length();
	const char32_t* str = p_name.ptr();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}
	return true;
}

bool Theme::is_valid_item_name(const String& p_name)
{
	if (p_name.is_empty()) {
		return false;
	}
	int len = p_name.length();
	const char32_t* str = p_name.ptr();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}
	return true;
}

String Theme::validate_type_name(const String& p_name)
{
	String type_name = p_name.strip_edges();
	int len = type_name.length();
	char32_t* buffer = type_name.ptrw();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(buffer[i])) {
			buffer[i] = '_';
		}
	}
	return type_name;
}

// Fallback values for theme item types, configurable per theme.
void Theme::set_default_base_scale(float p_base_scale)
{
	if (default_base_scale == p_base_scale) {
		return;
	}

	default_base_scale = p_base_scale;

	_emit_theme_changed();
}

float Theme::get_default_base_scale() const { return default_base_scale; }

bool Theme::has_default_base_scale() const { return default_base_scale > 0.0; }

Ref<Font> Theme::get_default_font() const { return default_font; }

bool Theme::has_default_font() const { return default_font.is_valid(); }

void Theme::set_default_font_size(int p_font_size)
{
	if (default_font_size == p_font_size) {
		return;
	}

	default_font_size = p_font_size;

	_emit_theme_changed();
}

int Theme::get_default_font_size() const { return default_font_size; }

bool Theme::has_default_font_size() const { return default_font_size > 0; }

Ref<Texture2D> Theme::get_icon(const StringName& p_name, const StringName& p_theme_type) const
{
	if (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) &&
		icon_map[p_theme_type][p_name].is_valid()) {
		return icon_map[p_theme_type][p_name];
	}
	else {
		return ThemeDB::get_singleton()->get_fallback_icon();
	}
}

bool Theme::has_icon(const StringName& p_name, const StringName& p_theme_type) const
{
	return (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) &&
			icon_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_icon_nocheck(const StringName& p_name, const StringName& p_theme_type) const
{
	return (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name));
}

void Theme::rename_icon(
	const StringName& p_old_name, const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!icon_map.has(p_theme_type), "Cannot rename the icon '" + String(p_old_name) +
													   "' because the node type '" +
													   String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(icon_map[p_theme_type].has(p_name),
		"Cannot rename the icon '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!icon_map[p_theme_type].has(p_old_name),
		"Cannot rename the icon '" + String(p_old_name) + "' because it does not exist.");

	icon_map[p_theme_type][p_name] = icon_map[p_theme_type][p_old_name];
	icon_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_icon_list(const StringName& p_theme_type, List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!icon_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Ref<Texture2D>>& E : icon_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_icon_type(const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (icon_map.has(p_theme_type)) {
		return;
	}
	icon_map[p_theme_type] = ThemeIconMap();
}

void Theme::rename_icon_type(const StringName& p_old_theme_type, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!icon_map.has(p_old_theme_type) || icon_map.has(p_theme_type)) {
		return;
	}

	icon_map[p_theme_type] = icon_map[p_old_theme_type];
	icon_map.erase(p_old_theme_type);
}

void Theme::get_icon_type_list(List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeIconMap>& E : icon_map) {
		p_list->push_back(E.key);
	}
}

Ref<StyleBox> Theme::get_stylebox(const StringName& p_name, const StringName& p_theme_type) const
{
	if (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) &&
		style_map[p_theme_type][p_name].is_valid()) {
		return style_map[p_theme_type][p_name];
	}
	else {
		return ThemeDB::get_singleton()->get_fallback_stylebox();
	}
}

bool Theme::has_stylebox(const StringName& p_name, const StringName& p_theme_type) const
{
	return (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) &&
			style_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_stylebox_nocheck(const StringName& p_name, const StringName& p_theme_type) const
{
	return (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name));
}

void Theme::rename_stylebox(
	const StringName& p_old_name, const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!style_map.has(p_theme_type),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(style_map[p_theme_type].has(p_name),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!style_map[p_theme_type].has(p_old_name),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because it does not exist.");

	style_map[p_theme_type][p_name] = style_map[p_theme_type][p_old_name];
	style_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_stylebox_list(const StringName& p_theme_type, List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!style_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Ref<StyleBox>>& E : style_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_stylebox_type(const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (style_map.has(p_theme_type)) {
		return;
	}
	style_map[p_theme_type] = ThemeStyleMap();
}

void Theme::rename_stylebox_type(const StringName& p_old_theme_type, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!style_map.has(p_old_theme_type) || style_map.has(p_theme_type)) {
		return;
	}

	style_map[p_theme_type] = style_map[p_old_theme_type];
	style_map.erase(p_old_theme_type);
}

void Theme::get_stylebox_type_list(List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeStyleMap>& E : style_map) {
		p_list->push_back(E.key);
	}
}

// Fonts.

Ref<Font> Theme::get_font(const StringName& p_name, const StringName& p_theme_type) const
{
	if (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
		font_map[p_theme_type][p_name].is_valid()) {
		return font_map[p_theme_type][p_name];
	}
	else if (has_default_font()) {
		return default_font;
	}
	else {
		return ThemeDB::get_singleton()->get_fallback_font();
	}
}

bool Theme::has_font(const StringName& p_name, const StringName& p_theme_type) const
{
	return ((font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
				font_map[p_theme_type][p_name].is_valid()) ||
			has_default_font());
}

bool Theme::has_font_no_default(const StringName& p_name, const StringName& p_theme_type) const
{
	return (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
			font_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_font_nocheck(const StringName& p_name, const StringName& p_theme_type) const
{
	return (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name));
}

void Theme::rename_font(
	const StringName& p_old_name, const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!font_map.has(p_theme_type), "Cannot rename the font '" + String(p_old_name) +
													   "' because the node type '" +
													   String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(font_map[p_theme_type].has(p_name),
		"Cannot rename the font '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!font_map[p_theme_type].has(p_old_name),
		"Cannot rename the font '" + String(p_old_name) + "' because it does not exist.");

	font_map[p_theme_type][p_name] = font_map[p_theme_type][p_old_name];
	font_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_font_list(const StringName& p_theme_type, List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!font_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Ref<Font>>& E : font_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_type(const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (font_map.has(p_theme_type)) {
		return;
	}
	font_map[p_theme_type] = ThemeFontMap();
}

void Theme::rename_font_type(const StringName& p_old_theme_type, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!font_map.has(p_old_theme_type) || font_map.has(p_theme_type)) {
		return;
	}

	font_map[p_theme_type] = font_map[p_old_theme_type];
	font_map.erase(p_old_theme_type);
}

void Theme::get_font_type_list(List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeFontMap>& E : font_map) {
		p_list->push_back(E.key);
	}
}

// Font sizes.
void Theme::set_font_size(const StringName& p_name, const StringName& p_theme_type, int p_font_size)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_font_size_nocheck(p_name, p_theme_type);
	font_size_map[p_theme_type][p_name] = p_font_size;

	_emit_theme_changed(!existing);
}

int Theme::get_font_size(const StringName& p_name, const StringName& p_theme_type) const
{
	if (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
		(font_size_map[p_theme_type][p_name] > 0)) {
		return font_size_map[p_theme_type][p_name];
	}
	else if (has_default_font_size()) {
		return default_font_size;
	}
	else {
		return ThemeDB::get_singleton()->get_fallback_font_size();
	}
}

bool Theme::has_font_size(const StringName& p_name, const StringName& p_theme_type) const
{
	return ((font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
				(font_size_map[p_theme_type][p_name] > 0)) ||
			has_default_font_size());
}

bool Theme::has_font_size_no_default(const StringName& p_name, const StringName& p_theme_type) const
{
	return (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
			(font_size_map[p_theme_type][p_name] > 0));
}

bool Theme::has_font_size_nocheck(const StringName& p_name, const StringName& p_theme_type) const
{
	return (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name));
}

void Theme::rename_font_size(
	const StringName& p_old_name, const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!font_size_map.has(p_theme_type),
		"Cannot rename the font size '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(font_size_map[p_theme_type].has(p_name),
		"Cannot rename the font size '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!font_size_map[p_theme_type].has(p_old_name),
		"Cannot rename the font size '" + String(p_old_name) + "' because it does not exist.");

	font_size_map[p_theme_type][p_name] = font_size_map[p_theme_type][p_old_name];
	font_size_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_font_size(const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!font_size_map.has(p_theme_type),
		"Cannot clear the font size '" + String(p_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!font_size_map[p_theme_type].has(p_name),
		"Cannot clear the font size '" + String(p_name) + "' because it does not exist.");

	font_size_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_font_size_list(const StringName& p_theme_type, List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!font_size_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, int>& E : font_size_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_size_type(const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (font_size_map.has(p_theme_type)) {
		return;
	}
	font_size_map[p_theme_type] = ThemeFontSizeMap();
}

void Theme::remove_font_size_type(const StringName& p_theme_type)
{
	if (!font_size_map.has(p_theme_type)) {
		return;
	}

	font_size_map.erase(p_theme_type);
}

void Theme::rename_font_size_type(
	const StringName& p_old_theme_type, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!font_size_map.has(p_old_theme_type) || font_size_map.has(p_theme_type)) {
		return;
	}

	font_size_map[p_theme_type] = font_size_map[p_old_theme_type];
	font_size_map.erase(p_old_theme_type);
}

void Theme::get_font_size_type_list(List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeFontSizeMap>& E : font_size_map) {
		p_list->push_back(E.key);
	}
}

// Colors.
void Theme::set_color(
	const StringName& p_name, const StringName& p_theme_type, const Color& p_color)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_color_nocheck(p_name, p_theme_type);
	color_map[p_theme_type][p_name] = p_color;

	_emit_theme_changed(!existing);
}

Color Theme::get_color(const StringName& p_name, const StringName& p_theme_type) const
{
	if (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name)) {
		return color_map[p_theme_type][p_name];
	}
	else {
		return Color();
	}
}

bool Theme::has_color(const StringName& p_name, const StringName& p_theme_type) const
{
	return (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name));
}

bool Theme::has_color_nocheck(const StringName& p_name, const StringName& p_theme_type) const
{
	return (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name));
}

void Theme::rename_color(
	const StringName& p_old_name, const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!color_map.has(p_theme_type),
		"Cannot rename the color '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(color_map[p_theme_type].has(p_name),
		"Cannot rename the color '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!color_map[p_theme_type].has(p_old_name),
		"Cannot rename the color '" + String(p_old_name) + "' because it does not exist.");

	color_map[p_theme_type][p_name] = color_map[p_theme_type][p_old_name];
	color_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_color(const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!color_map.has(p_theme_type), "Cannot clear the color '" + String(p_name) +
														"' because the node type '" +
														String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!color_map[p_theme_type].has(p_name),
		"Cannot clear the color '" + String(p_name) + "' because it does not exist.");

	color_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_color_list(const StringName& p_theme_type, List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!color_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, Color>& E : color_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_color_type(const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (color_map.has(p_theme_type)) {
		return;
	}
	color_map[p_theme_type] = ThemeColorMap();
}

void Theme::remove_color_type(const StringName& p_theme_type)
{
	if (!color_map.has(p_theme_type)) {
		return;
	}

	color_map.erase(p_theme_type);
}

void Theme::rename_color_type(const StringName& p_old_theme_type, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!color_map.has(p_old_theme_type) || color_map.has(p_theme_type)) {
		return;
	}

	color_map[p_theme_type] = color_map[p_old_theme_type];
	color_map.erase(p_old_theme_type);
}

void Theme::get_color_type_list(List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeColorMap>& E : color_map) {
		p_list->push_back(E.key);
	}
}

// Theme constants.
void Theme::set_constant(const StringName& p_name, const StringName& p_theme_type, int p_constant)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_constant_nocheck(p_name, p_theme_type);
	constant_map[p_theme_type][p_name] = p_constant;

	_emit_theme_changed(!existing);
}

int Theme::get_constant(const StringName& p_name, const StringName& p_theme_type) const
{
	if (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name)) {
		return constant_map[p_theme_type][p_name];
	}
	else {
		return 0;
	}
}

bool Theme::has_constant(const StringName& p_name, const StringName& p_theme_type) const
{
	return (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name));
}

bool Theme::has_constant_nocheck(const StringName& p_name, const StringName& p_theme_type) const
{
	return (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name));
}

void Theme::rename_constant(
	const StringName& p_old_name, const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!constant_map.has(p_theme_type),
		"Cannot rename the constant '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(constant_map[p_theme_type].has(p_name),
		"Cannot rename the constant '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!constant_map[p_theme_type].has(p_old_name),
		"Cannot rename the constant '" + String(p_old_name) + "' because it does not exist.");

	constant_map[p_theme_type][p_name] = constant_map[p_theme_type][p_old_name];
	constant_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_constant(const StringName& p_name, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!constant_map.has(p_theme_type),
		"Cannot clear the constant '" + String(p_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!constant_map[p_theme_type].has(p_name),
		"Cannot clear the constant '" + String(p_name) + "' because it does not exist.");

	constant_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_constant_list(const StringName& p_theme_type, List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!constant_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<StringName, int>& E : constant_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_constant_type(const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (constant_map.has(p_theme_type)) {
		return;
	}
	constant_map[p_theme_type] = ThemeConstantMap();
}

void Theme::remove_constant_type(const StringName& p_theme_type)
{
	if (!constant_map.has(p_theme_type)) {
		return;
	}

	constant_map.erase(p_theme_type);
}

void Theme::rename_constant_type(const StringName& p_old_theme_type, const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!constant_map.has(p_old_theme_type) || constant_map.has(p_theme_type)) {
		return;
	}

	constant_map[p_theme_type] = constant_map[p_old_theme_type];
	constant_map.erase(p_old_theme_type);
}

void Theme::get_constant_type_list(List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<StringName, ThemeConstantMap>& E : constant_map) {
		p_list->push_back(E.key);
	}
}

bool Theme::has_theme_item(
	DataType p_data_type, const StringName& p_name, const StringName& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return has_color(p_name, p_theme_type);
	case DATA_TYPE_CONSTANT:
		return has_constant(p_name, p_theme_type);
	case DATA_TYPE_FONT:
		if (!variation_map.has(p_theme_type)) {
			return has_font(p_name, p_theme_type);
		}
		else {
			return has_font_no_default(p_name, p_theme_type);
		}
	case DATA_TYPE_FONT_SIZE:
		if (!variation_map.has(p_theme_type)) {
			return has_font_size(p_name, p_theme_type);
		}
		else {
			return has_font_size_no_default(p_name, p_theme_type);
		}
	case DATA_TYPE_ICON:
		return has_icon(p_name, p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return has_stylebox(p_name, p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return false;
}

bool Theme::has_theme_item_nocheck(
	DataType p_data_type, const StringName& p_name, const StringName& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return has_color_nocheck(p_name, p_theme_type);
	case DATA_TYPE_CONSTANT:
		return has_constant_nocheck(p_name, p_theme_type);
	case DATA_TYPE_FONT:
		return has_font_nocheck(p_name, p_theme_type);
	case DATA_TYPE_FONT_SIZE:
		return has_font_size_nocheck(p_name, p_theme_type);
	case DATA_TYPE_ICON:
		return has_icon_nocheck(p_name, p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return has_stylebox_nocheck(p_name, p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return false;
}

void Theme::rename_theme_item(DataType p_data_type, const StringName& p_old_name,
	const StringName& p_name, const StringName& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		rename_color(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		rename_constant(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		rename_font(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		rename_font_size(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		rename_icon(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		rename_stylebox(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::clear_theme_item(
	DataType p_data_type, const StringName& p_name, const StringName& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		clear_color(p_name, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		clear_constant(p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		clear_font(p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		clear_font_size(p_name, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		clear_icon(p_name, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		clear_stylebox(p_name, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::get_theme_item_list(
	DataType p_data_type, const StringName& p_theme_type, List<StringName>* p_list) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		get_color_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_CONSTANT:
		get_constant_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_FONT:
		get_font_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_FONT_SIZE:
		get_font_size_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_ICON:
		get_icon_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_STYLEBOX:
		get_stylebox_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::add_theme_item_type(DataType p_data_type, const StringName& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		add_color_type(p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		add_constant_type(p_theme_type);
		break;
	case DATA_TYPE_FONT:
		add_font_type(p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		add_font_size_type(p_theme_type);
		break;
	case DATA_TYPE_ICON:
		add_icon_type(p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		add_stylebox_type(p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::remove_theme_item_type(DataType p_data_type, const StringName& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		remove_color_type(p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		remove_constant_type(p_theme_type);
		break;
	case DATA_TYPE_FONT:
		remove_font_type(p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		remove_font_size_type(p_theme_type);
		break;
	case DATA_TYPE_ICON:
		remove_icon_type(p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		remove_stylebox_type(p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::rename_theme_item_type(
	DataType p_data_type, const StringName& p_old_theme_type, const StringName& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		rename_color_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		rename_constant_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		rename_font_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		rename_font_size_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		rename_icon_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		rename_stylebox_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::get_theme_item_type_list(DataType p_data_type, List<StringName>* p_list) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		get_color_type_list(p_list);
		break;
	case DATA_TYPE_CONSTANT:
		get_constant_type_list(p_list);
		break;
	case DATA_TYPE_FONT:
		get_font_type_list(p_list);
		break;
	case DATA_TYPE_FONT_SIZE:
		get_font_size_type_list(p_list);
		break;
	case DATA_TYPE_ICON:
		get_icon_type_list(p_list);
		break;
	case DATA_TYPE_STYLEBOX:
		get_stylebox_type_list(p_list);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

// Theme type variations.
void Theme::set_type_variation(const StringName& p_theme_type, const StringName& p_base_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_base_type), vformat("Invalid type name: '%s'", p_base_type));
	ERR_FAIL_COND_MSG(p_theme_type == StringName(),
		vformat("An empty theme type cannot be marked as a variation of another type (\"%s\").",
			p_base_type));
	ERR_FAIL_COND_MSG(
		p_base_type == StringName(), "An empty theme type cannot be the base type of a variation. "
									 "Use clear_type_variation() instead if you want to unmark '" +
										 String(p_theme_type) + "' as a variation.");

	if (variation_map.has(p_theme_type)) {
		StringName old_base = variation_map[p_theme_type];
		variation_base_map[old_base].erase(p_theme_type);
	}

	variation_map[p_theme_type] = p_base_type;
	variation_base_map[p_base_type].push_back(p_theme_type);

	_emit_theme_changed(true);
}

bool Theme::is_type_variation(const StringName& p_theme_type, const StringName& p_base_type) const
{
	return (variation_map.has(p_theme_type) && variation_map[p_theme_type] == p_base_type);
}

void Theme::clear_type_variation(const StringName& p_theme_type)
{
	ERR_FAIL_COND_MSG(!variation_map.has(p_theme_type), "Cannot clear the type variation '" +
															String(p_theme_type) +
															"' because it does not exist.");

	StringName base_type = variation_map[p_theme_type];
	variation_base_map[base_type].erase(p_theme_type);
	variation_map.erase(p_theme_type);

	_emit_theme_changed(true);
}

StringName Theme::get_type_variation_base(const StringName& p_theme_type) const
{
	if (!variation_map.has(p_theme_type)) {
		return StringName();
	}

	return variation_map[p_theme_type];
}

void Theme::get_type_variation_list(const StringName& p_base_type, List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!variation_base_map.has(p_base_type)) {
		return;
	}

	for (const StringName& E : variation_base_map[p_base_type]) {
		// Prevent infinite loops if variants were set to be cross-dependent (that's still invalid
		// usage, but handling for stability sake).
		if (p_list->find(E)) {
			continue;
		}

		p_list->push_back(E);
		// Continue looking for sub-variations.
		get_type_variation_list(E, p_list);
	}
}

// Theme types.
void Theme::add_type(const StringName& p_theme_type)
{
	// Add a record to every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		add_theme_item_type(dt, p_theme_type);
	}

	_emit_theme_changed(true);
}

void Theme::remove_type(const StringName& p_theme_type)
{
	// Gracefully remove the record from every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		remove_theme_item_type(dt, p_theme_type);
	}

	// If type is a variation, remove that connection.
	if (get_type_variation_base(p_theme_type) != StringName()) {
		clear_type_variation(p_theme_type);
	}

	// If type is a variation base, remove all those connections.
	List<StringName> names;
	get_type_variation_list(p_theme_type, &names);
	for (const StringName& E : names) {
		clear_type_variation(E);
	}

	_emit_theme_changed(true);
}

void Theme::rename_type(const StringName& p_old_theme_type, const StringName& p_theme_type)
{
	// Gracefully rename the record in every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		rename_theme_item_type(dt, p_old_theme_type, p_theme_type);
	}

	// If type is a variation, replace that connection.
	const StringName base_type = get_type_variation_base(p_old_theme_type);
	if (base_type != StringName()) {
		clear_type_variation(p_old_theme_type);
		if (p_theme_type != StringName()) {
			set_type_variation(p_theme_type, base_type);
		}
	}

	// If type is a variation base, replace all those connections.
	List<StringName> names;
	get_type_variation_list(p_old_theme_type, &names);
	for (const StringName& E : names) {
		clear_type_variation(E);
		if (p_theme_type != StringName()) {
			set_type_variation(E, p_theme_type);
		}
	}

	_emit_theme_changed(true);
}

void Theme::get_type_list(List<StringName>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	// This Set guarantees uniqueness.
	// Because each map can have the same type defined, but for this method
	// we only want one occurrence of each type.
	HashSet<StringName> types;

	// Icons.
	for (const KeyValue<StringName, ThemeIconMap>& E : icon_map) {
		types.insert(E.key);
	}

	// Styles.
	for (const KeyValue<StringName, ThemeStyleMap>& E : style_map) {
		types.insert(E.key);
	}

	// Fonts.
	for (const KeyValue<StringName, ThemeFontMap>& E : font_map) {
		types.insert(E.key);
	}

	// Font sizes.
	for (const KeyValue<StringName, ThemeFontSizeMap>& E : font_size_map) {
		types.insert(E.key);
	}

	// Colors.
	for (const KeyValue<StringName, ThemeColorMap>& E : color_map) {
		types.insert(E.key);
	}

	// Constants.
	for (const KeyValue<StringName, ThemeConstantMap>& E : constant_map) {
		types.insert(E.key);
	}

	// Variations.
	for (const KeyValue<StringName, StringName>& E : variation_map) {
		types.insert(E.key);
	}

	for (const StringName& E : types) {
		p_list->push_back(E);
	}
}

void Theme::get_type_dependencies(
	const StringName& p_base_type, const StringName& p_type_variation, Vector<StringName>& r_result)
{
	// Build the dependency chain for type variations.
	if (p_type_variation != StringName()) {
		StringName variation_name = p_type_variation;
		while (variation_name != StringName()) {
			r_result.push_back(variation_name);
			variation_name = get_type_variation_base(variation_name);

			// If we have reached the base type dependency, it's safe to stop (assuming no funny
			// business was done to the Theme).
			if (variation_name == p_base_type) {
				break;
			}
		}
	}

	// Continue building the chain using native class hierarchy.
	ThemeDB::get_singleton()->get_native_type_dependencies(p_base_type, r_result);
}

// Internal methods for getting lists as a Vector of String (compatible with public API).
Vector<String> Theme::_get_icon_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<StringName> il;

	get_icon_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_icon_type_list() const
{
	Vector<String> ilret;
	List<StringName> il;

	get_icon_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<StringName> il;

	get_stylebox_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_type_list() const
{
	Vector<String> ilret;
	List<StringName> il;

	get_stylebox_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<StringName> il;

	get_font_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_type_list() const
{
	Vector<String> ilret;
	List<StringName> il;

	get_font_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<StringName> il;

	get_font_size_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_type_list() const
{
	Vector<String> ilret;
	List<StringName> il;

	get_font_size_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<StringName> il;

	get_color_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_type_list() const
{
	Vector<String> ilret;
	List<StringName> il;

	get_color_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<StringName> il;

	get_constant_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_type_list() const
{
	Vector<String> ilret;
	List<StringName> il;

	get_constant_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_theme_item_list(DataType p_data_type, const String& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return _get_color_list(p_theme_type);
	case DATA_TYPE_CONSTANT:
		return _get_constant_list(p_theme_type);
	case DATA_TYPE_FONT:
		return _get_font_list(p_theme_type);
	case DATA_TYPE_FONT_SIZE:
		return _get_font_size_list(p_theme_type);
	case DATA_TYPE_ICON:
		return _get_icon_list(p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return _get_stylebox_list(p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return Vector<String>();
}

Vector<String> Theme::_get_theme_item_type_list(DataType p_data_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return _get_color_type_list();
	case DATA_TYPE_CONSTANT:
		return _get_constant_type_list();
	case DATA_TYPE_FONT:
		return _get_font_type_list();
	case DATA_TYPE_FONT_SIZE:
		return _get_font_size_type_list();
	case DATA_TYPE_ICON:
		return _get_icon_type_list();
	case DATA_TYPE_STYLEBOX:
		return _get_stylebox_type_list();
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return Vector<String>();
}

Vector<String> Theme::_get_type_variation_list(const StringName& p_theme_type) const
{
	Vector<String> ilret;
	List<StringName> il;

	get_type_variation_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_type_list() const
{
	Vector<String> ilret;
	List<StringName> il;

	get_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<StringName>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

void Theme::_freeze_change_propagation() { no_change_propagation = true; }

void Theme::_unfreeze_and_propagate_changes()
{
	no_change_propagation = false;
	_emit_theme_changed(true);
}

void Theme::merge_with(const Ref<Theme>& p_other)
{
	if (p_other.is_null()) {
		return;
	}

	_freeze_change_propagation();

	// Colors.
	{
		for (const KeyValue<StringName, ThemeColorMap>& E : p_other->color_map) {
			for (const KeyValue<StringName, Color>& F : E.value) {
				set_color(F.key, E.key, F.value);
			}
		}
	}

	// Constants.
	{
		for (const KeyValue<StringName, ThemeConstantMap>& E : p_other->constant_map) {
			for (const KeyValue<StringName, int>& F : E.value) {
				set_constant(F.key, E.key, F.value);
			}
		}
	}

	// Fonts.
	{
		for (const KeyValue<StringName, ThemeFontMap>& E : p_other->font_map) {
			for (const KeyValue<StringName, Ref<Font>>& F : E.value) {
				set_font(F.key, E.key, F.value);
			}
		}
	}

	// Font sizes.
	{
		for (const KeyValue<StringName, ThemeFontSizeMap>& E : p_other->font_size_map) {
			for (const KeyValue<StringName, int>& F : E.value) {
				set_font_size(F.key, E.key, F.value);
			}
		}
	}

	// Icons.
	{
		for (const KeyValue<StringName, ThemeIconMap>& E : p_other->icon_map) {
			for (const KeyValue<StringName, Ref<Texture2D>>& F : E.value) {
				set_icon(F.key, E.key, F.value);
			}
		}
	}

	// Type variations.
	{
		for (const KeyValue<StringName, StringName>& E : p_other->variation_map) {
			set_type_variation(E.key, E.value);
		}
	}

	// Defaults.
	if (p_other->has_default_font()) {
		set_default_font(p_other->default_font);
	}
	if (p_other->has_default_font_size()) {
		set_default_font_size(p_other->default_font_size);
	}
	if (p_other->has_default_base_scale()) {
		set_default_base_scale(p_other->default_base_scale);
	}

	_unfreeze_and_propagate_changes();
}

void Theme::reset_state() { clear(); }

Theme::Theme() {}

Theme::~Theme() {}


