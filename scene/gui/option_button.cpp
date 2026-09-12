/**************************************************************************/
/*  option_button.cpp                                                     */
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

#include "option_button.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

static const int NONE_SELECTED = -1;

Size2 OptionButton::get_minimum_size() const
{
	Size2 minsize;
	if (fit_to_longest_item) {
		minsize = _cached_size;
	}
	else {
		minsize = Button::get_minimum_size();
	}

	if (has_theme_icon(SNAME("arrow"))) {
		const Size2 padding = _get_largest_stylebox_size();
		const Size2 arrow_size = theme_cache.arrow_icon->get_size();

		Size2 content_size = minsize - padding;
		content_size.width += arrow_size.width + MAX(0, theme_cache.h_separation);
		content_size.height = MAX(content_size.height, arrow_size.height);

		minsize = content_size + padding;
	}

	if (fit_to_longest_item) {
		// Make the width at least the width of the popup.
		minsize.width = MAX(minsize.width, popup->get_contents_minimum_size().width);
	}

	return minsize;
}

void OptionButton::_selected(int p_which) { _select(p_which, true); }

void OptionButton::pressed()
{
	if (popup->is_visible()) {
		popup->hide();
		return;
	}

	show_popup();
}

void OptionButton::add_icon_item(const Ref<Texture2D>& p_icon, const String& p_label, int p_id)
{
	bool first_selectable = !has_selectable_items();
	popup->add_icon_radio_check_item(p_icon, p_label, p_id);
	if (first_selectable) {
		select(get_item_count() - 1);
	}
	_queue_update_size_cache();
}

void OptionButton::add_item(const String& p_label, int p_id)
{
	bool first_selectable = !has_selectable_items();
	popup->add_radio_check_item(p_label, p_id);
	if (first_selectable) {
		select(get_item_count() - 1);
	}
	_queue_update_size_cache();
}

void OptionButton::set_item_text(int p_idx, const String& p_text)
{
	popup->set_item_text(p_idx, p_text);

	if (current == p_idx) {
		set_text(p_text);
	}
	_queue_update_size_cache();
}

void OptionButton::set_item_id(int p_idx, int p_id) { popup->set_item_id(p_idx, p_id); }

void OptionButton::set_item_tooltip(int p_idx, const String& p_tooltip)
{
	popup->set_item_tooltip(p_idx, p_tooltip);
}

void OptionButton::set_item_auto_translate_mode(int p_idx, AutoTranslateMode p_mode)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	if (popup->get_item_auto_translate_mode(p_idx) == p_mode) {
		return;
	}
	popup->set_item_auto_translate_mode(p_idx, p_mode);

	if (current == p_idx) {
		set_text(popup->get_item_text(p_idx));
	}
	_queue_update_size_cache();
}

void OptionButton::set_item_disabled(int p_idx, bool p_disabled)
{
	popup->set_item_disabled(p_idx, p_disabled);
}

String OptionButton::get_item_text(int p_idx) const { return popup->get_item_text(p_idx); }

Ref<Texture2D> OptionButton::get_item_icon(int p_idx) const { return popup->get_item_icon(p_idx); }

int OptionButton::get_item_id(int p_idx) const
{
	if (p_idx == NONE_SELECTED) {
		return NONE_SELECTED;
	}

	return popup->get_item_id(p_idx);
}

int OptionButton::get_item_index(int p_id) const { return popup->get_item_index(p_id); }

String OptionButton::get_item_tooltip(int p_idx) const { return popup->get_item_tooltip(p_idx); }

Node::AutoTranslateMode OptionButton::get_item_auto_translate_mode(int p_idx) const
{
	return popup->get_item_auto_translate_mode(p_idx);
}

bool OptionButton::is_item_disabled(int p_idx) const { return popup->is_item_disabled(p_idx); }

bool OptionButton::is_item_separator(int p_idx) const { return popup->is_item_separator(p_idx); }

bool OptionButton::has_selectable_items() const
{
	for (int i = 0; i < get_item_count(); i++) {
		if (!is_item_disabled(i) && !is_item_separator(i)) {
			return true;
		}
	}
	return false;
}

