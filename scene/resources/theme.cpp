/**************************************************************************/
/*  theme.cpp                                                             */
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

Ref<Texture2D> Theme::get_icon(const String& p_name, const String& p_theme_type) const
{
	if (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) &&
		icon_map[p_theme_type][p_name].is_valid()) {
		return icon_map[p_theme_type][p_name];
	}
	else {
		return ThemeDB::get_fallback_icon();
	}
}

bool Theme::has_icon(const String& p_name, const String& p_theme_type) const
{
	return (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) &&
			icon_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_icon_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name));
}

void Theme::rename_icon(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
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

void Theme::get_icon_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!icon_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<Texture2D>>& E : icon_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_icon_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (icon_map.has(p_theme_type)) {
		return;
	}
	icon_map[p_theme_type] = ThemeIconMap();
}

void Theme::rename_icon_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!icon_map.has(p_old_theme_type) || icon_map.has(p_theme_type)) {
		return;
	}

	icon_map[p_theme_type] = icon_map[p_old_theme_type];
	icon_map.erase(p_old_theme_type);
}

void Theme::get_icon_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeIconMap>& E : icon_map) {
		p_list->push_back(E.key);
	}
}

Ref<StyleBox> Theme::get_stylebox(const String& p_name, const String& p_theme_type) const
{
	if (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) &&
		style_map[p_theme_type][p_name].is_valid()) {
		return style_map[p_theme_type][p_name];
	}
	else {
		return ThemeDB::get_fallback_stylebox();
	}
}

bool Theme::has_stylebox(const String& p_name, const String& p_theme_type) const
{
	return (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) &&
			style_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_stylebox_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name));
}

void Theme::rename_stylebox(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
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

void Theme::get_stylebox_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!style_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<StyleBox>>& E : style_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_stylebox_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (style_map.has(p_theme_type)) {
		return;
	}
	style_map[p_theme_type] = ThemeStyleMap();
}

void Theme::rename_stylebox_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!style_map.has(p_old_theme_type) || style_map.has(p_theme_type)) {
		return;
	}

	style_map[p_theme_type] = style_map[p_old_theme_type];
	style_map.erase(p_old_theme_type);
}

void Theme::get_stylebox_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeStyleMap>& E : style_map) {
		p_list->push_back(E.key);
	}
}

// Fonts.

Ref<Font> Theme::get_font(const String& p_name, const String& p_theme_type) const
{
	if (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
		font_map[p_theme_type][p_name].is_valid()) {
		return font_map[p_theme_type][p_name];
	}
	else if (has_default_font()) {
		return default_font;
	}
	else {
		return ThemeDB::get_fallback_font();
	}
}

bool Theme::has_font(const String& p_name, const String& p_theme_type) const
{
	return ((font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
				font_map[p_theme_type][p_name].is_valid()) ||
			has_default_font());
}

bool Theme::has_font_no_default(const String& p_name, const String& p_theme_type) const
{
	return (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
			font_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_font_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name));
}

void Theme::rename_font(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
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

void Theme::get_font_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!font_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<Font>>& E : font_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (font_map.has(p_theme_type)) {
		return;
	}
	font_map[p_theme_type] = ThemeFontMap();
}

void Theme::rename_font_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!font_map.has(p_old_theme_type) || font_map.has(p_theme_type)) {
		return;
	}

	font_map[p_theme_type] = font_map[p_old_theme_type];
	font_map.erase(p_old_theme_type);
}

void Theme::get_font_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeFontMap>& E : font_map) {
		p_list->push_back(E.key);
	}
}

// Font sizes.
void Theme::set_font_size(const String& p_name, const String& p_theme_type, int p_font_size)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_font_size_nocheck(p_name, p_theme_type);
	font_size_map[p_theme_type][p_name] = p_font_size;

	_emit_theme_changed(!existing);
}

int Theme::get_font_size(const String& p_name, const String& p_theme_type) const
{
	if (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
		(font_size_map[p_theme_type][p_name] > 0)) {
		return font_size_map[p_theme_type][p_name];
	}
	else if (has_default_font_size()) {
		return default_font_size;
	}
	else {
		return ThemeDB::get_fallback_font_size();
	}
}

bool Theme::has_font_size(const String& p_name, const String& p_theme_type) const
{
	return ((font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
				(font_size_map[p_theme_type][p_name] > 0)) ||
			has_default_font_size());
}

bool Theme::has_font_size_no_default(const String& p_name, const String& p_theme_type) const
{
	return (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
			(font_size_map[p_theme_type][p_name] > 0));
}

bool Theme::has_font_size_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name));
}

void Theme::rename_font_size(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
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

void Theme::clear_font_size(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!font_size_map.has(p_theme_type),
		"Cannot clear the font size '" + String(p_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!font_size_map[p_theme_type].has(p_name),
		"Cannot clear the font size '" + String(p_name) + "' because it does not exist.");

	font_size_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_font_size_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!font_size_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, int>& E : font_size_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_size_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (font_size_map.has(p_theme_type)) {
		return;
	}
	font_size_map[p_theme_type] = ThemeFontSizeMap();
}

void Theme::remove_font_size_type(const String& p_theme_type)
{
	if (!font_size_map.has(p_theme_type)) {
		return;
	}

	font_size_map.erase(p_theme_type);
}

void Theme::rename_font_size_type(
	const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!font_size_map.has(p_old_theme_type) || font_size_map.has(p_theme_type)) {
		return;
	}

	font_size_map[p_theme_type] = font_size_map[p_old_theme_type];
	font_size_map.erase(p_old_theme_type);
}

void Theme::get_font_size_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeFontSizeMap>& E : font_size_map) {
		p_list->push_back(E.key);
	}
}

// Colors.
void Theme::set_color(
	const String& p_name, const String& p_theme_type, const Color& p_color)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_color_nocheck(p_name, p_theme_type);
	color_map[p_theme_type][p_name] = p_color;

	_emit_theme_changed(!existing);
}

Color Theme::get_color(const String& p_name, const String& p_theme_type) const
{
	if (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name)) {
		return color_map[p_theme_type][p_name];
	}
	else {
		return Color();
	}
}

bool Theme::has_color(const String& p_name, const String& p_theme_type) const
{
	return (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name));
}

bool Theme::has_color_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name));
}

void Theme::rename_color(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
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

void Theme::clear_color(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!color_map.has(p_theme_type), "Cannot clear the color '" + String(p_name) +
														"' because the node type '" +
														String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!color_map[p_theme_type].has(p_name),
		"Cannot clear the color '" + String(p_name) + "' because it does not exist.");

	color_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_color_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!color_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Color>& E : color_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_color_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (color_map.has(p_theme_type)) {
		return;
	}
	color_map[p_theme_type] = ThemeColorMap();
}

