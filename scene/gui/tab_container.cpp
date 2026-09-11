/**************************************************************************/
/*  tab_container.cpp                                                     */
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

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/popup.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "tab_container.h"

TabContainer::CachedTab* TabContainer::get_pending_tab(int p_idx) const
{
	if (p_idx >= pending_tabs.size()) {
		ERR_FAIL_COND_V(pending_tabs.resize(p_idx + 1) != OK, nullptr);
	}
	ERR_FAIL_INDEX_V(p_idx, pending_tabs.size(), nullptr);

	return pending_tabs.ptrw() + p_idx;
}

int TabContainer::_get_tab_height() const
{
	int height = 0;
	if (tabs_visible && get_tab_count() > 0) {
		height = tab_bar->get_minimum_size().height +
				 theme_cache.tabbar_style->get_margin(SIDE_TOP) +
				 theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);
	}

	return height;
}

void TabContainer::_repaint()
{
	layout_pending_start();
	_repaint_internal();
}

Vector<Control*> TabContainer::_get_tab_controls() const
{
	Vector<Control*> controls;
	ERR_THREAD_GUARD_V(controls);

	for (Node* child : iterate_children()) {
		Control* control = _as_tab_control(child);
		if (control) {
			controls.push_back(control);
		}
	}
	return controls;
}

void TabContainer::_drag_move_tab(int p_from_index, int p_to_index)
{
	move_child(get_tab_control(p_from_index), get_tab_control(p_to_index)->get_index(false));
}

void TabContainer::_on_tab_visibility_changed(Control* p_child)
{
	if (updating_visibility) {
		return;
	}
	int tab_index = get_tab_idx_from_control(p_child);
	if (tab_index == -1) {
		return;
	}
	// Only allow one tab to be visible.
	bool made_visible = p_child->is_visible();
	updating_visibility = true;

	if (!made_visible && get_current_tab() == tab_index) {
		if (get_deselect_enabled() || get_tab_count() == 0) {
			// Deselect.
			set_current_tab(-1);
		}
		else if (get_tab_count() == 1) {
			// Only tab, cannot deselect.
			p_child->show();
		}
		else {
			// Set a different tab to be the current tab.
			bool selected = select_next_available();
			if (!selected) {
				selected = select_previous_available();
			}
			if (!selected) {
				// No available tabs, deselect.
				set_current_tab(-1);
			}
		}
	}
	else if (made_visible && get_current_tab() != tab_index) {
		set_current_tab(tab_index);
	}

	updating_visibility = false;
}

TabBar* TabContainer::get_tab_bar() const { return tab_bar; }

int TabContainer::get_tab_count() const { return tab_bar->get_tab_count(); }

void TabContainer::set_current_tab(int p_current)
{
	if (!is_inside_tree()) {
		setup_current_tab = p_current;
		return;
	}

	tab_bar->set_current_tab(p_current);
}

int TabContainer::get_current_tab() const { return tab_bar->get_current_tab(); }

int TabContainer::get_previous_tab() const { return tab_bar->get_previous_tab(); }

bool TabContainer::select_previous_available() { return tab_bar->select_previous_available(); }

bool TabContainer::select_next_available() { return tab_bar->select_next_available(); }

void TabContainer::set_deselect_enabled(bool p_enabled)
{
	tab_bar->set_deselect_enabled(p_enabled);
}

bool TabContainer::get_deselect_enabled() const { return tab_bar->get_deselect_enabled(); }

Control* TabContainer::get_tab_control(int p_idx) const
{
	if (p_idx < 0) {
		return nullptr;
	}
	ERR_THREAD_GUARD_V(nullptr);

	for (Node* child : iterate_children()) {
		Control* control = _as_tab_control(child);
		if (!control) {
			continue;
		}

		if (p_idx > 0) {
			p_idx--;
		}
		else {
			return control;
		}
	}
	return nullptr;
}

Control* TabContainer::get_current_tab_control() const
{
	return get_tab_control(tab_bar->get_current_tab());
}

