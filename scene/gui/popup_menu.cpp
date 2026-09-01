/**************************************************************************/
/*  popup_menu.cpp                                                        */
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

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/fuzzy_search.h"
#include "popup_menu.compat.inc"
#include "popup_menu.h"
#include "scene/gui/box_container.h"
#include "scene/gui/graph_edit.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/panel_container.h"
#include "scene/main/timer.h"
#include "scene/resources/style_box_flat.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"

HashMap<NativeMenu::SystemMenus, PopupMenu*> PopupMenu::system_menus;

bool PopupMenu::_set_item_accelerator(int p_index, const Ref<InputEventKey>& p_ie)
{
	NativeMenu* nmenu = NativeMenu::get_singleton();
	if (p_ie->get_physical_keycode() == Key::NONE && p_ie->get_keycode() == Key::NONE &&
		p_ie->get_key_label() != Key::NONE) {
		nmenu->set_item_accelerator(global_menu, p_index, p_ie->get_key_label_with_modifiers());
		return true;
	}
	else if (p_ie->get_keycode() != Key::NONE) {
		nmenu->set_item_accelerator(global_menu, p_index, p_ie->get_keycode_with_modifiers());
		return true;
	}
	else if (p_ie->get_physical_keycode() != Key::NONE) {
		nmenu->set_item_accelerator(global_menu, p_index,
			DisplayServer::get_singleton()->keyboard_get_keycode_from_physical(
				p_ie->get_physical_keycode_with_modifiers()));
		return true;
	}
	return false;
}

void PopupMenu::_set_item_checkable_type(int p_index, int p_checkable_type)
{
	switch (p_checkable_type) {
	case Item::CHECKABLE_TYPE_NONE: {
		set_item_as_checkable(p_index, false);
	} break;
	case Item::CHECKABLE_TYPE_CHECK_BOX: {
		set_item_as_checkable(p_index, true);
	} break;
	case Item::CHECKABLE_TYPE_RADIO_BUTTON: {
		set_item_as_radio_checkable(p_index, true);
	} break;
	}
}

int PopupMenu::_get_item_checkable_type(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, items.size(), Item::CHECKABLE_TYPE_NONE);
	return items[p_index].checkable_type;
}

void PopupMenu::unbind_global_menu()
{
	if (global_menu.is_null()) {
		return;
	}

	if (global_menu == system_menu && system_menus[system_menu_id] == this) {
		system_menus.erase(system_menu_id);
	}

	for (int i = 0; i < items.size(); i++) {
		Item& item = items.write[i];
		if (item.submenu) {
			item.submenu->unbind_global_menu();
			item.submenu_bound = false;
		}
	}
	if (system_menu != global_menu) {
		NativeMenu::get_singleton()->free_menu(global_menu);
	}
	else {
		NativeMenu::get_singleton()->clear(global_menu);
	}

	system_menu = RID();
	global_menu = RID();
}

bool PopupMenu::is_system_menu() const
{
	return (global_menu == system_menu) && (system_menu_id != NativeMenu::INVALID_MENU_ID);
}

void PopupMenu::set_system_menu(NativeMenu::SystemMenus p_system_menu_id)
{
	if (is_inside_tree() && system_menu_id != NativeMenu::INVALID_MENU_ID) {
		unbind_global_menu();
	}
	system_menu_id = p_system_menu_id;
	if (is_inside_tree() && system_menu_id != NativeMenu::INVALID_MENU_ID) {
		bind_global_menu();
	}
}

NativeMenu::SystemMenus PopupMenu::get_system_menu() const { return system_menu_id; }

String PopupMenu::_get_accel_text(const Item& p_item) const
{
	if (p_item.shortcut.is_valid()) {
		return p_item.shortcut->get_as_text();
	}
	else if (p_item.accel != Key::NONE) {
		return keycode_get_string(p_item.accel);
	}
	return String();
}

Size2 PopupMenu::_get_item_icon_size(int p_idx) const
{
	const PopupMenu::Item& item = items[p_idx];
	Size2 icon_size = item.get_icon_size();

	int max_width = 0;
	if (theme_cache.icon_max_width > 0) {
		max_width = theme_cache.icon_max_width;
	}
	if (item.icon_max_width > 0 && (max_width == 0 || item.icon_max_width < max_width)) {
		max_width = item.icon_max_width;
	}

	if (max_width > 0 && icon_size.width > max_width) {
		icon_size.height = icon_size.height * max_width / icon_size.width;
		icon_size.width = max_width;
	}

	return icon_size;
}

Size2 PopupMenu::_get_contents_minimum_size() const
{
	Size2 minsize = theme_cache.panel_style->get_minimum_size();
	minsize.width += scroll_container->get_v_scroll_bar()->get_size().width;
	// Take shadows into account.
	minsize.width += panel->get_offset(SIDE_LEFT) - panel->get_offset(SIDE_RIGHT);
	minsize.height += panel->get_offset(SIDE_TOP) - panel->get_offset(SIDE_BOTTOM);

	real_t body_max_w = 0.0; // Indentation, text, and submenu arrow.
	real_t icon_max_w = 0.0;
	real_t accel_max_w = 0.0;
	bool has_check_gutter = false;
	bool gutter_compact = theme_cache.gutter_compact;

	for (int i = 0; i < items.size(); i++) {
		_shape_item(i);

		icon_max_w = MAX(_get_item_icon_size(i).width, icon_max_w);

		if (items[i].checkable_type && !items[i].separator) {
			has_check_gutter = true;
			if (items[i].icon.is_valid()) {
				gutter_compact = false;
			}
		}

		if (items[i].accel != Key::NONE ||
			(items[i].shortcut.is_valid() && items[i].shortcut->has_valid_event())) {
			real_t accel_w = theme_cache.h_separation * 2 + items[i].accel_text_buf->get_size().x;
			accel_max_w = MAX(accel_w, accel_max_w);
		}

		real_t body_w = items[i].indent * theme_cache.indent + items[i].text_buf->get_size().x;
		if (items[i].submenu) {
			body_w += theme_cache.submenu->get_width();
		}
		body_max_w = MAX(body_max_w, body_w);

		minsize.height += _get_item_height(i) + theme_cache.v_separation;
	}

	body_max_w =
		theme_cache.item_start_padding + body_max_w + accel_max_w + theme_cache.item_end_padding;

	const int check_w =
		MAX(theme_cache.checked->get_width(), theme_cache.radio_checked->get_width());
	if (gutter_compact) {
		body_max_w += MAX(icon_max_w, check_w) + theme_cache.h_separation;
	}
	else {
		if (icon_max_w > 0) {
			body_max_w += icon_max_w + theme_cache.h_separation;
		}
		if (has_check_gutter) {
			body_max_w += check_w + theme_cache.h_separation;
		}
	}

	if (search_bar->is_visible()) {
		Size2 sb_min_size = search_bar->get_minimum_size();
		minsize.width += MAX(body_max_w, sb_min_size.width);
		minsize.height += sb_min_size.height + theme_cache.search_bar_separation;
	}
	else {
		minsize.width += body_max_w;
	}

	if (is_inside_tree()) {
		int height_limit = get_usable_parent_rect().size.height;
		if (minsize.height > height_limit) {
			minsize.height = height_limit;
		}
	}

	minsize.height = Math::ceil(minsize.height); // Ensures enough height at fractional content
												 // scales to prevent the v_scroll_bar from showing.
	return minsize;
}

