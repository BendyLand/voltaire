/**************************************************************************/
/*  menu_bar.cpp                                                          */
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

#include "menu_bar.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

void MenuBar::_open_popup(int p_index, bool p_focus_item)
{
	ERR_FAIL_INDEX(p_index, menu_cache.size());

	PopupMenu* pm = get_menu_popup(p_index);
	if (pm->is_visible()) {
		pm->hide();
		return;
	}

	Rect2 item_rect = _get_menu_item_rect(p_index);
	item_rect.position *= get_screen_transform().get_scale();
	item_rect.size *= get_screen_transform().get_scale();

	Rect2 rect = get_screen_rect();
	rect.position.x += item_rect.position.x;
	rect.position.y += rect.size.height;
	if (get_viewport()->is_embedding_subwindows() && pm->get_force_native()) {
		Transform2D xform = get_viewport()->get_popup_base_transform_native();
		rect = xform.xform(rect);
	}

	active_menu = p_index;

	pm->set_size(Size2(item_rect.size.x, 0));
	if (is_layout_rtl()) {
		rect.position.x += rect.size.width - pm->get_size().width;
	}
	pm->set_position(rect.position);
	pm->popup();

	if (p_focus_item) {
		for (int i = 0; i < pm->get_item_count(); i++) {
			if (!pm->is_item_disabled(i)) {
				pm->set_focused_item(i);
				break;
			}
		}
	}

	queue_redraw();
}

void MenuBar::_popup_visibility_changed(bool p_visible)
{
	if (!p_visible) {
		active_menu = -1;
		focused_menu = -1;
		set_process_internal(false);
		queue_redraw();
		return;
	}

	if (switch_on_hover) {
		set_process_internal(true);
	}
}

bool MenuBar::is_native_menu() const
{
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		return false;
	}
#endif

	return (
		NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU) && prefer_native);
}

void MenuBar::unbind_global_menu()
{
	if (global_menu_tag.is_empty()) {
		return;
	}

	NativeMenu* nmenu = NativeMenu::get_singleton();
	RID main_menu = nmenu->get_system_menu(NativeMenu::MAIN_MENU_ID);

	Vector<PopupMenu*> popups = _get_popups();
	for (int i = menu_cache.size() - 1; i >= 0; i--) {
		if (!popups[i]->is_system_menu()) {
			if (menu_cache[i].submenu_rid.is_valid()) {
				int item_idx =
					nmenu->find_item_index_with_submenu(main_menu, menu_cache[i].submenu_rid);
				if (item_idx >= 0) {
					nmenu->remove_item(main_menu, item_idx);
				}
			}
			popups[i]->unbind_global_menu();
			menu_cache.write[i].submenu_rid = RID();
		}
		menu_cache.write[i].sysmenu_id = NativeMenu::INVALID_MENU_ID;
	}

	global_menu_tag = String();
}

int MenuBar::_get_index_at_point(const Point2& p_point) const
{
	Ref<StyleBox> style = theme_cache.normal;
	int offset = 0;
	Point2 point = p_point;
	if (is_layout_rtl()) {
		point.x = get_size().x - point.x;
	}

	for (int i = 0; i < menu_cache.size(); i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 size = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		if (point.x > offset && point.x < offset + size.x) {
			if (point.y > 0 && point.y < size.y) {
				return i;
			}
		}
		offset += size.x + theme_cache.h_separation;
	}
	return -1;
}

Rect2 MenuBar::_get_menu_item_rect(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, menu_cache.size(), Rect2());

	Ref<StyleBox> style = theme_cache.normal;

	int offset = 0;
	for (int i = 0; i < p_index; i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 size = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		offset += size.x + theme_cache.h_separation;
	}

	Size2 size = menu_cache[p_index].text_buf->get_size() + style->get_minimum_size();
	if (is_layout_rtl()) {
		return Rect2(Point2(get_size().x - offset - size.x, 0), size);
	}
	else {
		return Rect2(Point2(offset, 0), size);
	}
}