int TabContainer::get_tab_idx_at_point(const Point2& p_point) const
{
	return tab_bar->get_tab_idx_at_point(p_point);
}

int TabContainer::get_tab_idx_from_control(Control* p_child) const
{
	ERR_FAIL_NULL_V(p_child, -1);
	ERR_FAIL_COND_V(p_child->get_parent() != this, -1);
	ERR_THREAD_GUARD_V(-1);

	int idx = 0;
	for (Node* child : iterate_children()) {
		Control* control = _as_tab_control(child);
		if (!control) {
			continue;
		}

		if (control == p_child) {
			return idx;
		}
		idx++;
	}
	return -1;
}

void TabContainer::set_tab_alignment(TabBar::AlignmentMode p_alignment)
{
	if (tab_bar->get_tab_alignment() == p_alignment) {
		return;
	}

	tab_bar->set_tab_alignment(p_alignment);
	_update_margins();
}

TabBar::AlignmentMode TabContainer::get_tab_alignment() const
{
	return tab_bar->get_tab_alignment();
}

TabContainer::TabPosition TabContainer::get_tabs_position() const { return tabs_position; }

void TabContainer::set_tab_focus_mode(Control::FocusMode p_focus_mode)
{
	tab_bar->set_focus_mode(p_focus_mode);
}

Control::FocusMode TabContainer::get_tab_focus_mode() const { return tab_bar->get_focus_mode(); }

void TabContainer::set_clip_tabs(bool p_clip_tabs) { tab_bar->set_clip_tabs(p_clip_tabs); }

bool TabContainer::get_clip_tabs() const { return tab_bar->get_clip_tabs(); }

bool TabContainer::are_tabs_visible() const { return tabs_visible; }

#ifndef DISABLE_DEPRECATED
void TabContainer::set_all_tabs_in_front(bool p_in_front)
{
	if (p_in_front) {
		WARN_PRINT_ONCE("Due to internal changes, `all_tabs_in_front` doesn't do anything anymore, "
						"as they're always in front.");
	}
}

bool TabContainer::is_all_tabs_in_front() const { return false; }
#endif

String TabContainer::get_tab_title(int p_tab) const { return tab_bar->get_tab_title(p_tab); }

void TabContainer::set_tab_tooltip(int p_tab, const String& p_tooltip)
{
	tab_bar->set_tab_tooltip(p_tab, p_tooltip);
}

String TabContainer::get_tab_tooltip(int p_tab) const { return tab_bar->get_tab_tooltip(p_tab); }

Ref<Texture2D> TabContainer::get_tab_icon(int p_tab) const { return tab_bar->get_tab_icon(p_tab); }

int TabContainer::get_tab_icon_max_width(int p_tab) const
{
	return tab_bar->get_tab_icon_max_width(p_tab);
}

bool TabContainer::is_tab_disabled(int p_tab) const { return tab_bar->is_tab_disabled(p_tab); }

bool TabContainer::is_tab_hidden(int p_tab) const { return tab_bar->is_tab_hidden(p_tab); }

void TabContainer::set_tab_button_icon(int p_tab, const Ref<Texture2D>& p_icon)
{
	tab_bar->set_tab_button_icon(p_tab, p_icon);

	_update_margins();
	_repaint();
}

Ref<Texture2D> TabContainer::get_tab_button_icon(int p_tab) const
{
	return tab_bar->get_tab_button_icon(p_tab);
}