int PopupMenu::_get_item_height(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);

	Size2 icon_size = _get_item_icon_size(p_idx);
	int icon_height = icon_size.height;
	if (items[p_idx].checkable_type && !items[p_idx].separator) {
		icon_height = MAX(icon_height,
			MAX(theme_cache.checked->get_height(), theme_cache.radio_checked->get_height()));
	}

	int text_height = items[p_idx].text_buf->get_size().height;
	if (text_height == 0 && !items[p_idx].separator) {
		text_height = theme_cache.font->get_height(theme_cache.font_size);
	}

	int separator_height = 0;
	if (items[p_idx].separator) {
		separator_height = MAX(theme_cache.separator_style->get_minimum_size().height,
			MAX(theme_cache.labeled_separator_left->get_minimum_size().height,
				theme_cache.labeled_separator_right->get_minimum_size().height));
	}

	return MAX(separator_height, MAX(text_height, icon_height));
}

int PopupMenu::_get_items_total_height() const
{
	// Get total height of all items by taking max of icon height and font height
	int items_total_height = 0;
	for (int i = 0; i < items.size(); i++) {
		if (!items[i].visible) {
			continue;
		}
		items_total_height += _get_item_height(i) + theme_cache.v_separation;
	}

	return items_total_height;
}

int PopupMenu::_get_mouse_over(const Point2& p_over) const
{
	// Transform to scroll_container local coordinates.
	const Point2 scaled_pos = p_over / get_content_scale_factor();
	const Point2 over_scroll_container =
		scroll_container->get_global_transform_with_canvas().xform_inv(scaled_pos);

	// Check if point is inside the item control as clipped by scroll_container.
	const Rect2 scroll_container_rect = Rect2(Point2(), scroll_container->get_size());
	const Rect2 bounding_rect = scroll_container_rect.intersection(control->get_rect());
	if (!bounding_rect.has_point(over_scroll_container)) {
		return -1;
	}

	// Perform item hit check in control node local space,
	// so we don't need to worry about any of the container theming.
	const float over_control_y = control->get_transform().xform_inv(over_scroll_container).y;
	float bottom_edge = 0;
	for (int i = 0; i < items.size(); i++) {
		if (!items[i].visible) {
			continue;
		}
		bottom_edge += theme_cache.v_separation;
		bottom_edge += _get_item_height(i);
		if (bottom_edge > over_control_y) {
			return i;
		}
	}

	return -1;
}

void PopupMenu::_activate_submenu(int p_over, bool p_by_keyboard)
{
	ERR_FAIL_INDEX_MSG(
		p_over, items.size(), vformat("Invalid submenu index %d in _activate_submenu.", p_over));
	PopupMenu* submenu_popup = items[p_over].submenu;
	if (submenu_popup->is_visible()) {
		WARN_VERBOSE(vformat(
			"_activate_submenu should not be called on an open submenu - index: %d.", p_over));
		return;
	}
	submenu_popup->this_submenu_index = p_over;
	active_submenu_index = p_over;

	submenu_popup->get_window()->set_exclusive(
		false); // Ensure mouse inputs to parent menu are not inhibited by the submenu in exclusive
				// mode.

	const float win_scale = get_content_scale_factor();

	const Point2 this_pos = get_position();
	Rect2 this_rect = Rect2(this_pos, panel->get_size());

	submenu_popup->reset_size(); // Shrink the popup size to its contents.
	const Size2 submenu_size = submenu_popup->get_size();

	// Calculate the submenu's position.
	Point2 submenu_pos = Point2(0, 0);
	Rect2i screen_rect =
		is_embedded() ? Rect2i(get_embedder()->get_visible_rect()) : get_parent_rect();
	active_submenu_target_line.clear();

	panel_offset_start =
		Point2(panel->get_offset(SIDE_LEFT), panel->get_offset(SIDE_TOP)) * win_scale;
	const Point2 panel_offset_end =
		Point2(-panel->get_offset(SIDE_RIGHT), -panel->get_offset(SIDE_BOTTOM)) * win_scale;
	const Vector2 this_size = this_rect.size * win_scale;
	const float theme_v_separation = theme_cache.v_separation * win_scale;
	const float scroll_offset = control->get_position().y * win_scale;
	const float scroll_container_offset = scroll_container->get_global_position().y * win_scale;
	const float ofs_cache = items[p_over]._ofs_cache * win_scale;
	const float height_cache = items[p_over]._height_cache * win_scale;
	const float item_top_y =
		ofs_cache + scroll_offset + scroll_container_offset - int(theme_v_separation * 0.5);

	if (is_layout_rtl()) {
		is_active_submenu_left = true;
		submenu_pos.x = this_pos.x - submenu_size.width + panel_offset_end.x;
		if (submenu_pos.x < screen_rect.position.x) {
			submenu_pos.x = this_pos.x + this_rect.size.width - panel_offset_start.x;
			is_active_submenu_left = false;
		}
	}
	else {
		is_active_submenu_left = false;
		submenu_pos.x = this_pos.x + this_size.x + panel_offset_start.x;
		if (submenu_pos.x + submenu_size.width > screen_rect.position.x + screen_rect.size.width) {
			submenu_pos.x = this_pos.x - submenu_size.width + panel_offset_end.x;
			is_active_submenu_left = true;
		}
	}

	submenu_pos.y = this_pos.y + item_top_y -
					submenu_popup->theme_cache.panel_style->get_margin(SIDE_TOP) * win_scale;
	if (submenu_popup->search_bar->is_visible()) {
		submenu_pos.y -= (submenu_popup->search_bar->get_minimum_size().y +
							 submenu_popup->theme_cache.search_bar_separation) *
						 win_scale;
	}

	submenu_popup->set_position(submenu_pos);
	submenu_popup->activated_by_keyboard = p_by_keyboard;
	// If not triggered by the mouse, start the popup with its first enabled item focused.
	if (p_by_keyboard) {
		for (int i = 0; i < submenu_popup->get_item_count(); i++) {
			if (!submenu_popup->is_item_disabled(i)) {
				submenu_popup->set_focused_item(i);
				break;
			}
		}
	}
	submenu_popup->popup();
	// The autohide areas are set on the submenu, but are aligned over the parent menu,
	// so we spoof `this_rect` position as the negative relative offset of the parent from the
	// submenu.
	this_rect.position = -(submenu_popup->get_position() - this_pos);

	const Rect2 safe_area(get_position(), get_size());
	Viewport* vp = submenu_popup->get_embedder();
	if (vp) {
		vp->subwindow_set_popup_safe_rect(submenu_popup, safe_area);
	}
	else {
		DisplayServer::get_singleton()->window_set_popup_safe_rect(
			submenu_popup->get_window_id(), safe_area);
	}
	// Set the mouse movement target line at the top and bottom points of the submenu vertical side
	// abutting the parent menu.
	if (is_active_submenu_left) {
		active_submenu_target_line.push_back(
			Point2(submenu_popup->get_position().x + submenu_popup->get_size().x,
				submenu_popup->get_position().y));
	}
	else {
		active_submenu_target_line.push_back(submenu_popup->get_position());
	}
	active_submenu_target_line.push_back(Point2(active_submenu_target_line[0].x,
		active_submenu_target_line[0].y + submenu_popup->get_size().y));

	submenu_popup->clear_autohide_areas();
	// Add an autohide area above the submenu item unless it's the top item.
	// This avoids a narrow strip of area that can trigger the submenu to reload when reentering the
	// parent item from the top.
	const int y_to_item_top = item_top_y - panel_offset_start.y;
	Rect2 top_rect =
		Rect2(this_rect.position.x, this_rect.position.y, this_size.width, y_to_item_top);
	if (active_submenu_index != 0) {
		submenu_popup->add_autohide_area(top_rect);
	}
	// If there is an area below the submenu item, add an autohide area there unless it's the last
	// item.
	if (active_submenu_index != items.size() - 1) {
		const int y_to_item_bottom = y_to_item_top + height_cache + theme_v_separation;
		submenu_popup->add_autohide_area(Rect2(this_rect.position.x,
			this_rect.position.y + y_to_item_bottom, this_size.x, this_size.y - y_to_item_bottom));
	}
	queue_accessibility_update();
	control->queue_redraw();
}