void Theme::remove_color_type(const String& p_theme_type)
{
	if (!color_map.has(p_theme_type)) {
		return;
	}

	color_map.erase(p_theme_type);
}

void Theme::rename_color_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!color_map.has(p_old_theme_type) || color_map.has(p_theme_type)) {
		return;
	}

	color_map[p_theme_type] = color_map[p_old_theme_type];
	color_map.erase(p_old_theme_type);
}

void Theme::get_color_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeColorMap>& E : color_map) {
		p_list->push_back(E.key);
	}
}

// Theme constants.
void Theme::set_constant(const String& p_name, const String& p_theme_type, int p_constant)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_constant_nocheck(p_name, p_theme_type);
	constant_map[p_theme_type][p_name] = p_constant;

	_emit_theme_changed(!existing);
}

int Theme::get_constant(const String& p_name, const String& p_theme_type) const
{
	if (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name)) {
		return constant_map[p_theme_type][p_name];
	}
	else {
		return 0;
	}
}

bool Theme::has_constant(const String& p_name, const String& p_theme_type) const
{
	return (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name));
}

bool Theme::has_constant_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name));
}

void Theme::rename_constant(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
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

void Theme::clear_constant(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!constant_map.has(p_theme_type),
		"Cannot clear the constant '" + String(p_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!constant_map[p_theme_type].has(p_name),
		"Cannot clear the constant '" + String(p_name) + "' because it does not exist.");

	constant_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_constant_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!constant_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, int>& E : constant_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_constant_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (constant_map.has(p_theme_type)) {
		return;
	}
	constant_map[p_theme_type] = ThemeConstantMap();
}

void Theme::remove_constant_type(const String& p_theme_type)
{
	if (!constant_map.has(p_theme_type)) {
		return;
	}

	constant_map.erase(p_theme_type);
}

void Theme::rename_constant_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!constant_map.has(p_old_theme_type) || constant_map.has(p_theme_type)) {
		return;
	}

	constant_map[p_theme_type] = constant_map[p_old_theme_type];
	constant_map.erase(p_old_theme_type);
}

void Theme::get_constant_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeConstantMap>& E : constant_map) {
		p_list->push_back(E.key);
	}
}

bool Theme::has_theme_item(
	DataType p_data_type, const String& p_name, const String& p_theme_type) const
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
	DataType p_data_type, const String& p_name, const String& p_theme_type) const
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

void Theme::rename_theme_item(DataType p_data_type, const String& p_old_name,
	const String& p_name, const String& p_theme_type)
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
	DataType p_data_type, const String& p_name, const String& p_theme_type)
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
	DataType p_data_type, const String& p_theme_type, List<String>* p_list) const
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

void Theme::add_theme_item_type(DataType p_data_type, const String& p_theme_type)
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

void Theme::remove_theme_item_type(DataType p_data_type, const String& p_theme_type)
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
	DataType p_data_type, const String& p_old_theme_type, const String& p_theme_type)
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

void Theme::get_theme_item_type_list(DataType p_data_type, List<String>* p_list) const
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
void Theme::set_type_variation(const String& p_theme_type, const String& p_base_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_base_type), vformat("Invalid type name: '%s'", p_base_type));
	ERR_FAIL_COND_MSG(p_theme_type == String(),
		vformat("An empty theme type cannot be marked as a variation of another type (\"%s\").",
			p_base_type));
	ERR_FAIL_COND_MSG(
		p_base_type == String(), "An empty theme type cannot be the base type of a variation. "
									 "Use clear_type_variation() instead if you want to unmark '" +
										 String(p_theme_type) + "' as a variation.");

	if (variation_map.has(p_theme_type)) {
		String old_base = variation_map[p_theme_type];
		variation_base_map[old_base].erase(p_theme_type);
	}

	variation_map[p_theme_type] = p_base_type;
	variation_base_map[p_base_type].push_back(p_theme_type);

	_emit_theme_changed(true);
}

bool Theme::is_type_variation(const String& p_theme_type, const String& p_base_type) const
{
	return (variation_map.has(p_theme_type) && variation_map[p_theme_type] == p_base_type);
}

void Theme::clear_type_variation(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!variation_map.has(p_theme_type), "Cannot clear the type variation '" +
															String(p_theme_type) +
															"' because it does not exist.");

	String base_type = variation_map[p_theme_type];
	variation_base_map[base_type].erase(p_theme_type);
	variation_map.erase(p_theme_type);

	_emit_theme_changed(true);
}

void Theme::_emit_theme_changed(bool p_notify_list_changed) {}

String Theme::get_type_variation_base(const String& p_theme_type) const
{
	if (!variation_map.has(p_theme_type)) {
		return String();
	}

	return variation_map[p_theme_type];
}

void Theme::get_type_variation_list(const String& p_base_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!variation_base_map.has(p_base_type)) {
		return;
	}

	for (const String& E : variation_base_map[p_base_type]) {
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
void Theme::add_type(const String& p_theme_type)
{
	// Add a record to every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		add_theme_item_type(dt, p_theme_type);
	}

	_emit_theme_changed(true);
}

void Theme::remove_type(const String& p_theme_type)
{
	// Gracefully remove the record from every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		remove_theme_item_type(dt, p_theme_type);
	}

	// If type is a variation, remove that connection.
	if (get_type_variation_base(p_theme_type) != String()) {
		clear_type_variation(p_theme_type);
	}

	// If type is a variation base, remove all those connections.
	List<String> names;
	get_type_variation_list(p_theme_type, &names);
	for (const String& E : names) {
		clear_type_variation(E);
	}

	_emit_theme_changed(true);
}

void Theme::rename_type(const String& p_old_theme_type, const String& p_theme_type)
{
	// Gracefully rename the record in every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		rename_theme_item_type(dt, p_old_theme_type, p_theme_type);
	}

	// If type is a variation, replace that connection.
	const String base_type = get_type_variation_base(p_old_theme_type);
	if (base_type != String()) {
		clear_type_variation(p_old_theme_type);
		if (p_theme_type != String()) {
			set_type_variation(p_theme_type, base_type);
		}
	}

	// If type is a variation base, replace all those connections.
	List<String> names;
	get_type_variation_list(p_old_theme_type, &names);
	for (const String& E : names) {
		clear_type_variation(E);
		if (p_theme_type != String()) {
			set_type_variation(E, p_theme_type);
		}
	}

	_emit_theme_changed(true);
}

