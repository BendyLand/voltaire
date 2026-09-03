/**************************************************************************/
/*  editor_dock_manager.cpp                                               */
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

#include "editor/docks/dock_tab_container.h"
#include "editor/docks/editor_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/window_wrapper.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_dock_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"

////////////////////////////////////////////////
////////////////////////////////////////////////

void DockSplitContainer::_notification(int p_what)
{
	switch (p_what) {
	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (!EditorSettings::get_singleton()->check_changed_settings_in_group(
				"interface/touchscreen")) {
			return;
		}
	} break;
	}
}

void DockSplitContainer::_update_visibility()
{
	if (is_updating) {
		return;
	}
	is_updating = true;
	bool any_visible = false;
	set_visible(any_visible);
	is_updating = false;
}

////////////////////////////////////////////////
////////////////////////////////////////////////

void EditorDockManager::_dock_drag_stopped() { dock_tab_dragged = nullptr; }

void EditorDockManager::_dock_split_dragged(int p_offset)
{
	EditorNode::get_singleton()->save_editor_layout_delayed();
}

void EditorDockManager::_update_layout()
{
	if (!dock_context_popup->is_inside_tree() || EditorNode::get_singleton()->is_exiting()) {
		return;
	}
	dock_context_popup->docks_updated();
	update_docks_menu();
	EditorNode::get_singleton()->save_editor_layout_delayed();
}

DockTabContainer* EditorDockManager::get_dock_container(int p_slot) const
{
	ERR_FAIL_INDEX_V(p_slot, EditorDock::DOCK_SLOT_MAX, nullptr);
	return dock_slots[p_slot];
}

void EditorDockManager::_window_close_request(WindowWrapper* p_wrapper)
{
	// Give the dock back to the original owner.
	EditorDock* dock = _close_window(p_wrapper);
	ERR_FAIL_COND(!all_docks.has(dock));

	if (dock->dock_slot_index != EditorDock::DOCK_SLOT_NONE) {
		dock->is_open = false;
		focus_dock(dock);
	}
	else {
		close_dock(dock);
	}
}

void EditorDockManager::_open_dock_in_window(
	EditorDock* p_dock, bool p_show_window, bool p_reset_size)
{
	ERR_FAIL_NULL(p_dock);

	DockTabContainer* parent_container = p_dock->get_parent_container();
	const Rect2 floating_rect = parent_container
									? parent_container->get_floating_dock_rect(p_dock)
									: DockTabContainer::get_default_floating_dock_rect(p_dock);
	Size2 dock_size = floating_rect.size;
	Point2 dock_screen_pos = floating_rect.position;

	WindowWrapper* wrapper = memnew(WindowWrapper);
	wrapper->set_window_title(vformat(TTR("%s - Godot Engine"), TTR(p_dock->get_display_title())));
	wrapper->set_margins_enabled(true);

	EditorNode::get_singleton()->get_gui_base()->add_child(wrapper);

	_move_dock(p_dock, nullptr);
	p_dock->update_layout(EditorDock::DOCK_LAYOUT_FLOATING, EditorDock::DOCK_SLOT_NONE);
	p_dock->current_layout = EditorDock::DOCK_LAYOUT_FLOATING;
	wrapper->set_wrapped_control(p_dock);

	p_dock->dock_window = wrapper;
	p_dock->is_open = true;
	p_dock->show();

	dock_windows.push_back(wrapper);

	if (p_show_window) {
		wrapper->restore_window(Rect2i(dock_screen_pos, dock_size),
			EditorNode::get_singleton()->get_gui_base()->get_window()->get_current_screen());
		_update_layout();
		if (p_reset_size) {
			// Use a default size of one third the current window size.
			Size2i popup_size = EditorNode::get_singleton()->get_window()->get_size() / 3.0;
			p_dock->get_window()->set_size(popup_size);
			p_dock->get_window()->move_to_center();
		}
		p_dock->get_window()->grab_focus();
	}
}

void EditorDockManager::_update_dirty_dock_tabs()
{
	bool update_menu = false;
	for (EditorDock* dock : dirty_docks) {
		update_menu = update_menu || dock->global;
		dock->update_tab_style();
	}
	dirty_docks.clear();

	if (update_menu) {
		update_docks_menu();
	}
}