void MenuBar::_draw_menu_item(int p_index)
{
	ERR_FAIL_INDEX(p_index, menu_cache.size());

	RID ci = get_canvas_item();
	bool hovered = (focused_menu == p_index);
	bool pressed = (active_menu == p_index);
	bool rtl = is_layout_rtl();

	if (menu_cache[p_index].hidden) {
		return;
	}

	Color color;
	Ref<StyleBox> style;
	Rect2 item_rect = _get_menu_item_rect(p_index);

	if (menu_cache[p_index].disabled) {
		if (rtl && has_theme_stylebox(SNAME("disabled_mirrored"))) {
			style = theme_cache.disabled_mirrored;
		}
		else {
			style = theme_cache.disabled;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		color = theme_cache.font_disabled_color;
	}
	else if (hovered && pressed && has_theme_stylebox("hover_pressed")) {
		if (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored"))) {
			style = theme_cache.hover_pressed_mirrored;
		}
		else {
			style = theme_cache.hover_pressed;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		if (has_theme_color(SNAME("font_hover_pressed_color"))) {
			color = theme_cache.font_hover_pressed_color;
		}
	}
	else if (pressed) {
		if (rtl && has_theme_stylebox(SNAME("pressed_mirrored"))) {
			style = theme_cache.pressed_mirrored;
		}
		else {
			style = theme_cache.pressed;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		if (has_theme_color(SNAME("font_pressed_color"))) {
			color = theme_cache.font_pressed_color;
		}
		else {
			color = theme_cache.font_color;
		}
	}
	else if (hovered) {
		if (rtl && has_theme_stylebox(SNAME("hover_mirrored"))) {
			style = theme_cache.hover_mirrored;
		}
		else {
			style = theme_cache.hover;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		color = theme_cache.font_hover_color;
	}
	else {
		if (rtl && has_theme_stylebox(SNAME("normal_mirrored"))) {
			style = theme_cache.normal_mirrored;
		}
		else {
			style = theme_cache.normal;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		// Focus colors only take precedence over normal state.
		if (has_focus(true)) {
			color = theme_cache.font_focus_color;
		}
		else {
			color = theme_cache.font_color;
		}
	}

	Point2 text_ofs =
		item_rect.position + Point2(style->get_margin(SIDE_LEFT), style->get_margin(SIDE_TOP));

	Color font_outline_color = theme_cache.font_outline_color;
	int outline_size = theme_cache.outline_size;
	if (outline_size > 0 && font_outline_color.a > 0) {
		menu_cache[p_index].text_buf->draw_outline(ci, text_ofs, outline_size, font_outline_color);
	}
	menu_cache[p_index].text_buf->draw(ci, text_ofs, color);
}

int MenuBar::get_menu_idx_from_control(PopupMenu* p_child) const
{
	ERR_FAIL_NULL_V(p_child, -1);
	ERR_FAIL_COND_V(p_child->get_parent() != this, -1);

	Vector<PopupMenu*> popups = _get_popups();
	for (int i = 0; i < popups.size(); i++) {
		if (popups[i] == p_child) {
			return i;
		}
	}

	return -1;
}

void MenuBar::set_switch_on_hover(bool p_enabled) { switch_on_hover = p_enabled; }

bool MenuBar::is_switch_on_hover() { return switch_on_hover; }

void MenuBar::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

void MenuBar::set_text_direction(Control::TextDirection p_text_direction)
{
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (text_direction != p_text_direction) {
		text_direction = p_text_direction;
		update_minimum_size();
		queue_redraw();
	}
}

Control::TextDirection MenuBar::get_text_direction() const { return text_direction; }

void MenuBar::set_language(const String& p_language)
{
	if (language != p_language) {
		language = p_language;
		update_minimum_size();
		queue_redraw();
	}
}

String MenuBar::get_language() const { return language; }

void MenuBar::set_flat(bool p_enabled)
{
	if (flat != p_enabled) {
		flat = p_enabled;
		queue_redraw();
	}
}

bool MenuBar::is_flat() const { return flat; }

void MenuBar::set_start_index(int p_index)
{
	if (start_index != p_index) {
		start_index = p_index;
		if (!global_menu_tag.is_empty()) {
			unbind_global_menu();
			bind_global_menu();
		}
	}
}

int MenuBar::get_start_index() const { return start_index; }

void MenuBar::set_prefer_global_menu(bool p_enabled)
{
	if (prefer_native != p_enabled) {
		prefer_native = p_enabled;
		if (prefer_native) {
			bind_global_menu();
		}
		else {
			unbind_global_menu();
		}
	}
}

bool MenuBar::is_prefer_global_menu() const { return prefer_native; }

Size2 MenuBar::get_minimum_size() const
{
	if (is_native_menu()) {
		return Size2();
	}

	Ref<StyleBox> style = theme_cache.normal;

	Vector2 size;
	for (int i = 0; i < menu_cache.size(); i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 sz = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		size.y = MAX(size.y, sz.y);
		size.x += sz.x;
	}
	if (menu_cache.size() > 1) {
		size.x += theme_cache.h_separation * (menu_cache.size() - 1);
	}
	return size;
}

int MenuBar::get_menu_count() const { return menu_cache.size(); }

String MenuBar::get_menu_title(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), String());
	return menu_cache[p_menu].name;
}

String MenuBar::get_menu_tooltip(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), String());
	return menu_cache[p_menu].tooltip;
}

void MenuBar::set_menu_disabled(int p_menu, bool p_disabled)
{
	ERR_FAIL_INDEX(p_menu, menu_cache.size());
	menu_cache.write[p_menu].disabled = p_disabled;
	if (!global_menu_tag.is_empty() && menu_cache[p_menu].submenu_rid.is_valid()) {
		NativeMenu* nmenu = NativeMenu::get_singleton();
		RID main_menu = nmenu->get_system_menu(NativeMenu::MAIN_MENU_ID);
		int item_idx =
			nmenu->find_item_index_with_submenu(main_menu, menu_cache[p_menu].submenu_rid);
		if (item_idx >= 0) {
			nmenu->set_item_disabled(main_menu, item_idx, p_disabled);
		}
	}
}

bool MenuBar::is_menu_disabled(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), false);
	return menu_cache[p_menu].disabled;
}