void Theme::get_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	// This Set guarantees uniqueness.
	// Because each map can have the same type defined, but for this method
	// we only want one occurrence of each type.
	HashSet<String> types;

	// Icons.
	for (const KeyValue<String, ThemeIconMap>& E : icon_map) {
		types.insert(E.key);
	}

	// Styles.
	for (const KeyValue<String, ThemeStyleMap>& E : style_map) {
		types.insert(E.key);
	}

	// Fonts.
	for (const KeyValue<String, ThemeFontMap>& E : font_map) {
		types.insert(E.key);
	}

	// Font sizes.
	for (const KeyValue<String, ThemeFontSizeMap>& E : font_size_map) {
		types.insert(E.key);
	}

	// Colors.
	for (const KeyValue<String, ThemeColorMap>& E : color_map) {
		types.insert(E.key);
	}

	// Constants.
	for (const KeyValue<String, ThemeConstantMap>& E : constant_map) {
		types.insert(E.key);
	}

	// Variations.
	for (const KeyValue<String, String>& E : variation_map) {
		types.insert(E.key);
	}

	for (const String& E : types) {
		p_list->push_back(E);
	}
}

void Theme::get_type_dependencies(
	const String& p_base_type, const String& p_type_variation, Vector<String>& r_result)
{
	// Build the dependency chain for type variations.
	if (p_type_variation != String()) {
		String variation_name = p_type_variation;
		while (variation_name != String()) {
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
	ThemeDB::get_native_type_dependencies(p_base_type, r_result);
}

// Internal methods for getting lists as a Vector of String (compatible with public API).
Vector<String> Theme::_get_icon_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_icon_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_icon_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_icon_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_stylebox_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_stylebox_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_font_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_font_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_font_size_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_font_size_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_color_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_color_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_constant_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_constant_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
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

Vector<String> Theme::_get_type_variation_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_type_variation_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
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
		for (const KeyValue<String, ThemeColorMap>& E : p_other->color_map) {
			for (const KeyValue<String, Color>& F : E.value) {
				set_color(F.key, E.key, F.value);
			}
		}
	}

	// Constants.
	{
		for (const KeyValue<String, ThemeConstantMap>& E : p_other->constant_map) {
			for (const KeyValue<String, int>& F : E.value) {
				set_constant(F.key, E.key, F.value);
			}
		}
	}

	// Fonts.
	{
		for (const KeyValue<String, ThemeFontMap>& E : p_other->font_map) {
			for (const KeyValue<String, Ref<Font>>& F : E.value) {
				set_font(F.key, E.key, F.value);
			}
		}
	}

	// Font sizes.
	{
		for (const KeyValue<String, ThemeFontSizeMap>& E : p_other->font_size_map) {
			for (const KeyValue<String, int>& F : E.value) {
				set_font_size(F.key, E.key, F.value);
			}
		}
	}

	// Icons.
	{
		for (const KeyValue<String, ThemeIconMap>& E : p_other->icon_map) {
			for (const KeyValue<String, Ref<Texture2D>>& F : E.value) {
				set_icon(F.key, E.key, F.value);
			}
		}
	}

	// Type variations.
	{
		for (const KeyValue<String, String>& E : p_other->variation_map) {
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



void Theme::set_icon(String const&, String const&, Ref<Texture2D> const&) {}

void Theme::set_font(String const&, String const&, Ref<Font> const&) {}

void Theme::set_default_font(Ref<Font> const&) {}

void Theme::remove_stylebox_type(String const&) {}

void Theme::remove_icon_type(String const&) {}

void Theme::remove_font_type(String const&) {}

void Theme::clear_stylebox(String const&, String const&) {}

void Theme::clear_icon(String const&, String const&) {}

void Theme::clear_font(String const&, String const&) {}

void Theme::clear() {}

// --- style_box.cpp ---

#include "style_box.h"

#include <cfloat> // FLT_EPSILON
#include "core/config/engine.h"
#include "scene/main/canvas_item.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"

void StyleBox::set_style_type(StyleType p_type)
{
	if (style_type == p_type) {
		return;
	}
	style_type = p_type;
	emit_changed();
}

float StyleBox::get_style_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	switch (style_type) {
	case STYLE_EMPTY:
		return 0.0;
	case STYLE_FLAT:
		return border_width[p_side];
	case STYLE_TEXTURE:
		return texture_margin[p_side];
	case STYLE_LINE:
		if (vertical) {
			if (p_side == SIDE_LEFT || p_side == SIDE_RIGHT) {
				return thickness / 2.0;
			}
		}
		else if (p_side == SIDE_TOP || p_side == SIDE_BOTTOM) {
			return thickness / 2.0;
		}
		return 0.0;
	}
	return 0.0;
}

Size2 StyleBox::get_minimum_size() const
{
	Size2 min_size = Size2(get_margin(SIDE_LEFT) + get_margin(SIDE_RIGHT),
		get_margin(SIDE_TOP) + get_margin(SIDE_BOTTOM));
	Size2 custom_size;
	if (min_size.x < custom_size.x) {
		min_size.x = custom_size.x;
	}
	if (min_size.y < custom_size.y) {
		min_size.y = custom_size.y;
	}

	return min_size;
}

void StyleBox::set_content_margin(Side p_side, float p_value)
{
	ERR_FAIL_INDEX((int)p_side, 4);

	content_margin[p_side] = p_value;
	emit_changed();
}

void StyleBox::set_content_margin_all(float p_value)
{
	for (int i = 0; i < 4; i++) {
		content_margin[i] = p_value;
	}
	emit_changed();
}

void StyleBox::set_content_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	content_margin[SIDE_LEFT] = p_left;
	content_margin[SIDE_TOP] = p_top;
	content_margin[SIDE_RIGHT] = p_right;
	content_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_content_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return content_margin[p_side];
}

float StyleBox::get_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	if (content_margin[p_side] < 0) {
		return get_style_margin(p_side);
	}
	else {
		return content_margin[p_side];
	}
}

Point2 StyleBox::get_offset() const { return Point2(get_margin(SIDE_LEFT), get_margin(SIDE_TOP)); }

CanvasItem* StyleBox::get_current_item_drawn() const
{
	return CanvasItem::get_current_item_drawn();
}

StyleBox::StyleBox(StyleType p_type) : style_type(p_type)
{
	for (int i = 0; i < 4; i++) {
		content_margin[i] = -1;
	}
}