void EditorDockManager::set_dock_slot_highlighted(int p_slot, bool p_highlighted)
{
	ERR_FAIL_INDEX(p_slot, EditorDock::DOCK_SLOT_MAX);
	if (p_highlighted) {
		dock_slots[p_slot]->show_drag_hint();
	}
	else {
		dock_slots[p_slot]->get_drag_hint()->hide();
	}
	dock_slots[p_slot]->get_drag_hint()->set_highlighted(p_highlighted);
}

void EditorDockManager::set_dock_enabled(EditorDock* p_dock, bool p_enabled)
{
	ERR_FAIL_NULL(p_dock);
	ERR_FAIL_COND_MSG(!all_docks.has(p_dock),
		vformat("Cannot set enabled unknown dock '%s'.", p_dock->get_display_title()));

	if (p_dock->enabled == p_enabled) {
		return;
	}

	p_dock->enabled = p_enabled;
	if (p_enabled) {
		open_dock(p_dock, false);
	}
	else {
		close_dock(p_dock);
	}
}

void EditorDockManager::close_dock(EditorDock* p_dock)
{
	ERR_FAIL_NULL(p_dock);
	ERR_FAIL_COND_MSG(!all_docks.has(p_dock),
		vformat("Cannot close unknown dock '%s'.", p_dock->get_display_title()));

	if (!p_dock->is_open) {
		return;
	}

	p_dock->is_open = false;
	DockTabContainer* parent_container = p_dock->get_parent_container();
	if (parent_container) {
		parent_container->dock_closed(p_dock);
	}

	_move_dock(p_dock, closed_dock_parent);

	_update_layout();
}

void EditorDockManager::open_dock(EditorDock* p_dock, bool p_set_current)
{
	ERR_FAIL_NULL(p_dock);
	ERR_FAIL_COND_MSG(!all_docks.has(p_dock),
		vformat("Cannot open unknown dock '%s'.", p_dock->get_display_title()));

	if (p_dock->is_open) {
		// Show the dock if it is already open.
		if (p_set_current) {
			_make_dock_visible(p_dock, false);
		}
		return;
	}

	p_dock->is_open = true;

	// Open dock to its previous location.
	if (p_dock->dock_slot_index != EditorDock::DOCK_SLOT_NONE) {
		DockTabContainer* slot = dock_slots[p_dock->dock_slot_index];
		int tab_index = p_dock->previous_tab_index;
		if (tab_index < 0) {
			tab_index = slot->get_tab_count();
		}

		_move_dock(p_dock, slot, tab_index, p_set_current && slot->can_switch_dock());
	}
	else {
		_open_dock_in_window(p_dock, true, true);
		return;
	}

	_update_layout();
}

void EditorDockManager::make_dock_floating(EditorDock* p_dock)
{
	ERR_FAIL_NULL(p_dock);
	ERR_FAIL_COND_MSG(!all_docks.has(p_dock),
		vformat("Cannot make unknown dock '%s' floating.", p_dock->get_display_title()));

	if (!p_dock->dock_window) {
		_open_dock_in_window(p_dock);
	}
}

void EditorDockManager::_make_dock_visible(EditorDock* p_dock, bool p_grab_focus)
{
	if (p_dock->dock_window) {
		if (p_grab_focus) {
			p_dock->get_window()->grab_focus();
		}
		return;
	}

	DockTabContainer* tab_container = p_dock->get_parent_container();
	if (!tab_container || !tab_container->can_switch_dock()) {
		return;
	}

	if (p_grab_focus) {
		tab_container->get_tab_bar()->grab_focus();
	}

	if (!p_dock->is_visible_in_tree()) {
		int tab_index = tab_container->get_tab_idx_from_control(p_dock);
		tab_container->set_current_tab(tab_index);
	}
}

