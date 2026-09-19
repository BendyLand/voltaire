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

void EditorDockManager::_dock_drag_stopped() { dock_tab_dragged = nullptr; }

DockTabContainer* EditorDockManager::get_dock_container(int p_slot) const
{
	ERR_FAIL_INDEX_V(p_slot, EditorDock::DOCK_SLOT_MAX, nullptr);
	return dock_slots[p_slot];
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
	}
}

Size2 DockSlotGrid::get_minimum_size() const
{
	return GRID_SIZE * CELL_SIZE * EDSCALE + (GRID_SIZE - Vector2i(1, 0)) * MARGINS * EDSCALE;
}

void EditorDockManager::add_dock(EditorDock*) {}

void EditorDockManager::remove_dock(EditorDock*) {}

void EditorDockManager::update_docks_menu() {}

EditorDock* EditorDockManager::_get_dock_tab_dragged()
{
	EditorDock ed = EditorDock();
	return &ed;
}

void EditorDockManager::_move_dock(EditorDock*, Control*, int, bool) {}

void DockContextPopup::_update_buttons() {}