int OptionButton::get_selectable_item(bool p_from_last) const
{
	if (!p_from_last) {
		for (int i = 0; i < get_item_count(); i++) {
			if (!is_item_disabled(i) && !is_item_separator(i)) {
				return i;
			}
		}
	}
	else {
		for (int i = get_item_count() - 1; i >= 0; i--) {
			if (!is_item_disabled(i) && !is_item_separator(i)) {
				return i;
			}
		}
	}
	return -1;
}

int OptionButton::get_item_count() const { return popup->get_item_count(); }

void OptionButton::set_fit_to_longest_item(bool p_fit)
{
	if (p_fit == fit_to_longest_item) {
		return;
	}
	fit_to_longest_item = p_fit;

	_refresh_size_cache();
}

bool OptionButton::is_fit_to_longest_item() const { return fit_to_longest_item; }

void OptionButton::set_allow_reselect(bool p_allow) { allow_reselect = p_allow; }

bool OptionButton::get_allow_reselect() const { return allow_reselect; }

bool OptionButton::is_search_bar_enabled() const { return popup->is_search_bar_enabled(); }

int OptionButton::get_search_bar_min_item_count() const
{
	return popup->get_search_bar_min_item_count();
}

void OptionButton::set_search_bar_fuzzy_search_enabled(bool p_enabled)
{
	popup->set_search_bar_fuzzy_search_enabled(p_enabled);
}

bool OptionButton::is_search_bar_fuzzy_search_enabled() const
{
	return popup->is_search_bar_fuzzy_search_enabled();
}

void OptionButton::set_search_bar_fuzzy_search_max_misses(int p_max_misses)
{
	popup->set_search_bar_fuzzy_search_max_misses(p_max_misses);
}

int OptionButton::get_search_bar_fuzzy_search_max_misses() const
{
	return popup->get_search_bar_fuzzy_search_max_misses();
}

void OptionButton::add_separator(const String& p_text) { popup->add_separator(p_text); }

void OptionButton::_select_int(int p_which)
{
	if (p_which < NONE_SELECTED) {
		return;
	}
	if (p_which >= popup->get_item_count()) {
		if (!initialized) {
			queued_current = p_which;
		}
		return;
	}
	_select(p_which, false);
}

void OptionButton::select(int p_idx) { _select(p_idx, false); }

int OptionButton::get_selected() const { return current; }

int OptionButton::get_selected_id() const { return get_item_id(current); }

void OptionButton::remove_item(int p_idx)
{
	popup->remove_item(p_idx);
	if (current == p_idx) {
		_select(NONE_SELECTED);
	}
	_queue_update_size_cache();
}

PopupMenu* OptionButton::get_popup() const { return popup; }

void OptionButton::show_popup()
{
	if (!get_viewport()) {
		return;
	}

	// If not triggered by the mouse, start the popup with the checked item (or the first enabled
	// one) focused.
	if (current != NONE_SELECTED && !popup->is_item_disabled(current)) {
		if (!_was_pressed_by_mouse()) {
			popup->set_focused_item(current);
		}
		else {
			popup->scroll_to_item(current);
		}
	}
	else {
		for (int i = 0; i < popup->get_item_count(); i++) {
			if (!popup->is_item_disabled(i)) {
				if (!_was_pressed_by_mouse()) {
					popup->set_focused_item(i);
				}
				else {
					popup->scroll_to_item(i);
				}

				break;
			}
		}
	}

	Rect2 rect = get_screen_rect();
	rect.position.y += rect.size.height;
	if (get_viewport()->is_embedding_subwindows() && popup->get_force_native()) {
		Transform2D xform = get_viewport()->get_popup_base_transform_native();
		rect = xform.xform(rect);
	}
	rect.size.height = 0;
	popup->set_min_size(Size2(0, 0));
	popup->popup(rect);
}

void OptionButton::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

#ifdef TOOLS_ENABLED
PackedStringArray OptionButton::get_configuration_warnings() const
{
	PackedStringArray warnings = Button::get_configuration_warnings();
	warnings.append_array(popup->get_configuration_warnings());
	return warnings;
}
#endif

OptionButton::~OptionButton() {}