void EditorDockManager::set_docks_visible(bool p_show)
{
	if (docks_visible == p_show) {
		return;
	}
	docks_visible = p_show;
	for (int i = 0; i < EditorDock::DOCK_SLOT_MAX; i++) {
		// Show and hide in reverse order due to the SplitContainer prioritizing the last split
		// offset.
		dock_slots[docks_visible ? i : EditorDock::DOCK_SLOT_MAX - i - 1]->update_visibility();
	}
	_update_layout();
}

bool EditorDockManager::are_docks_visible() const { return docks_visible; }

void EditorDockManager::update_tab_styles()
{
	for (EditorDock* dock : all_docks) {
		dock->update_tab_style();
	}
}

void EditorDockManager::set_tab_icon_max_width(int p_max_width)
{
	for (int i = 0; i < EditorDock::DOCK_SLOT_MAX; i++) {
		dock_slots[i]->add_theme_constant_override(SNAME("icon_max_width"), p_max_width);
	}
}

int EditorDockManager::get_vsplit_count() const { return vsplits.size(); }

PopupMenu* EditorDockManager::get_docks_menu() { return docks_menu; }

////////////////////////////////////////////////
////////////////////////////////////////////////

void DockContextPopup::_notification(int p_what)
{
	switch (p_what) {
	case Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
	case NOTIFICATION_TRANSLATION_CHANGED:
	case NOTIFICATION_THEME_CHANGED: {
		if (make_float_button) {
			make_float_button->set_button_icon(get_editor_theme_icon(SNAME("MakeFloating")));
		}
		if (is_layout_rtl()) {
			tab_move_left_button->set_button_icon(get_editor_theme_icon(SNAME("Forward")));
			tab_move_right_button->set_button_icon(get_editor_theme_icon(SNAME("Back")));
			tab_move_left_button->set_tooltip_text(TTR("Move this dock right one tab."));
			tab_move_right_button->set_tooltip_text(TTR("Move this dock left one tab."));
		}
		else {
			tab_move_left_button->set_button_icon(get_editor_theme_icon(SNAME("Back")));
			tab_move_right_button->set_button_icon(get_editor_theme_icon(SNAME("Forward")));
			tab_move_left_button->set_tooltip_text(TTR("Move this dock left one tab."));
			tab_move_right_button->set_tooltip_text(TTR("Move this dock right one tab."));
		}
		close_button->set_button_icon(get_editor_theme_icon(SNAME("Close")));
	} break;
	}
}

void DockContextPopup::_slot_clicked(int p_slot)
{
	DockTabContainer* target_tab_container = dock_manager->dock_slots[p_slot];
	if (context_dock->get_parent_container() != target_tab_container) {
		dock_manager->_move_dock(
			context_dock, target_tab_container, target_tab_container->get_tab_count());
		dock_manager->_update_layout();
		hide();
	}
}

void DockContextPopup::_tab_move_left()
{
	TabContainer* tab_container = context_dock->get_parent_container();
	if (!tab_container) {
		return;
	}
	int new_index = tab_container->get_tab_idx_from_control(context_dock) - 1;
	context_dock->set_tab_index(new_index, true);
	dock_manager->_update_layout();
	dock_select->queue_redraw();
}

void DockContextPopup::_tab_move_right()
{
	TabContainer* tab_container = context_dock->get_parent_container();
	if (!tab_container) {
		return;
	}
	int new_index = tab_container->get_tab_idx_from_control(context_dock) + 1;
	context_dock->set_tab_index(new_index, true);
	dock_manager->_update_layout();
	dock_select->queue_redraw();
}

void DockContextPopup::_close_dock()
{
	hide();
	dock_manager->close_dock(context_dock);
}

void DockContextPopup::_float_dock()
{
	hide();
	dock_manager->_open_dock_in_window(context_dock);
}