void PopupMenu::_submenu_timeout()
{
	if (mouse_over == submenu_over) {
		_activate_submenu(mouse_over);
	}
}

void PopupMenu::_input_from_window(const Ref<InputEvent>& p_event)
{
	if (p_event.is_valid()) {
		_input_from_window_internal(p_event);
	}
	else {
		WARN_PRINT_ONCE(
			"PopupMenu has received an invalid InputEvent. Consider filtering out invalid events.");
	}
	Popup::_input_from_window(p_event);
}

bool PopupMenu::_is_mouse_moving_toward_submenu(const Vector2& p_relative, bool p_is_submenu_left,
	const Vector2& p_mouse_position, const Vector<Point2>& p_active_submenu_target_line) const
{
	Vector2 top_target = (p_active_submenu_target_line[0] - p_mouse_position)
							 .rotated(p_is_submenu_left ? -Math::PI * 0.5 : Math::PI * 0.5);
	Vector2 bottom_target = (p_active_submenu_target_line[1] - p_mouse_position)
								.rotated(p_is_submenu_left ? Math::PI * 0.5 : -Math::PI * 0.5);
	// The top_target vector is perpendicular to the vector between the mouse position and the top
	// point of the submenu target line. The dot product is > 0 if the relative vector is +/- 90
	// degrees of the top_vector. The bottom_target vector is perpendicular to the vector between
	// the mouse position and the bottom point of the submenu target line, but pointing in the
	// opposite direction of the top_target vector. Thus, the intersection of top and bottom test
	// semicircles is the area in which the relative vector is deemed to be moving toward the
	// submenu. Without normalization, the comparison is correct by testing only the sign, but would
	// not work against a range of +/- 1.
	return bottom_target.dot(p_relative) > 0 && top_target.dot(p_relative) > 0;
}