void MenuBar::set_menu_hidden(int p_menu, bool p_hidden)
{
	ERR_FAIL_INDEX(p_menu, menu_cache.size());
	menu_cache.write[p_menu].hidden = p_hidden;
	if (!global_menu_tag.is_empty() && menu_cache[p_menu].submenu_rid.is_valid()) {
		NativeMenu* nmenu = NativeMenu::get_singleton();
		RID main_menu = nmenu->get_system_menu(NativeMenu::MAIN_MENU_ID);
		int item_idx =
			nmenu->find_item_index_with_submenu(main_menu, menu_cache[p_menu].submenu_rid);
		if (item_idx >= 0) {
			nmenu->set_item_hidden(main_menu, item_idx, p_hidden);
		}
	}
	update_minimum_size();
}

bool MenuBar::is_menu_hidden(int p_menu)
const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), false);
	return menu_cache[p_menu].hidden;
}

PopupMenu* MenuBar::get_menu_popup(int p_idx) const
{
	Vector<PopupMenu*> controls = _get_popups();
	if (p_idx >= 0 && p_idx < controls.size()) {
		return controls[p_idx];
	}
	else {
		return nullptr;
	}
}

String MenuBar::get_tooltip(const Point2& p_pos) const
{
	int index = _get_index_at_point(p_pos);
	if (index >= 0 && index < menu_cache.size()) {
		return menu_cache[index].tooltip;
	}
	else {
		return String();
	}
}

MenuBar::MenuBar()
{
	set_focus_mode(FOCUS_ACCESSIBILITY);
	set_process_shortcut_input(true);
}

MenuBar::~MenuBar() {}