void DockContextPopup::_update_buttons()
{
	if (context_dock->global || context_dock->closable) {
		close_button->set_tooltip_text(TTRC("Close this dock."));
		close_button->set_disabled(false);
	}
	else {
		close_button->set_tooltip_text(TTRC("This dock can't be closed."));
		close_button->set_disabled(true);
	}
	if (EditorNode::get_singleton()->is_multi_window_enabled()) {
		if (!(context_dock->available_layouts & EditorDock::DOCK_LAYOUT_FLOATING)) {
			make_float_button->set_tooltip_text(TTRC("This dock does not support floating."));
			make_float_button->set_disabled(true);
		}
		else {
			make_float_button->set_tooltip_text(TTRC("Make this dock floating."));
			make_float_button->set_disabled(false);
		}
	}

	// Update tab move buttons.
	tab_move_left_button->set_disabled(true);
	tab_move_right_button->set_disabled(true);
	DockTabContainer* context_tab_container = context_dock->get_parent_container();
	if (context_tab_container && context_tab_container->get_tab_count() > 0) {
		int context_tab_index = context_tab_container->get_tab_idx_from_control(context_dock);
		tab_move_left_button->set_disabled(context_tab_index == 0);
		tab_move_right_button->set_disabled(
			context_tab_index >= context_tab_container->get_tab_count() - 1);
	}
	reset_size();
}

void DockContextPopup::set_dock(EditorDock* p_dock)
{
	context_dock = p_dock;
	dock_select->context_dock = p_dock;
	_update_buttons();
}

void DockContextPopup::docks_updated()
{
	if (!is_visible()) {
		return;
	}
	_update_buttons();
}

DockContextPopup::DockContextPopup()
{
	dock_manager = EditorDockManager::get_singleton();

	dock_select_popup_vb = memnew(VBoxContainer);
	add_child(dock_select_popup_vb);

	HBoxContainer* header_hb = memnew(HBoxContainer);
	tab_move_left_button = memnew(Button);
	tab_move_left_button->set_accessibility_name(TTRC("Move Tab Left"));
	tab_move_left_button->set_flat(true);
	tab_move_left_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	header_hb->add_child(tab_move_left_button);

	Label* position_label = memnew(Label);
	position_label->set_text(TTRC("Dock Position"));
	position_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	position_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	header_hb->add_child(position_label);

	tab_move_right_button = memnew(Button);
	tab_move_right_button->set_accessibility_name(TTRC("Move Tab Right"));
	tab_move_right_button->set_flat(true);
	tab_move_right_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);

	header_hb->add_child(tab_move_right_button);
	dock_select_popup_vb->add_child(header_hb);

	dock_select = memnew(DockSlotGrid);
	dock_select_popup_vb->add_child(dock_select);

	Control* separator = memnew(Control);
	separator->set_custom_minimum_size(Vector2(0, 8 * EDSCALE));
	dock_select_popup_vb->add_child(separator);

	make_float_button = memnew(Button);
	make_float_button->set_text(TTRC("Make Floating"));
	if (!EditorNode::get_singleton()->is_multi_window_enabled()) {
		make_float_button->set_disabled(true);
		make_float_button->set_tooltip_text(
			EditorNode::get_singleton()->get_multiwindow_support_tooltip_text());
	}
	make_float_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	make_float_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dock_select_popup_vb->add_child(make_float_button);

	close_button = memnew(Button);
	close_button->set_text(TTRC("Close"));
	close_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	close_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dock_select_popup_vb->add_child(close_button);
}

void DockSlotGrid::_update_rect_cache()
{
	for (int i = 0; i < EditorDock::DOCK_SLOT_MAX; i++) {
		Rect2 rect = EditorDockManager::get_singleton()->dock_slots[i]->grid_rect;
		if (is_layout_rtl()) {
			rect.position.x = GRID_SIZE.x - rect.position.x - rect.size.x;
		}
		rect.position = rect.position * CELL_SIZE * EDSCALE +
						(rect.position + Vector2i(0, 1)) * MARGINS * EDSCALE;
		rect.size =
			rect.size * CELL_SIZE * EDSCALE + (rect.size - Vector2i(1, 1)) * MARGINS * EDSCALE;
		rect_cache[i] = rect;
	}

	// Temporarily hard-coded, until main screen is registered as a slot.
	{
		Rect2 rect = Rect2i(2, 0, 4, 4);
		if (is_layout_rtl()) {
			rect.position.x = GRID_SIZE.x - rect.position.x - rect.size.x;
		}
		rect.position = rect.position * CELL_SIZE * EDSCALE +
						(rect.position + Vector2i(0, 1)) * MARGINS * EDSCALE;
		rect.size =
			rect.size * CELL_SIZE * EDSCALE + (rect.size - Vector2i(1, 1)) * MARGINS * EDSCALE;
		main_screen_rect = rect;
	}
}