void PopupMenu::_draw_items()
{
	control->set_custom_minimum_size(Size2(0, _get_items_total_height()));
	RID ci = control->get_canvas_item();

	// Space between the item content and the sides of popup menu.
	bool rtl = control->is_layout_rtl();
	// In Item::checkable_type enum order (less the non-checkable member), with disabled repeated at
	// the end.
	Ref<Texture2D> check[] = {theme_cache.checked, theme_cache.radio_checked,
		theme_cache.checked_disabled, theme_cache.radio_checked_disabled};
	Ref<Texture2D> uncheck[] = {theme_cache.unchecked, theme_cache.radio_unchecked,
		theme_cache.unchecked_disabled, theme_cache.radio_unchecked_disabled};
	Ref<Texture2D> indeterminate[] = {
		theme_cache.indeterminate, theme_cache.indeterminate_disabled};
	Ref<Texture2D> submenu;
	if (rtl) {
		submenu = theme_cache.submenu_mirrored;
	}
	else {
		submenu = theme_cache.submenu;
	}

	float display_width = control->get_size().width;

	// Find the widest icon and whether any items have a checkbox, and store the offsets for each.
	real_t icon_max_w = 0.0;
	real_t check_max_w = 0.0;
	bool has_check_gutter = false;
	bool gutter_compact = theme_cache.gutter_compact;
	for (int i = 0; i < items.size(); i++) {
		if (items[i].separator || !items[i].visible) {
			continue;
		}

		icon_max_w = MAX(_get_item_icon_size(i).width, icon_max_w);

		if (items[i].checkable_type) {
			has_check_gutter = true;
			if (items[i].icon.is_valid()) {
				gutter_compact = false;
			}
		}
	}
	if (has_check_gutter) {
		for (int i = 0; i < 4; i++) {
			check_max_w = MAX(check_max_w, check[i]->get_width());
			check_max_w = MAX(check_max_w, uncheck[i]->get_width());
		}
	}

	Point2 ofs;

	// Loop through all items and draw each.
	bool first_visible = true;
	for (int i = 0; i < items.size(); i++) {
		if (!items[i].visible) {
			continue;
		}

		// For the first visible item only add half a separation. For all other items, add a whole
		// separation to the offset.
		ofs.y += first_visible ? theme_cache.v_separation / 2 : theme_cache.v_separation;
		first_visible = false;

		_shape_item(i);

		Point2 item_ofs = ofs;
		Size2 icon_size = _get_item_icon_size(i);
		float h = _get_item_height(i);
		if ((active_submenu_index == -1 && i == mouse_over) || i == active_submenu_index) {
			theme_cache.hover_style->draw(
				ci, Rect2(item_ofs + Point2(0, -theme_cache.v_separation / 2),
						Size2(display_width, h + theme_cache.v_separation)));
		}

		String text = items[i].xl_text;

		// Separator
		item_ofs.x += items[i].indent * theme_cache.indent;
		if (items[i].separator) {
			if (!text.is_empty() || items[i].icon.is_valid()) {
				int content_size =
					items[i].text_buf->get_size().width + theme_cache.h_separation * 2;
				if (items[i].icon.is_valid()) {
					content_size += icon_size.width + theme_cache.h_separation;
				}

				int content_center = display_width / 2;
				int content_left = content_center - content_size / 2;
				int content_right = content_center + content_size / 2;
				if (content_left > item_ofs.x) {
					int sep_h = theme_cache.labeled_separator_left->get_minimum_size().height;
					int sep_ofs = Math::floor((h - sep_h) / 2.0);
					theme_cache.labeled_separator_left->draw(
						ci, Rect2(item_ofs + Point2(0, sep_ofs),
								Size2(MAX(0, content_left - item_ofs.x), sep_h)));
				}
				if (content_right < display_width) {
					int sep_h = theme_cache.labeled_separator_right->get_minimum_size().height;
					int sep_ofs = Math::floor((h - sep_h) / 2.0);
					theme_cache.labeled_separator_right->draw(
						ci, Rect2(Point2(content_right, item_ofs.y + sep_ofs),
								Size2(MAX(0, display_width - content_right), sep_h)));
				}
			}
			else {
				int sep_h = theme_cache.separator_style->get_minimum_size().height;
				int sep_ofs = Math::floor((h - sep_h) / 2.0);
				theme_cache.separator_style->draw(
					ci, Rect2(item_ofs + Point2(0, sep_ofs), Size2(display_width, sep_h)));
			}
		}

		Color icon_color = items[i].icon_modulate;

		// For non-separator items, add some padding for the content.
		if (!items[i].separator) {
			item_ofs.x += theme_cache.item_start_padding;
		}

		// Checkboxes
		if (items[i].checkable_type && !items[i].separator) {
			int disabled = int(items[i].disabled) * 2;
			Texture2D* icon;
			if (items[i].indeterminate) {
				icon = indeterminate[disabled].ptr();
			}
			else {
				icon = (items[i].checked ? check[items[i].checkable_type - 1 + disabled]
										 : uncheck[items[i].checkable_type - 1 + disabled])
						   .ptr();
			}

			if (rtl) {
				icon->draw(ci,
					Size2(control->get_size().width - item_ofs.x - icon->get_width(), item_ofs.y) +
						Point2(0, Math::floor((h - icon->get_height()) / 2.0)),
					icon_color);
			}
			else {
				icon->draw(ci, item_ofs + Point2(0, Math::floor((h - icon->get_height()) / 2.0)),
					icon_color);
			}
		}

		int separator_ofs = (display_width - items[i].text_buf->get_size().width) / 2;

		// Icon
		if (items[i].icon.is_valid()) {
			const Point2 icon_offset = Point2(0, Math::floor((h - icon_size.height) / 2.0));
			Point2 icon_pos;

			if (items[i].separator) {
				separator_ofs -= (icon_size.width + theme_cache.h_separation) / 2;

				if (rtl) {
					icon_pos = Size2(
						control->get_size().width - item_ofs.x - separator_ofs - icon_size.width,
						item_ofs.y);
				}
				else {
					icon_pos = item_ofs + Size2(separator_ofs, 0);
					separator_ofs += icon_size.width + theme_cache.h_separation;
				}
			}
			else {
				const real_t check_w = (gutter_compact || !has_check_gutter)
										   ? 0
										   : (check_max_w + theme_cache.h_separation);
				if (rtl) {
					icon_pos =
						Size2(control->get_size().width - item_ofs.x - check_w - icon_size.width,
							item_ofs.y);
				}
				else {
					icon_pos = item_ofs + Size2(check_w, 0);
				}
			}

			items[i].icon->draw_rect(
				ci, Rect2(icon_pos + icon_offset, icon_size), false, icon_color);
		}

		// Submenu arrow on right hand side.
		if (items[i].submenu) {
			if (rtl) {
				submenu->draw(ci,
					Point2(theme_cache.item_end_padding,
						item_ofs.y + Math::floor(h - submenu->get_height()) / 2),
					icon_color);
			}
			else {
				submenu->draw(ci,
					Point2(display_width - submenu->get_width() - theme_cache.item_end_padding,
						item_ofs.y + Math::floor(h - submenu->get_height()) / 2),
					icon_color);
			}
		}

		// Text
		if (items[i].separator) {
			if (!text.is_empty()) {
				Vector2 text_pos = Point2(separator_ofs,
					item_ofs.y + Math::floor((h - items[i].text_buf->get_size().y) / 2.0));

				if (theme_cache.font_separator_outline_size > 0 &&
					theme_cache.font_separator_outline_color.a > 0) {
					items[i].text_buf->draw_outline(ci, text_pos,
						theme_cache.font_separator_outline_size,
						theme_cache.font_separator_outline_color);
				}
				items[i].text_buf->draw(ci, text_pos, theme_cache.font_separator_color);
			}
		}
		else {
			if (gutter_compact) {
				item_ofs.x += MAX(icon_max_w, check_max_w) + theme_cache.h_separation;
			}
			else {
				if (icon_max_w > 0) {
					item_ofs.x += icon_max_w + theme_cache.h_separation;
				}
				if (has_check_gutter) {
					item_ofs.x += check_max_w + theme_cache.h_separation;
				}
			}
			if (rtl) {
				Vector2 text_pos =
					Size2(control->get_size().width - items[i].text_buf->get_size().width -
							  item_ofs.x,
						item_ofs.y) +
					Point2(0, Math::floor((h - items[i].text_buf->get_size().y) / 2.0));
				if (theme_cache.font_outline_size > 0 && theme_cache.font_outline_color.a > 0) {
					items[i].text_buf->draw_outline(ci, text_pos, theme_cache.font_outline_size,
						theme_cache.font_outline_color);
				}
				items[i].text_buf->draw(ci, text_pos,
					items[i].disabled ? theme_cache.font_disabled_color
									  : (((active_submenu_index == -1 && i == mouse_over) ||
											 i == active_submenu_index)
												? theme_cache.font_hover_color
												: theme_cache.font_color));
			}
			else {
				Vector2 text_pos =
					item_ofs + Point2(0, Math::floor((h - items[i].text_buf->get_size().y) / 2.0));
				if (theme_cache.font_outline_size > 0 && theme_cache.font_outline_color.a > 0) {
					items[i].text_buf->draw_outline(ci, text_pos, theme_cache.font_outline_size,
						theme_cache.font_outline_color);
				}
				items[i].text_buf->draw(ci, text_pos,
					items[i].disabled ? theme_cache.font_disabled_color
									  : (((active_submenu_index == -1 && i == mouse_over) ||
											 i == active_submenu_index)
												? theme_cache.font_hover_color
												: theme_cache.font_color));
			}
		}

		// Accelerator / Shortcut
		if (items[i].accel != Key::NONE ||
			(items[i].shortcut.is_valid() && items[i].shortcut->has_valid_event())) {
			if (rtl) {
				item_ofs.x = theme_cache.item_end_padding;
			}
			else {
				item_ofs.x = display_width - items[i].accel_text_buf->get_size().x -
							 theme_cache.item_end_padding;
			}
			Vector2 text_pos =
				item_ofs +
				Point2(0, Math::floor((h - items[i].accel_text_buf->get_size().y) / 2.0));
			if (theme_cache.font_outline_size > 0 && theme_cache.font_outline_color.a > 0) {
				items[i].accel_text_buf->draw_outline(
					ci, text_pos, theme_cache.font_outline_size, theme_cache.font_outline_color);
			}
			items[i].accel_text_buf->draw(ci, text_pos,
				((active_submenu_index == -1 && i == mouse_over) || i == active_submenu_index)
					? theme_cache.font_hover_color
					: theme_cache.font_accelerator_color);
		}

		// Cache the item vertical offset from the first item and the height.
		items.write[i]._ofs_cache = ofs.y;
		items.write[i]._height_cache = h;

		ofs.y += h;
	}
}