Rect2 StyleBox::get_draw_rect(const Rect2& p_rect) const
{
	switch (style_type) {
	case STYLE_FLAT: {
		Rect2 draw_rect = p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
			expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);

		if (shadow_size > 0) {
			Rect2 shadow_rect = draw_rect.grow(shadow_size);
			shadow_rect.position += shadow_offset;
			draw_rect = draw_rect.merge(shadow_rect);
		}

		return draw_rect;
	}
	case STYLE_TEXTURE:
		return p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
			expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);
	case STYLE_EMPTY:
	case STYLE_LINE:
	default:
		return p_rect;
	}
}

void StyleBox::draw(RID p_canvas_item, const Rect2& p_rect) const
{
	switch (style_type) {
	case STYLE_EMPTY:
		return;
	case STYLE_FLAT:
		_draw_flat(p_canvas_item, p_rect);
		return;
	case STYLE_TEXTURE:
		_draw_texture(p_canvas_item, p_rect);
		return;
	case STYLE_LINE:
		_draw_line(p_canvas_item, p_rect);
		return;
	}
}

bool StyleBox::test_mask(const Vector2& p_point, const Rect2& p_rect) const { return true; }

// Shared STYLE_FLAT / STYLE_TEXTURE properties.

void StyleBox::set_expand_margin(Side p_side, float p_size)
{
	ERR_FAIL_INDEX((int)p_side, 4);
	expand_margin[p_side] = p_size;
	emit_changed();
}

void StyleBox::set_expand_margin_all(float p_expand_margin_size)
{
	for (int i = 0; i < 4; i++) {
		expand_margin[i] = p_expand_margin_size;
	}
	emit_changed();
}

void StyleBox::set_expand_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	expand_margin[SIDE_LEFT] = p_left;
	expand_margin[SIDE_TOP] = p_top;
	expand_margin[SIDE_RIGHT] = p_right;
	expand_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_expand_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);
	return expand_margin[p_side];
}

void StyleBox::set_draw_center(bool p_enabled)
{
	draw_center = p_enabled;
	emit_changed();
}

bool StyleBox::is_draw_center_enabled() const { return draw_center; }

// STYLE_FLAT properties & drawing.

void StyleBox::set_bg_color(const Color& p_color)
{
	bg_color = p_color;
	emit_changed();
}

Color StyleBox::get_bg_color() const { return bg_color; }

void StyleBox::set_border_color(const Color& p_color)
{
	border_color = p_color;
	emit_changed();
}

Color StyleBox::get_border_color() const { return border_color; }

void StyleBox::set_border_width_all(int p_size)
{
	border_width[0] = p_size;
	border_width[1] = p_size;
	border_width[2] = p_size;
	border_width[3] = p_size;
	emit_changed();
}

int StyleBox::get_border_width_min() const
{
	return MIN(MIN(border_width[0], border_width[1]), MIN(border_width[2], border_width[3]));
}

void StyleBox::set_border_width(Side p_side, int p_width)
{
	ERR_FAIL_INDEX((int)p_side, 4);
	border_width[p_side] = p_width;
	emit_changed();
}

int StyleBox::get_border_width(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return border_width[p_side];
}

void StyleBox::set_border_blend(bool p_blend)
{
	blend_border = p_blend;
	emit_changed();
}

bool StyleBox::get_border_blend() const { return blend_border; }

void StyleBox::set_corner_radius(const Corner p_corner, const int radius)
{
	ERR_FAIL_INDEX((int)p_corner, 4);
	corner_radius[p_corner] = radius;
	emit_changed();
}

void StyleBox::set_corner_radius_all(int radius)
{
	for (int i = 0; i < 4; i++) {
		corner_radius[i] = radius;
	}

	emit_changed();
}

void StyleBox::set_corner_radius_individual(const int radius_top_left,
	const int radius_top_right, const int radius_bottom_right, const int radius_bottom_left)
{
	corner_radius[0] = radius_top_left;
	corner_radius[1] = radius_top_right;
	corner_radius[2] = radius_bottom_right;
	corner_radius[3] = radius_bottom_left;

	emit_changed();
}

int StyleBox::get_corner_radius(const Corner p_corner) const
{
	ERR_FAIL_INDEX_V((int)p_corner, 4, 0);
	return corner_radius[p_corner];
}

void StyleBox::set_corner_detail(const int& p_corner_detail)
{
	corner_detail = CLAMP(p_corner_detail, 1, 20);
	emit_changed();
}

int StyleBox::get_corner_detail() const { return corner_detail; }

void StyleBox::set_skew(Vector2 p_skew)
{
	skew = p_skew;
	emit_changed();
}

Vector2 StyleBox::get_skew() const { return skew; }

void StyleBox::set_shadow_color(const Color& p_color)
{
	shadow_color = p_color;
	emit_changed();
}

Color StyleBox::get_shadow_color() const { return shadow_color; }

void StyleBox::set_shadow_size(const int& p_size)
{
	shadow_size = p_size;
	emit_changed();
}

int StyleBox::get_shadow_size() const { return shadow_size; }

void StyleBox::set_shadow_offset(const Point2& p_offset)
{
	shadow_offset = p_offset;
	emit_changed();
}

Point2 StyleBox::get_shadow_offset() const { return shadow_offset; }

void StyleBox::set_anti_aliased(const bool& p_anti_aliased)
{
	anti_aliased = p_anti_aliased;
	emit_changed();
}

bool StyleBox::is_anti_aliased() const { return anti_aliased; }

void StyleBox::set_aa_size(const real_t p_aa_size)
{
	aa_size = CLAMP(p_aa_size, 0.01, 10);
	emit_changed();
}

real_t StyleBox::get_aa_size() const { return aa_size; }

static inline void set_inner_corner_radius(const Rect2 style_rect, const Rect2 inner_rect,
	const real_t corner_radius[4], real_t* inner_corner_radius)
{
	real_t border_left = inner_rect.position.x - style_rect.position.x;
	real_t border_top = inner_rect.position.y - style_rect.position.y;
	real_t border_right = style_rect.size.width - inner_rect.size.width - border_left;
	real_t border_bottom = style_rect.size.height - inner_rect.size.height - border_top;

	inner_corner_radius[0] = MAX(corner_radius[0] - MIN(border_top, border_left), 0);  // Top left.
	inner_corner_radius[1] = MAX(corner_radius[1] - MIN(border_top, border_right), 0); // Top right.
	inner_corner_radius[2] =
		MAX(corner_radius[2] - MIN(border_bottom, border_right), 0); // Bottom right.
	inner_corner_radius[3] =
		MAX(corner_radius[3] - MIN(border_bottom, border_left), 0); // Bottom left.
}