void DockSlotGrid::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
	case NOTIFICATION_TRANSLATION_CHANGED: {
		rect_cache_dirty = true;
	} break;

	case NOTIFICATION_DRAW: {
		if (rect_cache_dirty) {
			_update_rect_cache();
			rect_cache_dirty = false;
		}
		Color used_dock_color = Color(0.6, 0.6, 0.6, 0.8);
		Color hovered_dock_color = Color(0.8, 0.8, 0.8, 0.8);
		Color tab_selected_color = get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
		Color tab_unselected_color = used_dock_color;
		Color unused_dock_color = used_dock_color;
		unused_dock_color.a = 0.4;
		Color unusable_dock_color = unused_dock_color;
		unusable_dock_color.a = 0.1;
		Color tab_unusable_color = unusable_dock_color;

		TabContainer* context_tab_container = context_dock->get_parent_container();
		int context_tab_index = -1;
		if (context_tab_container && context_tab_container->get_tab_count() > 0) {
			context_tab_index = context_tab_container->get_tab_idx_from_control(context_dock);
		}

		for (int i = 0; i < EditorDock::DOCK_SLOT_MAX; i++) {
			const Rect2i slot_rect = rect_cache[i];
			int max_tabs =
				EditorDockManager::get_singleton()->dock_slots[i]->grid_rect.size.x * TABS_PER_CELL;

			DockTabContainer* dock_slot = EditorDockManager::get_singleton()->dock_slots[i];
			bool is_context_slot = context_tab_container == dock_slot;
			bool is_slot_available = context_dock->available_layouts & dock_slot->layout;
			int tabs_to_draw = MIN(max_tabs, dock_slot->get_tab_count());

			if (i == context_dock->dock_slot_index) {
				draw_rect(slot_rect, tab_selected_color);
			}
			else if (!is_slot_available) {
				draw_rect(slot_rect, unusable_dock_color);
			}
			else if (i == hovered_slot) {
				draw_rect(slot_rect, hovered_dock_color);
			}
			else if (tabs_to_draw == 0) {
				draw_rect(slot_rect, unused_dock_color);
			}
			else {
				draw_rect(slot_rect, used_dock_color);
			}

			real_t tab_width =
				((slot_rect.size.x - (max_tabs - 1) * TAB_MARGIN * EDSCALE) / max_tabs);
			real_t initial_offset =
				(slot_rect.size.x -
					(max_tabs * tab_width + (max_tabs - 1) * TAB_MARGIN * EDSCALE)) *
				0.5;

			for (int j = 0; j < tabs_to_draw; j++) {
				real_t pos_x = is_layout_rtl()
								   ? slot_rect.size.x - (initial_offset + (j + 1) * tab_width +
															j * TAB_MARGIN * EDSCALE)
								   : initial_offset + j * (tab_width + TAB_MARGIN * EDSCALE);
				const Rect2 tab_rect =
					Rect2(slot_rect.position +
							  Vector2(pos_x, -MARGINS.y * EDSCALE + MARGINS.y * EDSCALE / 4),
						Vector2(tab_width, MARGINS.y * EDSCALE / 2));
				if (is_context_slot && context_tab_index == j) {
					draw_rect(tab_rect, tab_selected_color);
				}
				else if (is_slot_available) {
					draw_rect(tab_rect, tab_unselected_color);
				}
				else {
					draw_rect(tab_rect, tab_unusable_color);
				}
			}
		}
		draw_rect(main_screen_rect, unusable_dock_color);
	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		if (hovered_slot > -1) {
			EditorDockManager::get_singleton()->set_dock_slot_highlighted(hovered_slot, false);
			hovered_slot = -1;
			queue_redraw();
		}
	} break;
	}
}

Size2 DockSlotGrid::get_minimum_size() const
{
	return GRID_SIZE * CELL_SIZE * EDSCALE + (GRID_SIZE - Vector2i(1, 0)) * MARGINS * EDSCALE;
}