void PopupMenu::_update_search_bar_visibility()
{
	if (search_bar) {
		if (search_bar_enabled) {
			int item_count = 0;
			for (const Item& item : items) {
				if (!item.separator) {
					item_count++;
				}
			}
			search_bar->set_visible(item_count >= search_bar_min_item_count);
		}
		else {
			search_bar->hide();
		}
	}
}

void PopupMenu::_search_bar_focus_entered()
{
	prev_mouse_over = mouse_over;
	mouse_over = -1;
	queue_accessibility_update();
	control->queue_redraw();
}

void PopupMenu::_filter_items(const String& p_query)
{
	for (PopupMenu::Item& item : items) {
		if (item.submenu) {
			item.submenu->_filter_items(p_query);
		}
	}

	for (PopupMenu::Item& item : items) {
		item.visible = true;
	}

	if (p_query.is_empty()) {
		return;
	}

	PackedStringArray search_candidates;
	search_candidates.reserve(items.size());

	Vector<int> search_candidate_to_item;
	search_candidate_to_item.reserve(items.size());

	for (int i = 0; i < items.size(); i++) {
		Item& item = items.write[i];
		item.visible = false;

		if (item.submenu) {
			for (const PopupMenu::Item& submenu_item : item.submenu->items) {
				if (submenu_item.visible) {
					item.visible = true;
					break;
				}
			}
		}

		if (!item.separator) {
			search_candidates.append(item.text);
			search_candidate_to_item.append(i);
		}
	}

	FuzzySearch fuzzy;
	fuzzy.set_max_results(search_candidates.size());
	fuzzy.set_max_misses(search_bar_fuzzy_search_max_misses);
	fuzzy.set_use_exact_tokens(!search_bar_fuzzy_search_enabled);

	for (const Ref<FuzzySearchMatch>& result : fuzzy.search_all(p_query, search_candidates)) {
		PopupMenu::Item& item = items.write[search_candidate_to_item[result->get_original_index()]];
		item.visible = true;
		if (item.submenu) {
			for (PopupMenu::Item& submenu_item : item.submenu->items) {
				submenu_item.visible = true;
			}
		}
	}
}

void PopupMenu::_close_pressed()
{
	if (this_submenu_index != -1 &&
		active_submenu_index != -1) { // Close secondary submenus on first submenu close.
		ERR_FAIL_INDEX_MSG(active_submenu_index, items.size(),
			vformat(
				"Invalid active_submenu_index index %d in _close_pressed.", active_submenu_index));
		items[active_submenu_index].submenu->_close_pressed();
	}
	Popup::_close_pressed();
}

void PopupMenu::_update_shadow_offsets() const
{
	if (!DisplayServer::get_singleton()->is_window_transparency_available() && !is_embedded()) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	Ref<StyleBoxFlat> sb = theme_cache.panel_style;
	if (sb.is_null()) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	const int shadow_size = sb->get_shadow_size();
	if (shadow_size == 0) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	// Offset the background panel so it leaves space inside the window for the shadows to be drawn.
	const Point2 shadow_offset = sb->get_shadow_offset();
	if (is_layout_rtl()) {
		panel->set_offset(SIDE_LEFT, MAX(0, shadow_size + shadow_offset.x));
		panel->set_offset(SIDE_RIGHT, MIN(0, -shadow_size + shadow_offset.x));
	}
	else {
		panel->set_offset(SIDE_LEFT, MAX(0, shadow_size - shadow_offset.x));
		panel->set_offset(SIDE_RIGHT, MIN(0, -shadow_size - shadow_offset.x));
	}
	panel->set_offset(SIDE_TOP, MAX(0, shadow_size - shadow_offset.y));
	panel->set_offset(SIDE_BOTTOM, MIN(0, -shadow_size - shadow_offset.y));
}

Rect2i PopupMenu::_popup_adjust_rect() const
{
	Rect2i current = Popup::_popup_adjust_rect();
	if (current == Rect2i()) {
		return current;
	}

	pre_popup_rect = current;

	_update_shadow_offsets();

	if (is_layout_rtl()) {
		current.position -= Vector2(-panel->get_offset(SIDE_RIGHT), panel->get_offset(SIDE_TOP)) *
							get_content_scale_factor();
	}
	else {
		current.position -= Vector2(panel->get_offset(SIDE_LEFT), panel->get_offset(SIDE_TOP)) *
							get_content_scale_factor();
	}
	current.size += Vector2(panel->get_offset(SIDE_LEFT) - panel->get_offset(SIDE_RIGHT),
						panel->get_offset(SIDE_TOP) - panel->get_offset(SIDE_BOTTOM)) *
					get_content_scale_factor();

	return current;
}

RID PopupMenu::get_focused_accessibility_element() const
{
	if (mouse_over == -1) {
		return get_accessibility_element();
	}
	else {
		const Item& item = items[mouse_over];
		return item.accessibility_item_element;
	}
}

/* Methods to add items with or without icon, checkbox, shortcut.
 * Be sure to keep them in sync when adding new properties in the Item struct.
 */

#define ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel)                                              \
	item.text = p_label;                                                                           \
	item.xl_text = atr(p_label);                                                                   \
	item.id = p_id == -1 ? items.size() : p_id;                                                    \
	item.accel = p_accel;

#define ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global, p_allow_echo)                         \
	ERR_FAIL_COND_MSG(p_shortcut.is_null(), "Cannot add item with invalid Shortcut.");             \
	_ref_shortcut(p_shortcut);                                                                     \
	item.text = p_shortcut->get_name();                                                            \
	item.xl_text = atr(item.text);                                                                 \
	item.id = p_id == -1 ? items.size() : p_id;                                                    \
	item.shortcut = p_shortcut;                                                                    \
	item.shortcut_is_global = p_global;                                                            \
	item.allow_echo = p_allow_echo;

#undef ITEM_SETUP_WITH_ACCEL
#undef ITEM_SETUP_WITH_SHORTCUT

/* Methods to modify existing items. */

void PopupMenu::set_item_text(int p_idx, const String& p_text)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].text == p_text) {
		return;
	}
	items.write[p_idx].text = p_text;
	items.write[p_idx].xl_text = _atr(p_idx, p_text);
	items.write[p_idx].dirty = true;
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_text(global_menu, p_idx, items[p_idx].xl_text);
	}

	_shape_item(p_idx);
	queue_accessibility_update();
	control->queue_redraw();

	child_controls_changed();
	_menu_changed();
}

