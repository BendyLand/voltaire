/**************************************************************************/
/*  editor_bottom_panel.cpp                                               */
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

#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/editor_dock.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/editor_version_button.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_bottom_panel.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"

void EditorBottomPanel::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		set_accessibility_region(true);
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		pin_button->set_button_icon(get_editor_theme_icon(SNAME("Pin")));
		expand_button->set_button_icon(get_editor_theme_icon(SNAME("ExpandBottomDock")));
	} break;
	}
}

void EditorBottomPanel::_on_tab_changed(int p_idx)
{
	_update_center_split_offset();
	_repaint();
	if (p_idx >= 0 && p_idx < get_tab_count()) {
		set_accessibility_name(get_tab_title(p_idx));
	}
}

void EditorBottomPanel::_theme_changed()
{
	if (get_current_tab() == -1) {
		// Hide panel when not showing anything.
		remove_theme_style_override(SceneStringName(panel));
	}
	else {
		add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SNAME("BottomPanel"), EditorStringName(EditorStyles)).ptr());
	}
}

void EditorBottomPanel::_repaint()
{
	bool panel_collapsed = get_current_tab() == -1;

	if (panel_collapsed && get_popup()) {
		set_popup(nullptr);
	}
	else if (!panel_collapsed && !get_popup()) {
		set_popup(dock_context_popup);
	}
	if (!panel_collapsed && (previous_tab != -1)) {
		return;
	}
	previous_tab = get_current_tab();

	DockSplitContainer* center_split = EditorNode::get_center_split();
	ERR_FAIL_NULL(center_split);

	center_split->set_dragger_visibility(
		panel_collapsed ? SplitContainer::DRAGGER_HIDDEN : SplitContainer::DRAGGER_VISIBLE);
	center_split->set_collapsed(panel_collapsed);

	expand_button->set_visible(!panel_collapsed);
	if (expand_button->is_pressed()) {
		_expand_button_toggled(!panel_collapsed);
	}
	else {
		_theme_changed();
	}
}

void EditorBottomPanel::dock_closed(EditorDock* p_dock)
{
	if (p_dock == get_current_tab_control()) {
		hide_bottom_panel();
	}
}

void EditorBottomPanel::dock_focused(EditorDock* p_dock, bool p_was_visible)
{
	if (p_was_visible && p_dock->is_visible()) {
		hide_bottom_panel();
	}
}

bool EditorBottomPanel::can_switch_dock() const { return !is_locked(); }

Rect2 EditorBottomPanel::get_floating_dock_rect(EditorDock* p_dock)
{
	if (p_dock->is_visible_in_tree()) {
		return DockTabContainer::get_default_floating_dock_rect(p_dock);
	}
	// If dock is  not visible, its size may not be initialized.
	Rect2 ret;
	ret.size = get_size();

	int* stored_size = dock_offsets.getptr(p_dock->get_effective_layout_key());
	if (stored_size) {
		ret.size.y = -(*stored_size);
	}
	ret.size.y -= get_tab_bar()->get_size().y;
	ret.position = get_tab_bar()->get_screen_position() - Vector2(0, ret.size.y);
	return ret;
}

void EditorBottomPanel::make_item_visible(Control* p_item, bool p_visible, bool p_ignore_lock)
{
	// Don't allow changing tabs involuntarily when tabs are locked.
	if (!p_ignore_lock && lock_panel_switching) {
		return;
	}

	EditorDock* dock = _get_dock_from_control(p_item);
	ERR_FAIL_NULL(dock);
	dock->set_visible(p_visible);
}

void EditorBottomPanel::hide_bottom_panel() { set_current_tab(-1); }

void EditorBottomPanel::toggle_last_opened_bottom_panel()
{
	set_current_tab(get_current_tab() == -1 ? get_previous_tab() : -1);
}

void EditorBottomPanel::_pin_button_toggled(bool p_pressed) { lock_panel_switching = p_pressed; }

void EditorBottomPanel::set_expanded(bool p_expanded) { expand_button->set_pressed(p_expanded); }

void EditorBottomPanel::_update_center_split_offset()
{
	DockSplitContainer* center_split = EditorNode::get_center_split();
	ERR_FAIL_NULL(center_split);

	center_split->set_split_offset(get_bottom_panel_offset());
}

void EditorBottomPanel::remove_item(Control* p_item)
{
	EditorDock* dock = _get_dock_from_control(p_item);
	ERR_FAIL_NULL_MSG(dock,
		vformat("Cannot remove unknown dock \"%s\" from the bottom panel.", p_item->get_name()));

	int item_idx = bottom_docks.find(dock);
	ERR_FAIL_COND(item_idx == -1);

	bottom_docks.remove_at(item_idx);

	legacy_buttons[item_idx]->queue_free();
	legacy_buttons.remove_at(item_idx);

	EditorDockManager::get_singleton()->remove_dock(dock);
	dock->remove_child(p_item);
	dock->queue_free();
}

void EditorBottomPanel::_on_button_visibility_changed(Button* p_button, EditorDock* p_dock)
{
	if (p_button->is_visible()) {
		p_dock->open();
	}
	else {
		p_dock->close();
	}
}

EditorBottomPanel::~EditorBottomPanel()
{
	for (Button* b : legacy_buttons) {
		memdelete(b);
	}
}

void ProgressIndicator::_notification(int p_what)
{
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		const Ref<Texture2D> ring_texture = get_editor_theme_icon(SNAME("ProgressRing"));
		set_progress_texture(ring_texture);
		set_tint_progress(get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
		set_under_texture(ring_texture);
	}
}

ProgressIndicator::ProgressIndicator()
{
	set_fill_mode(FILL_CLOCKWISE);
	set_tint_under(Color());
	set_step(0.0);
	set_max(1.0);
}