static inline void set_corner_scale(const Rect2& style_rect, const Rect2& inner_rect,
	const real_t corner_radius[4], Point2* inner_scale)
{
	real_t border_left = inner_rect.position.x - style_rect.position.x;
	real_t border_top = inner_rect.position.y - style_rect.position.y;
	real_t border_right = style_rect.size.width - inner_rect.size.width - border_left;
	real_t border_bottom = style_rect.size.height - inner_rect.size.height - border_top;

	// Amount of overflow along an edge.
	// Ex. SIDE_LEFT edge is the overflow between top_left and bottom_left corners.
	// MIN(0,) is to ignore underflow, and negating is to make values positive.
	real_t edge_overflow[4] = {-MIN(0, inner_rect.size.y - corner_radius[CORNER_TOP_LEFT] -
										   corner_radius[CORNER_BOTTOM_LEFT]),
		-MIN(0,
			inner_rect.size.x - corner_radius[CORNER_TOP_LEFT] - corner_radius[CORNER_TOP_RIGHT]),
		-MIN(0, inner_rect.size.y - corner_radius[CORNER_TOP_RIGHT] -
					corner_radius[CORNER_BOTTOM_RIGHT]),
		-MIN(0, inner_rect.size.x - corner_radius[CORNER_BOTTOM_LEFT] -
					corner_radius[CORNER_BOTTOM_RIGHT])};

	// Sums of borders.
	real_t hb_sum = border_left + border_right;
	real_t vb_sum = border_top + border_bottom;

	// Ratio of each side to the sum of itself and opposite side.
	// Since overflow only happens with opposite borders, you only need to get the ratio of each
	// border relative to the sum of involved borders.
	real_t ratios[4] = {// Prevent divide by 0 errors.
		hb_sum > 0 ? (border_left / hb_sum) : 0, vb_sum > 0 ? (border_top / vb_sum) : 0,
		hb_sum > 0 ? (border_right / hb_sum) : 0, vb_sum > 0 ? (border_bottom / vb_sum) : 0};

	// Raw amount each corner should shrink.
	Point2 corner_reduction[4] = {
		Point2(edge_overflow[SIDE_TOP] * ratios[SIDE_LEFT],
			edge_overflow[SIDE_LEFT] * ratios[SIDE_TOP]),
		Point2(edge_overflow[SIDE_TOP] * ratios[SIDE_RIGHT],
			edge_overflow[SIDE_RIGHT] * ratios[SIDE_TOP]),
		Point2(edge_overflow[SIDE_BOTTOM] * ratios[SIDE_RIGHT],
			edge_overflow[SIDE_RIGHT] * ratios[SIDE_BOTTOM]),
		Point2(edge_overflow[SIDE_BOTTOM] * ratios[SIDE_LEFT],
			edge_overflow[SIDE_LEFT] * ratios[SIDE_BOTTOM]),
	};

	// Corner Radii as Point2s.
	Point2 pcr[4] = {
		Point2(corner_radius[0], corner_radius[0]),
		Point2(corner_radius[1], corner_radius[1]),
		Point2(corner_radius[2], corner_radius[2]),
		Point2(corner_radius[3], corner_radius[3]),
	};

	// If corner radii are too small, they won't shrink the full amount.
	// Adjacent corners will have to shrink the leftovers if they can.
	// Minf(0) is to ignore non-leftovers, and negating is to make values positive.
	Point2 leftovers[4] = {
		-((pcr[0] - corner_reduction[0]).minf(0)),
		-((pcr[1] - corner_reduction[1]).minf(0)),
		-((pcr[2] - corner_reduction[2]).minf(0)),
		-((pcr[3] - corner_reduction[3]).minf(0)),
	};

	// New shrunken radii after distributing the leftovers.
	Point2 distributed[4] = {
		((pcr[0] - corner_reduction[0] - leftovers[3] - leftovers[1]).maxf(0)),
		((pcr[1] - corner_reduction[1] - leftovers[0] - leftovers[2]).maxf(0)),
		((pcr[2] - corner_reduction[2] - leftovers[1] - leftovers[3]).maxf(0)),
		((pcr[3] - corner_reduction[3] - leftovers[2] - leftovers[0]).maxf(0)),
	};

	// How much the curve should scale to achieve the shrunken radii.
	for (int i = 0; i < 4; i++) {
		// Unshrinkable is how much is still left over, even after distributing leftovers.
		// Exclude it from the final scale.
		Point2 unshrinkable =
			(leftovers[(i + 1) % 4] + leftovers[(i + 4 - 1) % 4] - distributed[i]).maxf(0);
		inner_scale[i] = distributed[i] / (pcr[i] - unshrinkable).maxf(FLT_EPSILON);
	}
}