void PopupMenu::set_item_text_direction(int p_idx, Control::TextDirection p_text_direction)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);

	if (items[p_idx].text_direction != p_text_direction) {
		items.write[p_idx].text_direction = p_text_direction;
		items.write[p_idx].dirty = true;
		items.write[p_idx].accessibility_item_dirty = true;

		_shape_item(p_idx);
		queue_accessibility_update();
		control->queue_redraw();
	}
}

void PopupMenu::set_item_language(int p_idx, const String& p_language)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].language != p_language) {
		items.write[p_idx].language = p_language;
		items.write[p_idx].dirty = true;
		items.write[p_idx].accessibility_item_dirty = true;

		_shape_item(p_idx);
		queue_accessibility_update();
		control->queue_redraw();
	}
}

void PopupMenu::set_item_auto_translate_mode(int p_idx, AutoTranslateMode p_mode)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].auto_translate_mode == p_mode) {
		return;
	}
	items.write[p_idx].auto_translate_mode = p_mode;
	items.write[p_idx].xl_text = _atr(p_idx, items[p_idx].text);
	items.write[p_idx].dirty = true;
	control->queue_redraw();
}

void PopupMenu::set_item_icon(int p_idx, const Ref<Texture2D>& p_icon)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].icon == p_icon) {
		return;
	}

	items.write[p_idx].icon = p_icon;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_icon(global_menu, p_idx, items[p_idx].icon);
	}

	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

void PopupMenu::set_item_icon_max_width(int p_idx, int p_width)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].icon_max_width == p_width) {
		return;
	}

	items.write[p_idx].icon_max_width = p_width;

	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

void PopupMenu::set_item_icon_modulate(int p_idx, const Color& p_modulate)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].icon_modulate == p_modulate) {
		return;
	}

	items.write[p_idx].icon_modulate = p_modulate;
	control->queue_redraw();
}

void PopupMenu::set_item_checked(int p_idx, bool p_checked)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].checked == p_checked) {
		return;
	}

	items.write[p_idx].checked = p_checked;
	items.write[p_idx].accessibility_item_dirty = true;
	items.write[p_idx].indeterminate = false;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_checked(global_menu, p_idx, p_checked);
	}

	queue_accessibility_update();
	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

void PopupMenu::set_item_indeterminate(int p_idx, bool p_indeterminate)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].indeterminate == p_indeterminate) {
		return;
	}

	items.write[p_idx].indeterminate = p_indeterminate;
	items.write[p_idx].accessibility_item_dirty = true;
	items.write[p_idx].checked = false;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_indeterminate(global_menu, p_idx, p_indeterminate);
	}

	queue_accessibility_update();
	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

void PopupMenu::set_item_id(int p_idx, int p_id)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].id == p_id) {
		return;
	}

	items.write[p_idx].id = p_id;

	// `global_menu` does not know about IDs so there is no need to update it.

	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

void PopupMenu::set_item_accelerator(int p_idx, Key p_accel)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].accel == p_accel) {
		return;
	}

	items.write[p_idx].accel = p_accel;
	items.write[p_idx].dirty = true;
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_accelerator(global_menu, p_idx, p_accel);
	}

	queue_accessibility_update();
	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

void PopupMenu::set_item_disabled(int p_idx, bool p_disabled)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].disabled == p_disabled) {
		return;
	}

	items.write[p_idx].disabled = p_disabled;
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_disabled(global_menu, p_idx, p_disabled);
	}

	queue_accessibility_update();
	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

void PopupMenu::_close_suspended_timeout()
{
	if (submenu_over != -1 && is_embedded()) {
		ERR_FAIL_INDEX_MSG(submenu_over, items.size(),
			vformat("Invalid submenu index %d in _close_suspended_timeout.", submenu_over));
		if (items[submenu_over].submenu->is_visible()) {
			items[submenu_over].submenu->_close_or_suspend();
		}
	}
	if (active_submenu_index != -1 && items[active_submenu_index].submenu->is_visible()) {
		// Closes the submenu if the mouse is moved off the parent item toward the submenu,
		// but comes to a stop before reaching the submenu and the timeout is reached.
		items[active_submenu_index].submenu->_close_pressed();
	}
}

void PopupMenu::_submenu_hidden()
{
	// Ensure the submenu_timer is not running to avoid any race conditions between opening and
	// closing submenus.
	if (!submenu_timer->is_stopped()) {
		WARN_VERBOSE("The submenu_timer should never be running when the _submenu_hidden signal is "
					 "emitted.");
		return;
	}
	if (active_submenu_index == -1) {
		WARN_VERBOSE(
			"The active_submenu_index should never be -1 when _submenu_hidden is entered.");
		return;
	}
	active_submenu_index = -1;
	submenu_over = -1;
	submenu_mouse_exited_ticks_msec = -1;
	mouse_movement_was_tested = false;
	close_was_suspended = false;
	queue_accessibility_update();
	control->queue_redraw();
	if (!activated_by_keyboard) {
		Point2 mouse_pos =
			is_embedded()
				? get_mouse_position() * get_content_scale_factor()
				: Point2(DisplayServer::get_singleton()->mouse_get_position() - get_position());
		_mouse_over_update(mouse_pos);
	}
}

void PopupMenu::toggle_item_checked(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].checked = !items[p_idx].checked;
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_checked(global_menu, p_idx, items[p_idx].checked);
	}

	queue_accessibility_update();
	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

String PopupMenu::get_item_text(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].text;
}

String PopupMenu::get_item_xl_text(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].xl_text;
}

Control::TextDirection PopupMenu::get_item_text_direction(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Control::TEXT_DIRECTION_INHERITED);
	return items[p_idx].text_direction;
}

String PopupMenu::get_item_language(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].language;
}

Node::AutoTranslateMode PopupMenu::get_item_auto_translate_mode(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), AUTO_TRANSLATE_MODE_INHERIT);
	return items[p_idx].auto_translate_mode;
}

int PopupMenu::get_item_idx_from_text(const String& text) const
{
	for (int idx = 0; idx < items.size(); idx++) {
		if (items[idx].text == text) {
			return idx;
		}
	}

	return -1;
}

Ref<Texture2D> PopupMenu::get_item_icon(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Texture2D>());
	return items[p_idx].icon;
}

int PopupMenu::get_item_icon_max_width(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);
	return items[p_idx].icon_max_width;
}

Color PopupMenu::get_item_icon_modulate(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());
	return items[p_idx].icon_modulate;
}

Key PopupMenu::get_item_accelerator(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Key::NONE);
	return items[p_idx].accel;
}

bool PopupMenu::is_item_disabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].disabled;
}

bool PopupMenu::is_item_checked(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checked;
}

bool PopupMenu::is_item_indeterminate(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].indeterminate;
}

int PopupMenu::get_item_id(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);
	return items[p_idx].id;
}