Size2 TabContainer::_get_minimum_size(bool p_use_desired_sizes) const
{
	Size2 ms;

	if (tabs_visible) {
		ms = p_use_desired_sizes ? tab_bar->get_bound_desired_size() : tab_bar->get_minimum_size();
		ms.width += theme_cache.tabbar_style->get_margin(SIDE_LEFT) +
					theme_cache.tabbar_style->get_margin(SIDE_RIGHT);
		ms.height += theme_cache.tabbar_style->get_margin(SIDE_TOP) +
					 theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);

		if (get_popup()) {
			ms.width += p_use_desired_sizes ? popup_button->get_bound_desired_size().x
											: popup_button->get_minimum_size().x;
		}

		if (theme_cache.side_margin > 0 && get_tab_alignment() != TabBar::ALIGNMENT_CENTER &&
			(get_tab_alignment() != TabBar::ALIGNMENT_RIGHT || !get_popup())) {
			ms.width += theme_cache.side_margin;
		}
	}

	Vector<Control*> controls = _get_tab_controls();
	Size2 largest_child_min_size;
	for (int i = 0; i < controls.size(); i++) {
		Control* c = controls[i];

		if (!c->is_visible() && !use_hidden_tabs_for_min_size) {
			continue;
		}

		Size2 cms = p_use_desired_sizes ? c->get_bound_desired_size() : c->get_bound_minimum_size();
		largest_child_min_size = largest_child_min_size.max(cms);
	}
	ms.height += largest_child_min_size.height;

	Size2 panel_ms = theme_cache.panel_style->get_minimum_size();

	ms.width = MAX(ms.width, largest_child_min_size.width + panel_ms.width);
	ms.height += panel_ms.height;

	return ms;
}

Size2 TabContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 TabContainer::get_desired_size() const { return _get_minimum_size(true); }

Size2 TabContainer::get_inner_combined_maximum_size() const
{
	Size2 ms = Container::get_inner_combined_maximum_size();

	if (tabs_visible && tab_bar) {
		Size2 tab_bar_ms = tab_bar->get_minimum_size();
		ms.height -= tab_bar_ms.height;

		if (theme_cache.tabbar_style.is_valid()) {
			ms.height -= theme_cache.tabbar_style->get_margin(SIDE_TOP) +
						 theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);
		}
	}

	if (theme_cache.panel_style.is_valid()) {
		ms -= theme_cache.panel_style->get_minimum_size();
	}

	return ms;
}

void TabContainer::_maximum_size_changed()
{
	if (!tab_bar) {
		return;
	}

	Size2 ms = get_combined_maximum_size();
	if (theme_cache.tabbar_style.is_valid()) {
		if (ms.width >= 0) {
			ms.width -= theme_cache.tabbar_style->get_margin(SIDE_LEFT) +
						theme_cache.tabbar_style->get_margin(SIDE_RIGHT);
			if (get_popup() && popup_button) {
				ms.width -= popup_button->get_minimum_size().x;
			}
			if (theme_cache.side_margin > 0 && get_tab_alignment() != TabBar::ALIGNMENT_CENTER &&
				(get_tab_alignment() != TabBar::ALIGNMENT_RIGHT || !get_popup())) {
				ms.width -= theme_cache.side_margin;
			}
			ms.width = MAX(ms.width, 0);
		}
		if (ms.height >= 0) {
			ms.height -= theme_cache.tabbar_style->get_margin(SIDE_TOP) +
						 theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);
			ms.height = MAX(ms.height, 0);
		}
	}
	internal_container->set_parent_maximum_size_cache(Size2(-1, -1));
	tab_bar->set_custom_maximum_size(ms);
}

void TabContainer::set_switch_on_drag_hover(bool p_enabled)
{
	tab_bar->set_switch_on_drag_hover(p_enabled);
}

bool TabContainer::get_switch_on_drag_hover() const { return tab_bar->get_switch_on_drag_hover(); }

void TabContainer::set_drag_to_rearrange_enabled(bool p_enabled)
{
	drag_to_rearrange_enabled = p_enabled;
}

bool TabContainer::get_drag_to_rearrange_enabled() const { return drag_to_rearrange_enabled; }

void TabContainer::set_tabs_rearrange_group(int p_group_id)
{
	tab_bar->set_tabs_rearrange_group(p_group_id);
}

int TabContainer::get_tabs_rearrange_group() const { return tab_bar->get_tabs_rearrange_group(); }

bool TabContainer::get_use_hidden_tabs_for_min_size() const { return use_hidden_tabs_for_min_size; }

Vector<int> TabContainer::get_allowed_size_flags_horizontal() const { return Vector<int>(); }

Vector<int> TabContainer::get_allowed_size_flags_vertical() const { return Vector<int>(); }