static inline void draw_rounded_rectangle(Vector<Vector2>& verts, Vector<int>& indices,
	Vector<Color>& colors, const Rect2& style_rect, const real_t corner_radius[4],
	const Rect2& ring_rect, const Rect2& inner_rect, const Color& inner_color,
	const Color& outer_color, const int corner_detail, const Vector2& skew, bool is_filled = false)
{
	int vert_offset = verts.size();
	int adapted_corner_detail = (corner_radius[0] > 0) || (corner_radius[1] > 0) ||
										(corner_radius[2] > 0) || (corner_radius[3] > 0)
									? corner_detail
									: 1;

	bool draw_border = !is_filled;

	real_t ring_corner_radius[4];
	set_inner_corner_radius(style_rect, ring_rect, corner_radius, ring_corner_radius);

	Point2 ring_scale[4];
	set_corner_scale(style_rect, ring_rect, ring_corner_radius, ring_scale);

	// Corner radius center points.
	Vector<Point2> outer_points = {
		ring_rect.position +
			Vector2(ring_corner_radius[0], ring_corner_radius[0]) * ring_scale[0], // tl
		Point2(ring_rect.position.x + ring_rect.size.x - ring_corner_radius[1] * ring_scale[1].x,
			ring_rect.position.y + ring_corner_radius[1] * ring_scale[1].y), // tr
		ring_rect.position + ring_rect.size -
			Vector2(ring_corner_radius[2], ring_corner_radius[2]) * ring_scale[2], // br
		Point2(ring_rect.position.x + ring_corner_radius[3] * ring_scale[3].x,
			ring_rect.position.y + ring_rect.size.y - ring_corner_radius[3] * ring_scale[3].y) // bl
	};

	real_t inner_corner_radius[4];
	set_inner_corner_radius(style_rect, inner_rect, corner_radius, inner_corner_radius);

	Point2 inner_scale[4];
	set_corner_scale(style_rect, inner_rect, inner_corner_radius, inner_scale);

	Vector<Point2> inner_points = {
		inner_rect.position +
			Vector2(inner_corner_radius[0], inner_corner_radius[0]) * inner_scale[0], // tl
		Point2(
			inner_rect.position.x + inner_rect.size.x - inner_corner_radius[1] * inner_scale[1].x,
			inner_rect.position.y + inner_corner_radius[1] * inner_scale[1].y), // tr
		inner_rect.position + inner_rect.size -
			Vector2(inner_corner_radius[2], inner_corner_radius[2]) * inner_scale[2], // br
		Point2(inner_rect.position.x + inner_corner_radius[3] * inner_scale[3].x,
			inner_rect.position.y + inner_rect.size.y -
				inner_corner_radius[3] * inner_scale[3].y) // bl
	};

	// Calculate the vertices.

	// If the center is filled, we do not draw the border and directly use the inner ring as
	// reference. Because all calls to this method either draw a ring or a filled rounded rectangle,
	// but not both.
	const real_t quarter_arc_rad = Math::PI / 2.0;
	const Point2 style_rect_center = style_rect.get_center();

	const int colors_size = colors.size();
	const int verts_size = verts.size();
	const int new_verts_amount = (adapted_corner_detail + 1) * (draw_border ? 8 : 4);

	colors.resize(colors_size + new_verts_amount);
	verts.resize(verts_size + new_verts_amount);
	Color* colors_ptr = colors.ptrw();
	Vector2* verts_ptr = verts.ptrw();

	for (int corner_idx = 0; corner_idx < 4; corner_idx++) {
		for (int detail = 0; detail <= adapted_corner_detail; detail++) {
			int idx_ofs = (adapted_corner_detail + 1) * corner_idx + detail;
			if (draw_border) {
				idx_ofs *= 2;
			}

			const real_t pt_angle =
				(corner_idx + detail / (double)adapted_corner_detail) * quarter_arc_rad + Math::PI;
			const real_t angle_cosine = Math::cos(pt_angle);
			const real_t angle_sine = Math::sin(pt_angle);

			{
				const real_t x =
					inner_corner_radius[corner_idx] * angle_cosine * inner_scale[corner_idx].x +
					inner_points[corner_idx].x;
				const real_t y =
					inner_corner_radius[corner_idx] * angle_sine * inner_scale[corner_idx].y +
					inner_points[corner_idx].y;
				const float x_skew = -skew.x * (y - style_rect_center.y);
				const float y_skew = -skew.y * (x - style_rect_center.x);
				verts_ptr[verts_size + idx_ofs] = Vector2(x + x_skew, y + y_skew);
				colors_ptr[colors_size + idx_ofs] = inner_color;
			}

			if (draw_border) {
				const real_t x =
					ring_corner_radius[corner_idx] * angle_cosine * ring_scale[corner_idx].x +
					outer_points[corner_idx].x;
				const real_t y =
					ring_corner_radius[corner_idx] * angle_sine * ring_scale[corner_idx].y +
					outer_points[corner_idx].y;
				const float x_skew = -skew.x * (y - style_rect_center.y);
				const float y_skew = -skew.y * (x - style_rect_center.x);
				verts_ptr[verts_size + idx_ofs + 1] = Vector2(x + x_skew, y + y_skew);
				colors_ptr[colors_size + idx_ofs + 1] = outer_color;
			}
		}
	}

	int ring_vert_count = verts.size() - vert_offset;

	// Fill the indices and the colors for the border.

	if (draw_border) {
		int indices_size = indices.size();
		indices.resize(indices_size + ring_vert_count * 3);
		int* indices_ptr = indices.ptrw();

		for (int i = 0; i < ring_vert_count; i++) {
			int idx_ofs = indices_size + i * 3;
			indices_ptr[idx_ofs] = vert_offset + i % ring_vert_count;
			indices_ptr[idx_ofs + 1] = vert_offset + (i + 2) % ring_vert_count;
			indices_ptr[idx_ofs + 2] = vert_offset + (i + 1) % ring_vert_count;
		}
	}

	if (is_filled) {
		// Compute the triangles pattern to draw the rounded rectangle.
		// Consists of vertical stripes of two triangles each.

		int stripes_count = ring_vert_count / 2 - 1;
		int last_vert_id = ring_vert_count - 1;

		int indices_size = indices.size();
		indices.resize(indices_size + stripes_count * 6);
		int* indices_ptr = indices.ptrw();

		for (int i = 0; i < stripes_count; i++) {
			int idx_ofs = indices_size + i * 6;
			// Polygon 1.
			indices_ptr[idx_ofs] = vert_offset + i;
			indices_ptr[idx_ofs + 1] = vert_offset + last_vert_id - i - 1;
			indices_ptr[idx_ofs + 2] = vert_offset + i + 1;
			// Polygon 2.
			indices_ptr[idx_ofs + 3] = vert_offset + i;
			indices_ptr[idx_ofs + 4] = vert_offset + last_vert_id - i;
			indices_ptr[idx_ofs + 5] = vert_offset + last_vert_id - i - 1;
		}
	}
}

static inline void adapt_values(int p_index_a, int p_index_b, real_t* adapted_values,
	const real_t* p_values, const real_t p_width, const real_t p_max_a, const real_t p_max_b)
{
	real_t value_a = p_values[p_index_a];
	real_t value_b = p_values[p_index_b];
	real_t factor = MIN(1.0, p_width / (value_a + value_b));
	adapted_values[p_index_a] = MIN(MIN(value_a * factor, p_max_a), adapted_values[p_index_a]);
	adapted_values[p_index_b] = MIN(MIN(value_b * factor, p_max_b), adapted_values[p_index_b]);
}