int PopupMenu::get_item_index(int p_id) const
{
	for (int i = 0; i < items.size(); i++) {
		if (items[i].id == p_id) {
			return i;
		}
	}

	return -1;
}

String PopupMenu::get_item_submenu(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].submenu_name;
}

PopupMenu* PopupMenu::get_item_submenu_node(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), nullptr);
	return items[p_idx].submenu;
}

String PopupMenu::get_item_tooltip(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].tooltip;
}

Ref<Shortcut> PopupMenu::get_item_shortcut(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Shortcut>());
	return items[p_idx].shortcut;
}

int PopupMenu::get_item_indent(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);
	return items[p_idx].indent;
}

int PopupMenu::get_item_max_states(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), -1);
	return items[p_idx].max_states;
}

int PopupMenu::get_item_state(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), -1);
	return items[p_idx].state;
}

void PopupMenu::set_item_as_separator(int p_idx, bool p_separator)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].separator == p_separator) {
		return;
	}

	items.write[p_idx].separator = p_separator;
	items.write[p_idx].accessibility_item_dirty = true;

	queue_accessibility_update();
	control->queue_redraw();
}

bool PopupMenu::is_item_separator(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].separator;
}

void PopupMenu::set_item_as_checkable(int p_idx, bool p_checkable)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	int type = (int)(p_checkable ? Item::CHECKABLE_TYPE_CHECK_BOX : Item::CHECKABLE_TYPE_NONE);
	if (type == items[p_idx].checkable_type) {
		return;
	}

	items.write[p_idx].checkable_type =
		p_checkable ? Item::CHECKABLE_TYPE_CHECK_BOX : Item::CHECKABLE_TYPE_NONE;
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_checkable(global_menu, p_idx, p_checkable);
	}

	queue_accessibility_update();
	control->queue_redraw();
	_menu_changed();
}

void PopupMenu::set_item_as_radio_checkable(int p_idx, bool p_radio_checkable)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	int type =
		(int)(p_radio_checkable ? Item::CHECKABLE_TYPE_RADIO_BUTTON : Item::CHECKABLE_TYPE_NONE);
	if (type == items[p_idx].checkable_type) {
		return;
	}

	items.write[p_idx].checkable_type =
		p_radio_checkable ? Item::CHECKABLE_TYPE_RADIO_BUTTON : Item::CHECKABLE_TYPE_NONE;
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_radio_checkable(
			global_menu, p_idx, p_radio_checkable);
	}

	queue_accessibility_update();
	control->queue_redraw();
	_menu_changed();
}

void PopupMenu::set_item_tooltip(int p_idx, const String& p_tooltip)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].tooltip == p_tooltip) {
		return;
	}

	items.write[p_idx].tooltip = p_tooltip;
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_tooltip(global_menu, p_idx, p_tooltip);
	}

	queue_accessibility_update();
	control->queue_redraw();
	_menu_changed();
}

void PopupMenu::set_item_indent(int p_idx, int p_indent)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items.write[p_idx].indent == p_indent) {
		return;
	}
	items.write[p_idx].indent = p_indent;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_indentation_level(global_menu, p_idx, p_indent);
	}

	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

void PopupMenu::set_item_max_states(int p_idx, int p_max_states)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].max_states == p_max_states) {
		return;
	}

	items.write[p_idx].max_states = p_max_states;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_max_states(global_menu, p_idx, p_max_states);
	}

	control->queue_redraw();
	_menu_changed();
}

void PopupMenu::set_item_multistate(int p_idx, int p_state)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].state == p_state) {
		return;
	}

	items.write[p_idx].state = p_state;
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_state(global_menu, p_idx, p_state);
	}

	queue_accessibility_update();
	control->queue_redraw();
	_menu_changed();
}

void PopupMenu::toggle_item_multistate(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, items.size());
	if (0 >= items[p_idx].max_states) {
		return;
	}

	++items.write[p_idx].state;
	if (items.write[p_idx].max_states <= items[p_idx].state) {
		items.write[p_idx].state = 0;
	}
	items.write[p_idx].accessibility_item_dirty = true;

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->set_item_state(global_menu, p_idx, items[p_idx].state);
	}

	queue_accessibility_update();
	control->queue_redraw();
	_menu_changed();
}

bool PopupMenu::is_item_checkable(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checkable_type;
}

bool PopupMenu::is_item_radio_checkable(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checkable_type == Item::CHECKABLE_TYPE_RADIO_BUTTON;
}

bool PopupMenu::is_item_shortcut_global(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].shortcut_is_global;
}

bool PopupMenu::is_item_shortcut_disabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].shortcut_is_disabled;
}

void PopupMenu::set_focused_item(int p_idx)
{
	if (p_idx != -1) {
		ERR_FAIL_INDEX(p_idx, items.size());
	}

	if (mouse_over == p_idx) {
		return;
	}

	prev_mouse_over = mouse_over;
	mouse_over = p_idx;
	if (mouse_over != -1) {
		scroll_to_item(mouse_over);
	}
	queue_accessibility_update();
	control->queue_redraw();
}

int PopupMenu::get_focused_item() const { return mouse_over; }

int PopupMenu::get_item_count() const { return items.size(); }

void PopupMenu::scroll_to_item(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, items.size());

	// Calculate the position of the item relative to the visible area.
	int item_y = items[p_idx]._ofs_cache;
	int visible_height = scroll_container->get_size().height;
	int relative_y = item_y - scroll_container->get_v_scroll();

	// If item is not fully visible, adjust scroll.
	if (relative_y < 0) {
		scroll_container->set_v_scroll(item_y);
	}
	else if (relative_y + items[p_idx]._height_cache > visible_height) {
		scroll_container->set_v_scroll(item_y + items[p_idx]._height_cache - visible_height);
	}
}

void PopupMenu::set_prefer_native_menu(bool p_enabled)
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

bool PopupMenu::is_prefer_native_menu() const { return prefer_native; }

bool PopupMenu::is_native_menu() const
{
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		return false;
	}
#endif

	return global_menu.is_valid();
}

bool PopupMenu::activate_item_by_event(const Ref<InputEvent>& p_event, bool p_for_global_only)
{
	ERR_FAIL_COND_V(p_event.is_null(), false);
	Key code = Key::NONE;
	Ref<InputEventKey> k = p_event;

	if (k.is_valid()) {
		code = k->get_keycode();
		if (code == Key::NONE) {
			code = (Key)k->get_unicode();
		}
		if (k->is_ctrl_pressed()) {
			code |= KeyModifierMask::CTRL;
		}
		if (k->is_alt_pressed()) {
			code |= KeyModifierMask::ALT;
		}
		if (k->is_meta_pressed()) {
			code |= KeyModifierMask::META;
		}
		if (k->is_shift_pressed()) {
			code |= KeyModifierMask::SHIFT;
		}
	}

	for (int i = 0; i < items.size(); i++) {
		if (is_item_disabled(i) || items[i].shortcut_is_disabled ||
			(!items[i].allow_echo && p_event->is_echo())) {
			continue;
		}

		if (items[i].shortcut.is_valid() && items[i].shortcut->matches_event(p_event) &&
			(items[i].shortcut_is_global || !p_for_global_only)) {
			activate_item(i);
			return true;
		}

		if (code != Key::NONE && items[i].accel == code) {
			activate_item(i);
			return true;
		}

		if (items[i].submenu) {
			if (items[i].submenu->activate_item_by_event(p_event, p_for_global_only)) {
				return true;
			}
		}
	}
	return false;
}