void StyleBox::_draw_flat(RID p_canvas_item, const Rect2& p_rect) const
{
	bool draw_border = (border_width[0] > 0) || (border_width[1] > 0) || (border_width[2] > 0) ||
					   (border_width[3] > 0);
	bool draw_shadow = (shadow_size > 0);
	if (!draw_border && !draw_center && !draw_shadow) {
		return;
	}

	Rect2 style_rect = p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
		expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);
	if (Math::is_zero_approx(style_rect.size.width) ||
		Math::is_zero_approx(style_rect.size.height)) {
		return;
	}

	const bool rounded_corners = (corner_radius[0] > 0) || (corner_radius[1] > 0) ||
								 (corner_radius[2] > 0) || (corner_radius[3] > 0);
	// Only enable antialiasing if it is actually needed. This improves performance
	// and maximizes sharpness for non-skewed StyleBoxes with sharp corners.
	const bool aa_on = (rounded_corners || !skew.is_zero_approx()) && anti_aliased;

	const bool blend_on = blend_border && draw_border;

	Color border_color_alpha = Color(border_color.r, border_color.g, border_color.b, 0);
	Color border_color_blend = (draw_center ? bg_color : border_color_alpha);
	Color border_color_inner = blend_on ? border_color_blend : border_color;

	// Adapt borders (prevent weird overlapping/glitchy drawings).
	real_t width = MAX(style_rect.size.width, 0);
	real_t height = MAX(style_rect.size.height, 0);
	real_t adapted_border[4] = {1000000.0, 1000000.0, 1000000.0, 1000000.0};
	adapt_values(SIDE_TOP, SIDE_BOTTOM, adapted_border, border_width, height, height, height);
	adapt_values(SIDE_LEFT, SIDE_RIGHT, adapted_border, border_width, width, width, width);

	// Adapt corners (prevent weird overlapping/glitchy drawings).
	real_t adapted_corner[4] = {1000000.0, 1000000.0, 1000000.0, 1000000.0};
	adapt_values(CORNER_TOP_RIGHT, CORNER_BOTTOM_RIGHT, adapted_corner, corner_radius, height,
		height - adapted_border[SIDE_BOTTOM], height - adapted_border[SIDE_TOP]);
	adapt_values(CORNER_TOP_LEFT, CORNER_BOTTOM_LEFT, adapted_corner, corner_radius, height,
		height - adapted_border[SIDE_BOTTOM], height - adapted_border[SIDE_TOP]);
	adapt_values(CORNER_TOP_LEFT, CORNER_TOP_RIGHT, adapted_corner, corner_radius, width,
		width - adapted_border[SIDE_RIGHT], width - adapted_border[SIDE_LEFT]);
	adapt_values(CORNER_BOTTOM_LEFT, CORNER_BOTTOM_RIGHT, adapted_corner, corner_radius, width,
		width - adapted_border[SIDE_RIGHT], width - adapted_border[SIDE_LEFT]);

	Rect2 infill_rect = style_rect.grow_individual(-adapted_border[SIDE_LEFT],
		-adapted_border[SIDE_TOP], -adapted_border[SIDE_RIGHT], -adapted_border[SIDE_BOTTOM]);

	Rect2 border_style_rect = style_rect;

	real_t aa_size_scaled = 1.0f;
	if (aa_on) {
		real_t scale_factor = TextServer::get_current_drawn_item_oversampling();
		if (scale_factor == 0.0) {
			scale_factor = 1.0;
		}

		// Adjust AA feather size to account for the 2D scale factor, so that
		// antialiasing doesn't become blurry at viewport resolutions higher
		// than the default when using the `canvas_items` stretch mode
		// (or when using `oversampling` values different than `1.0`).
		aa_size_scaled = aa_size / scale_factor;
	}

	if (aa_on) {
		for (int i = 0; i < 4; i++) {
			if (border_width[i] > 0) {
				border_style_rect = border_style_rect.grow_side((Side)i, -aa_size_scaled);
			}
		}
	}

	Vector<Point2> verts;
	Vector<int> indices;
	Vector<Color> colors;
	Vector<Point2> uvs;

	// Create shadow.
	if (draw_shadow) {
		Rect2 shadow_inner_rect = style_rect;
		shadow_inner_rect.position += shadow_offset;

		Rect2 shadow_rect = style_rect.grow(shadow_size);
		shadow_rect.position += shadow_offset;

		Color shadow_color_transparent = Color(shadow_color.r, shadow_color.g, shadow_color.b, 0);

		draw_rounded_rectangle(verts, indices, colors, shadow_inner_rect, adapted_corner,
			shadow_rect, shadow_inner_rect, shadow_color, shadow_color_transparent, corner_detail,
			skew);

		if (draw_center) {
			draw_rounded_rectangle(verts, indices, colors, shadow_inner_rect, adapted_corner,
				shadow_inner_rect, shadow_inner_rect, shadow_color, shadow_color, corner_detail,
				skew, true);
		}
	}

	// Create border (no AA).
	if (draw_border && !aa_on) {
		draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
			border_style_rect, infill_rect, border_color_inner, border_color, corner_detail, skew);
	}

	// Create infill (no AA).
	if (draw_center && (!aa_on || blend_on)) {
		draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
			infill_rect, infill_rect, bg_color, bg_color, corner_detail, skew, true);
	}

	if (aa_on) {
		real_t aa_border_width[4];
		real_t aa_border_width_half[4];
		real_t aa_fill_width[4];
		real_t aa_fill_width_half[4];

		if (draw_border) {
			for (int i = 0; i < 4; i++) {
				if (border_width[i] > 0) {
					aa_border_width[i] = aa_size_scaled;
					aa_border_width_half[i] = aa_size_scaled * 0.5;
					aa_fill_width[i] = 0;
					aa_fill_width_half[i] = 0;
				}
				else {
					aa_border_width[i] = 0;
					aa_border_width_half[i] = 0;
					aa_fill_width[i] = aa_size_scaled;
					aa_fill_width_half[i] = aa_size_scaled * 0.5;
				}
			}
		}
		else {
			for (int i = 0; i < 4; i++) {
				aa_border_width[i] = 0;
				aa_border_width_half[i] = 0;
				aa_fill_width[i] = aa_size_scaled;
				aa_fill_width_half[i] = aa_size_scaled * 0.5;
			}
		}

		if (draw_center) {
			// Infill rect, transparent side of antialiasing gradient (base infill rect enlarged by
			// AA size)
			Rect2 infill_rect_aa_transparent = infill_rect.grow_individual(
				aa_fill_width_half[SIDE_LEFT], aa_fill_width_half[SIDE_TOP],
				aa_fill_width_half[SIDE_RIGHT], aa_fill_width_half[SIDE_BOTTOM]);
			// Infill rect, colored side of antialiasing gradient (base infill rect shrunk by AA
			// size)
			Rect2 infill_rect_aa_colored = infill_rect_aa_transparent.grow_individual(
				-aa_fill_width[SIDE_LEFT], -aa_fill_width[SIDE_TOP], -aa_fill_width[SIDE_RIGHT],
				-aa_fill_width[SIDE_BOTTOM]);
			if (!blend_on) {
				// Create center fill, not antialiased yet
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					infill_rect_aa_colored, infill_rect_aa_colored, bg_color, bg_color,
					corner_detail, skew, true);
			}
			if (!blend_on || !draw_border) {
				Color alpha_bg = Color(bg_color.r, bg_color.g, bg_color.b, 0);
				// Add antialiasing on the center fill
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					infill_rect_aa_transparent, infill_rect_aa_colored, bg_color, alpha_bg,
					corner_detail, skew);
			}
		}

		if (draw_border) {
			// Inner border rect, fully colored side of antialiasing gradient (base inner rect
			// enlarged by AA size)
			Rect2 inner_rect_aa_colored = infill_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);
			// Inner border rect, transparent side of antialiasing gradient (base inner rect shrunk
			// by AA size)
			Rect2 inner_rect_aa_transparent = inner_rect_aa_colored.grow_individual(
				-aa_border_width[SIDE_LEFT], -aa_border_width[SIDE_TOP],
				-aa_border_width[SIDE_RIGHT], -aa_border_width[SIDE_BOTTOM]);
			// Outer border rect, transparent side of antialiasing gradient (base outer rect
			// enlarged by AA size)
			Rect2 outer_rect_aa_transparent = style_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);
			// Outer border rect, colored side of antialiasing gradient (base outer rect shrunk by
			// AA size)
			Rect2 outer_rect_aa_colored = border_style_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);

			// Create border ring, not antialiased yet
			draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
				outer_rect_aa_colored, ((blend_on) ? infill_rect : inner_rect_aa_colored),
				border_color_inner, border_color, corner_detail, skew);
			if (!blend_on) {
				// Add antialiasing on the ring inner border
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					inner_rect_aa_colored, inner_rect_aa_transparent, border_color_blend,
					border_color, corner_detail, skew);
			}
			// Add antialiasing on the ring outer border
			draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
				outer_rect_aa_transparent, outer_rect_aa_colored, border_color, border_color_alpha,
				corner_detail, skew);
		}
	}

	// Compute UV coordinates.
	Rect2 uv_rect = style_rect.grow(aa_on ? aa_size_scaled : 0);
	uvs.resize(verts.size());
	Point2* uvs_ptr = uvs.ptrw();
	for (int i = 0; i < verts.size(); i++) {
		uvs_ptr[i].x = (verts[i].x - uv_rect.position.x) / uv_rect.size.width;
		uvs_ptr[i].y = (verts[i].y - uv_rect.position.y) / uv_rect.size.height;
	}

	// Draw stylebox.
	Renderer::canvas_item_add_triangle_array(p_canvas_item, indices, verts, colors, uvs);
}

// STYLE_TEXTURE properties & drawing.

void StyleBox::set_texture(Ref<Texture2D> p_texture)
{
	if (texture == p_texture) {
		return;
	}
	texture = p_texture;
	emit_changed();
}

Ref<Texture2D> StyleBox::get_texture() const { return texture; }

void StyleBox::set_texture_margin(Side p_side, float p_size)
{
	ERR_FAIL_INDEX((int)p_side, 4);

	texture_margin[p_side] = p_size;
	emit_changed();
}

void StyleBox::set_texture_margin_all(float p_size)
{
	for (int i = 0; i < 4; i++) {
		texture_margin[i] = p_size;
	}
	emit_changed();
}

void StyleBox::set_texture_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	texture_margin[SIDE_LEFT] = p_left;
	texture_margin[SIDE_TOP] = p_top;
	texture_margin[SIDE_RIGHT] = p_right;
	texture_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_texture_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return texture_margin[p_side];
}

void StyleBox::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}

	region_rect = p_region_rect;
	emit_changed();
}

Rect2 StyleBox::get_region_rect() const { return region_rect; }

void StyleBox::set_h_axis_stretch_mode(AxisStretchMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, 3);
	axis_h = p_mode;
	emit_changed();
}

StyleBox::AxisStretchMode StyleBox::get_h_axis_stretch_mode() const { return axis_h; }

void StyleBox::set_v_axis_stretch_mode(AxisStretchMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, 3);
	axis_v = p_mode;
	emit_changed();
}

StyleBox::AxisStretchMode StyleBox::get_v_axis_stretch_mode() const { return axis_v; }

void StyleBox::set_modulate(const Color& p_modulate)
{
	if (modulate == p_modulate) {
		return;
	}
	modulate = p_modulate;
	emit_changed();
}

Color StyleBox::get_modulate() const { return modulate; }

void StyleBox::_draw_texture(RID p_canvas_item, const Rect2& p_rect) const
{
	if (texture.is_null()) {
		return;
	}

	Rect2 rect = p_rect;
	Rect2 src_rect = region_rect;

	texture->get_rect_region(rect, src_rect, rect, src_rect);

	rect.position.x -= expand_margin[SIDE_LEFT];
	rect.position.y -= expand_margin[SIDE_TOP];
	rect.size.x += expand_margin[SIDE_LEFT] + expand_margin[SIDE_RIGHT];
	rect.size.y += expand_margin[SIDE_TOP] + expand_margin[SIDE_BOTTOM];

	Vector2 start_offset = Vector2(texture_margin[SIDE_LEFT], texture_margin[SIDE_TOP]);
	Vector2 end_offset = Vector2(texture_margin[SIDE_RIGHT], texture_margin[SIDE_BOTTOM]);

	Renderer::canvas_item_add_nine_patch(p_canvas_item, rect, src_rect,
		texture->get_scaled_rid(), start_offset, end_offset, RSE::NinePatchAxisMode(axis_h),
		RSE::NinePatchAxisMode(axis_v), draw_center, modulate);
}

// STYLE_LINE properties & drawing.

void StyleBox::set_color(const Color& p_color)
{
	color = p_color;
	emit_changed();
}

Color StyleBox::get_color() const { return color; }

void StyleBox::set_thickness(int p_thickness)
{
	thickness = p_thickness;
	emit_changed();
}

int StyleBox::get_thickness() const { return thickness; }

void StyleBox::set_vertical(bool p_vertical)
{
	vertical = p_vertical;
	emit_changed();
}

bool StyleBox::is_vertical() const { return vertical; }

void StyleBox::set_grow_end(float p_grow_end)
{
	grow_end = p_grow_end;
	emit_changed();
}

float StyleBox::get_grow_end() const { return grow_end; }

void StyleBox::set_grow_begin(float p_grow_begin)
{
	grow_begin = p_grow_begin;
	emit_changed();
}

float StyleBox::get_grow_begin() const { return grow_begin; }

void StyleBox::_draw_line(RID p_canvas_item, const Rect2& p_rect) const
{
	Rect2i r = p_rect;

	if (vertical) {
		r.position.y -= grow_begin;
		r.size.y += (grow_begin + grow_end);
		r.size.x = thickness;
	}
	else {
		r.position.x -= grow_begin;
		r.size.x += (grow_begin + grow_end);
		r.size.y = thickness;
	}

	Renderer::canvas_item_add_rect(p_canvas_item, r, color);
}