void PopupMenu::remove_item(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].accessibility_item_element.is_valid()) {
		AccessibilityServer::get_singleton()->free_element(
			items.write[p_idx].accessibility_item_element);
		items.write[p_idx].accessibility_item_element = RID();
	}
	if (items[p_idx].shortcut.is_valid()) {
		_unref_shortcut(items[p_idx].shortcut);
	}

	items.remove_at(p_idx);

	if (global_menu.is_valid()) {
		NativeMenu::get_singleton()->remove_item(global_menu, p_idx);
	}

	control->queue_redraw();
	child_controls_changed();
	_menu_changed();
}

// Hide on item selection determines whether or not the popup will close after item selection
void PopupMenu::set_hide_on_item_selection(bool p_enabled) { hide_on_item_selection = p_enabled; }

bool PopupMenu::is_hide_on_item_selection() const { return hide_on_item_selection; }

void PopupMenu::set_hide_on_checkable_item_selection(bool p_enabled)
{
	hide_on_checkable_item_selection = p_enabled;
}

bool PopupMenu::is_hide_on_checkable_item_selection() const
{
	return hide_on_checkable_item_selection;
}

void PopupMenu::set_hide_on_multistate_item_selection(bool p_enabled)
{
	hide_on_multistate_item_selection = p_enabled;
}

bool PopupMenu::is_hide_on_multistate_item_selection() const
{
	return hide_on_multistate_item_selection;
}

void PopupMenu::set_submenu_popup_delay(float p_time)
{
	if (p_time <= 0) {
		p_time = 0.01;
	}
	submenu_timer_popup_delay = p_time;
	submenu_timer->set_wait_time(p_time);
}

float PopupMenu::get_submenu_popup_delay() const { return submenu_timer->get_wait_time(); }

void PopupMenu::set_allow_search(bool p_allow) { allow_search = p_allow; }

bool PopupMenu::get_allow_search() const { return allow_search; }

void PopupMenu::set_search_bar_enabled(bool p_enabled)
{
	search_bar_enabled = p_enabled;
	_update_search_bar_visibility();
}

bool PopupMenu::is_search_bar_enabled() const { return search_bar_enabled; }

void PopupMenu::set_search_bar_min_item_count(int p_count)
{
	ERR_FAIL_COND(p_count < 0);
	search_bar_min_item_count = p_count;
	_update_search_bar_visibility();
}

int PopupMenu::get_search_bar_min_item_count() const { return search_bar_min_item_count; }

void PopupMenu::set_search_bar_fuzzy_search_enabled(bool p_enabled)
{
	search_bar_fuzzy_search_enabled = p_enabled;
}

bool PopupMenu::is_search_bar_fuzzy_search_enabled() const
{
	return search_bar_fuzzy_search_enabled;
}

void PopupMenu::set_search_bar_fuzzy_search_max_misses(int p_max_misses)
{
	ERR_FAIL_COND(p_max_misses < 0);
	search_bar_fuzzy_search_max_misses = p_max_misses;
}

int PopupMenu::get_search_bar_fuzzy_search_max_misses() const
{
	return search_bar_fuzzy_search_max_misses;
}

#ifdef TOOLS_ENABLED
PackedStringArray PopupMenu::get_configuration_warnings() const
{
	PackedStringArray warnings = Popup::get_configuration_warnings();

	if (!DisplayServer::get_singleton()->is_window_transparency_available() &&
		!GLOBAL_GET_CACHED(bool, "display/window/subwindows/embed_subwindows")) {
		Ref<StyleBoxFlat> sb = theme_cache.panel_style;
		if (sb.is_valid() &&
			(sb->get_shadow_size() > 0 || sb->get_corner_radius(CORNER_TOP_LEFT) > 0 ||
				sb->get_corner_radius(CORNER_TOP_RIGHT) > 0 ||
				sb->get_corner_radius(CORNER_BOTTOM_LEFT) > 0 ||
				sb->get_corner_radius(CORNER_BOTTOM_RIGHT) > 0)) {
			warnings.push_back(RTR(
				"The current theme style has shadows and/or rounded corners for popups, but those "
				"won't display correctly if \"display/window/per_pixel_transparency/allowed\" "
				"isn't enabled in the Project Settings, nor if it isn't supported."));
		}
	}

	return warnings;
}
#endif

void PopupMenu::add_autohide_area(const Rect2& p_area) { autohide_areas.push_back(p_area); }

void PopupMenu::clear_autohide_areas() { autohide_areas.clear(); }

void PopupMenu::_native_popup(const Rect2i& p_rect)
{
	Point2i popup_pos = p_rect.position;
	if (is_embedded()) {
		popup_pos = get_embedder()->get_screen_transform().xform(
			popup_pos); // Note: for embedded windows "screen transform" is transform relative to
						// embedder not the actual screen.
		DisplayServerEnums::WindowID wid = get_window_id();
		if (wid == DisplayServerEnums::INVALID_WINDOW_ID) {
			wid = DisplayServerEnums::MAIN_WINDOW_ID;
		}
		popup_pos += DisplayServer::get_singleton()->window_get_position(wid);
	}
	float win_scale = get_parent_visible_window()->get_content_scale_factor();
	NativeMenu::get_singleton()->set_minimum_width(global_menu, p_rect.size.x * win_scale);
	NativeMenu::get_singleton()->popup(global_menu, popup_pos);
}

void PopupMenu::_popup_base(const Rect2i& p_bounds)
{
	bool native = global_menu.is_valid();
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		native = false;
	}
#endif

	if (native) {
		_native_popup(p_bounds != Rect2i() ? p_bounds : Rect2i(get_position(), Size2i()));
	}
	else {
		if (is_inside_tree()) {
			set_flag(FLAG_POPUP, true);
			set_flag(FLAG_NO_FOCUS, !is_embedded());
		}

		moved = Vector2();
		popup_time_msec = OS::get_singleton()->get_ticks_msec();

		Popup::_popup_base(p_bounds);
	}
}

void PopupMenu::set_shrink_height(bool p_shrink) { shrink_height = p_shrink; }

bool PopupMenu::get_shrink_height() const { return shrink_height; }

void PopupMenu::set_shrink_width(bool p_shrink) { shrink_width = p_shrink; }

bool PopupMenu::get_shrink_width() const { return shrink_width; }

RID PopupMenuItems::get_focused_accessibility_element() const
{
	return popup->get_focused_accessibility_element();
}

PopupMenu::~PopupMenu() { unbind_global_menu(); }


